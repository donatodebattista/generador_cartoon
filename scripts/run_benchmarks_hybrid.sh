#!/usr/bin/env bash
# ==============================================================================
# Script de Automatización de Benchmarks Híbridos (MPI + OpenMP - Hito 4)
# Evalúa configuraciones P x T (Procesos MPI x Threads OpenMP)
# Calcula Speedup (Sp = T1 / Tp) y Eficiencia (Ep = Sp / (P * T))
# ==============================================================================

set -e

BUILD_HYB="./build/cartoon_hybrid"
BUILD_SEQ="./build/cartoon_seq"
INPUT_DIR="images/input"
OUTPUT_DIR="images/output"
REPORT_CSV="benchmark_results_hybrid.csv"

# Verificar binarios
if [ ! -f "$BUILD_HYB" ]; then
    echo "Compilando versión Híbrida..."
    make hybrid
fi
if [ ! -f "$BUILD_SEQ" ]; then
    echo "Compilando versión Secuencial..."
    make seq
fi

echo "==> Verificando imágenes de prueba..."
python3 scripts/generate_test_images.py 5000

echo "==> Iniciando batería de pruebas Híbridas (MPI + OpenMP)..."
echo "Resolucion,Filtro,Posterizado,Procesos_P,Threads_T,Total_Workers,T_Seq_T1_ms,T_Hyb_Tp_ms,Speedup_Sp,Eficiencia_Ep" > "$REPORT_CSV"

RESOLUTIONS=("800x800" "2000x2000" "5000x5000")
FILTERS=("3" "5")
POSTERIZES=("3" "9")

# Configuraciones P x T: (1x1, 2x1, 1x2, 2x2, 4x1, 1x4, 4x2, 2x4)
CONFIGS=(
    "1 1"
    "2 1"
    "1 2"
    "2 2"
    "4 1"
    "1 4"
    "2 4"
    "4 2"
)

for res in "${RESOLUTIONS[@]}"; do
    input_file="${INPUT_DIR}/test_${res}.png"
    if [ ! -f "$input_file" ]; then
        echo "Error: No se encontró $input_file"
        continue
    fi

    for f in "${FILTERS[@]}"; do
        for p in "${POSTERIZES[@]}"; do
            echo "================================================================="
            echo " Evaluando Híbrido: ${res} | Filtro ${f}x${f} | Posterizado ${p} niveles"
            echo "================================================================="

            # 1. Obtener tiempo base secuencial T1
            seq_out="${OUTPUT_DIR}/seq_bench_${res}_f${f}_p${p}.png"
            seq_log=$("$BUILD_SEQ" "$input_file" "$seq_out" "$f" "$p" --iterations 1)
            t1=$(echo "$seq_log" | grep "TIEMPO TOTAL DE PROCESAMIENTO (T1):" | awk '{print $(NF-1)}')

            echo " -> Línea Base Secuencial (T1): ${t1} ms"

            # 2. Evaluar cada configuración (P x T)
            for cfg in "${CONFIGS[@]}"; do
                np=$(echo "$cfg" | awk '{print $1}')
                th=$(echo "$cfg" | awk '{print $2}')
                workers=$(( np * th ))

                hyb_out="${OUTPUT_DIR}/hyb_${res}_f${f}_p${p}_p${np}_t${th}.png"
                hyb_log=$(mpirun -np "$np" --oversubscribe "$BUILD_HYB" "$input_file" "$hyb_out" "$f" "$p" --threads "$th")
                tp=$(echo "$hyb_log" | grep "TIEMPO TOTAL DE PROCESAMIENTO HIBRIDO (Tp):" | awk '{print $(NF-1)}')

                metrics=$(awk -v t1="$t1" -v tp="$tp" -v w="$workers" 'BEGIN {
                    sp = (tp > 0) ? (t1 / tp) : 0;
                    ep = (w > 0) ? (sp / w) : 0;
                    printf "%.3f,%.3f", sp, ep;
                }')

                sp=$(echo "$metrics" | cut -d',' -f1)
                ep=$(echo "$metrics" | cut -d',' -f2)

                printf "    [%d Proc x %d Th = %2d Workers] Tp: %9.3f ms | Speedup: %5.2fx | Eficiencia: %5.2f%%\n" \
                       "$np" "$th" "$workers" "$tp" "$sp" "$(awk -v e="$ep" 'BEGIN {printf "%.1f", e*100}')"

                echo "${res},${f}x${f},${p},${np},${th},${workers},${t1},${tp},${sp},${ep}" >> "$REPORT_CSV"
            done
        done
    done
done

echo "================================================================="
echo " Benchmarking Híbrido finalizado."
echo " Resultados completos guardados en: ${REPORT_CSV}"
echo "================================================================="
