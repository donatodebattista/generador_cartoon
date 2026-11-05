#include "posterize.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

std::vector<uint8_t> create_posterize_lut(int levels) {
    if (levels < 2 || levels > 256) {
        throw std::invalid_argument("create_posterize_lut: levels debe estar entre 2 y 256");
    }

    std::vector<uint8_t> lut(256);
    const double delta = 256.0 / static_cast<double>(levels);

    for (int v = 0; v < 256; ++v) {
        int index = static_cast<int>(std::floor(static_cast<double>(v) / delta));
        if (index >= levels) {
            index = levels - 1;
        }

        int quantized = static_cast<int>(std::floor((index + 0.5) * delta));
        lut[v] = static_cast<uint8_t>(std::clamp(quantized, 0, 255));
    }

    return lut;
}

Image apply_posterize(const Image& rgb_img, int levels) {
    if (rgb_img.channels != 3 && rgb_img.channels != 1) {
        throw std::invalid_argument("apply_posterize: se espera imagen RGB o Grayscale");
    }

    std::vector<uint8_t> lut = create_posterize_lut(levels);

    const int w = rgb_img.width;
    const int h = rgb_img.height;
    const int c = rgb_img.channels;
    Image posterized(w, h, c);

    const uint8_t* src = rgb_img.data.data();
    uint8_t* dst = posterized.data.data();
    const size_t total_bytes = rgb_img.data.size();

    for (size_t i = 0; i < total_bytes; ++i) {
        dst[i] = lut[src[i]];
    }

    return posterized;
}
