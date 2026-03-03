#pragma once
#include "size.hpp"
#include <algorithm>
#include <limits>

namespace aigui {

/// Box-model constraints handed to widgets during layout (Flutter-style).
struct Constraints {
    float minWidth{0.0f};
    float maxWidth{std::numeric_limits<float>::infinity()};
    float minHeight{0.0f};
    float maxHeight{std::numeric_limits<float>::infinity()};

    constexpr Constraints() = default;
    constexpr Constraints(float minW, float maxW, float minH, float maxH)
        : minWidth(minW), maxWidth(maxW), minHeight(minH), maxHeight(maxH) {}

    /// Tight constraints that force an exact size.
    static constexpr Constraints tight(float w, float h) {
        return {w, w, h, h};
    }
    static constexpr Constraints tight(const Size& s) {
        return tight(s.width, s.height);
    }

    /// Loose constraints (min = 0, max = given).
    static constexpr Constraints loose(float w, float h) {
        return {0, w, 0, h};
    }
    static constexpr Constraints loose(const Size& s) {
        return loose(s.width, s.height);
    }

    /// Unconstrained (infinite) in both axes.
    static Constraints unconstrained() {
        constexpr float inf = std::numeric_limits<float>::infinity();
        return {0, inf, 0, inf};
    }

    /// Clamp a size so it satisfies these constraints.
    constexpr Size constrain(const Size& desired) const {
        return {
            std::clamp(desired.width,  minWidth,  maxWidth),
            std::clamp(desired.height, minHeight, maxHeight)
        };
    }

    /// Biggest size that still satisfies the constraints.
    constexpr Size biggest() const { return {maxWidth, maxHeight}; }

    /// Smallest size that still satisfies the constraints.
    constexpr Size smallest() const { return {minWidth, minHeight}; }

    constexpr bool isTight() const {
        return minWidth == maxWidth && minHeight == maxHeight;
    }
    constexpr bool hasBoundedWidth()  const {
        return maxWidth  != std::numeric_limits<float>::infinity();
    }
    constexpr bool hasBoundedHeight() const {
        return maxHeight != std::numeric_limits<float>::infinity();
    }

    /// Remove a fixed amount of space (e.g. padding) from the constraints.
    constexpr Constraints deflate(float horizontal, float vertical) const {
        float newMinW = std::max(0.0f, minWidth  - horizontal);
        float newMaxW = std::max(0.0f, maxWidth  - horizontal);
        float newMinH = std::max(0.0f, minHeight - vertical);
        float newMaxH = std::max(0.0f, maxHeight - vertical);
        return {newMinW, newMaxW, newMinH, newMaxH};
    }
};

} // namespace aigui
