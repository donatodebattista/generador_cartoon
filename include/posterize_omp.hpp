#ifndef CARTOON_POSTERIZE_OMP_HPP
#define CARTOON_POSTERIZE_OMP_HPP

#include "image.hpp"

/**
 * @brief Aplica la cuantización tonal / posterizado en paralelo con OpenMP.
 */
Image apply_posterize_omp(const Image& rgb_img, int levels, int num_threads = 0);

/**
 * @brief Convierte RGB a escala de grises en paralelo con OpenMP.
 */
Image rgb_to_grayscale_omp(const Image& rgb_img, int num_threads = 0);

#endif // CARTOON_POSTERIZE_OMP_HPP
