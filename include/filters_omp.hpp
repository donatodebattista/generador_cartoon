#ifndef CARTOON_FILTERS_OMP_HPP
#define CARTOON_FILTERS_OMP_HPP

#include "image.hpp"
#include "filters.hpp" // Para GradientMetric

enum class OmpSchedule {
    STATIC,
    DYNAMIC,
    GUIDED
};

/**
 * @brief Versión paralela OpenMP del filtro de media (borroneado).
 * Convolución espacial con padding neutro negro (0) en los bordes.
 */
Image apply_mean_blur_omp(const Image& gray_img, int kernel_size,
                          int num_threads = 0, OmpSchedule sched = OmpSchedule::STATIC);

/**
 * @brief Versión paralela OpenMP del operador Sobel (3x3 y 5x5).
 */
Image apply_sobel_omp(const Image& gray_img, int kernel_size,
                      GradientMetric metric = GradientMetric::EUCLIDEAN,
                      bool normalize = true,
                      int num_threads = 0, OmpSchedule sched = OmpSchedule::STATIC);

/**
 * @brief Versión paralela OpenMP de binarización por umbral.
 */
Image apply_threshold_omp(const Image& grad_img, uint8_t threshold,
                          bool invert_for_cartoon = true,
                          int num_threads = 0);

#endif // CARTOON_FILTERS_OMP_HPP
