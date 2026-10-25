// PixelForge — self-contained test runner (no external test framework).
// Every pixel op is checked against hand-computed expected values on
// small synthetic images.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "bmp.hpp"
#include "image.hpp"
#include "ops.hpp"
#include "ppm.hpp"

namespace fs = std::filesystem;

static int g_pass = 0;
static int g_fail = 0;
static const char* g_current = "";

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) {                                                              \
            ++g_pass;                                                            \
        } else {                                                                 \
            ++g_fail;                                                            \
            std::cout << "  FAIL [" << g_current << "] line " << __LINE__ << ": " \
                      << #cond << "\n";                                          \
        }                                                                        \
    } while (0)

#define RUN(name)                          \
    do {                                   \
        g_current = #name;                  \
        std::cout << "  " << #name << "\n"; \
        name();                            \
    } while (0)

namespace {

// 2x2 image with known pixels:
//   (0,0)=red  (1,0)=green
//   (0,1)=blue (1,1)=white
pf::Image make_2x2() {
    pf::Image img;
    img.w = 2;
    img.h = 2;
    img.px = {
        255, 0, 0,   0, 255, 0,    // row 0
        0, 0, 255,   255, 255, 255  // row 1
    };
    return img;
}

bool px_eq(const pf::Image& img, int x, int y, int r, int g, int b) {
    const std::uint8_t* p = img.at(x, y);
    return p[0] == r && p[1] == g && p[2] == b;
}

std::string tmp_path(const std::string& name) {
    static int n = 0;
    auto p = fs::temp_directory_path() /
             ("pf_test_" + std::to_string(::getpid()) + "_" +
              std::to_string(n++) + "_" + name);
    return p.string();
}

// ---- operations ------------------------------------------------------------

void op_grayscale_known_values() {
    pf::Image g = pf::grayscale(make_2x2());
    // Rec.601: red -> 76, green -> 150, blue -> 29, white -> 255
    CHECK(px_eq(g, 0, 0, 76, 76, 76));
    CHECK(px_eq(g, 1, 0, 150, 150, 150));
    CHECK(px_eq(g, 0, 1, 29, 29, 29));
    CHECK(px_eq(g, 1, 1, 255, 255, 255));
}

void op_invert_known_values() {
    pf::Image inv = pf::invert(make_2x2());
    CHECK(px_eq(inv, 0, 0, 0, 255, 255));    // red -> cyan
    CHECK(px_eq(inv, 1, 0, 255, 0, 255));    // green -> magenta
    CHECK(px_eq(inv, 0, 1, 255, 255, 0));    // blue -> yellow
    CHECK(px_eq(inv, 1, 1, 0, 0, 0));        // white -> black
    // Double invert restores the original.
    pf::Image back = pf::invert(inv);
    CHECK(back.px == make_2x2().px);
}

void op_brightness_clamps() {
    pf::Image img;
    img.w = 1;
    img.h = 1;
    img.px = {250, 10, 128};
    pf::Image up = pf::brightness(img, 10);
    CHECK(px_eq(up, 0, 0, 255, 20, 138));  // 250+10 clamps to 255
    pf::Image down = pf::brightness(img, -20);
    CHECK(px_eq(down, 0, 0, 230, 0, 108));  // 10-20 clamps to 0
}

void op_contrast_identity() {
    pf::Image img = make_2x2();
    pf::Image same = pf::contrast(img, 1.0f);
    CHECK(same.px == img.px);  // factor 1.0 is the identity
    pf::Image flat = pf::contrast(img, 0.0f);
    // factor 0 collapses everything to mid-gray 128
    CHECK(px_eq(flat, 0, 0, 128, 128, 128));
    CHECK(px_eq(flat, 1, 1, 128, 128, 128));
}

void op_box_blur_uniform() {
    pf::Image img;
    img.w = 4;
    img.h = 4;
    img.px.assign(4 * 4 * 3, 100);
    pf::Image b = pf::box_blur(img, 2);
    for (std::size_t i = 0; i < b.px.size(); ++i) {
        if (b.px[i] != 100) {
            CHECK(false);
            return;
        }
    }
    CHECK(true);  // blurring a flat image changes nothing
}

void op_box_blur_center_dot() {
    // 3x3 black image with a single white center pixel.
    pf::Image img;
    img.w = 3;
    img.h = 3;
    img.px.assign(3 * 3 * 3, 0);
    std::uint8_t* c = img.at(1, 1);
    c[0] = c[1] = c[2] = 255;
    pf::Image b = pf::box_blur(img, 1);
    // Every pixel's 3x3 window (clipped at edges) sees exactly one white
    // pixel: center window has 9 pixels -> 255/9 = 28, corner window has
    // 4 pixels -> 255/4 = 63, edge window has 6 -> 255/6 = 42.
    CHECK(px_eq(b, 1, 1, 28, 28, 28));
    CHECK(px_eq(b, 0, 0, 63, 63, 63));
    CHECK(px_eq(b, 1, 0, 42, 42, 42));
}

void op_sobel_uniform_is_zero() {
    pf::Image img;
    img.w = 5;
    img.h = 5;
    img.px.assign(5 * 5 * 3, 120);
    pf::Image e = pf::sobel(img);
    for (std::size_t i = 0; i < e.px.size(); ++i) {
        if (e.px[i] != 0) {
            CHECK(false);
            return;
        }
    }
    CHECK(true);  // no gradients -> no edges
}

void op_sobel_detects_step_edge() {
    // 5x5: left half black, right half white -> vertical edge at x=2.
    pf::Image img;
    img.w = 5;
    img.h = 5;
    img.px.assign(5 * 5 * 3, 0);
    for (int y = 0; y < 5; ++y) {
        for (int x = 3; x < 5; ++x) {
            std::uint8_t* p = img.at(x, y);
            p[0] = p[1] = p[2] = 255;
        }
    }
    pf::Image e = pf::sobel(img);
    // Strong response straddling the edge, silence far from it.
    CHECK(e.at(2, 2)[0] > 200);
    CHECK(e.at(0, 2)[0] == 0);
    CHECK(e.at(4, 2)[0] == 0);
}

void op_grayscale_idempotent() {
    // Grayscale output is already gray, so a second pass must be a no-op.
    pf::Image gray = pf::grayscale(make_2x2());
    pf::Image twice = pf::grayscale(gray);
    CHECK(twice.px == gray.px);
}

void op_resize_corners_preserved() {
    pf::Image img = make_2x2();
    pf::Image big = pf::resize_bilinear(img, 4, 4);
    CHECK(big.w == 4 && big.h == 4);
    // Corner pixels of an upscale stay (very close to) the source corners.
    const std::uint8_t* tl = big.at(0, 0);
    CHECK(tl[0] > 200 && tl[1] < 60 && tl[2] < 60);  // ~red
    const std::uint8_t* br = big.at(3, 3);
    CHECK(br[0] > 200 && br[1] > 200 && br[2] > 200);  // ~white
}

void op_resize_downsample_uniform() {
    pf::Image img;
    img.w = 8;
    img.h = 8;
    img.px.assign(8 * 8 * 3, 77);
    pf::Image small = pf::resize_bilinear(img, 2, 2);
    CHECK(small.w == 2 && small.h == 2);
    bool ok = true;
    for (std::size_t i = 0; i < small.px.size(); ++i) {
        if (small.px[i] != 77) {
            ok = false;
        }
    }
    CHECK(ok);
}

void op_rotate90_mapping() {
    // 2x2: [R G; B W] clockwise -> [B R; W G]
    pf::Image r = pf::rotate90_cw(make_2x2());
    CHECK(r.w == 2 && r.h == 2);
    CHECK(px_eq(r, 0, 0, 0, 0, 255));      // blue
    CHECK(px_eq(r, 1, 0, 255, 0, 0));      // red
    CHECK(px_eq(r, 0, 1, 255, 255, 255));  // white
    CHECK(px_eq(r, 1, 1, 0, 255, 0));      // green
    // Four rotations restore the original.
    pf::Image back = pf::rotate90_cw(pf::rotate90_cw(pf::rotate90_cw(r)));
    CHECK(back.px == make_2x2().px);
}

// ---- codecs ----------------------------------------------------------------

void codec_bmp_roundtrip() {
    pf::Image img = make_2x2();
    std::string path = tmp_path("rt.bmp");
    std::string err;
    CHECK(pf::write_bmp(path, img, err));
    pf::Image back;
    CHECK(pf::read_bmp(path, back, err));
    CHECK(back.w == 2 && back.h == 2);
    CHECK(back.px == img.px);  // lossless, row order preserved
    fs::remove(path);
}

void codec_bmp_bottom_up_order() {
    // 1x2: top pixel red, bottom pixel blue. BMP stores rows bottom-up;
    // the reader must still hand back top-first rows.
    pf::Image img;
    img.w = 1;
    img.h = 2;
    img.px = {255, 0, 0, 0, 0, 255};
    std::string path = tmp_path("order.bmp");
    std::string err;
    CHECK(pf::write_bmp(path, img, err));
    pf::Image back;
    CHECK(pf::read_bmp(path, back, err));
    CHECK(px_eq(back, 0, 0, 255, 0, 0));
    CHECK(px_eq(back, 0, 1, 0, 0, 255));
    fs::remove(path);
}

void codec_bmp_rejects_garbage() {
    std::string path = tmp_path("junk.bmp");
    {
        std::ofstream f(path, std::ios::binary);
        f << "definitely not a bitmap";
    }
    pf::Image img;
    std::string err;
    CHECK(!pf::read_bmp(path, img, err));
    CHECK(!err.empty());
    fs::remove(path);
}

void codec_ppm_roundtrip() {
    pf::Image img = make_2x2();
    std::string path = tmp_path("rt.ppm");
    std::string err;
    CHECK(pf::write_ppm(path, img, err));
    pf::Image back;
    CHECK(pf::read_ppm(path, back, err));
    CHECK(back.w == 2 && back.h == 2);
    CHECK(back.px == img.px);
    fs::remove(path);
}

void codec_ppm_reads_comments() {
    // Hand-written PPM with comments and odd whitespace.
    std::string path = tmp_path("cmt.ppm");
    {
        std::ofstream f(path, std::ios::binary);
        f << "P6\n# a comment\n# another\n2 1\n255\n";
        const std::uint8_t px[6] = {1, 2, 3, 4, 5, 6};
        f.write(reinterpret_cast<const char*>(px), 6);
    }
    pf::Image img;
    std::string err;
    CHECK(pf::read_ppm(path, img, err));
    CHECK(img.w == 2 && img.h == 1);
    CHECK(px_eq(img, 0, 0, 1, 2, 3));
    CHECK(px_eq(img, 1, 0, 4, 5, 6));
    fs::remove(path);
}

}  // namespace

int main() {
    std::cout << "pixelforge tests\n";
    RUN(op_grayscale_known_values);
    RUN(op_grayscale_idempotent);
    RUN(op_invert_known_values);
    RUN(op_brightness_clamps);
    RUN(op_contrast_identity);
    RUN(op_box_blur_uniform);
    RUN(op_box_blur_center_dot);
    RUN(op_sobel_detects_step_edge);
    RUN(op_sobel_uniform_is_zero);
    RUN(op_resize_corners_preserved);
    RUN(op_resize_downsample_uniform);
    RUN(op_rotate90_mapping);
    RUN(codec_bmp_roundtrip);
    RUN(codec_bmp_bottom_up_order);
    RUN(codec_bmp_rejects_garbage);
    RUN(codec_ppm_roundtrip);
    RUN(codec_ppm_reads_comments);

    std::cout << "\n" << g_pass << " assertions passed, " << g_fail
              << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
