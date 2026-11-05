#include "cartoon_omp.hpp"
#include <omp.h>
#include <iostream>
#include <algorithm>

Image blend_cartoon_omp(const Image& posterized, const Image& edges_mask,
                        BlendMode mode, int num_threads) {
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    const int w = posterized.width;
    const int h = posterized.height;
    Image result(w, h, 3);

    const uint8_t* post = posterized.data.data();
    const uint8_t* edge = edges_mask.data.data();
    uint8_t* dst = result.data.data();
    const size_t total_pixels = static_cast<size_t>(w * h);

    if (mode == BlendMode::MASK) {
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < total_pixels; ++i) {
            if (edge[i] == 0) {
                dst[i * 3 + 0] = 0;
                dst[i * 3 + 1] = 0;
                dst[i * 3 + 2] = 0;
            } else {
                dst[i * 3 + 0] = post[i * 3 + 0];
                dst[i * 3 + 1] = post[i * 3 + 1];
                dst[i * 3 + 2] = post[i * 3 + 2];
            }
        }
    } else {
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < total_pixels; ++i) {
            uint8_t ev = edge[i];
            dst[i * 3 + 0] = static_cast<uint8_t>(std::min(255, post[i * 3 + 0] + ev));
            dst[i * 3 + 1] = static_cast<uint8_t>(std::min(255, post[i * 3 + 1] + ev));
            dst[i * 3 + 2] = static_cast<uint8_t>(std::min(255, post[i * 3 + 2] + ev));
        }
    }

    return result;
}

Image run_cartoon_pipeline_omp(const Image& input_rgb,
                               const PipelineConfigOMP& config,
                               PipelineStats* stats) {
    if (config.num_threads > 0) {
        omp_set_num_threads(config.num_threads);
    }

    double total_start = omp_get_wtime();

    // --- RAMA A: BORDES ---

    // Paso A.0: Conversión a escala de grises
    double t0 = omp_get_wtime();
    Image gray = rgb_to_grayscale_omp(input_rgb, config.num_threads);
    double t_gray = (omp_get_wtime() - t0) * 1000.0;

    // Paso A.1: Filtro de Media (Borroneado)
    t0 = omp_get_wtime();
    Image blur = apply_mean_blur_omp(gray, config.filter_size, config.num_threads, config.schedule);
    double t_blur = (omp_get_wtime() - t0) * 1000.0;

    // Paso A.2: Resaltado con Gradiente Sobel
    t0 = omp_get_wtime();
    Image grad = apply_sobel_omp(blur, config.filter_size, config.metric, true, config.num_threads, config.schedule);
    double t_sobel = (omp_get_wtime() - t0) * 1000.0;

    // Paso A.3: Binarización / Umbralado
    t0 = omp_get_wtime();
    bool invert = (config.blend_mode == BlendMode::MASK);
    Image edges = apply_threshold_omp(grad, config.edge_threshold, invert, config.num_threads);
    double t_thresh = (omp_get_wtime() - t0) * 1000.0;

    // --- RAMA B: POSTERIZADO ---
    t0 = omp_get_wtime();
    Image posterized = apply_posterize_omp(input_rgb, config.posterize_levels, config.num_threads);
    double t_post = (omp_get_wtime() - t0) * 1000.0;

    // --- COMBINACIÓN FINAL ---
    t0 = omp_get_wtime();
    Image cartoon = blend_cartoon_omp(posterized, edges, config.blend_mode, config.num_threads);
    double t_blend = (omp_get_wtime() - t0) * 1000.0;

    double t_total = (omp_get_wtime() - total_start) * 1000.0;

    if (stats) {
        stats->time_grayscale_ms = t_gray;
        stats->time_blur_ms = t_blur;
        stats->time_sobel_ms = t_sobel;
        stats->time_threshold_ms = t_thresh;
        stats->time_posterize_ms = t_post;
        stats->time_blend_ms = t_blend;
        stats->time_total_proc_ms = t_total;
    }

    if (config.save_intermediates && !config.intermediate_prefix.empty()) {
        save_image(config.intermediate_prefix + "_1_gray.png", gray);
        save_image(config.intermediate_prefix + "_2_blur.png", blur);
        save_image(config.intermediate_prefix + "_3_sobel_grad.png", grad);
        save_image(config.intermediate_prefix + "_4_edges.png", edges);
        save_image(config.intermediate_prefix + "_5_posterized.png", posterized);
    }

    return cartoon;
}
