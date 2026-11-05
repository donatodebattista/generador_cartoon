import matplotlib.pyplot as plt
import numpy as np

# Set general style
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['font.size'] = 10

# 1. Pipeline Diagram
def generate_pipeline_diagram():
    fig, ax = plt.subplots(figsize=(10, 5.2), dpi=200)
    ax.axis('off')
    
    # Draw boxes
    bbox_props = dict(boxstyle="round,pad=0.5", fc="#EBF3FA", ec="#1F4E79", lw=1.8)
    bbox_rama_a = dict(boxstyle="round,pad=0.4", fc="#FEF3D6", ec="#D97706", lw=1.5)
    bbox_rama_b = dict(boxstyle="round,pad=0.4", fc="#E0F2FE", ec="#0284C7", lw=1.5)
    bbox_merge = dict(boxstyle="round,pad=0.5", fc="#D1FAE5", ec="#059669", lw=1.8)
    
    # Nodes
    ax.text(0.12, 0.5, "Imagen Original\nRGB (W x H)\n24 bits", ha="center", va="center", bbox=bbox_props, fontsize=10, weight='bold')
    
    # Rama A
    ax.text(0.42, 0.85, "RAMA A: DETECCIÓN DE BORDES\n(Filtros y Detección Estructural)", ha="center", va="center", fontsize=11, weight='bold', color="#B45309")
    ax.text(0.42, 0.72, "1. Luminancia (Grises)\nY = 0.30R + 0.59G + 0.11B", ha="center", va="center", bbox=bbox_rama_a, fontsize=9)
    ax.text(0.42, 0.58, "2. Filtro de Media (Blur)\nMáscara 3x3 o 5x5 (Padding 0)", ha="center", va="center", bbox=bbox_rama_a, fontsize=9)
    ax.text(0.42, 0.44, "3. Gradiente de Sobel\nDerivadas Gx, Gy y Magnitud ||G||", ha="center", va="center", bbox=bbox_rama_a, fontsize=9)
    ax.text(0.42, 0.30, "4. Umbralado / Binarización\nBorde = 0 (Negro), Fondo = 255", ha="center", va="center", bbox=bbox_rama_a, fontsize=9)
    
    # Rama B
    ax.text(0.72, 0.85, "RAMA B: POSTERIZADO\n(Simplificación Cromática)", ha="center", va="center", fontsize=11, weight='bold', color="#0369A1")
    ax.text(0.72, 0.65, "1. Discretización Tonal\n3 o 9 rangos por canal R, G, B", ha="center", va="center", bbox=bbox_rama_b, fontsize=9)
    ax.text(0.72, 0.45, "2. Optimización por LUT\nTabla precalculada de 256 bytes\nCómputo en O(1) por píxel", ha="center", va="center", bbox=bbox_rama_b, fontsize=9)
    
    # Fusión
    ax.text(0.92, 0.5, "FUSIÓN Y SÍNTESIS\nMultiplicación /\nComposición Final\n→ Imagen Cartoon RGB", ha="center", va="center", bbox=bbox_merge, fontsize=9.5, weight='bold')
    
    # Arrows
    arrow_props = dict(arrowstyle="->", color="#334155", lw=1.5, connectionstyle="arc3,rad=0")
    
    # From input to branches
    ax.annotate("", xy=(0.28, 0.72), xytext=(0.20, 0.55), arrowprops=arrow_props)
    ax.annotate("", xy=(0.58, 0.65), xytext=(0.20, 0.45), arrowprops=arrow_props)
    
    # Rama A sequence
    ax.annotate("", xy=(0.42, 0.63), xytext=(0.42, 0.67), arrowprops=arrow_props)
    ax.annotate("", xy=(0.42, 0.49), xytext=(0.42, 0.53), arrowprops=arrow_props)
    ax.annotate("", xy=(0.42, 0.35), xytext=(0.42, 0.39), arrowprops=arrow_props)
    
    # Rama B sequence
    ax.annotate("", xy=(0.72, 0.51), xytext=(0.72, 0.59), arrowprops=arrow_props)
    
    # To Merge
    ax.annotate("", xy=(0.84, 0.52), xytext=(0.54, 0.30), arrowprops=dict(arrowstyle="->", color="#334155", lw=1.5, connectionstyle="arc3,rad=-0.1"))
    ax.annotate("", xy=(0.84, 0.48), xytext=(0.72, 0.39), arrowprops=dict(arrowstyle="->", color="#334155", lw=1.5, connectionstyle="arc3,rad=0.1"))
    
    ax.set_xlim(0, 1.02)
    ax.set_ylim(0.18, 0.95)
    plt.tight_layout()
    plt.savefig("images/charts/pipeline_flow.png", bbox_inches='tight')
    plt.close()
    print("pipeline_flow.png generated")

