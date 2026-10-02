// PixelForge — binary PPM (P6) reader/writer (hand-rolled parser).
#pragma once

#include <string>

#include "image.hpp"

namespace pf {

bool read_ppm(const std::string& path, Image& out, std::string& err);
bool write_ppm(const std::string& path, const Image& img, std::string& err);

}  // namespace pf
