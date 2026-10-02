// PixelForge — uncompressed 24-bit BMP reader/writer (hand-rolled parser).
#pragma once

#include <string>

#include "image.hpp"

namespace pf {

// Returns false and fills err on any malformed input.
bool read_bmp(const std::string& path, Image& out, std::string& err);
bool write_bmp(const std::string& path, const Image& img, std::string& err);

}  // namespace pf
