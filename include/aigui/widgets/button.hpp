#pragma once
#include "../widget/widget.hpp"
#include "../renderer/canvas.hpp"
#include <functional>
#include <string>

namespace aigui {

/// Button visual states.
enum class ButtonState { Normal, Hovered, Pressed, Disabled };

/// A clickable button widget.
class Button : public Widget {
public:
    using ClickCallback = std::function<void()>;

    explicit Button(std::string label, ClickCallback onClick = nullptr)
        : label_(std::move(label)), onClick_(std::move(onClick)) {}

    // Builder setters
    Button& withLabel(std::string l)         { label_  = std::move(l); return *this; }
    Button& withOnClick(ClickCallback cb)    { onClick_= std::move(cb); return *this; }
    Button& withEnabled(bool enabled)        { enabled_= enabled;       return *this; }
    Button& withFont(Font f)                 { font_   = std::move(f);  return *this; }

    Button& withColors(Color bg, Color fg, Color hover, Color pressed) {
        bgColor_      = bg;
        fgColor_      = fg;
        hoverColor_   = hover;
        pressedColor_ = pressed;
        return *this;
    }

    Size layout(const Constraints& constraints) override {
        // Natural size: text width + padding.
        constexpr float padH = 24.0f;
        constexpr float padV = 12.0f;
        float textW = static_cast<float>(label_.size()) * font_.size * 0.6f; // approx
        float textH = font_.size * 1.4f;
        size_ = constraints.constrain({textW + padH * 2.0f, textH + padV * 2.0f});
        return size_;
    }

    void paint(Canvas& canvas) override {
        Rect rect{{0, 0}, size_};

        Color bg = currentBg();
        canvas.drawRoundedRect(rect, BorderRadius::all(6.0f), bg);

        Font labelFont = font_;
        canvas.drawText(label_, rect, labelFont, fgColor_, TextAlign::Center);
    }

    bool hitTest(const Point& windowPt, const Point& myOrigin) const override {
        Rect r{myOrigin, size_};
        return r.contains(windowPt) && enabled_;
    }

    void onPointerDown(const Point&) override {
        state_ = ButtonState::Pressed;
        if (onClick_ && enabled_) onClick_();
    }
    void onPointerUp(const Point&) override {
        state_ = ButtonState::Hovered;
    }

private:
    std::string   label_;
    ClickCallback onClick_;
    bool          enabled_{true};
    ButtonState   state_{ButtonState::Normal};
    Font          font_{};
    Color         bgColor_{Color::blue()};
    Color         fgColor_{Color::white()};
    Color         hoverColor_{Color(30, 100, 210)};
    Color         pressedColor_{Color(10, 70, 180)};

    Color currentBg() const {
        if (!enabled_) return Color::gray();
        switch (state_) {
        case ButtonState::Hovered:  return hoverColor_;
        case ButtonState::Pressed:  return pressedColor_;
        default:                    return bgColor_;
        }
    }
};

} // namespace aigui
