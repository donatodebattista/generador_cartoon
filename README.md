# Trabajo Práctico Final Integrador - Computación Avanzada (UADER)
## Generador de Efecto Cartoon mediante Procesamiento Digital de Imágenes

Este repositorio contiene la implementación del Trabajo Práctico Final de la cátedra **Computación Avanzada (UADER)**.  
El objetivo es transformar imágenes digitales (PNG, JPG, BMP) aplicando un efecto estilizado tipo **Cartoon (dibujo animado)** mediante dos ramas independientes de procesamiento digital: **Detección de Bordes** y **Posterizado (Cuantización Tonal)**, implementado en cuatro paradigmas de cómputo:
1. **Versión 1 - Secuencial** (Línea Base $T_1$)
2. **Versión 2 - Memoria Compartida (OpenMP)**
3. **Versión 3 - Memoria Distribuida (MPI)**
4. **Versión 4 - Híbrida (MPI + OpenMP)**

---

## 1. Estructura del Proyecto

```
tp-final/
├── include/
│   ├── cartoon.hpp          # Pipeline cartoon secuencial
│   ├── cartoon_omp.hpp      # Pipeline cartoon paralelo OpenMP
│   ├── cartoon_mpi.hpp      # Pipeline cartoon distribuido MPI (intercambio de halos)
│   ├── cartoon_hybrid.hpp   # Pipeline cartoon híbrido (MPI + OpenMP)
│   ├── filters.hpp          # Filtros espaciales secuenciales (Borroneado y Sobel 3x3 / 5x5)
│   ├── filters_omp.hpp      # Filtros espaciales paralelos con OpenMP
│   ├── image.hpp            # Estructura Image RGB/Gray contigua y operaciones I/O
│   ├── mpi_utils.hpp        # Descomposición de dominio 1D (franjas y desplazamientos)
│   ├── posterize.hpp        # Cuantización tonal / LUT secuencial
│   ├── posterize_omp.hpp    # Posterizado y conversión a grises paralela OpenMP
│   ├── timer.hpp            # Temporizador de alta precisión (std::chrono)
│   ├── stb_image.h          # Decodificador de imágenes PNG, JPG, BMP (single-header)
│   └── stb_image_write.h    # Codificador de imágenes PNG, JPG, BMP (single-header)
├── src/
│   ├── cartoon.cpp          # Implementación del pipeline secuencial
│   ├── cartoon_omp.cpp      # Implementación del pipeline paralelo con omp_get_wtime()
│   ├── cartoon_mpi.cpp      # Implementación distribuida con MPI_Sendrecv y MPI_Scatterv/Gatherv
│   ├── cartoon_hybrid.cpp   # Implementación híbrida MPI_THREAD_FUNNELED + OpenMP
│   ├── filters.cpp          # Convoluciones espaciales secuenciales (padding neutro 0)
│   ├── filters_omp.cpp      # Convoluciones espaciales con directivas OpenMP
│   ├── image.cpp            # Carga, guardado y conversión a escala de grises
│   ├── main_seq.cpp         # CLI para versión Secuencial (Línea Base T1)
│   ├── main_omp.cpp         # CLI para versión OpenMP
│   ├── main_mpi.cpp         # CLI para versión MPI Pura
│   ├── main_hybrid.cpp      # CLI para versión Híbrida (MPI + OpenMP)
│   ├── mpi_utils.cpp        # Cálculo de franjas balanceadas para Scatterv/Gatherv
│   ├── posterize.cpp        # Precomputación de Look-Up Table (LUT)
│   └── posterize_omp.cpp    # Cuantización tonal paralelizada
├── scripts/
│   ├── generate_test_images.py  # Generador de imágenes de prueba (800x800, 2000x2000, 5000x5000)
│   ├── run_benchmarks_seq.sh    # Benchmark secuencial automático (genera benchmark_baseline_seq.csv)
│   ├── run_benchmarks_omp.sh    # Benchmark OpenMP (evalúa 1, 2, 4, 8 threads)
│   ├── run_benchmarks_mpi.sh    # Benchmark MPI (evalúa 1, 2, 4, 8 procesos)
│   └── run_benchmarks_hybrid.sh # Benchmark Híbrido (evalúa combinaciones P x T)
├── images/
│   ├── input/               # Imágenes de prueba de entrada
│   └── output/              # Resultados generados e imágenes intermedias
├── build/                   # Objetos compilados y ejecutables binarios
├── Makefile                 # Compilación modular automática (Secuencial, OpenMP, MPI e Híbrido)
├── REQUERIMIENTOS.md        # Especificación técnica oficial del TP
├── consignas-tp-final.pdf   # Consigna formal de la cátedra
└── Apuntes.pdf              # Material teórico de referencia
```