# 2. Sequential Breakdown Chart
def generate_sequential_breakdown():
    fig, ax = plt.subplots(figsize=(7, 4.2), dpi=200)
    stages = ['Conversión\na Grises', 'Filtro de Media\n(Borroneado)', 'Gradiente\nde Sobel', 'Umbralado\n(Binarización)', 'Posterizado\n(LUT)', 'Fusión\nFinal']
    percentages_3x3 = [6.65, 35.59, 44.06, 1.01, 6.97, 5.71]
    percentages_5x5 = [5.08, 43.08, 42.54, 0.65, 5.23, 3.42]
    
    x = np.arange(len(stages))
    width = 0.38
    
    rects1 = ax.bar(x - width/2, percentages_3x3, width, label='Máscara 3x3 (N=3)', color='#2563EB', edgecolor='black', linewidth=0.5)
    rects2 = ax.bar(x + width/2, percentages_5x5, width, label='Máscara 5x5 (N=9)', color='#D97706', edgecolor='black', linewidth=0.5)
    
    ax.set_ylabel('Porcentaje del Tiempo de CPU (%)', fontsize=10, weight='bold')
    ax.set_title('Desglose del Tiempo de Cómputo Secuencial por Etapa', fontsize=11, weight='bold', pad=12)
    ax.set_xticks(x)
    ax.set_xticklabels(stages, fontsize=8.5)
    ax.legend(frameon=True, facecolor='white', edgecolor='#CBD5E1')
    ax.grid(axis='y', linestyle='--', alpha=0.6)
    
    # Add values on top of bars
    for rect in rects1:
        height = rect.get_height()
        ax.annotate(f'{height:.1f}%', xy=(rect.get_x() + rect.get_width()/2, height),
                    xytext=(0, 3), textcoords="offset points", ha='center', va='bottom', fontsize=7.5)
    for rect in rects2:
        height = rect.get_height()
        ax.annotate(f'{height:.1f}%', xy=(rect.get_x() + rect.get_width()/2, height),
                    xytext=(0, 3), textcoords="offset points", ha='center', va='bottom', fontsize=7.5)
        
    # Annotation highlight
    ax.annotate('Convoluciones espaciales concentran\nentre el 79.6% y el 85.6% del tiempo',
                xy=(1.5, 45), xytext=(2.2, 50),
                arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=5),
                fontsize=8.5, weight='bold', bbox=dict(boxstyle="round,pad=0.3", fc="#FEF3C7", ec="#F59E0B"))
    
    ax.set_ylim(0, 58)
    plt.tight_layout()
    plt.savefig("images/charts/chart_hotspots_seq.png", bbox_inches='tight')
    plt.close()
    print("chart_hotspots_seq.png generated")

