#ifndef CARTOON_PIPELINE_HPP
#define CARTOON_PIPELINE_HPP

#include "image.hpp"
#include "filters.hpp"
#include "posterize.hpp"
#include "timer.hpp"
#include <string>

enum class BlendMode {
    MASK, // Multiplicación / Máscara (Bordes negros = 0, fondo = Posterizado)
    SUM   // Suma directa (min(255, Posterizado + Borde))
};

struct PipelineConfig {
    int filter_size = 3;                                // 3 o 5
    int posterize_levels = 3;                           // 3 o 9
    uint8_t edge_threshold = 100;                       // Umbral T (típicamente 80-128)
    GradientMetric metric = GradientMetric::EUCLIDEAN;  // EUCLIDEAN o MANHATTAN
    BlendMode blend_mode = BlendMode::MASK;             // MASK o SUM
    bool save_intermediates = false;                    // Guardar imágenes intermedias para análisis
    std::string intermediate_prefix = "";               // Prefijo de ruta para intermedias
};

struct PipelineStats {
    double time_grayscale_ms = 0.0;
    double time_blur_ms = 0.0;
    double time_sobel_ms = 0.0;
    double time_threshold_ms = 0.0;
    double time_posterize_ms = 0.0;
    double time_blend_ms = 0.0;
    double time_total_proc_ms = 0.0;
};

/**
 * @brief Combina la imagen posterizada con el mapa de bordes.
 * 
 * @param posterized Imagen RGB posterizada.
 * @param edges_mask Imagen de 1 canal con la máscara de bordes.
 * @param mode Modo de combinación (MASK o SUM).
 * @return Imagen final Cartoon RGB.
 */
Image blend_cartoon(const Image& posterized, const Image& edges_mask, BlendMode mode);

/**
 * @brief Ejecuta el pipeline completo de Cartoonización de forma secuencial.
 * 
 * @param input_rgb Imagen RGB original.
 * @param config Configuración del algoritmo (filtros, niveles, modos).
 * @param stats Puntero opcional a PipelineStats para recolectar mediciones de tiempo de procesamiento.
 * @return Imagen RGB Cartoon resultante.
 */
Image run_cartoon_pipeline_seq(const Image& input_rgb,
                               const PipelineConfig& config,
                               PipelineStats* stats = nullptr);

#endif // CARTOON_PIPELINE_HPP
