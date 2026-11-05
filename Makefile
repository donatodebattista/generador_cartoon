# ==============================================================================
# Makefile - TP Final Computación Avanzada (UADER)
# Generador de Efecto Cartoon (Secuencial, OpenMP, MPI e Híbrido)
# ==============================================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -pedantic -Iinclude -fopenmp
LDFLAGS  ?= -fopenmp -lm

MPICXX   ?= mpicxx
MPIFLAGS ?= -std=c++17 -O3 -Wall -Wextra -pedantic -Iinclude -fopenmp
MPILDFLAGS ?= -fopenmp -lm

SRC_DIR  := src
INC_DIR  := include
BUILD_DIR:= build

# Detectar si mpicxx está disponible en el sistema
HAS_MPI  := $(shell which $(MPICXX) 2>/dev/null)

# Fuentes comunes base
BASE_SRCS := $(SRC_DIR)/image.cpp \
             $(SRC_DIR)/filters.cpp \
             $(SRC_DIR)/posterize.cpp

BASE_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(BASE_SRCS))

# Fuentes Secuencial
SEQ_OBJS  := $(BASE_OBJS) $(BUILD_DIR)/cartoon.o $(BUILD_DIR)/main_seq.o

# Fuentes OpenMP
OMP_OBJS  := $(BASE_OBJS) \
             $(BUILD_DIR)/filters_omp.o \
             $(BUILD_DIR)/posterize_omp.o \
             $(BUILD_DIR)/cartoon_omp.o \
             $(BUILD_DIR)/main_omp.o

# Fuentes MPI
MPI_OBJS  := $(BUILD_DIR)/image_mpi.o \
             $(BUILD_DIR)/filters_mpi.o \
             $(BUILD_DIR)/posterize_mpi.o \
             $(BUILD_DIR)/mpi_utils.o \
             $(BUILD_DIR)/cartoon_mpi.o \
             $(BUILD_DIR)/main_mpi.o

# Fuentes Híbrido (MPI + OpenMP)
HYBRID_OBJS := $(BUILD_DIR)/image_mpi.o \
               $(BUILD_DIR)/filters_mpi.o \
               $(BUILD_DIR)/posterize_mpi.o \
               $(BUILD_DIR)/mpi_utils.o \
               $(BUILD_DIR)/cartoon_mpi.o \
               $(BUILD_DIR)/cartoon_hybrid.o \
               $(BUILD_DIR)/main_hybrid.o

# Ejecutables
TARGET_SEQ    := $(BUILD_DIR)/cartoon_seq
TARGET_OMP    := $(BUILD_DIR)/cartoon_omp
TARGET_MPI    := $(BUILD_DIR)/cartoon_mpi
TARGET_HYBRID := $(BUILD_DIR)/cartoon_hybrid

.PHONY: all seq omp mpi hybrid clean help dirs test test_omp test_mpi test_hybrid check_mpi

ifeq ($(HAS_MPI),)
all: dirs $(TARGET_SEQ) $(TARGET_OMP)
	@echo " [INFO] OpenMPI no está instalado en el sistema. Se compilaron 'seq' y 'omp'."
	@echo "        Para habilitar 'mpi' e 'hybrid', instale: sudo apt install libopenmpi-dev openmpi-bin"
else
all: dirs $(TARGET_SEQ) $(TARGET_OMP) $(TARGET_MPI) $(TARGET_HYBRID)
endif

seq: dirs $(TARGET_SEQ)

omp: dirs $(TARGET_OMP)

mpi: dirs check_mpi $(TARGET_MPI)

hybrid: dirs check_mpi $(TARGET_HYBRID)

check_mpi:
ifeq ($(HAS_MPI),)
	@echo "Error: '$(MPICXX)' no encontrado en el sistema."
	@echo "Por favor instale OpenMPI en Ubuntu ejecutando:"
	@echo "  sudo apt update && sudo apt install -y libopenmpi-dev openmpi-bin"
	@exit 1
endif

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p images/input
	@mkdir -p images/output

# Compilación del ejecutable secuencial
$(TARGET_SEQ): $(SEQ_OBJS)
	@echo " [LD] $@"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo " Compilación exitosa: $@"

