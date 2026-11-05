#ifndef CARTOON_PIPELINE_OMP_HPP
#define CARTOON_PIPELINE_OMP_HPP

#include "image.hpp"
#include "filters.hpp"
#include "cartoon.hpp"
#include "filters_omp.hpp"
#include "posterize_omp.hpp"
#include <string>

struct PipelineConfigOMP {
    int filter_size = 3;                                // 3 o 5
    int posterize_levels = 3;                           // 3 o 9
    uint8_t edge_threshold = 100;                       // Umbral T
    GradientMetric metric = GradientMetric::EUCLIDEAN;  // EUCLIDEAN o MANHATTAN
    BlendMode blend_mode = BlendMode::MASK;             // MASK o SUM
    int num_threads = 0;                                // 0 = automatico, o 1, 2, 4, 8...
    OmpSchedule schedule = OmpSchedule::STATIC;         // STATIC, DYNAMIC o GUIDED
    bool save_intermediates = false;
    std::string intermediate_prefix = "";
};

/**
 * @brief Fusión cartoon en paralelo con OpenMP.
 */
Image blend_cartoon_omp(const Image& posterized, const Image& edges_mask,
                        BlendMode mode, int num_threads = 0);

/**
 * @brief Ejecuta el pipeline completo de Cartoonización en memoria compartida (OpenMP).
 * Mide los tiempos con omp_get_wtime() según la consigna.
 */
Image run_cartoon_pipeline_omp(const Image& input_rgb,
                               const PipelineConfigOMP& config,
                               PipelineStats* stats = nullptr);

#endif // CARTOON_PIPELINE_OMP_HPP
