// PixelForge — hand-rolled binary PPM (P6) codec.
#include "ppm.hpp"

#include <cctype>
#include <fstream>
#include <string>

namespace pf {

namespace {

// Read the next whitespace-separated token, skipping '#' comments.
bool next_token(std::ifstream& f, std::string& tok) {
    tok.clear();
    char c;
    // Skip whitespace and comments.
    while (f.get(c)) {
        if (c == '#') {
            std::string dummy;
            std::getline(f, dummy);
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            break;
        }
    }
    if (!f) {
        return false;
    }
    tok.push_back(c);
    while (f.get(c) && !std::isspace(static_cast<unsigned char>(c))) {
        tok.push_back(c);
    }
    return true;
}

bool parse_int(const std::string& tok, int& out) {
    if (tok.empty()) {
        return false;
    }
    int v = 0;
    for (char c : tok) {
        if (c < '0' || c > '9') {
            return false;
        }
        v = v * 10 + (c - '0');
    }
    out = v;
    return true;
}

}  // namespace

bool read_ppm(const std::string& path, Image& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot open file";
        return false;
    }
    std::string magic, ws, hs, ms;
    if (!next_token(f, magic) || magic != "P6") {
        err = "not a binary PPM (P6) file";
        return false;
    }
    int w = 0, h = 0, maxval = 0;
    if (!next_token(f, ws) || !parse_int(ws, w) || w <= 0) {
        err = "bad width";
        return false;
    }
    if (!next_token(f, hs) || !parse_int(hs, h) || h <= 0) {
        err = "bad height";
        return false;
    }
    if (!next_token(f, ms) || !parse_int(ms, maxval) || maxval != 255) {
        err = "only maxval 255 supported";
        return false;
    }
    // next_token stopped at the single whitespace after maxval; the byte
    // stream is now positioned at the first raster byte.
    Image img;
    img.w = w;
    img.h = h;
    img.px.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3);
    f.read(reinterpret_cast<char*>(img.px.data()), img.px.size());
    if (static_cast<std::size_t>(f.gcount()) != img.px.size()) {
        err = "truncated raster data";
        return false;
    }
    out = std::move(img);
    return true;
}

bool write_ppm(const std::string& path, const Image& img, std::string& err) {
    if (img.empty()) {
        err = "empty image";
        return false;
    }
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        err = "cannot open file for writing";
        return false;
    }
    f << "P6\n# created by PixelForge\n"
      << img.w << " " << img.h << "\n255\n";
    f.write(reinterpret_cast<const char*>(img.px.data()), img.px.size());
    if (!f) {
        err = "write failed";
        return false;
    }
    return true;
}

}  // namespace pf
