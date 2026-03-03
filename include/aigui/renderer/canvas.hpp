#pragma once
#include "../core/color.hpp"
#include "../core/rect.hpp"
#include "../core/point.hpp"
#include <string>
#include <cstdint>

namespace aigui {

/// Text alignment options.
enum class TextAlign { Left, Center, Right };

/// Font weight.
enum class FontWeight { Normal, Bold };

/// Font descriptor.
struct Font {
    std::string  family{"sans-serif"};
    float        size{14.0f};
    FontWeight   weight{FontWeight::Normal};
    bool         italic{false};

    Font() = default;
    Font(std::string family, float size, FontWeight w = FontWeight::Normal, bool italic = false)
        : family(std::move(family)), size(size), weight(w), italic(italic) {}
};

/// Border-radius (uniform).
struct BorderRadius {
    float topLeft{0}, topRight{0}, bottomRight{0}, bottomLeft{0};

    static constexpr BorderRadius all(float r) { return {r, r, r, r}; }
    static constexpr BorderRadius zero()       { return {0, 0, 0, 0}; }
};

/// Stroke style.
struct Stroke {
    Color color;
    float width{1.0f};
};

/// Abstract 2-D drawing canvas.
///
/// The renderer back-end implements this interface for DX12 (Windows) and
/// Vulkan (Linux).  Widget::paint() receives a Canvas& and emits draw calls
/// without any knowledge of the underlying GPU API.
class Canvas {
public:
    virtual ~Canvas() = default;

    // ── State ──────────────────────────────────────────────────────────────

    /// Push a clip rectangle (all subsequent draws are clipped to it).
    virtual void pushClipRect(const Rect& rect) = 0;
    /// Pop the most recent clip rectangle.
    virtual void popClipRect() = 0;

    /// Push a translation transform.
    virtual void pushTranslate(float dx, float dy) = 0;
    /// Pop the most recent transform.
    virtual void popTransform() = 0;

    // ── Drawing primitives ────────────────────────────────────────────────

    /// Fill a rectangle with a solid colour.
    virtual void drawRect(const Rect& rect, const Color& color) = 0;

    /// Draw a rectangle outline.
    virtual void drawRectOutline(const Rect& rect, const Stroke& stroke) = 0;

    /// Fill a rounded rectangle.
    virtual void drawRoundedRect(const Rect& rect, const BorderRadius& radius,
                                 const Color& color) = 0;

    /// Draw a circle.
    virtual void drawCircle(const Point& center, float radius,
                            const Color& fill) = 0;

    /// Draw a line segment.
    virtual void drawLine(const Point& a, const Point& b,
                          const Stroke& stroke) = 0;

    /// Draw a single-line text string.
    virtual void drawText(const std::string& text, const Rect& rect,
                          const Font& font, const Color& color,
                          TextAlign align = TextAlign::Left) = 0;

    /// Measure the rendered size of a text string (no clipping).
    virtual Size measureText(const std::string& text, const Font& font) const = 0;

    // ── Frame boundaries ──────────────────────────────────────────────────

    /// Called once per frame before any draw calls.
    virtual void beginFrame(uint32_t width, uint32_t height) = 0;

    /// Called once per frame after all draw calls – flushes to GPU.
    virtual void endFrame() = 0;
};

} // namespace aigui
