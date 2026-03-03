#pragma once
#include "../widget/widget.hpp"
#include "../renderer/canvas.hpp"
#include <functional>
#include <string>

namespace aigui {

/// Edge-insets for padding.
struct EdgeInsets {
    float left{0}, top{0}, right{0}, bottom{0};

    static constexpr EdgeInsets all(float v)           { return {v, v, v, v}; }
    static constexpr EdgeInsets symmetric(float h, float v) { return {h, v, h, v}; }
    static constexpr EdgeInsets only(float l=0, float t=0, float r=0, float b=0) {
        return {l, t, r, b};
    }
    static constexpr EdgeInsets zero() { return {0, 0, 0, 0}; }

    constexpr float horizontal() const { return left + right; }
    constexpr float vertical()   const { return top  + bottom; }
};

/// Background decoration applied to a Container.
struct Decoration {
    Color        color{Color::transparent()};
    BorderRadius borderRadius{BorderRadius::zero()};
    Stroke       border{Color::transparent(), 0.0f};

    static Decoration filled(Color c)    { Decoration d; d.color = c; return d; }
    static Decoration rounded(Color c, float r) {
        Decoration d; d.color = c; d.borderRadius = BorderRadius::all(r); return d;
    }
};

/// A box widget that can hold a single child, apply padding, decoration, or
/// constrain its own size – similar to Flutter's Container.
class Container : public Widget {
public:
    explicit Container(WidgetPtr child = nullptr) : child_(std::move(child)) {}

    // Builder-style configuration
    Container& withChild(WidgetPtr child)     { child_ = std::move(child); return *this; }
    Container& withPadding(EdgeInsets p)      { padding_   = p;            return *this; }
    Container& withDecoration(Decoration d)   { decoration_= d;            return *this; }
    Container& withWidth(float w)             { fixedW_    = w;            return *this; }
    Container& withHeight(float h)            { fixedH_    = h;            return *this; }
    Container& withSize(float w, float h)     { fixedW_=w; fixedH_=h;      return *this; }
    Container& withColor(Color c)             { decoration_.color = c;     return *this; }

    Size layout(const Constraints& constraints) override {
        Constraints inner = constraints.deflate(padding_.horizontal(),
                                                padding_.vertical());
        if (fixedW_ >= 0) {
            inner.minWidth  = fixedW_;
            inner.maxWidth  = fixedW_;
        }
        if (fixedH_ >= 0) {
            inner.minHeight = fixedH_;
            inner.maxHeight = fixedH_;
        }

        Size childSize{};
        if (child_) {
            child_->setOffset({padding_.left, padding_.top});
            childSize = child_->layout(inner);
            children_ = {child_};
        }

        float w = fixedW_ >= 0 ? fixedW_ : childSize.width  + padding_.horizontal();
        float h = fixedH_ >= 0 ? fixedH_ : childSize.height + padding_.vertical();
        size_ = constraints.constrain({w, h});
        return size_;
    }

    void paint(Canvas& canvas) override {
        const Rect selfRect{{0, 0}, size_};

        if (decoration_.color != Color::transparent() ||
            decoration_.border.width > 0) {
            canvas.drawRoundedRect(selfRect, decoration_.borderRadius,
                                   decoration_.color);
            if (decoration_.border.width > 0) {
                canvas.drawRectOutline(selfRect, decoration_.border);
            }
        }
        paintChildren(canvas);
    }

private:
    WidgetPtr  child_;
    EdgeInsets padding_{EdgeInsets::zero()};
    Decoration decoration_;
    float      fixedW_{-1};
    float      fixedH_{-1};
};

/// Simple padding widget – wraps a single child with uniform padding.
class Padding : public Widget {
public:
    Padding(EdgeInsets insets, WidgetPtr child)
        : insets_(insets), child_(std::move(child)) {}

    Size layout(const Constraints& constraints) override {
        Constraints inner = constraints.deflate(insets_.horizontal(),
                                                insets_.vertical());
        Size childSize{};
        if (child_) {
            child_->setOffset({insets_.left, insets_.top});
            childSize = child_->layout(inner);
            children_ = {child_};
        }
        size_ = constraints.constrain(
            {childSize.width + insets_.horizontal(),
             childSize.height + insets_.vertical()});
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    EdgeInsets insets_;
    WidgetPtr  child_;
};

} // namespace aigui
