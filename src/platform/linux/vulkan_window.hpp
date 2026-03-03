#pragma once
#include <aigui/core/platform.hpp>

#ifdef AIGUI_PLATFORM_LINUX

#include <aigui/renderer/canvas.hpp>
#include <aigui/app/window.hpp>

// Minimal Vulkan / X11 forward declarations.
// The actual Vulkan headers are only included in the .cpp file.
using VkInstance_T           = struct VkInstance_T;
using VkPhysicalDevice_T     = struct VkPhysicalDevice_T;
using VkDevice_T             = struct VkDevice_T;
using VkQueue_T              = struct VkQueue_T;
using VkSurfaceKHR_T         = struct VkSurfaceKHR_T;
using VkSwapchainKHR_T       = struct VkSwapchainKHR_T;
using VkRenderPass_T         = struct VkRenderPass_T;
using VkFramebuffer_T        = struct VkFramebuffer_T;
using VkCommandPool_T        = struct VkCommandPool_T;
using VkCommandBuffer_T      = struct VkCommandBuffer_T;
using VkSemaphore_T          = struct VkSemaphore_T;
using VkFence_T              = struct VkFence_T;
using VkPipeline_T           = struct VkPipeline_T;
using VkPipelineLayout_T     = struct VkPipelineLayout_T;
using VkBuffer_T             = struct VkBuffer_T;
using VkDeviceMemory_T       = struct VkDeviceMemory_T;
using VkShaderModule_T       = struct VkShaderModule_T;
using VkDescriptorSetLayout_T= struct VkDescriptorSetLayout_T;
using VkDescriptorPool_T     = struct VkDescriptorPool_T;
using VkDescriptorSet_T      = struct VkDescriptorSet_T;

using VkInstance          = VkInstance_T*;
using VkPhysicalDevice    = VkPhysicalDevice_T*;
using VkDevice            = VkDevice_T*;
using VkQueue             = VkQueue_T*;
using VkSurfaceKHR        = VkSurfaceKHR_T*;
using VkSwapchainKHR      = VkSwapchainKHR_T*;
using VkRenderPass        = VkRenderPass_T*;
using VkFramebuffer       = VkFramebuffer_T*;
using VkCommandPool       = VkCommandPool_T*;
using VkCommandBuffer     = VkCommandBuffer_T*;
using VkSemaphore         = VkSemaphore_T*;
using VkFence             = VkFence_T*;
using VkPipeline          = VkPipeline_T*;
using VkPipelineLayout    = VkPipelineLayout_T*;
using VkBuffer            = VkBuffer_T*;
using VkDeviceMemory      = VkDeviceMemory_T*;
using VkShaderModule      = VkShaderModule_T*;
using VkDescriptorSetLayout = VkDescriptorSetLayout_T*;
using VkDescriptorPool    = VkDescriptorPool_T*;
using VkDescriptorSet     = VkDescriptorSet_T*;

struct Display;  // X11 forward declaration

#include <cstdint>
#include <memory>
#include <vector>
#include <stack>
#include <string>

namespace aigui {

static constexpr uint32_t kVkFrameCount = 2;

/// Vulkan Canvas implementation.
///
/// Emits GPU draw calls via Vulkan command buffers.  Uses a simple
/// triangle-list pipeline for 2-D geometry rendering.
class VulkanCanvas : public Canvas {
public:
    VulkanCanvas() = default;
    ~VulkanCanvas() override;

    /// Initialise Vulkan instance, device, swap chain and render pass.
    bool init(Display* display, uint64_t xwindow,
              uint32_t width, uint32_t height);

    /// Release all Vulkan resources.
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

private:
    // Vulkan objects
    VkInstance       instance_{nullptr};
    VkPhysicalDevice physicalDevice_{nullptr};
    VkDevice         device_{nullptr};
    VkQueue          graphicsQueue_{nullptr};
    VkSurfaceKHR     surface_{nullptr};
    VkSwapchainKHR   swapChain_{nullptr};
    VkRenderPass     renderPass_{nullptr};
    VkPipelineLayout pipelineLayout_{nullptr};
    VkPipeline       pipeline_{nullptr};
    VkCommandPool    commandPool_{nullptr};

    std::vector<VkFramebuffer>    framebuffers_;
    std::vector<VkCommandBuffer>  commandBuffers_;
    VkSemaphore imageAvailableSem_{nullptr};
    VkSemaphore renderFinishedSem_{nullptr};
    VkFence     inFlightFence_{nullptr};

    uint32_t graphicsFamily_{0};
    uint32_t swapImageIndex_{0};
    uint32_t width_{0}, height_{0};

    std::stack<Rect>  clipStack_;
    std::stack<Point> translateStack_;

    bool createInstance();
    bool createSurface(Display* display, uint64_t xwindow);
    bool createDevice();
    bool createSwapChain(uint32_t width, uint32_t height);
    bool createRenderPass();
    bool createPipeline();
    bool createFramebuffers();
    bool createCommandBuffers();
    bool createSyncObjects();

    Point currentTranslation() const;
    void  recordQuad(const Rect& rect, const Color& color);
    void  beginCommandBuffer();
};

/// X11 + Vulkan window implementation.
class X11VulkanWindow : public Window {
public:
    X11VulkanWindow() = default;
    ~X11VulkanWindow() override;

    bool open(const WindowConfig& config) override;
    void close() override;
    bool isOpen() const override;
    bool pollEvents() override;
    Canvas& canvas() override { return *canvas_; }
    void    present() override;

private:
    Display*                     display_{nullptr};
    uint64_t                     xwindow_{0};
    std::unique_ptr<VulkanCanvas> canvas_;
    uint32_t                     width_{0}, height_{0};
    bool                         open_{false};
};

} // namespace aigui

#endif // AIGUI_PLATFORM_LINUX