---

## 2. Fundamentos Matemáticos y Algoritmo

El pipeline implementa de manera estricta las especificaciones de `REQUERIMIENTOS.md` y `Apuntes.pdf`:

### 2.1 Tratamiento de Bordes (Padding)
Para cualquier operación de convolución espacial (filtros de $3\times3$ y $5\times5$), los píxeles adyacentes donde la máscara sobresalga asumen el **valor neutro negro `(0, 0, 0)`** conforme a la consigna.

### 2.2 Rama A: Detección de Bordes
1. **Conversión a Escala de Grises:**
   $$Y = 0.30 \cdot R + 0.59 \cdot G + 0.11 \cdot B$$
2. **Filtrado de Ruido (Borroneado por Media):**
   - Máscara $3\times3$: promedio vecindad $1/9 \sum_{i,j} P(x+i, y+j)$.
   - Máscara $5\times5$: promedio vecindad $1/25 \sum_{i,j} P(x+i, y+j)$.
3. **Resaltado de Bordes (Gradiente Sobel):**
   - **Sobel $3\times3$:**
     $$S_x = \begin{bmatrix} -1 & 0 & 1 \\ -2 & 0 & 2 \\ -1 & 0 & 1 \end{bmatrix}, \quad S_y = \begin{bmatrix} 1 & 2 & 1 \\ 0 & 0 & 0 \\ -1 & -2 & -1 \end{bmatrix}$$
   - **Sobel $5\times5$:**
     $$S_x^{(5)} = \begin{bmatrix} -1 & -2 & 0 & 2 & 1 \\ -2 & -3 & 0 & 3 & 2 \\ -3 & -5 & 0 & 5 & 3 \\ -2 & -3 & 0 & 3 & 2 \\ -1 & -2 & 0 & 2 & 1 \end{bmatrix}, \quad S_y^{(5)} = (S_x^{(5)})^T$$
   - Magnitud del gradiente:
     $$\|\nabla f(x, y)\| = \sqrt{G_x(x, y)^2 + G_y(x, y)^2} \quad \text{o} \quad |G_x| + |G_y|$$
4. **Binarización / Umbralado:**
   - Dado un umbral $T$ (configurable, por defecto $100$):
     $$Borde(x, y) = \begin{cases} 0 \text{ (trazo negro de contorno)} & \text{si } \|\nabla f(x, y)\| \ge T \\ 255 \text{ (zona plana)} & \text{si } \|\nabla f(x, y)\| < T \end{cases}$$

### 2.3 Rama B: Posterizado (Cuantización Tonal)
Se cuantiza cada canal RGB de forma independiente en $N$ intervalos discretos ($N=3$ o $N=9$):
- $\Delta = \frac{256}{N}$
- $\text{índice} = \min\left(\left\lfloor \frac{v}{\Delta} \right\rfloor, N - 1\right)$
- $v_{post} = \min\left(255, \left\lfloor (\text{índice} + 0.5) \cdot \Delta \right\rfloor\right)$
*(Se utiliza una Look-Up Table (LUT) de 256 bytes para acceso $O(1)$ por byte sin divisiones).*

### 2.4 Fusión Final (Cartoon)
- **Modo Máscara (recomendado y predeterminado):**
  $$Cartoon_c(x, y) = \begin{cases} 0 & \text{si es borde} \\ Posterizado_c(x, y) & \text{si no es borde} \end{cases}$$
- **Modo Suma Directa:**
  $$Cartoon_c(x, y) = \min(255, Posterizado_c(x, y) + Borde(x, y))$$

---

## 3. Arquitecturas Paralelas y Distribuidas

### 3.1 Versión 2: Memoria Compartida (OpenMP)
- Paraleliza los bucles intensivos (`#pragma omp parallel for`) distribuyendo filas entre hilos.
- Mide los tiempos con `omp_get_wtime()`.

