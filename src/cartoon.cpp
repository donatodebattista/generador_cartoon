#include "cartoon.hpp"
#include <iostream>
#include <algorithm>

Image blend_cartoon(const Image& posterized, const Image& edges_mask, BlendMode mode) {
    const int w = posterized.width;
    const int h = posterized.height;
    Image result(w, h, 3);

    const uint8_t* post = posterized.data.data();
    const uint8_t* edge = edges_mask.data.data();
    uint8_t* dst = result.data.data();
    const size_t total_pixels = static_cast<size_t>(w * h);

    if (mode == BlendMode::MASK) {
        // En MASK mode: edge == 0 representa trazo de contorno negro.
        // edge == 255 representa zona plana donde se conserva el color posterizado.
        for (size_t i = 0; i < total_pixels; ++i) {
            if (edge[i] == 0) {
                // Trazo de borde negro (0, 0, 0)
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
        // Modo SUM: edge == 255 en bordes, 0 en no-bordes.
        // Suma directa clamped en [0, 255]
        for (size_t i = 0; i < total_pixels; ++i) {
            uint8_t ev = edge[i];
            dst[i * 3 + 0] = static_cast<uint8_t>(std::min(255, post[i * 3 + 0] + ev));
            dst[i * 3 + 1] = static_cast<uint8_t>(std::min(255, post[i * 3 + 1] + ev));
            dst[i * 3 + 2] = static_cast<uint8_t>(std::min(255, post[i * 3 + 2] + ev));
        }
    }

    return result;
}

Image run_cartoon_pipeline_seq(const Image& input_rgb,
                               const PipelineConfig& config,
                               PipelineStats* stats) {
    Timer total_timer;
    Timer stage_timer;

    // --- RAMA A: BORDES ---

    // Paso A.0: Conversión a escala de grises
    stage_timer.reset();
    Image gray = rgb_to_grayscale(input_rgb);
    double t_gray = stage_timer.elapsed_ms();

    // Paso A.1: Filtro de Media (Borroneado)
    stage_timer.reset();
    Image blur = apply_mean_blur(gray, config.filter_size);
    double t_blur = stage_timer.elapsed_ms();

    // Paso A.2: Resaltado con Gradiente Sobel
    stage_timer.reset();
    Image grad = apply_sobel(blur, config.filter_size, config.metric, true);
    double t_sobel = stage_timer.elapsed_ms();

    // Paso A.3: Binarización / Umbralado
    stage_timer.reset();
    bool invert = (config.blend_mode == BlendMode::MASK);
    Image edges = apply_threshold(grad, config.edge_threshold, invert);
    double t_thresh = stage_timer.elapsed_ms();

    // --- RAMA B: POSTERIZADO ---
    stage_timer.reset();
    Image posterized = apply_posterize(input_rgb, config.posterize_levels);
    double t_post = stage_timer.elapsed_ms();

    // --- COMBINACIÓN FINAL ---
    stage_timer.reset();
    Image cartoon = blend_cartoon(posterized, edges, config.blend_mode);
    double t_blend = stage_timer.elapsed_ms();

    double t_total = total_timer.elapsed_ms();

    if (stats) {
        stats->time_grayscale_ms = t_gray;
        stats->time_blur_ms = t_blur;
        stats->time_sobel_ms = t_sobel;
        stats->time_threshold_ms = t_thresh;
        stats->time_posterize_ms = t_post;
        stats->time_blend_ms = t_blend;
        stats->time_total_proc_ms = t_total;
    }

    // Guardar intermedias si fue solicitado
    if (config.save_intermediates && !config.intermediate_prefix.empty()) {
        save_image(config.intermediate_prefix + "_1_gray.png", gray);
        save_image(config.intermediate_prefix + "_2_blur.png", blur);
        save_image(config.intermediate_prefix + "_3_sobel_grad.png", grad);
        save_image(config.intermediate_prefix + "_4_edges.png", edges);
        save_image(config.intermediate_prefix + "_5_posterized.png", posterized);
    }

    return cartoon;
}
