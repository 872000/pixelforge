// PixelForge — CLI entry point: pixelforge <op> [args] -i in -o out
#include <iostream>
#include <string>
#include <vector>

#include "bmp.hpp"
#include "image.hpp"
#include "ops.hpp"
#include "ppm.hpp"

namespace {

bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool read_image(const std::string& path, pf::Image& img, std::string& err) {
    if (ends_with(path, ".bmp") || ends_with(path, ".BMP")) {
        return pf::read_bmp(path, img, err);
    }
    if (ends_with(path, ".ppm") || ends_with(path, ".PPM")) {
        return pf::read_ppm(path, img, err);
    }
    err = "unknown input format (use .bmp or .ppm)";
    return false;
}

bool write_image(const std::string& path, const pf::Image& img,
                 std::string& err) {
    if (ends_with(path, ".bmp") || ends_with(path, ".BMP")) {
        return pf::write_bmp(path, img, err);
    }
    if (ends_with(path, ".ppm") || ends_with(path, ".PPM")) {
        return pf::write_ppm(path, img, err);
    }
    err = "unknown output format (use .bmp or .ppm)";
    return false;
}

void usage(const char* prog) {
    std::cout
        << "PixelForge — image processing from raw pixels, zero dependencies.\n\n"
        << "Usage: " << prog << " <op> [op-args] -i INPUT -o OUTPUT\n\n"
        << "Operations:\n"
        << "  gray                      grayscale (luminance)\n"
        << "  invert                    invert colors\n"
        << "  bright <amount>           brightness, e.g. bright 30 / bright -20\n"
        << "  contrast <factor>         contrast, e.g. contrast 1.5\n"
        << "  blur <radius>             box blur, e.g. blur 2\n"
        << "  sobel                     Sobel edge detection\n"
        << "  resize <w> <h>            bilinear resize, e.g. resize 800 600\n"
        << "  rot90                     rotate 90 degrees clockwise\n\n"
        << "Formats are detected from the file extension (.bmp, .ppm).\n"
        << "Example: " << prog << " sobel -i photo.bmp -o edges.ppm\n";
}

bool parse_int_arg(const std::string& s, int& out) {
    try {
        std::size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) {
            return false;
        }
        out = v;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_float_arg(const std::string& s, float& out) {
    try {
        std::size_t pos = 0;
        float v = std::stof(s, &pos);
        if (pos != s.size()) {
            return false;
        }
        out = v;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    std::string op = argv[1];
    std::string input, output;
    std::vector<std::string> op_args;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-i" || a == "--input") && i + 1 < argc) {
            input = argv[++i];
        } else if ((a == "-o" || a == "--output") && i + 1 < argc) {
            output = argv[++i];
        } else {
            op_args.push_back(a);
        }
    }
    if (input.empty() || output.empty()) {
        std::cerr << "error: need -i INPUT and -o OUTPUT\n\n";
        usage(argv[0]);
        return 1;
    }

    pf::Image img;
    std::string err;
    if (!read_image(input, img, err)) {
        std::cerr << "error reading input: " << err << "\n";
        return 1;
    }

    pf::Image result;
    bool ok = true;
    if (op == "gray") {
        result = pf::grayscale(img);
    } else if (op == "invert") {
        result = pf::invert(img);
    } else if (op == "bright") {
        int amount = 0;
        ok = op_args.size() == 1 && parse_int_arg(op_args[0], amount);
        if (ok) {
            result = pf::brightness(img, amount);
        }
    } else if (op == "contrast") {
        float factor = 1.0f;
        ok = op_args.size() == 1 && parse_float_arg(op_args[0], factor);
        if (ok) {
            result = pf::contrast(img, factor);
        }
    } else if (op == "blur") {
        int radius = 0;
        ok = op_args.size() == 1 && parse_int_arg(op_args[0], radius) &&
             radius >= 0;
        if (ok) {
            result = pf::box_blur(img, radius);
        }
    } else if (op == "sobel") {
        result = pf::sobel(img);
    } else if (op == "resize") {
        int w = 0, h = 0;
        ok = op_args.size() == 2 && parse_int_arg(op_args[0], w) &&
             parse_int_arg(op_args[1], h) && w > 0 && h > 0;
        if (ok) {
            result = pf::resize_bilinear(img, w, h);
        }
    } else if (op == "rot90") {
        result = pf::rotate90_cw(img);
    } else {
        std::cerr << "error: unknown operation '" << op << "'\n\n";
        usage(argv[0]);
        return 1;
    }
    if (!ok) {
        std::cerr << "error: bad arguments for '" << op << "'\n\n";
        usage(argv[0]);
        return 1;
    }

    if (!write_image(output, result, err)) {
        std::cerr << "error writing output: " << err << "\n";
        return 1;
    }
    std::cout << "wrote " << output << " (" << result.w << "x" << result.h
              << ")\n";
    return 0;
}
