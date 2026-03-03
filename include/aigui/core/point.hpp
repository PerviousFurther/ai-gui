#pragma once
#include <cmath>

namespace aigui {

/// 2-D point / vector.
struct Point {
    float x{0.0f}, y{0.0f};

    constexpr Point() = default;
    constexpr Point(float x, float y) : x(x), y(y) {}

    constexpr Point operator+(const Point& o) const { return {x + o.x, y + o.y}; }
    constexpr Point operator-(const Point& o) const { return {x - o.x, y - o.y}; }
    constexpr Point operator*(float s)        const { return {x * s,   y * s};   }
    constexpr Point operator/(float s)        const { return {x / s,   y / s};   }
    constexpr Point& operator+=(const Point& o) { x += o.x; y += o.y; return *this; }
    constexpr Point& operator-=(const Point& o) { x -= o.x; y -= o.y; return *this; }

    constexpr bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Point& o) const { return !(*this == o); }

    float length() const { return std::sqrt(x * x + y * y); }

    static constexpr Point zero() { return {0.0f, 0.0f}; }
};

} // namespace aigui
