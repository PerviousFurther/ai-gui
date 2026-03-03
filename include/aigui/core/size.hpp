#pragma once
#include <algorithm>
#include <limits>

namespace aigui {

/// 2-D size (width × height).
struct Size {
    float width{0.0f}, height{0.0f};

    constexpr Size() = default;
    constexpr Size(float w, float h) : width(w), height(h) {}

    constexpr float area()          const { return width * height; }
    constexpr bool  isEmpty()       const { return width <= 0.0f || height <= 0.0f; }

    constexpr Size operator+(const Size& o) const { return {width + o.width, height + o.height}; }
    constexpr Size operator-(const Size& o) const { return {width - o.width, height - o.height}; }
    constexpr Size operator*(float s)       const { return {width * s,        height * s};        }
    constexpr Size operator/(float s)       const { return {width / s,        height / s};        }

    constexpr bool operator==(const Size& o) const { return width == o.width && height == o.height; }
    constexpr bool operator!=(const Size& o) const { return !(*this == o); }

    /// Clamp both dimensions to [min, max].
    constexpr Size clamp(const Size& mn, const Size& mx) const {
        return {std::clamp(width,  mn.width,  mx.width),
                std::clamp(height, mn.height, mx.height)};
    }

    static constexpr Size zero()     { return {0.0f, 0.0f}; }
    static constexpr Size infinite() {
        constexpr float inf = std::numeric_limits<float>::infinity();
        return {inf, inf};
    }
};

} // namespace aigui
