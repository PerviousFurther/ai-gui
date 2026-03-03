#pragma once
#include "../widget/widget.hpp"
#include "../renderer/canvas.hpp"
#include <string>

namespace aigui {

/// A widget that displays a line of styled text.
class Text : public Widget {
public:
    explicit Text(std::string text,
                  Font        font  = Font{},
                  Color       color = Color::black())
        : text_(std::move(text)), font_(std::move(font)), color_(color) {}

    // Builder-style setters
    Text& withFont(Font f)       { font_  = std::move(f); return *this; }
    Text& withColor(Color c)     { color_ = c;            return *this; }
    Text& withAlign(TextAlign a) { align_ = a;            return *this; }

    Size layout(const Constraints& constraints) override {
        // The natural size of the text; respect the max-width constraint.
        // (Canvas::measureText is called at paint time – we approximate here.)
        naturalSize_ = {constraints.maxWidth, font_.size * 1.4f};
        size_        = constraints.constrain(naturalSize_);
        return size_;
    }

    void paint(Canvas& canvas) override {
        canvas.drawText(text_, Rect{{0, 0}, size_}, font_, color_, align_);
    }

    void setText(std::string t) { text_ = std::move(t); }

private:
    std::string text_;
    Font        font_;
    Color       color_;
    TextAlign   align_{TextAlign::Left};
    Size        naturalSize_{};
};

} // namespace aigui
