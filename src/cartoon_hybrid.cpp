#include "cartoon_hybrid.hpp"
#include "cartoon_mpi.hpp"   // Para exchange_halo_rows
#include "posterize.hpp"     // Para create_posterize_lut
#include <omp.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>

// Sobel Kernels
static const int SOBEL_3X3_X[3][3] = {
    { -1,  0,  1 },
    { -2,  0,  2 },
    { -1,  0,  1 }
};

static const int SOBEL_3X3_Y[3][3] = {
    {  1,  2,  1 },
    {  0,  0,  0 },
    { -1, -2, -1 }
};

static const int SOBEL_5X5_X[5][5] = {
    { -1, -2, 0, 2, 1 },
    { -2, -3, 0, 3, 2 },
    { -3, -5, 0, 5, 3 },
    { -2, -3, 0, 3, 2 },
    { -1, -2, 0, 2, 1 }
};

static const int SOBEL_5X5_Y[5][5] = {
    { -1, -2, -3, -2, -1 },
    { -2, -3, -5, -3, -2 },
    {  0,  0,  0,  0,  0 },
    {  2,  3,  5,  3,  2 },
    {  1,  2,  3,  2,  1 }
};

Image run_cartoon_pipeline_hybrid(const Image& input_rgb,
                                 const PipelineConfigHybrid& config,
                                 const Domain1D& domain,
                                 PipelineStats* stats,
                                 MPI_Comm comm) {
    if (config.num_threads > 0) {
        omp_set_num_threads(config.num_threads);
    }

    const int w = domain.width;
    const int local_h = domain.local_height;
    const int halo = domain.halo_radius;
    const int padded_h = local_h + 2 * halo;

    MPI_Barrier(comm);
    double t_total_start = MPI_Wtime();

    // 0. Dispersión MPI de franjas RGB entre procesos
    std::vector<uint8_t> local_rgb(static_cast<size_t>(local_h * w * 3));
    const uint8_t* global_rgb_ptr = (domain.rank == 0) ? input_rgb.data.data() : nullptr;

    MPI_Scatterv(global_rgb_ptr, domain.sendcounts.data(), domain.displs.data(), MPI_UNSIGNED_CHAR,
                 local_rgb.data(), local_h * w * 3, MPI_UNSIGNED_CHAR, 0, comm);

    // --- RAMA A: BORDES (Acelerada con OpenMP) ---

    // Paso A.0: Conversión a escala de grises local
    double t0 = MPI_Wtime();
    std::vector<uint8_t> local_gray(static_cast<size_t>(local_h * w));

    #pragma omp parallel for schedule(static)
    for (int ly = 0; ly < local_h; ++ly) {
        int row_offset_gray = ly * w;
        int row_offset_rgb = ly * w * 3;
        for (int x = 0; x < w; ++x) {
            double r = local_rgb[row_offset_rgb + x * 3 + 0];
            double g = local_rgb[row_offset_rgb + x * 3 + 1];
            double b = local_rgb[row_offset_rgb + x * 3 + 2];
            double y = 0.30 * r + 0.59 * g + 0.11 * b;
            local_gray[row_offset_gray + x] = static_cast<uint8_t>(std::clamp(y + 0.5, 0.0, 255.0));
        }
    }
    double t_gray_local = (MPI_Wtime() - t0) * 1000.0;

    // Paso A.1: Filtro de Media con Halo Exchange
    t0 = MPI_Wtime();
    std::vector<uint8_t> gray_padded(static_cast<size_t>(padded_h * w), 0);
    std::memcpy(gray_padded.data() + (halo * w), local_gray.data(), local_h * w);

    // Intercambio punto a punto de filas fantasma
    exchange_halo_rows(gray_padded.data(), domain, 1, comm);

    // Convolución de media paralelizada internamente con OpenMP
    std::vector<uint8_t> local_blur(static_cast<size_t>(local_h * w));
    const float divisor = static_cast<float>(config.filter_size * config.filter_size);

    #pragma omp parallel for schedule(static)
    for (int ly = 0; ly < local_h; ++ly) {
        int py = ly + halo;
        int dst_row_offset = ly * w;
        for (int x = 0; x < w; ++x) {
            int sum = 0;
            for (int dy = -halo; dy <= halo; ++dy) {
                int row_offset = (py + dy) * w;
                for (int dx = -halo; dx <= halo; ++dx) {
                    int nx = x + dx;
                    if (nx >= 0 && nx < w) {
                        sum += gray_padded[row_offset + nx];
                    }
                }
            }
            int avg = static_cast<int>(std::round(static_cast<float>(sum) / divisor));
            local_blur[dst_row_offset + x] = static_cast<uint8_t>(std::clamp(avg, 0, 255));
        }
    }
    double t_blur_local = (MPI_Wtime() - t0) * 1000.0;

    // Paso A.2: Gradiente Sobel con Halo Exchange de imagen suavizada
    t0 = MPI_Wtime();
    std::vector<uint8_t> blur_padded(static_cast<size_t>(padded_h * w), 0);
    std::memcpy(blur_padded.data() + (halo * w), local_blur.data(), local_h * w);

    exchange_halo_rows(blur_padded.data(), domain, 1, comm);

    // Sobel y Binarización paralelizados con OpenMP
    std::vector<uint8_t> local_edges(static_cast<size_t>(local_h * w));
    const double scale = (config.filter_size == 5) ? (1.0 / 6.0) : 1.0;
    const bool invert = (config.blend_mode == BlendMode::MASK);

    #pragma omp parallel for schedule(static)
    for (int ly = 0; ly < local_h; ++ly) {
        int py = ly + halo;
        int dst_row_offset = ly * w;
        for (int x = 0; x < w; ++x) {
            int gx = 0;
            int gy = 0;

            if (config.filter_size == 3) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int row_offset = (py + dy) * w;
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        if (nx >= 0 && nx < w) {
                            int val = blur_padded[row_offset + nx];
                            gx += SOBEL_3X3_X[dy + 1][dx + 1] * val;
                            gy += SOBEL_3X3_Y[dy + 1][dx + 1] * val;
                        }
                    }
                }
            } else { // 5x5
                for (int dy = -2; dy <= 2; ++dy) {
                    int row_offset = (py + dy) * w;
                    for (int dx = -2; dx <= 2; ++dx) {
                        int nx = x + dx;
                        if (nx >= 0 && nx < w) {
                            int val = blur_padded[row_offset + nx];
                            gx += SOBEL_5X5_X[dy + 2][dx + 2] * val;
                            gy += SOBEL_5X5_Y[dy + 2][dx + 2] * val;
                        }
                    }
                }
            }

            double scaled_gx = gx * scale;
            double scaled_gy = gy * scale;
            double mag = 0.0;
            if (config.metric == GradientMetric::EUCLIDEAN) {
                mag = std::sqrt(scaled_gx * scaled_gx + scaled_gy * scaled_gy);
            } else {
                mag = std::abs(scaled_gx) + std::abs(scaled_gy);
            }

            bool is_edge = (mag >= config.edge_threshold);
            local_edges[dst_row_offset + x] = invert ? (is_edge ? 0 : 255) : (is_edge ? 255 : 0);
        }
    }
    double t_sobel_local = (MPI_Wtime() - t0) * 1000.0;

    // --- RAMA B: POSTERIZADO LOCAL (Acelerado con OpenMP) ---
    t0 = MPI_Wtime();
    std::vector<uint8_t> lut = create_posterize_lut(config.posterize_levels);
    std::vector<uint8_t> local_post(static_cast<size_t>(local_h * w * 3));
    const size_t local_rgb_bytes = static_cast<size_t>(local_h * w * 3);
    const uint8_t* lut_ptr = lut.data();

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < local_rgb_bytes; ++i) {
        local_post[i] = lut_ptr[local_rgb[i]];
    }
    double t_post_local = (MPI_Wtime() - t0) * 1000.0;

    // --- COMBINACIÓN FINAL (Acelerada con OpenMP) ---
    t0 = MPI_Wtime();
    std::vector<uint8_t> local_cartoon(static_cast<size_t>(local_h * w * 3));
    const size_t local_pixels = static_cast<size_t>(local_h * w);

    if (config.blend_mode == BlendMode::MASK) {
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < local_pixels; ++i) {
            if (local_edges[i] == 0) {
                local_cartoon[i * 3 + 0] = 0;
                local_cartoon[i * 3 + 1] = 0;
                local_cartoon[i * 3 + 2] = 0;
            } else {
                local_cartoon[i * 3 + 0] = local_post[i * 3 + 0];
                local_cartoon[i * 3 + 1] = local_post[i * 3 + 1];
                local_cartoon[i * 3 + 2] = local_post[i * 3 + 2];
            }
        }
    } else {
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < local_pixels; ++i) {
            uint8_t ev = local_edges[i];
            local_cartoon[i * 3 + 0] = static_cast<uint8_t>(std::min(255, local_post[i * 3 + 0] + ev));
            local_cartoon[i * 3 + 1] = static_cast<uint8_t>(std::min(255, local_post[i * 3 + 1] + ev));
            local_cartoon[i * 3 + 2] = static_cast<uint8_t>(std::min(255, local_post[i * 3 + 2] + ev));
        }
    }
    double t_blend_local = (MPI_Wtime() - t0) * 1000.0;

    // 8. Recolección final en Rank 0 (MPI_Gatherv)
    Image final_img;
    if (domain.rank == 0) {
        final_img = Image(w, domain.total_height, 3);
    }

    MPI_Gatherv(local_cartoon.data(), local_h * w * 3, MPI_UNSIGNED_CHAR,
                (domain.rank == 0) ? final_img.data.data() : nullptr,
                domain.sendcounts.data(), domain.displs.data(), MPI_UNSIGNED_CHAR, 0, comm);

    MPI_Barrier(comm);
    double t_total_proc = (MPI_Wtime() - t_total_start) * 1000.0;

    double t_total_max = 0.0;
    MPI_Reduce(&t_total_proc, &t_total_max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

    if (stats && domain.rank == 0) {
        stats->time_grayscale_ms = t_gray_local;
        stats->time_blur_ms = t_blur_local;
        stats->time_sobel_ms = t_sobel_local;
        stats->time_posterize_ms = t_post_local;
        stats->time_blend_ms = t_blend_local;
        stats->time_total_proc_ms = t_total_max;
    }

    return final_img;
}
