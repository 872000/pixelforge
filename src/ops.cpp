// PixelForge — pixel operations, implemented from first principles.
#include "ops.hpp"

#include <cmath>
#include <cstddef>

namespace pf {

namespace {

std::uint8_t luminance(const std::uint8_t* p) {
    // Rec. 601 luma.
    float y = 0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2];
    return clamp8f(y);
}

}  // namespace

Image grayscale(const Image& img) {
    Image out = img;
    for (int y = 0; y < img.h; ++y) {
        for (int x = 0; x < img.w; ++x) {
            std::uint8_t* p = out.at(x, y);
            std::uint8_t g = luminance(img.at(x, y));
            p[0] = p[1] = p[2] = g;
        }
    }
    return out;
}

Image invert(const Image& img) {
    Image out = img;
    for (std::size_t i = 0; i < out.px.size(); ++i) {
        out.px[i] = static_cast<std::uint8_t>(255 - out.px[i]);
    }
    return out;
}

Image brightness(const Image& img, int amount) {
    Image out = img;
    for (int y = 0; y < img.h; ++y) {
        for (int x = 0; x < img.w; ++x) {
            const std::uint8_t* s = img.at(x, y);
            std::uint8_t* d = out.at(x, y);
            d[0] = clamp8(static_cast<int>(s[0]) + amount);
            d[1] = clamp8(static_cast<int>(s[1]) + amount);
            d[2] = clamp8(static_cast<int>(s[2]) + amount);
        }
    }
    return out;
}

Image contrast(const Image& img, float factor) {
    Image out = img;
    for (int y = 0; y < img.h; ++y) {
        for (int x = 0; x < img.w; ++x) {
            const std::uint8_t* s = img.at(x, y);
            std::uint8_t* d = out.at(x, y);
            for (int c = 0; c < 3; ++c) {
                d[c] = clamp8f((static_cast<float>(s[c]) - 128.0f) * factor +
                               128.0f);
            }
        }
    }
    return out;
}

Image box_blur(const Image& img, int radius) {
    if (radius < 1) {
        return img;
    }
    Image out = img;
    for (int y = 0; y < img.h; ++y) {
        for (int x = 0; x < img.w; ++x) {
            long sum[3] = {0, 0, 0};
            long count = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                int sy = y + dy;
                if (sy < 0 || sy >= img.h) {
                    continue;
                }
                for (int dx = -radius; dx <= radius; ++dx) {
                    int sx = x + dx;
                    if (sx < 0 || sx >= img.w) {
                        continue;
                    }
                    const std::uint8_t* s = img.at(sx, sy);
                    sum[0] += s[0];
                    sum[1] += s[1];
                    sum[2] += s[2];
                    ++count;
                }
            }
            std::uint8_t* d = out.at(x, y);
            d[0] = static_cast<std::uint8_t>(sum[0] / count);
            d[1] = static_cast<std::uint8_t>(sum[1] / count);
            d[2] = static_cast<std::uint8_t>(sum[2] / count);
        }
    }
    return out;
}

Image sobel(const Image& img) {
    // 3x3 Sobel kernels on the luminance channel; borders stay black.
    static const int GX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    static const int GY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    Image out;
    out.w = img.w;
    out.h = img.h;
    out.px.assign(img.px.size(), 0);
    for (int y = 1; y < img.h - 1; ++y) {
        for (int x = 1; x < img.w - 1; ++x) {
            int gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int lum = luminance(img.at(x + kx, y + ky));
                    gx += GX[ky + 1][kx + 1] * lum;
                    gy += GY[ky + 1][kx + 1] * lum;
                }
            }
            int mag = static_cast<int>(
                std::sqrt(static_cast<float>(gx * gx + gy * gy)));
            std::uint8_t* d = out.at(x, y);
            d[0] = d[1] = d[2] = clamp8(mag);
        }
    }
    return out;
}

Image resize_bilinear(const Image& img, int new_w, int new_h) {
    if (new_w < 1 || new_h < 1) {
        return Image{};
    }
    Image out;
    out.w = new_w;
    out.h = new_h;
    out.px.resize(static_cast<std::size_t>(new_w) * static_cast<std::size_t>(new_h) * 3);
    const float x_ratio = static_cast<float>(img.w) / new_w;
    const float y_ratio = static_cast<float>(img.h) / new_h;
    for (int y = 0; y < new_h; ++y) {
        float sy = (static_cast<float>(y) + 0.5f) * y_ratio - 0.5f;
        int y0 = static_cast<int>(std::floor(sy));
        float fy = sy - y0;
        if (y0 < 0) {
            y0 = 0;
            fy = 0.0f;
        }
        int y1 = y0 + 1;
        if (y1 >= img.h) {
            y1 = img.h - 1;
            fy = 0.0f;
        }
        for (int x = 0; x < new_w; ++x) {
            float sx = (static_cast<float>(x) + 0.5f) * x_ratio - 0.5f;
            int x0 = static_cast<int>(std::floor(sx));
            float fx = sx - x0;
            if (x0 < 0) {
                x0 = 0;
                fx = 0.0f;
            }
            int x1 = x0 + 1;
            if (x1 >= img.w) {
                x1 = img.w - 1;
                fx = 0.0f;
            }
            std::uint8_t* d = out.at(x, y);
            for (int c = 0; c < 3; ++c) {
                float v00 = img.at(x0, y0)[c];
                float v10 = img.at(x1, y0)[c];
                float v01 = img.at(x0, y1)[c];
                float v11 = img.at(x1, y1)[c];
                float v = v00 * (1 - fx) * (1 - fy) + v10 * fx * (1 - fy) +
                          v01 * (1 - fx) * fy + v11 * fx * fy;
                d[c] = clamp8f(v);
            }
        }
    }
    return out;
}

Image rotate90_cw(const Image& img) {
    Image out;
    out.w = img.h;
    out.h = img.w;
    out.px.resize(static_cast<std::size_t>(out.w) * static_cast<std::size_t>(out.h) * 3);
    for (int y = 0; y < out.h; ++y) {
        for (int x = 0; x < out.w; ++x) {
            const std::uint8_t* s = img.at(y, img.h - 1 - x);
            std::uint8_t* d = out.at(x, y);
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
        }
    }
    return out;
}

}  // namespace pf
