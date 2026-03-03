#include "dx12_window.hpp"

#ifdef AIGUI_PLATFORM_WINDOWS

#ifndef AIGUI_BACKEND_ONLY
#   define AIGUI_BACKEND_ONLY
#endif

// DX12 / DXGI headers (require Windows 10 SDK)
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <stdexcept>
#include <string>
#include <cassert>
#include <cstring>
#include <algorithm>

namespace aigui {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) throw std::runtime_error("DX12 error: HRESULT 0x" +
                    std::to_string(static_cast<unsigned>(hr)));
}

// ─────────────────────────────────────────────────────────────────────────────
// HLSL shaders (compiled inline for minimal dependencies)
// ─────────────────────────────────────────────────────────────────────────────

// Simple 2-D colour-fill vertex/pixel shader pair.
static const char* kShaderSrc = R"HLSL(
struct VSIn {
    float2 pos   : POSITION;
    float4 color : COLOR;
};
struct PSIn {
    float4 pos   : SV_POSITION;
    float4 color : COLOR;
};
cbuffer Transform : register(b0) {
    float2 invViewport;   // (2/W, -2/H)
};
PSIn VSMain(VSIn v) {
    PSIn o;
    // Map pixel coords to NDC: x = x*2/W - 1, y = 1 - y*2/H
    o.pos   = float4(v.pos.x * invViewport.x - 1.0,
                     1.0 - v.pos.y * invViewport.y, 0, 1);
    o.color = v.color;
    return o;
}
float4 PSMain(PSIn p) : SV_TARGET { return p.color; }
)HLSL";

// ─────────────────────────────────────────────────────────────────────────────
// DX12Canvas
// ─────────────────────────────────────────────────────────────────────────────

bool DX12Canvas::init(HWND hwnd, uint32_t width, uint32_t height) {
    width_  = width;
    height_ = height;

#ifdef _DEBUG
    ID3D12Debug* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif

    // Factory
    IDXGIFactory4* factory = nullptr;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    // Device
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(&device_)));

    // Command queue
    D3D12_COMMAND_QUEUE_DESC qDesc{};
    qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(device_->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&commandQueue_)));

    // Swap chain
    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width       = width;
    scDesc.Height      = height;
    scDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferCount = kDX12FrameCount;
    scDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;

    IDXGISwapChain1* sc1 = nullptr;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        commandQueue_, hwnd, &scDesc, nullptr, nullptr, &sc1));
    ThrowIfFailed(sc1->QueryInterface(IID_PPV_ARGS(&swapChain_)));
    sc1->Release();
    factory->Release();

    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

    // RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = kDX12FrameCount;
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap_)));

    UINT rtvSize = device_->GetDescriptorHandleIncrementSize(
                       D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    for (uint32_t i = 0; i < kDX12FrameCount; ++i) {
        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i])));
        device_->CreateRenderTargetView(renderTargets_[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvSize;

        ThrowIfFailed(device_->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators_[i])));
    }

    // Command list
    ThrowIfFailed(device_->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocators_[frameIndex_], nullptr, IID_PPV_ARGS(&commandList_)));
    commandList_->Close();

    // Fence
    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                        IID_PPV_ARGS(&fence_)));
    for (auto& v : fenceValues_) v = 1;

    if (!createPipeline()) return false;
    return true;
}

bool DX12Canvas::createPipeline() {
    // Root signature: one CBV (transform constants).
    D3D12_ROOT_PARAMETER rootParam{};
    rootParam.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters   = &rootParam;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* rsBlob = nullptr, *errBlob = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                           &rsBlob, &errBlob))) {
        if (errBlob) errBlob->Release();
        return false;
    }
    ThrowIfFailed(device_->CreateRootSignature(0, rsBlob->GetBufferPointer(),
                  rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)));
    rsBlob->Release();

    // Compile shaders.
    ID3DBlob* vsBlob = nullptr, *psBlob = nullptr;
    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ThrowIfFailed(D3DCompile(kShaderSrc, strlen(kShaderSrc), nullptr, nullptr,
                              nullptr, "VSMain", "vs_5_0", compileFlags, 0,
                              &vsBlob, nullptr));
    ThrowIfFailed(D3DCompile(kShaderSrc, strlen(kShaderSrc), nullptr, nullptr,
                              nullptr, "PSMain", "ps_5_0", compileFlags, 0,
                              &psBlob, nullptr));

    // Input layout: POSITION (float2), COLOR (float4).
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature        = rootSignature_;
    psoDesc.VS                    = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
    psoDesc.PS                    = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
    psoDesc.InputLayout           = {inputLayout, 2};
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count      = 1;
    psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable   = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    // Alpha blending.
    psoDesc.BlendState.RenderTarget[0].BlendEnable           = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    ThrowIfFailed(device_->CreateGraphicsPipelineState(&psoDesc,
                  IID_PPV_ARGS(&pipelineState_)));
    vsBlob->Release();
    psBlob->Release();
    return true;
}

