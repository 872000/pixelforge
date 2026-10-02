// PixelForge — hand-rolled 24-bit uncompressed BMP codec.
//
// Layout: 14-byte file header, 40-byte BITMAPINFOHEADER, then pixel rows
// stored bottom-up, each row padded to a 4-byte boundary, pixels as BGR.
#include "bmp.hpp"

#include <cstdint>
#include <fstream>
#include <vector>

namespace pf {

namespace {

std::uint16_t rd16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}
std::uint32_t rd32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) |
                                      (p[3] << 24));
}
void wr16(std::vector<std::uint8_t>& b, std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
    b.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}
void wr32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
    b.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    b.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    b.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

}  // namespace

bool read_bmp(const std::string& path, Image& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot open file";
        return false;
    }
    std::uint8_t hdr[54];
    f.read(reinterpret_cast<char*>(hdr), 54);
    if (f.gcount() != 54) {
        err = "truncated header";
        return false;
    }
    if (hdr[0] != 'B' || hdr[1] != 'M') {
        err = "not a BMP file";
        return false;
    }
    std::uint32_t pixel_offset = rd32(hdr + 10);
    std::uint32_t dib_size = rd32(hdr + 14);
    if (dib_size != 40) {
        err = "unsupported DIB header (need BITMAPINFOHEADER)";
        return false;
    }
    std::int32_t w = static_cast<std::int32_t>(rd32(hdr + 18));
    std::int32_t h = static_cast<std::int32_t>(rd32(hdr + 22));
    if (w <= 0 || h <= 0) {
        err = "unsupported dimensions (need positive width/height)";
        return false;
    }
    if (rd16(hdr + 26) != 1 || rd16(hdr + 28) != 24) {
        err = "only 24-bit BMP supported";
        return false;
    }
    if (rd32(hdr + 30) != 0) {
        err = "compressed BMP not supported";
        return false;
    }

    Image img;
    img.w = w;
    img.h = h;
    img.px.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3,
                  0);
    const std::size_t stride = (static_cast<std::size_t>(w) * 3 + 3) & ~3ULL;
    std::vector<std::uint8_t> row(stride);

    f.seekg(pixel_offset, std::ios::beg);
    // BMP rows are stored bottom-up.
    for (int y = h - 1; y >= 0; --y) {
        f.read(reinterpret_cast<char*>(row.data()), stride);
        if (static_cast<std::size_t>(f.gcount()) != stride) {
            err = "truncated pixel data";
            return false;
        }
        for (int x = 0; x < w; ++x) {
            std::uint8_t* p = img.at(x, y);
            p[0] = row[static_cast<std::size_t>(x) * 3 + 2];  // BGR -> RGB
            p[1] = row[static_cast<std::size_t>(x) * 3 + 1];
            p[2] = row[static_cast<std::size_t>(x) * 3 + 0];
        }
    }
    out = std::move(img);
    return true;
}

bool write_bmp(const std::string& path, const Image& img, std::string& err) {
    if (img.empty()) {
        err = "empty image";
        return false;
    }
    const std::size_t stride = (static_cast<std::size_t>(img.w) * 3 + 3) & ~3ULL;
    const std::uint32_t pixel_bytes =
        static_cast<std::uint32_t>(stride * static_cast<std::size_t>(img.h));

    std::vector<std::uint8_t> buf;
    buf.reserve(54 + pixel_bytes);
    // File header (14 bytes).
    buf.push_back('B');
    buf.push_back('M');
    wr32(buf, 54 + pixel_bytes);
    wr16(buf, 0);
    wr16(buf, 0);
    wr32(buf, 54);
    // BITMAPINFOHEADER (40 bytes).
    wr32(buf, 40);
    wr32(buf, static_cast<std::uint32_t>(img.w));
    wr32(buf, static_cast<std::uint32_t>(img.h));  // positive = bottom-up
    wr16(buf, 1);
    wr16(buf, 24);
    wr32(buf, 0);  // BI_RGB, uncompressed
    wr32(buf, pixel_bytes);
    wr32(buf, 2835);  // ~72 DPI
    wr32(buf, 2835);
    wr32(buf, 0);
    wr32(buf, 0);

    std::vector<std::uint8_t> pad(stride - static_cast<std::size_t>(img.w) * 3,
                                  0);
    for (int y = img.h - 1; y >= 0; --y) {
        for (int x = 0; x < img.w; ++x) {
            const std::uint8_t* p = img.at(x, y);
            buf.push_back(p[2]);  // RGB -> BGR
            buf.push_back(p[1]);
            buf.push_back(p[0]);
        }
        buf.insert(buf.end(), pad.begin(), pad.end());
    }

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        err = "cannot open file for writing";
        return false;
    }
    f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    if (!f) {
        err = "write failed";
        return false;
    }
    return true;
}

}  // namespace pf
