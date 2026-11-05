#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>
#include <omp.h>

#include "image.hpp"
#include "filters.hpp"
#include "posterize.hpp"
#include "cartoon.hpp"
#include "filters_omp.hpp"
#include "cartoon_omp.hpp"
#include "timer.hpp"

static void print_usage(const char* prog_name) {
    std::cout << "========================================================================\n"
              << " Generador de Efecto Cartoon - Computacion Avanzada UADER (OpenMP)\n"
              << "========================================================================\n"
              << "Uso:\n"
              << "  " << prog_name << " <input_img> <output_img> <filter_size> <posterize_levels> [opciones]\n\n"
              << "Argumentos obligatorios:\n"
              << "  <input_img>         Ruta a la imagen de entrada (PNG, JPG, BMP)\n"
              << "  <output_img>        Ruta donde guardar la imagen resultante (PNG, JPG)\n"
              << "  <filter_size>       Tamaño del filtro espacial: 3 (3x3) o 5 (5x5)\n"
              << "  <posterize_levels>  Niveles de cuantizacion tonal: 3 o 9\n\n"
              << "Opciones OpenMP:\n"
              << "  --threads <N>       Cantidad de threads OpenMP (1, 2, 4, 8...; defecto: todos)\n"
              << "  --schedule <sched>  Politica de scheduling: 'static' (defecto), 'dynamic', 'guided'\n\n"
              << "Opciones adicionales:\n"
              << "  --threshold <val>   Umbral T para binarizacion Sobel (defecto: 100, rango 0-255)\n"
              << "  --metric <m>        Metrica de gradiente: 'euclidean' (defecto) o 'manhattan'\n"
              << "  --blend <mode>      Modo de fusion: 'mask' (defecto, contorno negro) o 'sum' (suma)\n"
              << "  --intermediates     Guarda las imagenes intermedias (gray, blur, grad, edges, post)\n"
              << "  --iterations <N>    Ejecuta el procesamiento N veces y reporta promedio (defecto: 1)\n"
              << "  -h, --help          Muestra esta ayuda y finaliza\n\n"
              << "Ejemplo:\n"
              << "  " << prog_name << " images/input/foto.jpg images/output/cartoon_omp.png 3 3 --threads 4\n"
              << "========================================================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "-h" || arg1 == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    if (argc < 5) {
        std::cerr << "Error: Faltan argumentos requeridos.\n\n";
        print_usage(argv[0]);
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];
    int filter_size = std::atoi(argv[3]);
    int posterize_levels = std::atoi(argv[4]);

    if (filter_size != 3 && filter_size != 5) {
        std::cerr << "Error: filter_size debe ser 3 o 5 (recibido: " << filter_size << ")\n";
        return 1;
    }

    if (posterize_levels != 3 && posterize_levels != 9) {
        std::cerr << "Advertencia: La consigna especifica 3 o 9 niveles de posterizado (recibido: "
                  << posterize_levels << "). Continuando...\n";
        if (posterize_levels < 2 || posterize_levels > 256) {
            std::cerr << "Error: posterize_levels fuera de rango valido [2, 256].\n";
            return 1;
        }
    }

    PipelineConfigOMP config;
    config.filter_size = filter_size;
    config.posterize_levels = posterize_levels;
    config.edge_threshold = 100;
    config.metric = GradientMetric::EUCLIDEAN;
    config.blend_mode = BlendMode::MASK;
    config.num_threads = omp_get_max_threads();
    config.schedule = OmpSchedule::STATIC;
    config.save_intermediates = false;

    int benchmark_iterations = 1;
    std::string sched_str = "static";

    for (int i = 5; i < argc; ++i) {
        std::string flag = argv[i];
        if (flag == "--threads" && i + 1 < argc) {
            config.num_threads = std::max(1, std::atoi(argv[++i]));
        } else if (flag == "--schedule" && i + 1 < argc) {
            sched_str = argv[++i];
            if (sched_str == "static") {
                config.schedule = OmpSchedule::STATIC;
            } else if (sched_str == "dynamic") {
                config.schedule = OmpSchedule::DYNAMIC;
            } else if (sched_str == "guided") {
                config.schedule = OmpSchedule::GUIDED;
            } else {
                std::cerr << "Error: schedule desconocido '" << sched_str
                          << "'. Use 'static', 'dynamic' o 'guided'.\n";
                return 1;
            }
        } else if (flag == "--threshold" && i + 1 < argc) {
            int t = std::atoi(argv[++i]);
            if (t < 0 || t > 255) {
                std::cerr << "Error: threshold debe estar entre 0 y 255.\n";
                return 1;
            }
            config.edge_threshold = static_cast<uint8_t>(t);
        } else if (flag == "--metric" && i + 1 < argc) {
            std::string m = argv[++i];
            if (m == "euclidean") {
                config.metric = GradientMetric::EUCLIDEAN;
            } else if (m == "manhattan") {
                config.metric = GradientMetric::MANHATTAN;
            } else {
                std::cerr << "Error: metrica desconocida '" << m << "'. Use 'euclidean' o 'manhattan'.\n";
                return 1;
            }
        } else if (flag == "--blend" && i + 1 < argc) {
            std::string b = argv[++i];
            if (b == "mask") {
                config.blend_mode = BlendMode::MASK;
            } else if (b == "sum") {
                config.blend_mode = BlendMode::SUM;
            } else {
                std::cerr << "Error: blend mode desconocido '" << b << "'. Use 'mask' o 'sum'.\n";
                return 1;
            }
        } else if (flag == "--intermediates") {
            config.save_intermediates = true;
            auto dot_pos = output_path.find_last_of('.');
            if (dot_pos != std::string::npos) {
                config.intermediate_prefix = output_path.substr(0, dot_pos);
            } else {
                config.intermediate_prefix = output_path;
            }
        } else if (flag == "--iterations" && i + 1 < argc) {
            benchmark_iterations = std::max(1, std::atoi(argv[++i]));
        } else {
            std::cerr << "Advertencia: Opcion desconocida o sin argumento: " << flag << "\n";
        }
    }

    std::cout << "\n=======================================================\n";
    std::cout << "        TP FINAL - COMPUTACION AVANZADA (UADER)        \n";
    std::cout << "        Pipeline Cartoonizador Paralelo (OpenMP)       \n";
    std::cout << "=======================================================\n";
    std::cout << " Configuracion Paralela:\n";
    std::cout << "  - Threads Activos     : " << config.num_threads << " (Max HW: " << omp_get_max_threads() << ")\n";
    std::cout << "  - Scheduling Loop     : " << sched_str << "\n";
    std::cout << "  - Imagen de entrada   : " << input_path << "\n";
    std::cout << "  - Imagen de salida    : " << output_path << "\n";
    std::cout << "  - Filtro Espacial     : " << config.filter_size << "x" << config.filter_size << "\n";
    std::cout << "  - Niveles Posterizado : " << config.posterize_levels << "\n";
    std::cout << "  - Umbral Sobel (T)    : " << static_cast<int>(config.edge_threshold) << "\n";
    std::cout << "  - Metrica Gradiente   : " << (config.metric == GradientMetric::EUCLIDEAN ? "Euclidea (sqrt)" : "Manhattan (|Gx|+|Gy|)") << "\n";
    std::cout << "  - Modo de Fusion      : " << (config.blend_mode == BlendMode::MASK ? "Mascara (Contorno negro)" : "Suma directa") << "\n";
    if (config.save_intermediates) {
        std::cout << "  - Guardar Intermedias : SI (Prefijo: " << config.intermediate_prefix << ")\n";
    }
    std::cout << "-------------------------------------------------------\n";

    // 1. Carga de imagen (medida fuera del procesamiento del algoritmo)
    Timer io_load_timer;
    Image input_img;
    try {
        input_img = load_image(input_path, 3);
    } catch (const std::exception& e) {
        std::cerr << "Error critico al cargar imagen: " << e.what() << "\n";
        return 1;
    }
    double t_load_ms = io_load_timer.elapsed_ms();

    std::cout << " Imagen cargada con exito:\n";
    std::cout << "  - Dimensiones: " << input_img.width << " x " << input_img.height << " px ("
              << std::fixed << std::setprecision(2) << (input_img.width * input_img.height / 1e6) << " Megapixels)\n";
    std::cout << "  - Canales    : " << input_img.channels << " (RGB 24-bit)\n";
    std::cout << "  - Tiempo I/O Carga: " << std::fixed << std::setprecision(2) << t_load_ms << " ms\n";
    std::cout << "-------------------------------------------------------\n";

    // 2. Ejecución del pipeline de procesamiento OpenMP
    std::cout << " Ejecutando algoritmo cartoon en paralelo con OpenMP...\n";
    Image output_img;
    PipelineStats final_stats;

    if (benchmark_iterations == 1) {
        output_img = run_cartoon_pipeline_omp(input_img, config, &final_stats);
    } else {
        std::cout << " Realizando benchmark con " << benchmark_iterations << " repeticiones...\n";
        double sum_total = 0.0;
        for (int it = 0; it < benchmark_iterations; ++it) {
            PipelineStats it_stats;
            PipelineConfigOMP it_config = config;
            if (it > 0) it_config.save_intermediates = false;

            output_img = run_cartoon_pipeline_omp(input_img, it_config, &it_stats);
            sum_total += it_stats.time_total_proc_ms;
            final_stats = it_stats;
        }
        std::cout << " Tiempo promedio (" << benchmark_iterations << " it): "
                  << std::fixed << std::setprecision(3) << (sum_total / benchmark_iterations) << " ms\n";
    }

    // Reporte de tiempos con omp_get_wtime()
    std::cout << "\n=======================================================\n";
    std::cout << "     DESGLOSE DE TIEMPOS DE PROCESAMIENTO (OpenMP)     \n";
    std::cout << "=======================================================\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << " [Rama A: Bordes]\n";
    std::cout << "  - Grayscale Y = 0.3R+0.59G+0.11B : " << std::setw(8) << final_stats.time_grayscale_ms << " ms\n";
    std::cout << "  - Filtro Media (" << config.filter_size << "x" << config.filter_size << ")          : "
              << std::setw(8) << final_stats.time_blur_ms << " ms\n";
    std::cout << "  - Gradiente Sobel (" << config.filter_size << "x" << config.filter_size << ")        : "
              << std::setw(8) << final_stats.time_sobel_ms << " ms\n";
    std::cout << "  - Binarizacion (Umbral T=" << (int)config.edge_threshold << ")      : "
              << std::setw(8) << final_stats.time_threshold_ms << " ms\n";
    std::cout << " [Rama B: Posterizado]\n";
    std::cout << "  - Cuantizacion (" << config.posterize_levels << " niveles)           : "
              << std::setw(8) << final_stats.time_posterize_ms << " ms\n";
    std::cout << " [Combinacion Final]\n";
    std::cout << "  - Fusion Cartoon                 : " << std::setw(8) << final_stats.time_blend_ms << " ms\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << " TIEMPO TOTAL DE PROCESAMIENTO (Tp): "
              << std::setw(8) << final_stats.time_total_proc_ms << " ms\n";
    std::cout << "=======================================================\n\n";

    // 3. Guardado de imagen (medida fuera del procesamiento del algoritmo)
    Timer io_save_timer;
    bool ok = save_image(output_path, output_img);
    double t_save_ms = io_save_timer.elapsed_ms();

    if (!ok) {
        std::cerr << "Error al guardar imagen de salida en: " << output_path << "\n";
        return 1;
    }

    std::cout << " Imagen cartoon guardada exitosamente en: " << output_path << "\n";
    std::cout << " Tiempo I/O Guardado: " << std::fixed << std::setprecision(2) << t_save_ms << " ms\n";
    std::cout << "=======================================================\n\n";

    return 0;
}
