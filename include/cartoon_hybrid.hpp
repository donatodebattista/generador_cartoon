#ifndef CARTOON_PIPELINE_HYBRID_HPP
#define CARTOON_PIPELINE_HYBRID_HPP

#include "image.hpp"
#include "filters.hpp"
#include "cartoon.hpp"
#include "mpi_utils.hpp"
#include <mpi.h>

struct PipelineConfigHybrid {
    int filter_size = 3;                                // 3 o 5
    int posterize_levels = 3;                           // 3 o 9
    uint8_t edge_threshold = 100;                       // Umbral T
    GradientMetric metric = GradientMetric::EUCLIDEAN;  // EUCLIDEAN o MANHATTAN
    BlendMode blend_mode = BlendMode::MASK;             // MASK o SUM
    int num_threads = 0;                                // Threads OpenMP por proceso (0 = auto)
    bool save_intermediates = false;
    std::string intermediate_prefix = "";
};

/**
 * @brief Ejecuta el pipeline completo de Cartoonización híbrido (MPI + OpenMP).
 * Inicializado con MPI_THREAD_FUNNELED: comunicaciones gestionadas por el hilo principal
 * y cómputos locales acelerados con hilos OpenMP.
 * 
 * @param input_rgb Imagen original (solo en Rank 0)
 * @param config Configuración híbrida (filtros, niveles, threads)
 * @param domain Descomposición 1D por franjas
 * @param stats Métricas de tiempo medidas con MPI_Wtime()
 * @param comm Comunicador MPI
 * @return Imagen cartoon combinada en Rank 0 (vacía en otros rangos).
 */
Image run_cartoon_pipeline_hybrid(const Image& input_rgb,
                                 const PipelineConfigHybrid& config,
                                 const Domain1D& domain,
                                 PipelineStats* stats = nullptr,
                                 MPI_Comm comm = MPI_COMM_WORLD);

#endif // CARTOON_PIPELINE_HYBRID_HPP
