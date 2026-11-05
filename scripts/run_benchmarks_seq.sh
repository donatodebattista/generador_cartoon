#!/usr/bin/env bash
# ==============================================================================
# Script de Automatización de Benchmarks Secuenciales (Línea Base T1)
# Matriz de Pruebas:
#   - Resoluciones: 800x800, 2000x2000, 5000x5000 px
#   - Filtros espaciales: 3x3, 5x5
#   - Posterizado: 3, 9 niveles
# ==============================================================================

set -e

BUILD_BIN="./build/cartoon_seq"
INPUT_DIR="images/input"
OUTPUT_DIR="images/output"
REPORT_CSV="benchmark_baseline_seq.csv"

# Asegurar compilación
make -s seq

# Generar imágenes de prueba si faltan
echo "==> Verificando imágenes de prueba..."
python3 scripts/generate_test_images.py 5000

echo "==> Iniciando corrida de benchmark secuencial (Línea Base T1)..."
echo "Resolucion,Filtro,Posterizado,T_Proc_ms,T_IO_Carga_ms,T_IO_Guardado_ms" > "$REPORT_CSV"

RESOLUTIONS=("800x800" "2000x2000" "5000x5000")
FILTERS=("3" "5")
POSTERIZES=("3" "9")

for res in "${RESOLUTIONS[@]}"; do
    input_file="${INPUT_DIR}/test_${res}.png"
    if [ ! -f "$input_file" ]; then
        echo "Error: No se encontró $input_file"
        continue
    fi

    for f in "${FILTERS[@]}"; do
        for p in "${POSTERIZES[@]}"; do
            out_file="${OUTPUT_DIR}/out_${res}_f${f}_p${p}.png"
            echo "--------------------------------------------------------"
            echo " Ejecutando: ${res} | Filtro: ${f}x${f} | Posterizado: ${p}"
            
            # Ejecutar y capturar salida
            output=$("$BUILD_BIN" "$input_file" "$out_file" "$f" "$p")

            # Extraer tiempos usando awk/grep
            t_proc=$(echo "$output" | grep "TIEMPO TOTAL DE PROCESAMIENTO" | awk '{print $(NF-1)}')
            t_load=$(echo "$output" | grep "Tiempo I/O Carga:" | awk '{print $(NF-1)}')
            t_save=$(echo "$output" | grep "Tiempo I/O Guardado:" | awk '{print $(NF-1)}')

            echo "   -> T_Proc: ${t_proc} ms | T_Carga: ${t_load} ms | T_Guardado: ${t_save} ms"
            echo "${res},${f}x${f},${p},${t_proc},${t_load},${t_save}" >> "$REPORT_CSV"
        done
    done
done

echo "========================================================"
echo " Benchmarks finalizados con éxito."
echo " Resultados guardados en: ${REPORT_CSV}"
echo "========================================================"
cat "$REPORT_CSV"
