#include "filters_omp.hpp"
#include <omp.h>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

static void configure_omp_env(int num_threads, OmpSchedule sched) {
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }
    switch (sched) {
        case OmpSchedule::DYNAMIC:
            omp_set_schedule(omp_sched_dynamic, 16);
            break;
        case OmpSchedule::GUIDED:
            omp_set_schedule(omp_sched_guided, 16);
            break;
        case OmpSchedule::STATIC:
        default:
            omp_set_schedule(omp_sched_static, 0);
            break;
    }
}

Image apply_mean_blur_omp(const Image& gray_img, int kernel_size,
                          int num_threads, OmpSchedule sched) {
    if (kernel_size != 3 && kernel_size != 5) {
        throw std::invalid_argument("apply_mean_blur_omp: kernel_size debe ser 3 o 5");
    }

    configure_omp_env(num_threads, sched);

    const int w = gray_img.width;
    const int h = gray_img.height;
    Image result(w, h, 1);

    const uint8_t* src = gray_img.data.data();
    uint8_t* dst = result.data.data();

    const int radius = kernel_size / 2;
    const float divisor = static_cast<float>(kernel_size * kernel_size);

    #pragma omp parallel for schedule(runtime)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sum = 0;

            for (int dy = -radius; dy <= radius; ++dy) {
                int ny = y + dy;
                if (ny < 0 || ny >= h) continue; // Relleno neutro negro (0)

                int row_offset = ny * w;
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = x + dx;
                    if (nx >= 0 && nx < w) {
                        sum += src[row_offset + nx];
                    }
                }
            }

            int avg = static_cast<int>(std::round(static_cast<float>(sum) / divisor));
            dst[y * w + x] = static_cast<uint8_t>(std::clamp(avg, 0, 255));
        }
    }

    return result;
}

// Sobel 3x3 Kernels
static const int SOBEL_3X3_X[3][3] = {
    { -1,  0,  1 },
    { -2,  0,  2 },
    { -1,  0,  1 }
};

static const int SOBEL_3X3_Y[3][3] = {
    {  1,  2,  1 },
    {  0,  0,  0 },
    { -1, -2, -1 }
};

// Sobel 5x5 Kernels
static const int SOBEL_5X5_X[5][5] = {
    { -1, -2, 0, 2, 1 },
    { -2, -3, 0, 3, 2 },
    { -3, -5, 0, 5, 3 },
    { -2, -3, 0, 3, 2 },
    { -1, -2, 0, 2, 1 }
};

static const int SOBEL_5X5_Y[5][5] = {
    { -1, -2, -3, -2, -1 },
    { -2, -3, -5, -3, -2 },
    {  0,  0,  0,  0,  0 },
    {  2,  3,  5,  3,  2 },
    {  1,  2,  3,  2,  1 }
};

Image apply_sobel_omp(const Image& gray_img, int kernel_size,
                      GradientMetric metric, bool normalize,
                      int num_threads, OmpSchedule sched) {
    if (kernel_size != 3 && kernel_size != 5) {
        throw std::invalid_argument("apply_sobel_omp: kernel_size debe ser 3 o 5");
    }

    configure_omp_env(num_threads, sched);

    const int w = gray_img.width;
    const int h = gray_img.height;
    Image grad(w, h, 1);

    const uint8_t* src = gray_img.data.data();
    uint8_t* dst = grad.data.data();

    const double scale = (kernel_size == 5 && normalize) ? (1.0 / 6.0) : 1.0;

    #pragma omp parallel for schedule(runtime)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int gx = 0;
            int gy = 0;

            if (kernel_size == 3) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= h) continue;

                    int row_offset = ny * w;
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        if (nx >= 0 && nx < w) {
                            int val = src[row_offset + nx];
                            gx += SOBEL_3X3_X[dy + 1][dx + 1] * val;
                            gy += SOBEL_3X3_Y[dy + 1][dx + 1] * val;
                        }
                    }
                }
            } else { // 5x5
                for (int dy = -2; dy <= 2; ++dy) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= h) continue;

                    int row_offset = ny * w;
                    for (int dx = -2; dx <= 2; ++dx) {
                        int nx = x + dx;
                        if (nx >= 0 && nx < w) {
                            int val = src[row_offset + nx];
                            gx += SOBEL_5X5_X[dy + 2][dx + 2] * val;
                            gy += SOBEL_5X5_Y[dy + 2][dx + 2] * val;
                        }
                    }
                }
            }

            double scaled_gx = gx * scale;
            double scaled_gy = gy * scale;
            double mag = 0.0;

            if (metric == GradientMetric::EUCLIDEAN) {
                mag = std::sqrt(scaled_gx * scaled_gx + scaled_gy * scaled_gy);
            } else {
                mag = std::abs(scaled_gx) + std::abs(scaled_gy);
            }

            dst[y * w + x] = static_cast<uint8_t>(std::clamp(std::round(mag), 0.0, 255.0));
        }
    }

    return grad;
}

Image apply_threshold_omp(const Image& grad_img, uint8_t threshold,
                          bool invert_for_cartoon, int num_threads) {
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    const int w = grad_img.width;
    const int h = grad_img.height;
    Image binary(w, h, 1);

    const uint8_t* src = grad_img.data.data();
    uint8_t* dst = binary.data.data();
    const size_t total_pixels = static_cast<size_t>(w * h);

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < total_pixels; ++i) {
        bool is_edge = (src[i] >= threshold);
        dst[i] = invert_for_cartoon ? (is_edge ? 0 : 255) : (is_edge ? 255 : 0);
    }

    return binary;
}
