#ifndef CARTOON_IMAGE_HPP
#define CARTOON_IMAGE_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

/**
 * @brief Representación simple de imagen en RGB / Escala de Grises.
 * 
 * Almacena los píxeles en un vector contiguo en orden por filas (row-major) de enteros sin signo de 8 bits.
 * Para RGB (canales = 3): índice = (y * width + x) * 3 + c (donde c está en {0, 1, 2})
 * Para Grises (canales = 1): índice = y * width + x
 */
struct Image {
    int width = 0;
    int height = 0;
    int channels = 3;
    std::vector<uint8_t> data;

    Image() = default;
    Image(int w, int h, int c = 3)
        : width(w), height(h), channels(c), data(static_cast<size_t>(w * h * c), 0) {}
    Image(int w, int h, int c, uint8_t fill_val)
        : width(w), height(h), channels(c), data(static_cast<size_t>(w * h * c), fill_val) {}

    inline bool is_valid() const {
        return width > 0 && height > 0 && channels > 0 && data.size() == static_cast<size_t>(width * height * channels);
    }

    inline size_t pixel_count() const {
        return static_cast<size_t>(width * height);
    }

    inline size_t total_bytes() const {
        return data.size();
    }

    inline uint8_t& at(int x, int y, int c = 0) {
        return data[static_cast<size_t>((y * width + x) * channels + c)];
    }

    inline const uint8_t& at(int x, int y, int c = 0) const {
        return data[static_cast<size_t>((y * width + x) * channels + c)];
    }

    inline uint8_t* raw() { return data.data(); }
    inline const uint8_t* raw() const { return data.data(); }
};

/**
 * @brief Carga una imagen de disco usando stb_image.
 * @param filepath Ruta al archivo de imagen (PNG, JPG, BMP, etc.)
 * @param desired_channels Cantidad de canales deseados (3 para RGB, 1 para Grises)
 * @return Estructura Image con los datos cargados.
 */
Image load_image(const std::string& filepath, int desired_channels = 3);

/**
 * @brief Guarda una imagen a disco usando stb_image_write.
 * Detecta formato según extensión (.png, .jpg, .jpeg, .bmp).
 * @param filepath Ruta de destino.
 * @param img Estructura Image a guardar.
 * @param quality Calidad JPEG (1-100, por defecto 95).
 * @return true si se guardó exitosamente, false en caso de error.
 */
bool save_image(const std::string& filepath, const Image& img, int quality = 95);

/**
 * @brief Convierte imagen RGB a escala de grises (1 canal).
 * Fórmula: Y = 0.30*R + 0.59*G + 0.11*B (Apuntes pág 12 / Requerimientos 3.2).
 */
Image rgb_to_grayscale(const Image& rgb_img);

/**
 * @brief Convierte imagen de escala de grises (1 canal) a RGB (3 canales replicados).
 */
Image grayscale_to_rgb(const Image& gray_img);

#endif // CARTOON_IMAGE_HPP
