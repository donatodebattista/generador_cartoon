#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>
#include <mpi.h>

#include "image.hpp"
#include "filters.hpp"
#include "posterize.hpp"
#include "cartoon.hpp"
#include "mpi_utils.hpp"
#include "cartoon_mpi.hpp"
#include "timer.hpp"

static void print_usage(const char* prog_name) {
    std::cout << "========================================================================\n"
              << " Generador de Efecto Cartoon - Computacion Avanzada UADER (MPI)\n"
              << "========================================================================\n"
              << "Uso:\n"
              << "  mpirun -np <P> " << prog_name << " <input_img> <output_img> <filter_size> <posterize_levels> [opciones]\n\n"
              << "Argumentos obligatorios:\n"
              << "  <input_img>         Ruta a la imagen de entrada (PNG, JPG, BMP)\n"
              << "  <output_img>        Ruta donde guardar la imagen resultante (PNG, JPG)\n"
              << "  <filter_size>       Tamaño del filtro espacial: 3 (3x3) o 5 (5x5)\n"
              << "  <posterize_levels>  Niveles de cuantizacion tonal: 3 o 9\n\n"
              << "Opciones adicionales:\n"
              << "  --threshold <val>   Umbral T para binarizacion Sobel (defecto: 100, rango 0-255)\n"
              << "  --metric <m>        Metrica de gradiente: 'euclidean' (defecto) o 'manhattan'\n"
              << "  --blend <mode>      Modo de fusion: 'mask' (defecto, contorno negro) o 'sum' (suma)\n"
              << "  -h, --help          Muestra esta ayuda y finaliza\n\n"
              << "Ejemplo:\n"
              << "  mpirun -np 4 " << prog_name << " images/input/foto.jpg images/output/cartoon_mpi.png 3 3\n"
              << "========================================================================\n";
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) print_usage(argv[0]);
        MPI_Finalize();
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "-h" || arg1 == "--help") {
        if (rank == 0) print_usage(argv[0]);
        MPI_Finalize();
        return 0;
    }

    if (argc < 5) {
        if (rank == 0) {
            std::cerr << "Error: Faltan argumentos requeridos.\n\n";
            print_usage(argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];
    int filter_size = std::atoi(argv[3]);
    int posterize_levels = std::atoi(argv[4]);

    if (filter_size != 3 && filter_size != 5) {
        if (rank == 0) {
            std::cerr << "Error: filter_size debe ser 3 o 5 (recibido: " << filter_size << ")\n";
        }
        MPI_Finalize();
        return 1;
    }

    PipelineConfigMPI config;
    config.filter_size = filter_size;
    config.posterize_levels = posterize_levels;
    config.edge_threshold = 100;
    config.metric = GradientMetric::EUCLIDEAN;
    config.blend_mode = BlendMode::MASK;

    for (int i = 5; i < argc; ++i) {
        std::string flag = argv[i];
        if (flag == "--threshold" && i + 1 < argc) {
            int t = std::atoi(argv[++i]);
            config.edge_threshold = static_cast<uint8_t>(std::clamp(t, 0, 255));
        } else if (flag == "--metric" && i + 1 < argc) {
            std::string m = argv[++i];
            if (m == "manhattan") config.metric = GradientMetric::MANHATTAN;
            else config.metric = GradientMetric::EUCLIDEAN;
        } else if (flag == "--blend" && i + 1 < argc) {
            std::string b = argv[++i];
            if (b == "sum") config.blend_mode = BlendMode::SUM;
            else config.blend_mode = BlendMode::MASK;
        }
    }

    Image input_img;
    int img_dims[2] = {0, 0}; // {width, height}
    double t_load_ms = 0.0;

    if (rank == 0) {
        std::cout << "\n=======================================================\n";
        std::cout << "        TP FINAL - COMPUTACION AVANZADA (UADER)        \n";
        std::cout << "        Pipeline Cartoonizador Distribuido (MPI)       \n";
        std::cout << "=======================================================\n";
        std::cout << " Configuracion MPI:\n";
        std::cout << "  - Procesos MPI        : " << size << "\n";
        std::cout << "  - Imagen de entrada   : " << input_path << "\n";
        std::cout << "  - Imagen de salida    : " << output_path << "\n";
        std::cout << "  - Filtro Espacial     : " << config.filter_size << "x" << config.filter_size
                  << " (Halo: " << (config.filter_size / 2) << " filas fantasma)\n";
        std::cout << "  - Niveles Posterizado : " << config.posterize_levels << "\n";
        std::cout << "  - Umbral Sobel (T)    : " << static_cast<int>(config.edge_threshold) << "\n";
        std::cout << "  - Metrica Gradiente   : " << (config.metric == GradientMetric::EUCLIDEAN ? "Euclidea" : "Manhattan") << "\n";
        std::cout << "  - Modo de Fusion      : " << (config.blend_mode == BlendMode::MASK ? "Mascara (Contorno negro)" : "Suma directa") << "\n";
        std::cout << "-------------------------------------------------------\n";

        Timer io_load_timer;
        try {
            input_img = load_image(input_path, 3);
        } catch (const std::exception& e) {
            std::cerr << "Error critico al cargar imagen: " << e.what() << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        t_load_ms = io_load_timer.elapsed_ms();

        img_dims[0] = input_img.width;
        img_dims[1] = input_img.height;

        std::cout << " Imagen cargada con exito en Rank 0:\n";
        std::cout << "  - Dimensiones: " << img_dims[0] << " x " << img_dims[1] << " px ("
                  << std::fixed << std::setprecision(2) << (img_dims[0] * img_dims[1] / 1e6) << " Megapixels)\n";
        std::cout << "  - Tiempo I/O Carga: " << std::fixed << std::setprecision(2) << t_load_ms << " ms\n";
        std::cout << "-------------------------------------------------------\n";
    }

    // Difusión de dimensiones de la imagen a todos los procesos
    MPI_Bcast(img_dims, 2, MPI_INT, 0, MPI_COMM_WORLD);

    int halo_radius = config.filter_size / 2;
    Domain1D domain = compute_domain_1d(img_dims[1], img_dims[0], rank, size, halo_radius, 3);

    if (rank == 0) {
        std::cout << " Reparto de franjas horizontales (1D Domain Decomposition):\n";
        for (int r = 0; r < size; ++r) {
            std::cout << "  - Rank " << std::setw(2) << r << ": filas ["
                      << std::setw(5) << domain.row_starts[r] << " .. "
                      << std::setw(5) << (domain.row_starts[r] + domain.row_counts[r] - 1)
                      << "] (" << std::setw(5) << domain.row_counts[r] << " filas)\n";
        }
        std::cout << "-------------------------------------------------------\n";
        std::cout << " Ejecutando pipeline distribuido con intercambio de halos...\n";
    }

    PipelineStats stats;
    Image final_img = run_cartoon_pipeline_mpi(input_img, config, domain, &stats, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "\n=======================================================\n";
        std::cout << "      DESGLOSE DE TIEMPOS DE PROCESAMIENTO (MPI)       \n";
        std::cout << "=======================================================\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << " [Rama A: Bordes]\n";
        std::cout << "  - Grayscale local (Y=0.3R+0.59G+0.11B) : " << std::setw(8) << stats.time_grayscale_ms << " ms\n";
        std::cout << "  - Media + Intercambio Halo (" << config.filter_size << "x" << config.filter_size << ")  : "
                  << std::setw(8) << stats.time_blur_ms << " ms\n";
        std::cout << "  - Sobel + Intercambio Halo (" << config.filter_size << "x" << config.filter_size << ")  : "
                  << std::setw(8) << stats.time_sobel_ms << " ms\n";
        std::cout << " [Rama B: Posterizado]\n";
        std::cout << "  - Cuantizacion local (" << config.posterize_levels << " niveles)       : "
                  << std::setw(8) << stats.time_posterize_ms << " ms\n";
        std::cout << " [Combinacion Final]\n";
        std::cout << "  - Fusion Cartoon local                 : " << std::setw(8) << stats.time_blend_ms << " ms\n";
        std::cout << "-------------------------------------------------------\n";
        std::cout << " TIEMPO TOTAL DE PROCESAMIENTO MPI (Tp): "
                  << std::setw(8) << stats.time_total_proc_ms << " ms\n";
        std::cout << "=======================================================\n\n";

        Timer io_save_timer;
        bool ok = save_image(output_path, final_img);
        double t_save_ms = io_save_timer.elapsed_ms();

        if (!ok) {
            std::cerr << "Error al guardar imagen resultante en: " << output_path << "\n";
        } else {
            std::cout << " Imagen cartoon guardada exitosamente en: " << output_path << "\n";
            std::cout << " Tiempo I/O Guardado: " << std::fixed << std::setprecision(2) << t_save_ms << " ms\n";
            std::cout << "=======================================================\n\n";
        }
    }

    MPI_Finalize();
    return 0;
}