# 3. Speedup 5000x5000 Comparison Chart
def generate_speedup_5000():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.5), dpi=200)
    
    workers = ['1', '2', '4', '8']
    x = np.arange(len(workers))
    width = 0.25
    
    # Caso 3x3
    sp_omp_3x3 = [1.101, 1.707, 2.848, 2.037]
    sp_mpi_3x3 = [1.399, 2.170, 2.600, 2.367]
    sp_hyb_3x3 = [1.278, 1.926, 3.133, 2.683] # Melhores configuraciones híbridas
    
    ax1.bar(x - width, sp_omp_3x3, width, label='OpenMP', color='#1D4ED8')
    ax1.bar(x, sp_mpi_3x3, width, label='MPI', color='#059669')
    ax1.bar(x + width, sp_hyb_3x3, width, label='Híbrido (Óptimo)', color='#D97706')
    ax1.axhline(y=1.0, color='gray', linestyle=':', label='Línea Base (1x)')
    
    ax1.set_xlabel('Unidades de Cómputo (Trabajadores)', fontsize=9.5, weight='bold')
    ax1.set_ylabel('Speedup (Sp = T1 / Tp)', fontsize=9.5, weight='bold')
    ax1.set_title('Imagen 5000x5000 - Filtro 3x3', fontsize=10.5, weight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(workers)
    ax1.set_ylim(0, 3.6)
    ax1.grid(axis='y', linestyle='--', alpha=0.5)
    ax1.legend(loc='upper left', fontsize=8.5)
    
    # Caso 5x5
    sp_omp_5x5 = [0.983, 1.405, 2.467, 2.210]
    sp_mpi_5x5 = [1.145, 1.857, 2.737, 2.453]
    sp_hyb_5x5 = [1.120, 1.742, 3.003, 2.586]
    
    ax2.bar(x - width, sp_omp_5x5, width, label='OpenMP', color='#1D4ED8')
    ax2.bar(x, sp_mpi_5x5, width, label='MPI', color='#059669')
    ax2.bar(x + width, sp_hyb_5x5, width, label='Híbrido (Óptimo)', color='#D97706')
    ax2.axhline(y=1.0, color='gray', linestyle=':')
    
    ax2.set_xlabel('Unidades de Cómputo (Trabajadores)', fontsize=9.5, weight='bold')
    ax2.set_ylabel('Speedup (Sp = T1 / Tp)', fontsize=9.5, weight='bold')
    ax2.set_title('Imagen 5000x5000 - Filtro 5x5', fontsize=10.5, weight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(workers)
    ax2.set_ylim(0, 3.6)
    ax2.grid(axis='y', linestyle='--', alpha=0.5)
    ax2.legend(loc='upper left', fontsize=8.5)
    
    plt.suptitle('Comparación de Escalabilidad y Speedup (Alta Resolución: 25 Megapíxeles)', fontsize=12, weight='bold', y=1.02)
    plt.tight_layout()
    plt.savefig("images/charts/chart_speedup_5000.png", bbox_inches='tight')
    plt.close()
    print("chart_speedup_5000.png generated")

# 4. Hybrid Topologies Comparison
def generate_hybrid_topologies():
    fig, ax = plt.subplots(figsize=(8, 4.2), dpi=200)
    
    topos_4 = ['4 Proc x 1 Hilo\n(4 Trabajadores)', '2 Proc x 2 Hilos\n(4 Trabajadores)', '1 Proc x 4 Hilos\n(4 Trabajadores)']
    times_4 = [542.99, 854.25, 1411.92]
    speedup_4 = [3.13, 1.99, 1.21]
    
    topos_8 = ['4 Proc x 2 Hilos\n(8 Trabajadores)', '2 Proc x 4 Hilos\n(8 Trabajadores)']
    times_8 = [633.86, 849.93]
    speedup_8 = [2.68, 2.00]
    
    all_labels = topos_4 + [''] + topos_8
    all_speedups = speedup_4 + [0] + speedup_8
    colors = ['#059669', '#2563EB', '#D97706', 'white', '#059669', '#D97706']
    
    x = np.arange(len(all_labels))
    bars = ax.bar(x, all_speedups, color=colors, edgecolor='#334155', linewidth=0.8)
    
    ax.set_ylabel('Speedup Logrado (Sp = T1 / Tp)', fontsize=10, weight='bold')
    ax.set_title('Impacto de la Topología de Procesos vs. Hilos en el Modelo Híbrido (5000x5000, 3x3)', fontsize=11, weight='bold', pad=12)
    ax.set_xticks(x)
    ax.set_xticklabels(all_labels, fontsize=8.5)
    ax.set_ylim(0, 3.6)
    ax.grid(axis='y', linestyle='--', alpha=0.5)
    
    # Add values on top
    for i, bar in enumerate(bars):
        h = bar.get_height()
        if h > 0:
            ax.annotate(f'{h:.2f}x\n({times_4[i] if i<3 else times_8[i-4]:.0f} ms)',
                        xy=(bar.get_x() + bar.get_width()/2, h),
                        xytext=(0, 4), textcoords="offset points", ha='center', va='bottom', fontsize=8, weight='bold')
            
    plt.tight_layout()
    plt.savefig("images/charts/chart_hybrid_topologies.png", bbox_inches='tight')
    plt.close()
    print("chart_hybrid_topologies.png generated")

if __name__ == "__main__":
    generate_pipeline_diagram()
    generate_sequential_breakdown()
    generate_speedup_5000()
    generate_hybrid_topologies()

