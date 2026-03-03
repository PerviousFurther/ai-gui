#pragma once
#include "../core/size.hpp"
#include "../core/color.hpp"
#include "../renderer/canvas.hpp"
#include "../widget/widget.hpp"
#include <string>
#include <memory>
#include <functional>

namespace aigui {

/// Configuration for a top-level window.
struct WindowConfig {
    std::string title{"AI-GUI Window"};
    uint32_t    width{1280};
    uint32_t    height{720};
    bool        resizable{true};
    bool        decorated{true};
    Color       clearColor{Color::darkGray()};
};

/// Abstract window.  Platform back-ends (Win32+DX12, X11+Vulkan) derive from this.
class Window {
public:
    virtual ~Window() = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────
    virtual bool open(const WindowConfig& config) = 0;
    virtual void close()                          = 0;
    virtual bool isOpen() const                   = 0;

    // ── Event pump ────────────────────────────────────────────────────────
    /// Process pending OS messages.  Returns false when the window is closed.
    virtual bool pollEvents() = 0;

    // ── Rendering ─────────────────────────────────────────────────────────
    virtual Canvas& canvas()              = 0;
    virtual void    present()             = 0;

    // ── Resize callback ───────────────────────────────────────────────────
    using ResizeCallback = std::function<void(uint32_t w, uint32_t h)>;
    void setResizeCallback(ResizeCallback cb) { resizeCb_ = std::move(cb); }

    // ── Root widget ───────────────────────────────────────────────────────
    void setRoot(WidgetPtr root) { root_ = std::move(root); }

    /// Perform a full layout + paint cycle.
    void render(uint32_t width, uint32_t height) {
        if (!root_) return;

        auto& c = canvas();
        c.beginFrame(width, height);
        c.pushClipRect(Rect{0, 0, static_cast<float>(width),
                                   static_cast<float>(height)});

        root_->layout(Constraints::tight(static_cast<float>(width),
                                         static_cast<float>(height)));
        root_->paint(c);

        c.popClipRect();
        c.endFrame();
    }

protected:
    WidgetPtr      root_;
    ResizeCallback resizeCb_;
};

} // namespace aigui
