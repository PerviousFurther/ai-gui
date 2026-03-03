#pragma once
#include <cstdint>
#include <algorithm>

namespace aigui {

/// RGBA color value.
struct Color {
    uint8_t r{0}, g{0}, b{0}, a{255};

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    /// Construct from a packed 0xRRGGBBAA integer.
    static constexpr Color fromRGBA(uint32_t rgba) {
        return Color(
            static_cast<uint8_t>((rgba >> 24) & 0xFF),
            static_cast<uint8_t>((rgba >> 16) & 0xFF),
            static_cast<uint8_t>((rgba >>  8) & 0xFF),
            static_cast<uint8_t>( rgba        & 0xFF)
        );
    }

    /// Pack to 0xRRGGBBAA.
    constexpr uint32_t toRGBA() const {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) <<  8) |
                static_cast<uint32_t>(a);
    }

    /// Return a copy with modified alpha.
    constexpr Color withAlpha(uint8_t alpha) const { return Color(r, g, b, alpha); }

    /// Normalised [0,1] components for GPU upload.
    constexpr float rf() const { return r / 255.0f; }
    constexpr float gf() const { return g / 255.0f; }
    constexpr float bf() const { return b / 255.0f; }
    constexpr float af() const { return a / 255.0f; }

    constexpr bool operator==(const Color& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    constexpr bool operator!=(const Color& o) const { return !(*this == o); }

    // Predefined colours
    static constexpr Color transparent() { return Color(0, 0, 0, 0); }
    static constexpr Color white()       { return Color(255, 255, 255); }
    static constexpr Color black()       { return Color(0, 0, 0); }
    static constexpr Color red()         { return Color(255, 0, 0); }
    static constexpr Color green()       { return Color(0, 255, 0); }
    static constexpr Color blue()        { return Color(0, 0, 255); }
    static constexpr Color gray()        { return Color(128, 128, 128); }
    static constexpr Color darkGray()    { return Color(64, 64, 64); }
    static constexpr Color lightGray()   { return Color(192, 192, 192); }
    static constexpr Color cyan()        { return Color(0, 255, 255); }
    static constexpr Color magenta()     { return Color(255, 0, 255); }
    static constexpr Color yellow()      { return Color(255, 255, 0); }
    static constexpr Color orange()      { return Color(255, 165, 0); }
};

} // namespace aigui