void DX12Canvas::shutdown() {
    flushAndWait();
    for (uint32_t i = 0; i < kDX12FrameCount; ++i) {
        if (renderTargets_[i])    { renderTargets_[i]->Release();    renderTargets_[i]    = nullptr; }
        if (commandAllocators_[i]){ commandAllocators_[i]->Release(); commandAllocators_[i]= nullptr; }
    }
    if (pipelineState_) { pipelineState_->Release();  pipelineState_  = nullptr; }
    if (rootSignature_) { rootSignature_->Release();  rootSignature_  = nullptr; }
    if (commandList_)   { commandList_->Release();    commandList_    = nullptr; }
    if (fence_)         { fence_->Release();           fence_          = nullptr; }
    if (rtvHeap_)       { rtvHeap_->Release();         rtvHeap_        = nullptr; }
    if (swapChain_)     { swapChain_->Release();       swapChain_      = nullptr; }
    if (commandQueue_)  { commandQueue_->Release();    commandQueue_   = nullptr; }
    if (device_)        { device_->Release();           device_         = nullptr; }
}

DX12Canvas::~DX12Canvas() { shutdown(); }

// ── Frame management ──────────────────────────────────────────────────────────

void DX12Canvas::beginFrame(uint32_t width, uint32_t height) {
    width_  = width;
    height_ = height;
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

    ThrowIfFailed(commandAllocators_[frameIndex_]->Reset());
    ThrowIfFailed(commandList_->Reset(commandAllocators_[frameIndex_],
                                       pipelineState_));

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = renderTargets_[frameIndex_];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList_->ResourceBarrier(1, &barrier);

    UINT rtvSize = device_->GetDescriptorHandleIncrementSize(
                       D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += frameIndex_ * rtvSize;

    commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    D3D12_VIEWPORT viewport{0, 0, static_cast<float>(width),
                             static_cast<float>(height), 0, 1};
    D3D12_RECT scissor{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    commandList_->RSSetViewports(1, &viewport);
    commandList_->RSSetScissorRects(1, &scissor);
}

void DX12Canvas::endFrame() {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = renderTargets_[frameIndex_];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &barrier);
    ThrowIfFailed(commandList_->Close());

    ID3D12CommandList* lists[] = {commandList_};
    commandQueue_->ExecuteCommandLists(1, lists);
    ThrowIfFailed(swapChain_->Present(1, 0));
    flushAndWait();
}

void DX12Canvas::flushAndWait() {
    const uint64_t value = fenceValues_[frameIndex_];
    ThrowIfFailed(commandQueue_->Signal(fence_, value));
    fenceValues_[frameIndex_]++;

    if (fence_->GetCompletedValue() < value) {
        HANDLE ev = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(fence_->SetEventOnCompletion(value, ev));
        WaitForSingleObjectEx(ev, INFINITE, FALSE);
        CloseHandle(ev);
    }
}

// ── Draw primitives ────────────────────────────────────────────────────────────

Point DX12Canvas::currentTranslation() const {
    return translateStack_.empty() ? Point{0, 0} : translateStack_.top();
}

void DX12Canvas::uploadQuad(const Rect& rect, const Color& color) {
    auto t = currentTranslation();
    float x = rect.x + t.x, y = rect.y + t.y;
    float r = x + rect.width, b = y + rect.height;
    float cr = color.rf(), cg = color.gf(), cb = color.bf(), ca = color.af();

    // 6 vertices for 2 triangles (CCW).
    struct Vertex { float x, y, r, g, b, a; };
    Vertex verts[6] = {
        {x, y, cr, cg, cb, ca}, {r, y, cr, cg, cb, ca}, {x, b, cr, cg, cb, ca},
        {r, y, cr, cg, cb, ca}, {r, b, cr, cg, cb, ca}, {x, b, cr, cg, cb, ca},
    };

    // Upload to a transient upload heap.
    const UINT dataSize = sizeof(verts);
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC rd{};
    rd.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width            = dataSize;
    rd.Height           = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels        = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* vb = nullptr;
    ThrowIfFailed(device_->CreateCommittedResource(
        &hp, D3D12_HEAP_FLAG_NONE, &rd,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vb)));

    void* mapped = nullptr;
    ThrowIfFailed(vb->Map(0, nullptr, &mapped));
    std::memcpy(mapped, verts, dataSize);
    vb->Unmap(0, nullptr);

    // Set transform constants (invViewport).
    float cbData[2] = {2.0f / static_cast<float>(width_),
                        2.0f / static_cast<float>(height_)};

    commandList_->SetGraphicsRootSignature(rootSignature_);
    commandList_->SetPipelineState(pipelineState_);
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vbv{vb->GetGPUVirtualAddress(), dataSize,
                                  sizeof(Vertex)};
    commandList_->IASetVertexBuffers(0, 1, &vbv);
    commandList_->DrawInstanced(6, 1, 0, 0);

    // Release immediately (safe after flush in endFrame).
    vb->Release();
}

