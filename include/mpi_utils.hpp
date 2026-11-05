#ifndef CARTOON_MPI_UTILS_HPP
#define CARTOON_MPI_UTILS_HPP

#include <vector>
#include <cstddef>

/**
 * @brief Estructura que describe la descomposición de dominio 1D por franjas horizontales.
 */
struct Domain1D {
    int total_height = 0;       // Altura total de la imagen H
    int width = 0;              // Ancho de la imagen W
    int rank = 0;               // Rango del proceso actual
    int size = 0;               // Cantidad total de procesos MPI
    int local_height = 0;       // Cantidad de filas asignadas al proceso actual
    int start_row = 0;          // Índice de la primera fila asignada en la imagen global
    int end_row = 0;            // Índice de la última fila asignada (exclusivo)
    int halo_radius = 1;        // Radio del halo (1 para filtro 3x3, 2 para 5x5)

    // Arreglos de distribución para MPI_Scatterv y MPI_Gatherv
    std::vector<int> sendcounts;
    std::vector<int> displs;
    std::vector<int> row_counts;
    std::vector<int> row_starts;

    inline bool is_first_rank() const { return rank == 0; }
    inline bool is_last_rank() const { return rank == size - 1; }
    inline int prev_rank() const { return rank - 1; }
    inline int next_rank() const { return rank + 1; }
};

/**
 * @brief Calcula la partición 1D balanceada de filas entre los procesos MPI.
 * Si H no es múltiplo de size, distribuye el resto equitativamente entre los primeros rangos.
 * 
 * @param total_height Altura total H
 * @param width Ancho W
 * @param rank Rango actual
 * @param size Cantidad de procesos
 * @param halo_radius Radio de filas fantasma (1 o 2)
 * @param channels_per_pixel Canales por pixel (3 para RGB, 1 para Grises)
 * @return Estructura Domain1D inicializada.
 */
Domain1D compute_domain_1d(int total_height, int width, int rank, int size,
                           int halo_radius, int channels_per_pixel = 3);

#endif // CARTOON_MPI_UTILS_HPP