# Compilación del ejecutable OpenMP
$(TARGET_OMP): $(OMP_OBJS)
	@echo " [LD] $@"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo " Compilación exitosa: $@"

# Compilación del ejecutable MPI
$(TARGET_MPI): $(MPI_OBJS)
	@echo " [LD MPI] $@"
	@$(MPICXX) $(MPIFLAGS) -o $@ $^ $(MPILDFLAGS)
	@echo " Compilación MPI exitosa: $@"

# Compilación del ejecutable Híbrido (MPI + OpenMP)
$(TARGET_HYBRID): $(HYBRID_OBJS)
	@echo " [LD HYBRID] $@"
	@$(MPICXX) $(MPIFLAGS) -o $@ $^ $(MPILDFLAGS)
	@echo " Compilación Híbrida exitosa: $@"

# Reglas para compilar objetos base y OpenMP con g++
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo " [CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Reglas específicas para compilar objetos MPI e Híbridos con mpicxx
$(BUILD_DIR)/image_mpi.o: $(SRC_DIR)/image.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/filters_mpi.o: $(SRC_DIR)/filters.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/posterize_mpi.o: $(SRC_DIR)/posterize.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/mpi_utils.o: $(SRC_DIR)/mpi_utils.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/cartoon_mpi.o: $(SRC_DIR)/cartoon_mpi.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/main_mpi.o: $(SRC_DIR)/main_mpi.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/cartoon_hybrid.o: $(SRC_DIR)/cartoon_hybrid.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

$(BUILD_DIR)/main_hybrid.o: $(SRC_DIR)/main_hybrid.cpp
	@mkdir -p $(dir $@)
	@echo " [MPICXX] $<"
	@$(MPICXX) $(MPIFLAGS) -c $< -o $@

clean:
	@echo " Limpiando artefactos de compilación..."
	@rm -rf $(BUILD_DIR)/*.o $(TARGET_SEQ) $(TARGET_OMP) $(TARGET_MPI) $(TARGET_HYBRID)
	@echo " Limpieza completada."

test: all
	@echo " Verificando imágenes de prueba..."
	@python3 scripts/generate_test_images.py
	@echo "\n [Test Secuencial] 800x800, filtro 3x3, posterizado 3 niveles..."
	@$(TARGET_SEQ) images/input/test_800x800.png images/output/test_seq_800.png 3 3

test_omp: all
	@echo " Verificando imágenes de prueba..."
	@python3 scripts/generate_test_images.py
	@echo "\n [Test OpenMP] 800x800, filtro 3x3, posterizado 3 niveles, 4 threads..."
	@$(TARGET_OMP) images/input/test_800x800.png images/output/test_omp_800.png 3 3 --threads 4

test_mpi: mpi
	@echo " Verificando imágenes de prueba..."
	@python3 scripts/generate_test_images.py
	@echo "\n [Test MPI] 800x800, filtro 3x3, posterizado 3 niveles, 4 procesos..."
	@mpirun -np 4 --oversubscribe $(TARGET_MPI) images/input/test_800x800.png images/output/test_mpi_800.png 3 3

test_hybrid: hybrid
	@echo " Verificando imágenes de prueba..."
	@python3 scripts/generate_test_images.py
	@echo "\n [Test Híbrido] 800x800, filtro 3x3, posterizado 3 niveles, 2 procesos x 2 threads..."
	@mpirun -np 2 --oversubscribe $(TARGET_HYBRID) images/input/test_800x800.png images/output/test_hyb_800.png 3 3 --threads 2

help:
	@echo "Comandos disponibles:"
	@echo "  make          - Compila ejecutables disponibles"
	@echo "  make seq      - Compila la versión Secuencial"
	@echo "  make omp      - Compila la versión OpenMP"
	@echo "  make mpi      - Compila la versión MPI pura"
	@echo "  make hybrid   - Compila la versión Híbrida (MPI + OpenMP)"
	@echo "  make test     - Prueba rápida secuencial"
	@echo "  make test_omp - Prueba rápida OpenMP"
	@echo "  make test_mpi - Prueba rápida MPI"
	@echo "  make test_hybrid - Prueba rápida Híbrida (2 procesos x 2 threads)"
	@echo "  make clean    - Elimina archivos binarios y objetos"
	@echo "  make help     - Muestra este mensaje"
