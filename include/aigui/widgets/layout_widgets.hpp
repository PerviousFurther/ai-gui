#pragma once
#include "../widget/widget.hpp"
#include "../renderer/canvas.hpp"

namespace aigui {

/// Alignment for Center and Align widgets.
struct Alignment {
    float x{0.0f};  ///< -1 = left/top, 0 = center, 1 = right/bottom
    float y{0.0f};

    static constexpr Alignment center()      { return { 0.0f,  0.0f}; }
    static constexpr Alignment topLeft()     { return {-1.0f, -1.0f}; }
    static constexpr Alignment topCenter()   { return { 0.0f, -1.0f}; }
    static constexpr Alignment topRight()    { return { 1.0f, -1.0f}; }
    static constexpr Alignment centerLeft()  { return {-1.0f,  0.0f}; }
    static constexpr Alignment centerRight() { return { 1.0f,  0.0f}; }
    static constexpr Alignment bottomLeft()  { return {-1.0f,  1.0f}; }
    static constexpr Alignment bottomCenter(){ return { 0.0f,  1.0f}; }
    static constexpr Alignment bottomRight() { return { 1.0f,  1.0f}; }
};

/// Centers its single child within the available space.
class Center : public Widget {
public:
    explicit Center(WidgetPtr child) : child_(std::move(child)) {}

    Size layout(const Constraints& constraints) override {
        Size selfSize = constraints.biggest();
        if (!std::isfinite(selfSize.width))  selfSize.width  = 0.0f;
        if (!std::isfinite(selfSize.height)) selfSize.height = 0.0f;

        if (child_) {
            Size childSize = child_->layout(Constraints::loose(selfSize));
            float cx = (selfSize.width  - childSize.width)  * 0.5f;
            float cy = (selfSize.height - childSize.height) * 0.5f;
            child_->setOffset({std::max(0.0f, cx), std::max(0.0f, cy)});
            children_ = {child_};
        }
        size_ = selfSize;
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    WidgetPtr child_;
};

/// A fixed-size box.  Optionally wraps a child.
class SizedBox : public Widget {
public:
    SizedBox(float width, float height, WidgetPtr child = nullptr)
        : w_(width), h_(height), child_(std::move(child)) {}

    Size layout(const Constraints& constraints) override {
        size_ = constraints.constrain({w_, h_});
        if (child_) {
            child_->layout(Constraints::tight(size_));
            child_->setOffset({0, 0});
            children_ = {child_};
        }
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    float     w_, h_;
    WidgetPtr child_;
};

/// Expands to fill available space in a Flex container.
class Expanded : public Widget {
public:
    explicit Expanded(WidgetPtr child, int flex = 1)
        : child_(std::move(child)), flex_(flex) {}

    int flex() const { return flex_; }

    Size layout(const Constraints& constraints) override {
        size_ = constraints.biggest();
        if (!std::isfinite(size_.width))  size_.width  = 0;
        if (!std::isfinite(size_.height)) size_.height = 0;
        if (child_) {
            child_->layout(Constraints::tight(size_));
            child_->setOffset({0, 0});
            children_ = {child_};
        }
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    WidgetPtr child_;
    int       flex_;
};

/// Stacks children on top of each other (z-order = declaration order).
class Stack : public Widget {
public:
    explicit Stack(std::vector<WidgetPtr> children = {},
                   Alignment alignment = Alignment::topLeft())
        : alignment_(alignment)
    {
        children_ = std::move(children);
    }

    Size layout(const Constraints& constraints) override {
        Size selfSize{};
        for (auto& child : children_) {
            Size s = child->layout(Constraints::loose(constraints.biggest()));
            selfSize.width  = std::max(selfSize.width,  s.width);
            selfSize.height = std::max(selfSize.height, s.height);
        }
        selfSize = constraints.constrain(selfSize);
        size_    = selfSize;

        for (auto& child : children_) {
            float ox = (alignment_.x + 1.0f) * 0.5f * (selfSize.width  - child->size().width);
            float oy = (alignment_.y + 1.0f) * 0.5f * (selfSize.height - child->size().height);
            child->setOffset({std::max(0.0f, ox), std::max(0.0f, oy)});
        }
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    Alignment alignment_;
};

} // namespace aigui
