#include "posterize_omp.hpp"
#include "posterize.hpp" // Para create_posterize_lut
#include <omp.h>
#include <cmath>
#include <algorithm>
#include <stdexcept>

Image apply_posterize_omp(const Image& rgb_img, int levels, int num_threads) {
    if (rgb_img.channels != 3 && rgb_img.channels != 1) {
        throw std::invalid_argument("apply_posterize_omp: se espera imagen RGB o Grayscale");
    }

    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    std::vector<uint8_t> lut = create_posterize_lut(levels);

    const int w = rgb_img.width;
    const int h = rgb_img.height;
    const int c = rgb_img.channels;
    Image posterized(w, h, c);

    const uint8_t* src = rgb_img.data.data();
    uint8_t* dst = posterized.data.data();
    const size_t total_bytes = rgb_img.data.size();
    const uint8_t* lut_ptr = lut.data();

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < total_bytes; ++i) {
        dst[i] = lut_ptr[src[i]];
    }

    return posterized;
}

Image rgb_to_grayscale_omp(const Image& rgb_img, int num_threads) {
    if (rgb_img.channels < 3) {
        return rgb_img;
    }

    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    const int w = rgb_img.width;
    const int h = rgb_img.height;
    Image gray(w, h, 1);

    const uint8_t* src = rgb_img.data.data();
    uint8_t* dst = gray.data.data();
    const size_t total_pixels = static_cast<size_t>(w * h);

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < total_pixels; ++i) {
        double r = src[i * 3 + 0];
        double g = src[i * 3 + 1];
        double b = src[i * 3 + 2];
        double y = 0.30 * r + 0.59 * g + 0.11 * b;
        dst[i] = static_cast<uint8_t>(std::clamp(y + 0.5, 0.0, 255.0));
    }

    return gray;
}
