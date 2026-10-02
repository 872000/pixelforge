// PixelForge — pixel operations, all implemented from first principles.
#pragma once

#include "image.hpp"

namespace pf {

Image grayscale(const Image& img);    // luminance -> RGB gray
Image invert(const Image& img);       // 255 - v
Image brightness(const Image& img, int amount);       // add, clamped
Image contrast(const Image& img, float factor);       // (v-128)*f+128
Image box_blur(const Image& img, int radius);         // mean filter
Image sobel(const Image& img);                        // edge magnitude, gray
Image resize_bilinear(const Image& img, int new_w, int new_h);
Image rotate90_cw(const Image& img);  // 90 degrees clockwise

}  // namespace pf
