#ifndef CARTOON_POSTERIZE_HPP
#define CARTOON_POSTERIZE_HPP

#include "image.hpp"
#include <vector>

/**
 * @brief Precalcula una tabla de búsqueda (LUT) de 256 valores para posterizado
 * según la especificación técnica:
 *   Delta = 256.0 / N
 *   indice = min(floor(v / Delta), N - 1)
 *   v_posterizado = min(255, floor((indice + 0.5) * Delta))
 * 
 * @param levels Cantidad de niveles N (ej. 3 o 9).
 * @return Vector de 256 uint8_t mapeados.
 */
std::vector<uint8_t> create_posterize_lut(int levels);

/**
 * @brief Aplica la cuantización tonal / posterizado de forma independiente
 * a cada canal (R, G, B) de la imagen.
 * 
 * @param rgb_img Imagen RGB de entrada.
 * @param levels Cantidad de intervalos discretos N (3 o 9).
 * @return Imagen RGB posterizada.
 */
Image apply_posterize(const Image& rgb_img, int levels);

#endif // CARTOON_POSTERIZE_HPP
