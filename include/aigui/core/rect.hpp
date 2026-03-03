#pragma once
#include "point.hpp"
#include "size.hpp"

namespace aigui {

/// Axis-aligned rectangle (x, y are the top-left corner).
struct Rect {
    float x{0.0f}, y{0.0f}, width{0.0f}, height{0.0f};

    constexpr Rect() = default;
    constexpr Rect(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h) {}
    constexpr Rect(const Point& origin, const Size& size)
        : x(origin.x), y(origin.y), width(size.width), height(size.height) {}

    constexpr float left()   const { return x; }
    constexpr float top()    const { return y; }
    constexpr float right()  const { return x + width; }
    constexpr float bottom() const { return y + height; }

    constexpr Point topLeft()     const { return {x,         y};          }
    constexpr Point topRight()    const { return {x + width, y};          }
    constexpr Point bottomLeft()  const { return {x,         y + height}; }
    constexpr Point bottomRight() const { return {x + width, y + height}; }
    constexpr Point center()      const { return {x + width * 0.5f, y + height * 0.5f}; }

    constexpr Size size()   const { return {width, height}; }
    constexpr Point origin() const { return {x, y}; }

    constexpr bool contains(const Point& p) const {
        return p.x >= x && p.x <= right() && p.y >= y && p.y <= bottom();
    }

    constexpr bool intersects(const Rect& o) const {
        return !(o.x >= right() || o.right() <= x || o.y >= bottom() || o.bottom() <= y);
    }

    /// Translate by a point offset.
    constexpr Rect translated(float dx, float dy) const { return {x + dx, y + dy, width, height}; }
    constexpr Rect translated(const Point& d)     const { return translated(d.x, d.y); }

    /// Deflate all sides by d.
    constexpr Rect deflated(float d) const {
        return {x + d, y + d, width - 2.0f * d, height - 2.0f * d};
    }

    constexpr bool operator==(const Rect& o) const {
        return x == o.x && y == o.y && width == o.width && height == o.height;
    }
    constexpr bool operator!=(const Rect& o) const { return !(*this == o); }

    static constexpr Rect zero() { return {0, 0, 0, 0}; }
};

} // namespace aigui
