#pragma once
#include <aigui/core/platform.hpp>

#ifdef AIGUI_PLATFORM_WINDOWS

#include <aigui/renderer/canvas.hpp>
#include <aigui/app/window.hpp>

// Minimal Windows / DX12 forward declarations to keep this header
// self-contained even when the DX12 SDK is not installed.
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct IDXGISwapChain3;
struct ID3D12Fence;
struct ID3D12CommandAllocator;
struct ID3D12PipelineState;
struct ID3D12RootSignature;

#ifndef AIGUI_BACKEND_ONLY   // guard against double-inclusion of Windows.h
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <Windows.h>
#endif

#include <cstdint>
#include <memory>
#include <vector>
#include <stack>
#include <string>

namespace aigui {

static constexpr uint32_t kDX12FrameCount = 2;

/// DirectX 12 Canvas implementation.
///
/// Emits draw calls through a DX12 command list.  The geometry is rendered
/// via a simple 2-D rendering pipeline (full screen quads + SDF fonts).
class DX12Canvas : public Canvas {
public:
    DX12Canvas() = default;
    ~DX12Canvas() override;

    /// Initialise the DX12 device, swap chain and PSO.
    bool init(HWND hwnd, uint32_t width, uint32_t height);

    /// Release all GPU resources.
    void shutdown();

    // ── Canvas interface ──────────────────────────────────────────────────
    void pushClipRect(const Rect& rect) override;
    void popClipRect() override;
    void pushTranslate(float dx, float dy) override;
    void popTransform() override;

    void drawRect(const Rect& rect, const Color& color) override;
    void drawRectOutline(const Rect& rect, const Stroke& stroke) override;
    void drawRoundedRect(const Rect& rect, const BorderRadius& radius,
                         const Color& color) override;
    void drawCircle(const Point& center, float radius, const Color& fill) override;
    void drawLine(const Point& a, const Point& b, const Stroke& stroke) override;
    void drawText(const std::string& text, const Rect& rect,
                  const Font& font, const Color& color,
                  TextAlign align = TextAlign::Left) override;
    Size measureText(const std::string& text, const Font& font) const override;

    void beginFrame(uint32_t width, uint32_t height) override;
    void endFrame() override;

    // ── DX12-specific ─────────────────────────────────────────────────────
    IDXGISwapChain3*           swapChain()    const { return swapChain_; }
    ID3D12Device*              device()       const { return device_; }
    ID3D12GraphicsCommandList* commandList()  const { return commandList_; }

private:
    // Core DX12 objects (raw COM pointers – lifetime managed manually).
    IDXGISwapChain3*           swapChain_{nullptr};
    ID3D12Device*              device_{nullptr};
    ID3D12CommandQueue*        commandQueue_{nullptr};
    ID3D12CommandAllocator*    commandAllocators_[kDX12FrameCount]{};
    ID3D12GraphicsCommandList* commandList_{nullptr};
    ID3D12DescriptorHeap*      rtvHeap_{nullptr};
    ID3D12Resource*            renderTargets_[kDX12FrameCount]{};
    ID3D12Fence*               fence_{nullptr};
    ID3D12PipelineState*       pipelineState_{nullptr};
    ID3D12RootSignature*       rootSignature_{nullptr};

    uint64_t fenceValues_[kDX12FrameCount]{};
    uint32_t frameIndex_{0};
    uint32_t width_{0}, height_{0};

    // CPU-side transform/clip stacks.
    std::stack<Rect>  clipStack_;
    std::stack<Point> translateStack_;

    // Geometry upload helpers.
    void flushAndWait();
    bool createPipeline();
    void uploadQuad(const Rect& rect, const Color& color);
    Point currentTranslation() const;
};

/// Win32 + DX12 window implementation.
class Win32Window : public Window {
public:
    Win32Window() = default;
    ~Win32Window() override;

    bool open(const WindowConfig& config) override;
    void close() override;
    bool isOpen()    const override { return hwnd_ != nullptr; }
    bool pollEvents() override;

    Canvas& canvas() override { return *canvas_; }
    void    present() override;

private:
    HWND                     hwnd_{nullptr};
    std::unique_ptr<DX12Canvas> canvas_;
    uint32_t                 width_{0}, height_{0};

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg,
                                    WPARAM wp, LPARAM lp);
};

} // namespace aigui

#endif // AIGUI_PLATFORM_WINDOWS
