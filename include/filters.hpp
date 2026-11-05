#ifndef CARTOON_FILTERS_HPP
#define CARTOON_FILTERS_HPP

#include "image.hpp"

enum class GradientMetric {
    EUCLIDEAN, // ||G|| = sqrt(Gx^2 + Gy^2)
    MANHATTAN  // ||G|| = |Gx| + |Gy|
};

/**
 * @brief Aplica un filtro de media (borroneado / blur) de 3x3 o 5x5 sobre una imagen en escala de grises.
 * los píxeles adyacentes donde la máscara sobresalga asumen valor neutro negro (0).
 * 
 * @param gray_img Imagen de 1 canal.
 * @param kernel_size Tamaño del kernel (3 o 5).
 * @return Imagen suavizada de 1 canal.
 */
Image apply_mean_blur(const Image& gray_img, int kernel_size);

/**
 * @brief Aplica el operador Sobel (3x3 o 5x5) para calcular el mapa de magnitudes de gradiente.
 * Utiliza relleno negro (0) en las fronteras.
 * 
 * @param gray_img Imagen de 1 canal suavizada.
 * @param kernel_size Tamaño del kernel (3 o 5).
 * @param metric Métrica para la magnitud (EUCLIDEAN o MANHATTAN).
 * @param normalize Si es true, normaliza el gradiente de 5x5 a la escala de 3x3 (factor 1/6)
 *                  para mantener consistencia de umbral.
 * @return Imagen de 1 canal con los valores de magnitud del gradiente [0, 255].
 */
Image apply_sobel(const Image& gray_img, int kernel_size,
                  GradientMetric metric = GradientMetric::EUCLIDEAN,
                  bool normalize = true);

/**
 * @brief Binariza el mapa de gradiente según un umbral T.
 * 
 * @param grad_img Imagen de magnitudes de gradiente (1 canal).
 * @param threshold Valor de umbral T (ej. 100-128).
 * @param invert_for_cartoon Si es true: 0 en bordes (negro) y 255 en no-bordes (blanco).
 *                           Si es false: 255 en bordes (blanco) y 0 en no-bordes (negro).
 * @return Imagen binaria de 1 canal.
 */
Image apply_threshold(const Image& grad_img, uint8_t threshold, bool invert_for_cartoon = true);

/**
 * @brief Ejecuta la Rama A completa: Blur -> Sobel -> Threshold.
 */
Image detect_edges_pipeline(const Image& gray_img, int kernel_size, uint8_t threshold,
                           GradientMetric metric = GradientMetric::EUCLIDEAN,
                           bool invert_for_cartoon = true,
                           Image* out_blur = nullptr,
                           Image* out_grad = nullptr);

#endif // CARTOON_FILTERS_HPP