void DX12Canvas::drawRect(const Rect& rect, const Color& color) {
    uploadQuad(rect, color);
}

void DX12Canvas::drawRectOutline(const Rect& rect, const Stroke& stroke) {
    // Draw 4 thin quads forming the border.
    float w = stroke.width;
    drawRect({rect.x,              rect.y,               rect.width, w},       stroke.color);
    drawRect({rect.x,              rect.bottom() - w,    rect.width, w},       stroke.color);
    drawRect({rect.x,              rect.y + w,           w, rect.height - 2*w}, stroke.color);
    drawRect({rect.right() - w,    rect.y + w,           w, rect.height - 2*w}, stroke.color);
}

void DX12Canvas::drawRoundedRect(const Rect& rect, const BorderRadius& /*radius*/,
                                   const Color& color) {
    // For the GPU back-end a pixel-shader SDF approach would be used.
    // This fallback fills a plain rectangle (to keep the implementation minimal).
    uploadQuad(rect, color);
}

void DX12Canvas::drawCircle(const Point& center, float radius, const Color& fill) {
    Rect r{center.x - radius, center.y - radius, radius * 2, radius * 2};
    uploadQuad(r, fill);
}

void DX12Canvas::drawLine(const Point& a, const Point& b, const Stroke& stroke) {
    // Approximate a line with a thin rectangle.
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 1e-4f) return;
    Rect r{std::min(a.x, b.x), std::min(a.y, b.y), len, stroke.width};
    uploadQuad(r, stroke.color);
}

void DX12Canvas::drawText(const std::string& /*text*/, const Rect& rect,
                            const Font& /*font*/, const Color& color,
                            TextAlign /*align*/) {
    // Text rendering requires a glyph atlas and SDF font pipeline.
    // As a placeholder, draw a coloured rectangle.
    // Full implementation would use DirectWrite + a custom SDF pipeline.
    uploadQuad(rect, color.withAlpha(60));
}

Size DX12Canvas::measureText(const std::string& text, const Font& font) const {
    return {static_cast<float>(text.size()) * font.size * 0.6f, font.size * 1.4f};
}

// ── Clip & transform ─────────────────────────────────────────────────────────

void DX12Canvas::pushClipRect(const Rect& rect) {
    clipStack_.push(rect);
    D3D12_RECT sr{static_cast<LONG>(rect.x), static_cast<LONG>(rect.y),
                  static_cast<LONG>(rect.right()), static_cast<LONG>(rect.bottom())};
    commandList_->RSSetScissorRects(1, &sr);
}
void DX12Canvas::popClipRect() {
    if (!clipStack_.empty()) clipStack_.pop();
    D3D12_RECT sr{0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
    commandList_->RSSetScissorRects(1, &sr);
}

void DX12Canvas::pushTranslate(float dx, float dy) {
    Point cur = currentTranslation();
    translateStack_.push({cur.x + dx, cur.y + dy});
}
void DX12Canvas::popTransform() {
    if (!translateStack_.empty()) translateStack_.pop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Win32Window
// ─────────────────────────────────────────────────────────────────────────────

LRESULT CALLBACK Win32Window::wndProc(HWND hwnd, UINT msg,
                                       WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<Win32Window*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_SIZE:
        if (self) {
            self->width_  = LOWORD(lp);
            self->height_ = HIWORD(lp);
            if (self->resizeCb_) self->resizeCb_(self->width_, self->height_);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool Win32Window::open(const WindowConfig& config) {
    width_  = config.width;
    height_ = config.height;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"AIGUIWindow";
    RegisterClassExW(&wc);

    std::wstring title(config.title.begin(), config.title.end());
    hwnd_ = CreateWindowExW(
        0, L"AIGUIWindow", title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(width_), static_cast<int>(height_),
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

    if (!hwnd_) return false;
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    canvas_ = std::make_unique<DX12Canvas>();
    if (!canvas_->init(hwnd_, width_, height_)) return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

bool Win32Window::pollEvents() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            hwnd_ = nullptr;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

void Win32Window::present() {
    // endFrame() inside DX12Canvas already calls Present().
}

void Win32Window::close() {
    if (hwnd_) {
        canvas_->shutdown();
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

Win32Window::~Win32Window() { close(); }

} // namespace aigui

#endif // AIGUI_PLATFORM_WINDOWS
