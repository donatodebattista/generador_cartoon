#ifndef CARTOON_PIPELINE_MPI_HPP
#define CARTOON_PIPELINE_MPI_HPP

#include "image.hpp"
#include "filters.hpp"
#include "cartoon.hpp"
#include "mpi_utils.hpp"
#include <mpi.h>

struct PipelineConfigMPI {
    int filter_size = 3;                                // 3 o 5
    int posterize_levels = 3;                           // 3 o 9
    uint8_t edge_threshold = 100;                       // Umbral T
    GradientMetric metric = GradientMetric::EUCLIDEAN;  // EUCLIDEAN o MANHATTAN
    BlendMode blend_mode = BlendMode::MASK;             // MASK o SUM
    bool save_intermediates = false;
    std::string intermediate_prefix = "";
};

/**
 * @brief Intercambia filas fantasma (halo) con los procesos vecinos superior e inferior.
 * Si no hay vecino (borde exterior de la imagen global), rellena con valor neutro negro (0).
 * 
 * @param padded_buffer Arreglo que contiene (halo_radius + local_height + halo_radius) * width * channels
 * @param domain Información de la descomposición 1D
 * @param channels Canales por pixel (1 para grises, 3 para RGB)
 * @param comm Comunicador MPI
 */
void exchange_halo_rows(uint8_t* padded_buffer, const Domain1D& domain,
                        int channels, MPI_Comm comm = MPI_COMM_WORLD);

/**
 * @brief Ejecuta el pipeline completo de Cartoonización distribuido con MPI.
 * 
 * @param input_rgb Imagen de entrada (solo debe estar cargada en Rank 0, en otros puede estar vacía)
 * @param config Configuración del algoritmo
 * @param domain Descomposición de dominio calculada
 * @param stats Puntero opcional a PipelineStats para almacenar tiempos medidos con MPI_Wtime()
 * @param comm Comunicador MPI
 * @return Imagen final cartoon reconstruida en Rank 0 (en otros rangos retorna imagen vacía).
 */
Image run_cartoon_pipeline_mpi(const Image& input_rgb,
                               const PipelineConfigMPI& config,
                               const Domain1D& domain,
                               PipelineStats* stats = nullptr,
                               MPI_Comm comm = MPI_COMM_WORLD);

#endif // CARTOON_PIPELINE_MPI_HPP
