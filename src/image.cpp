#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "image.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

Image load_image(const std::string& filepath, int desired_channels) {
    int w = 0, h = 0, c = 0;
    unsigned char* raw_data = stbi_load(filepath.c_str(), &w, &h, &c, desired_channels);
    if (!raw_data) {
        throw std::runtime_error("Error al cargar imagen '" + filepath + "': " + stbi_failure_reason());
    }

    int actual_channels = (desired_channels > 0) ? desired_channels : c;
    Image img(w, h, actual_channels);
    std::copy(raw_data, raw_data + (w * h * actual_channels), img.data.begin());
    stbi_image_free(raw_data);
    return img;
}

static std::string get_file_extension(const std::string& path) {
    auto dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) return "";
    std::string ext = path.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool save_image(const std::string& filepath, const Image& img, int quality) {
    if (!img.is_valid()) {
        std::cerr << "save_image error: Imagen inválida." << std::endl;
        return false;
    }

    std::string ext = get_file_extension(filepath);
    int res = 0;

    if (ext == "png") {
        res = stbi_write_png(filepath.c_str(), img.width, img.height, img.channels,
                             img.data.data(), img.width * img.channels);
    } else if (ext == "jpg" || ext == "jpeg") {
        res = stbi_write_jpg(filepath.c_str(), img.width, img.height, img.channels,
                             img.data.data(), quality);
    } else if (ext == "bmp") {
        res = stbi_write_bmp(filepath.c_str(), img.width, img.height, img.channels,
                             img.data.data());
    } else {
        // Por defecto guardar como PNG
        std::string fallback_path = filepath + ".png";
        std::cerr << "Formato no reconocido para '" << filepath << "'. Guardando como PNG en: "
                  << fallback_path << std::endl;
        res = stbi_write_png(fallback_path.c_str(), img.width, img.height, img.channels,
                             img.data.data(), img.width * img.channels);
    }

    return res != 0;
}

Image rgb_to_grayscale(const Image& rgb_img) {
    if (rgb_img.channels < 3) {
        // Ya es de 1 canal
        return rgb_img;
    }

    int w = rgb_img.width;
    int h = rgb_img.height;
    Image gray(w, h, 1);

    const uint8_t* src = rgb_img.data.data();
    uint8_t* dst = gray.data.data();
    size_t total_pixels = static_cast<size_t>(w * h);

    for (size_t i = 0; i < total_pixels; ++i) {
        // Luminancia estándar de cátedra: Y = 0.30*R + 0.59*G + 0.11*B
        // Con redondeo apropiado (+0.5)
        double r = src[i * 3 + 0];
        double g = src[i * 3 + 1];
        double b = src[i * 3 + 2];
        double y = 0.30 * r + 0.59 * g + 0.11 * b;
        dst[i] = static_cast<uint8_t>(std::clamp(y + 0.5, 0.0, 255.0));
    }

    return gray;
}

Image grayscale_to_rgb(const Image& gray_img) {
    int w = gray_img.width;
    int h = gray_img.height;
    Image rgb(w, h, 3);

    const uint8_t* src = gray_img.data.data();
    uint8_t* dst = rgb.data.data();
    size_t total_pixels = static_cast<size_t>(w * h);

    for (size_t i = 0; i < total_pixels; ++i) {
        uint8_t v = src[i];
        dst[i * 3 + 0] = v;
        dst[i * 3 + 1] = v;
        dst[i * 3 + 2] = v;
    }

    return rgb;
}
