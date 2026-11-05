#!/usr/bin/env bash
# ==============================================================================
# Script de Automatización de Benchmarks OpenMP (Hito 2)
# Evalúa la matriz requerida por la cátedra para 1, 2, 4, 8 threads
# Calcula Speedup (Sp = T1 / Tp) y Eficiencia (Ep = Sp / p)
# ==============================================================================

set -e

BUILD_OMP="./build/cartoon_omp"
BUILD_SEQ="./build/cartoon_seq"
INPUT_DIR="images/input"
OUTPUT_DIR="images/output"
REPORT_CSV="benchmark_results_omp.csv"

# Asegurar compilación
make -s all

# Asegurar imágenes de prueba (800x800, 2000x2000 y 5000x5000)
echo "==> Verificando imágenes de prueba..."
python3 scripts/generate_test_images.py 5000

echo "==> Iniciando batería de pruebas OpenMP..."
echo "Resolucion,Filtro,Posterizado,Threads,T_Seq_T1_ms,T_Par_Tp_ms,Speedup_Sp,Eficiencia_Ep" > "$REPORT_CSV"

RESOLUTIONS=("800x800" "2000x2000" "5000x5000")
FILTERS=("3" "5")
POSTERIZES=("3" "9")
THREADS_LIST=("1" "2" "4" "8")

for res in "${RESOLUTIONS[@]}"; do
    input_file="${INPUT_DIR}/test_${res}.png"
    if [ ! -f "$input_file" ]; then
        echo "Error: No se encontró $input_file"
        continue
    fi

    for f in "${FILTERS[@]}"; do
        for p in "${POSTERIZES[@]}"; do
            echo "================================================================="
            echo " Evaluando caso: ${res} | Filtro ${f}x${f} | Posterizado ${p} niveles"
            echo "================================================================="

            # 1. Obtener tiempo base secuencial T1
            seq_out="${OUTPUT_DIR}/seq_bench_${res}_f${f}_p${p}.png"
            seq_log=$("$BUILD_SEQ" "$input_file" "$seq_out" "$f" "$p" --iterations 1)
            t1=$(echo "$seq_log" | grep "TIEMPO TOTAL DE PROCESAMIENTO (T1):" | awk '{print $(NF-1)}')

            echo " -> Línea Base Secuencial (T1): ${t1} ms"

            # 2. Evaluar para cada configuración de threads en OpenMP
            for th in "${THREADS_LIST[@]}"; do
                omp_out="${OUTPUT_DIR}/omp_${res}_f${f}_p${p}_th${th}.png"
                omp_log=$("$BUILD_OMP" "$input_file" "$omp_out" "$f" "$p" --threads "$th" --iterations 1)
                tp=$(echo "$omp_log" | grep "TIEMPO TOTAL DE PROCESAMIENTO (Tp):" | awk '{print $(NF-1)}')

                # Calcular Sp y Ep con awk
                metrics=$(awk -v t1="$t1" -v tp="$tp" -v p="$th" 'BEGIN {
                    sp = (tp > 0) ? (t1 / tp) : 0;
                    ep = (p > 0) ? (sp / p) : 0;
                    printf "%.3f,%.3f", sp, ep;
                }')

                sp=$(echo "$metrics" | cut -d',' -f1)
                ep=$(echo "$metrics" | cut -d',' -f2)

                printf "    [Threads: %2d] Tp: %9.3f ms | Speedup: %5.2fx | Eficiencia: %5.2f%%\n" \
                       "$th" "$tp" "$sp" "$(awk -v e="$ep" 'BEGIN {printf "%.1f", e*100}')"

                echo "${res},${f}x${f},${p},${th},${t1},${tp},${sp},${ep}" >> "$REPORT_CSV"
            done
        done
    done
done

echo "================================================================="
echo " Benchmarking OpenMP finalizado."
echo " Resultados completos guardados en: ${REPORT_CSV}"
echo "================================================================="
