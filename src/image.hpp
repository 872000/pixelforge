// PixelForge — the in-memory image: RGB pixels, row-major, no libraries.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pf {

struct Image {
    int w = 0;
    int h = 0;
    std::vector<std::uint8_t> px;  // RGB triplets, row-major, top row first

    bool empty() const { return w <= 0 || h <= 0 || px.empty(); }

    std::uint8_t* at(int x, int y) {
        return &px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(x)) *
                   3];
    }
    const std::uint8_t* at(int x, int y) const {
        return &px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(x)) *
                   3];
    }
};

inline std::uint8_t clamp8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<std::uint8_t>(v);
}

inline std::uint8_t clamp8f(float v) {
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return static_cast<std::uint8_t>(v + 0.5f);
}

}  // namespace pf