### 3.2 Versión 3: Memoria Distribuida (MPI)
- **Descomposición 1D por Franjas Horizontales:** Divide $H$ filas entre $P$ procesos utilizando `MPI_Scatterv` y `MPI_Gatherv`.
- **Manejo de Filas Fantasma (*Ghost Rows / Halo*):**
  - Radio $R_{halo} = 1$ para filtro $3\times3$, $R_{halo} = 2$ para filtro $5\times5$.
  - Intercambio bidireccional punto a punto con `MPI_Sendrecv` con los vecinos superior e inferior.
  - Procesos extremos asumen **valor neutro negro `(0, 0, 0)`** en el halo exterior.

### 3.3 Versión 4: Híbrida (MPI + OpenMP)
- Inicialización con `MPI_Init_thread` bajo nivel **`MPI_THREAD_FUNNELED`**: el hilo principal coordina los pasajes de mensajes entre nodos y un equipo de hilos OpenMP paraleliza internamente las convoluciones espaciales y cuantizaciones de cada franja local.
- Permite evaluar el balance entre granularidad de procesos ($P$) e hilos ($T$) para un número dado de núcleos totales ($P \times T$).

---

## 4. Instalación y Compilación

### Instalación de dependencias MPI (Ubuntu / Debian):
```bash
sudo apt update && sudo apt install -y libopenmpi-dev openmpi-bin
```

### Compilación con `make`:
```bash
# Compilar todas las versiones disponibles:
make

# O compilar específicamente cada versión:
make seq      # Genera build/cartoon_seq
make omp      # Genera build/cartoon_omp
make mpi      # Genera build/cartoon_mpi (requiere mpicxx)
make hybrid   # Genera build/cartoon_hybrid (requiere mpicxx)
```

---

## 5. Uso de las Aplicaciones por Consola (CLI)

### 5.1 Versión Secuencial (Hito 1 - Línea Base $T_1$)
```bash
./build/cartoon_seq <input_img> <output_img> <filter_size: 3|5> <posterize_levels: 3|9> [opciones]
```

### 5.2 Versión Memoria Compartida (Hito 2 - OpenMP)
```bash
./build/cartoon_omp <input_img> <output_img> <filter_size: 3|5> <posterize_levels: 3|9> --threads <N> [opciones]
```

### 5.3 Versión Memoria Distribuida (Hito 3 - MPI)
```bash
mpirun -np <P> [--oversubscribe] ./build/cartoon_mpi <input_img> <output_img> <filter_size: 3|5> <posterize_levels: 3|9> [opciones]
```

### 5.4 Versión Híbrida (Hito 4 - MPI + OpenMP)
```bash
mpirun -np <P> [--oversubscribe] ./build/cartoon_hybrid <input_img> <output_img> <filter_size: 3|5> <posterize_levels: 3|9> --threads <T> [opciones]
```

#### Parámetros Comunes:
- `<filter_size>`: `3` ($3\times3$) o `5` ($5\times5$).
- `<posterize_levels>`: `3` o `9`.
- `--threshold <val>`: Umbral Sobel $T$ (defecto: `100`).
- `--metric <m>`: Métrica de gradiente (`euclidean` o `manhattan`).
- `--blend <mode>`: Modo de fusión (`mask` o `sum`).

---

## 6. Pruebas y Análisis de Rendimiento

### 6.1 Pruebas Rápidas con `make`
```bash
make test          # Test secuencial (800x800)
make test_omp      # Test OpenMP (4 threads)
make test_mpi      # Test MPI (4 procesos)
make test_hybrid   # Test Híbrido (2 procesos x 2 threads)
```

### 6.2 Baterías Automatizadas para el Informe
- **Línea Base Secuencial:** `./scripts/run_benchmarks_seq.sh` $\to$ `benchmark_baseline_seq.csv`.
- **Escalabilidad OpenMP:** `./scripts/run_benchmarks_omp.sh` $\to$ `benchmark_results_omp.csv`.
- **Escalabilidad MPI:** `./scripts/run_benchmarks_mpi.sh` $\to$ `benchmark_results_mpi.csv`.
- **Comparativa Híbrida ($P \times T$):** `./scripts/run_benchmarks_hybrid.sh` $\to$ `benchmark_results_hybrid.csv`.

Calculando automáticamente para cada configuración:
- **Speedup:**
  $$S_p = \frac{T_1}{T_p}$$
- **Eficiencia:**
  $$E_p = \frac{S_p}{\text{Workers}} = \frac{T_1}{(P \cdot T) \cdot T_p}$$
