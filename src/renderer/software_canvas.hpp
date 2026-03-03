#pragma once
#include <aigui/renderer/canvas.hpp>
#include <vector>
#include <stack>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace aigui {

/// A draw-command record (for inspection in tests).
struct DrawCommand {
    enum class Kind {
        BeginFrame, EndFrame,
        PushClip,  PopClip,
        PushTranslate, PopTransform,
        DrawRect, DrawRectOutline, DrawRoundedRect,
        DrawCircle, DrawLine, DrawText
    };

    Kind        kind;
    Rect        rect;
    Color       color;
    Stroke      stroke;
    BorderRadius borderRadius;
    Point       p1, p2;
    float       radius{0};
    std::string text;
    Font        font;
    TextAlign   align{TextAlign::Left};
    uint32_t    frameWidth{0}, frameHeight{0};
};

/// Lightweight software canvas that records draw commands.
/// Used for unit-testing the widget/layout pipeline without a GPU.
class SoftwareCanvas : public Canvas {
public:
    SoftwareCanvas() = default;

    // ── Canvas interface ──────────────────────────────────────────────────

    void pushClipRect(const Rect& rect) override {
        clipStack_.push(rect);
        record(DrawCommand{DrawCommand::Kind::PushClip, rect});
    }
    void popClipRect() override {
        if (!clipStack_.empty()) clipStack_.pop();
        record(DrawCommand{DrawCommand::Kind::PopClip});
    }

    void pushTranslate(float dx, float dy) override {
        Point cur = transformStack_.empty() ? Point{0, 0} : transformStack_.top();
        transformStack_.push({cur.x + dx, cur.y + dy});
        DrawCommand cmd;
        cmd.kind = DrawCommand::Kind::PushTranslate;
        cmd.p1   = {dx, dy};
        record(cmd);
    }
    void popTransform() override {
        if (!transformStack_.empty()) transformStack_.pop();
        record(DrawCommand{DrawCommand::Kind::PopTransform});
    }

    void drawRect(const Rect& rect, const Color& color) override {
        DrawCommand cmd;
        cmd.kind  = DrawCommand::Kind::DrawRect;
        cmd.rect  = transformed(rect);
        cmd.color = color;
        record(cmd);
        rasterize(cmd.rect, color);
    }

    void drawRectOutline(const Rect& rect, const Stroke& stroke) override {
        DrawCommand cmd;
        cmd.kind   = DrawCommand::Kind::DrawRectOutline;
        cmd.rect   = transformed(rect);
        cmd.stroke = stroke;
        record(cmd);
    }

    void drawRoundedRect(const Rect& rect, const BorderRadius& radius,
                         const Color& color) override {
        DrawCommand cmd;
        cmd.kind         = DrawCommand::Kind::DrawRoundedRect;
        cmd.rect         = transformed(rect);
        cmd.borderRadius = radius;
        cmd.color        = color;
        record(cmd);
        rasterize(cmd.rect, color);
    }

    void drawCircle(const Point& center, float radius,
                    const Color& fill) override {
        DrawCommand cmd;
        cmd.kind   = DrawCommand::Kind::DrawCircle;
        cmd.p1     = translate(center);
        cmd.radius = radius;
        cmd.color  = fill;
        record(cmd);
    }

    void drawLine(const Point& a, const Point& b, const Stroke& stroke) override {
        DrawCommand cmd;
        cmd.kind   = DrawCommand::Kind::DrawLine;
        cmd.p1     = translate(a);
        cmd.p2     = translate(b);
        cmd.stroke = stroke;
        record(cmd);
    }

    void drawText(const std::string& text, const Rect& rect,
                  const Font& font, const Color& color,
                  TextAlign align = TextAlign::Left) override {
        DrawCommand cmd;
        cmd.kind  = DrawCommand::Kind::DrawText;
        cmd.text  = text;
        cmd.rect  = transformed(rect);
        cmd.font  = font;
        cmd.color = color;
        cmd.align = align;
        record(cmd);
    }

    Size measureText(const std::string& text, const Font& font) const override {
        // Approximate measurement (real back-end does glyph metrics).
        return {static_cast<float>(text.size()) * font.size * 0.6f,
                font.size * 1.4f};
    }

    void beginFrame(uint32_t width, uint32_t height) override {
        commands_.clear();
        framebuffer_.assign(static_cast<size_t>(width) * height, Color::transparent());
        fbWidth_  = width;
        fbHeight_ = height;
        DrawCommand cmd;
        cmd.kind        = DrawCommand::Kind::BeginFrame;
        cmd.frameWidth  = width;
        cmd.frameHeight = height;
        record(cmd);
    }

    void endFrame() override {
        record(DrawCommand{DrawCommand::Kind::EndFrame});
    }

    // ── Test helpers ──────────────────────────────────────────────────────

    const std::vector<DrawCommand>& commands() const { return commands_; }

    /// Number of draw commands of the given kind.
    size_t countOf(DrawCommand::Kind k) const {
        return static_cast<size_t>(
            std::count_if(commands_.begin(), commands_.end(),
                          [k](const DrawCommand& c){ return c.kind == k; }));
    }

    /// Return the colour at a given framebuffer pixel (after rasterization).
    Color pixelAt(uint32_t x, uint32_t y) const {
        if (x >= fbWidth_ || y >= fbHeight_) return Color::transparent();
        return framebuffer_[y * fbWidth_ + x];
    }

    uint32_t frameWidth()  const { return fbWidth_;  }
    uint32_t frameHeight() const { return fbHeight_; }

    void clear() { commands_.clear(); }

private:
    std::vector<DrawCommand> commands_;

    // Minimal software rasterizer for pixel-level testing.
    std::vector<Color> framebuffer_;
    uint32_t fbWidth_{0}, fbHeight_{0};

    std::stack<Rect>  clipStack_;
    std::stack<Point> transformStack_;

    void record(DrawCommand cmd) { commands_.push_back(std::move(cmd)); }

    Point currentTranslation() const {
        return transformStack_.empty() ? Point{0, 0} : transformStack_.top();
    }

    Point translate(const Point& p) const {
        auto t = currentTranslation();
        return {p.x + t.x, p.y + t.y};
    }

    Rect transformed(const Rect& r) const {
        auto t = currentTranslation();
        return {r.x + t.x, r.y + t.y, r.width, r.height};
    }

    /// Minimal axis-aligned rect fill into the software framebuffer.
    void rasterize(const Rect& rect, const Color& color) {
        if (fbWidth_ == 0 || fbHeight_ == 0) return;
        int x0 = std::max(0, static_cast<int>(rect.x));
        int y0 = std::max(0, static_cast<int>(rect.y));
        int x1 = std::min(static_cast<int>(fbWidth_),
                          static_cast<int>(rect.x + rect.width));
        int y1 = std::min(static_cast<int>(fbHeight_),
                          static_cast<int>(rect.y + rect.height));
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                framebuffer_[static_cast<size_t>(y) * fbWidth_ + x] = color;
            }
        }
    }
};

} // namespace aigui
