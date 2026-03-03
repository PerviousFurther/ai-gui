#pragma once
#include "../widget/widget.hpp"
#include "../renderer/canvas.hpp"
#include <functional>
#include <string>

namespace aigui {

/// Axis alignment for Column/Row children.
enum class MainAxisAlignment {
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

enum class CrossAxisAlignment {
    Start,
    End,
    Center,
    Stretch,
};

/// Lays children out vertically.
class Column : public Widget {
public:
    explicit Column(std::vector<WidgetPtr> children = {},
                    MainAxisAlignment  main  = MainAxisAlignment::Start,
                    CrossAxisAlignment cross = CrossAxisAlignment::Start)
        : mainAxis_(main), crossAxis_(cross)
    {
        children_ = std::move(children);
    }

    Column& withMainAxisAlignment(MainAxisAlignment a) { mainAxis_ = a; return *this; }
    Column& withCrossAxisAlignment(CrossAxisAlignment a){ crossAxis_= a; return *this; }

    Size layout(const Constraints& constraints) override {
        float totalH = 0.0f;
        float maxW   = 0.0f;

        // First pass: layout each child with unconstrained height.
        Constraints childConstraints{
            crossAxis_ == CrossAxisAlignment::Stretch ? constraints.maxWidth  : 0.0f,
            constraints.maxWidth,
            0.0f,
            constraints.maxHeight
        };

        for (auto& child : children_) {
            Size s = child->layout(childConstraints);
            totalH += s.height;
            maxW    = std::max(maxW, s.width);
        }

        // Resolve cross-axis width.
        float selfW = crossAxis_ == CrossAxisAlignment::Stretch
                    ? constraints.maxWidth
                    : std::max(constraints.minWidth, maxW);

        // Second pass: assign positions.
        float y      = resolveStartY(totalH, constraints.maxHeight);
        float gapY   = resolveGap(totalH, constraints.maxHeight);

        for (auto& child : children_) {
            float childX = resolveCrossX(child->size().width, selfW);
            child->setOffset({childX, y});
            y += child->size().height + gapY;
        }

        size_ = {selfW, std::min(totalH, constraints.maxHeight)};
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    MainAxisAlignment  mainAxis_;
    CrossAxisAlignment crossAxis_;

    float resolveStartY(float totalH, float maxH) const {
        switch (mainAxis_) {
        case MainAxisAlignment::End:         return std::max(0.0f, maxH - totalH);
        case MainAxisAlignment::Center:      return std::max(0.0f, (maxH - totalH) * 0.5f);
        case MainAxisAlignment::SpaceBetween:return 0.0f;
        case MainAxisAlignment::SpaceAround: return children_.empty() ? 0.0f :
            std::max(0.0f, (maxH - totalH)) / (2.0f * static_cast<float>(children_.size()));
        case MainAxisAlignment::SpaceEvenly: return children_.empty() ? 0.0f :
            std::max(0.0f, (maxH - totalH)) / (static_cast<float>(children_.size()) + 1.0f);
        default: return 0.0f;
        }
    }

    float resolveGap(float totalH, float maxH) const {
        if (children_.size() < 2) return 0.0f;
        float extra = std::max(0.0f, maxH - totalH);
        float n     = static_cast<float>(children_.size());
        switch (mainAxis_) {
        case MainAxisAlignment::SpaceBetween: return extra / (n - 1.0f);
        case MainAxisAlignment::SpaceAround:  return extra / n;
        case MainAxisAlignment::SpaceEvenly:  return extra / (n + 1.0f);
        default: return 0.0f;
        }
    }

    float resolveCrossX(float childW, float selfW) const {
        switch (crossAxis_) {
        case CrossAxisAlignment::End:    return selfW - childW;
        case CrossAxisAlignment::Center: return (selfW - childW) * 0.5f;
        default: return 0.0f;
        }
    }
};

/// Lays children out horizontally.
class Row : public Widget {
public:
    explicit Row(std::vector<WidgetPtr> children = {},
                 MainAxisAlignment  main  = MainAxisAlignment::Start,
                 CrossAxisAlignment cross = CrossAxisAlignment::Start)
        : mainAxis_(main), crossAxis_(cross)
    {
        children_ = std::move(children);
    }

    Row& withMainAxisAlignment(MainAxisAlignment a) { mainAxis_ = a; return *this; }
    Row& withCrossAxisAlignment(CrossAxisAlignment a){ crossAxis_= a; return *this; }

    Size layout(const Constraints& constraints) override {
        float totalW = 0.0f;
        float maxH   = 0.0f;

        Constraints childConstraints{
            0.0f,
            constraints.maxWidth,
            crossAxis_ == CrossAxisAlignment::Stretch ? constraints.maxHeight : 0.0f,
            constraints.maxHeight
        };

        for (auto& child : children_) {
            Size s = child->layout(childConstraints);
            totalW += s.width;
            maxH    = std::max(maxH, s.height);
        }

        float selfH = crossAxis_ == CrossAxisAlignment::Stretch
                    ? constraints.maxHeight
                    : std::max(constraints.minHeight, maxH);

        float x    = resolveStartX(totalW, constraints.maxWidth);
        float gapX = resolveGap(totalW, constraints.maxWidth);

        for (auto& child : children_) {
            float childY = resolveCrossY(child->size().height, selfH);
            child->setOffset({x, childY});
            x += child->size().width + gapX;
        }

        size_ = {std::min(totalW, constraints.maxWidth), selfH};
        return size_;
    }

    void paint(Canvas& canvas) override { paintChildren(canvas); }

private:
    MainAxisAlignment  mainAxis_;
    CrossAxisAlignment crossAxis_;

    float resolveStartX(float totalW, float maxW) const {
        switch (mainAxis_) {
        case MainAxisAlignment::End:         return std::max(0.0f, maxW - totalW);
        case MainAxisAlignment::Center:      return std::max(0.0f, (maxW - totalW) * 0.5f);
        case MainAxisAlignment::SpaceBetween:return 0.0f;
        case MainAxisAlignment::SpaceAround: return children_.empty() ? 0.0f :
            std::max(0.0f, (maxW - totalW)) / (2.0f * static_cast<float>(children_.size()));
        case MainAxisAlignment::SpaceEvenly: return children_.empty() ? 0.0f :
            std::max(0.0f, (maxW - totalW)) / (static_cast<float>(children_.size()) + 1.0f);
        default: return 0.0f;
        }
    }

    float resolveGap(float totalW, float maxW) const {
        if (children_.size() < 2) return 0.0f;
        float extra = std::max(0.0f, maxW - totalW);
        float n     = static_cast<float>(children_.size());
        switch (mainAxis_) {
        case MainAxisAlignment::SpaceBetween: return extra / (n - 1.0f);
        case MainAxisAlignment::SpaceAround:  return extra / n;
        case MainAxisAlignment::SpaceEvenly:  return extra / (n + 1.0f);
        default: return 0.0f;
        }
    }

    float resolveCrossY(float childH, float selfH) const {
        switch (crossAxis_) {
        case CrossAxisAlignment::End:    return selfH - childH;
        case CrossAxisAlignment::Center: return (selfH - childH) * 0.5f;
        default: return 0.0f;
        }
    }
};

} // namespace aigui
