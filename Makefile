# Makefile pour compiler le projet Subvision avec Emscripten via Docker

# Variables
DOCKER_IMAGE = ghcr.io/subvision-soft/subvision-emscripten:2025.6.1
OUTPUT_DIR = build_wasm
SRC_DIR = .

# Sources pour la bibliothèque statique
LIB_SOURCES = src/utils.cpp \
			src/image_processing.cpp \
			src/target_detection.cpp \
			src/impact_detection.cpp \
			src/sheet_detection.cpp \
			src/logging.cpp

# Options de compilation emscripten
EMCC_FLAGS = -O3 -std=c++20 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_ZLIB=1 \
			-s MODULARIZE=1 -s ENVIRONMENT=web,worker \
			-s DISABLE_EXCEPTION_CATCHING=0 -s SINGLE_FILE \
			-s USE_ES6_IMPORT_META=0 -s NO_EXIT_RUNTIME=1 \
			-s EXPORTED_FUNCTIONS=['_malloc','_free'] \
			-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString'] \
			-s EXPORT_NAME='Subvision' -s ASSERTIONS=1

# Cibles
.PHONY: all subvision subvision_es6

all: subvision subvision_es6

# Création du répertoire de sortie
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Compilation de la bibliothèque et du binding WebAssembly
$(OUTPUT_DIR)/subvision.js: $(OUTPUT_DIR)
	docker run --rm -v $${PWD}:/src -w /src $(DOCKER_IMAGE) bash -c "emcc $(LIB_SOURCES) emscripten_binding.cpp \
		-I./include \
		\`pkg-config --cflags --libs opencv4\` \
		-o $(OUTPUT_DIR)/subvision.js \
		$(EMCC_FLAGS) \
		--bind"

# Copie du fichier HTML de test
$(OUTPUT_DIR)/index.html: $(OUTPUT_DIR) web/index.html
	cp web/index.html $(OUTPUT_DIR)/

# Compilation de l'application complète Subvision
subvision: $(OUTPUT_DIR)
	@echo "Compilation de Subvision..."
	docker run --rm -v $${PWD}:/src -w /src $(DOCKER_IMAGE) bash -c "emcc $(LIB_SOURCES) emscripten_binding.cpp \
		-I./include \
		\`pkg-config --cflags --libs opencv4\` \
		-o $(OUTPUT_DIR)/subvision.js \
		$(EMCC_FLAGS) \
		--bind"
	cp web/index.html $(OUTPUT_DIR)/
	@echo "Subvision compilé avec succès. Les fichiers sont dans $(OUTPUT_DIR)/"

# Compilation de l'application complète Subvision
subvision_es6: $(OUTPUT_DIR)
	@echo "Compilation de Subvision en mode ES6..."
	docker run --rm -v $${PWD}:/src -w /src $(DOCKER_IMAGE) bash -c "emcc $(LIB_SOURCES) emscripten_binding.cpp \
		-I./include \
		\`pkg-config --cflags --libs opencv4\` \
		-o $(OUTPUT_DIR)/subvision.mjs \
		$(EMCC_FLAGS) -s EXPORT_ES6=1 \
		--bind"
	cp web/index.html $(OUTPUT_DIR)/
	@echo "Subvision compilé avec succès. Les fichiers sont dans $(OUTPUT_DIR)/"
	
# --- .NET wrapper builds ---

# Build x64 wrapper
subvision_dotnet: subvision_dotnet_x64

subvision_dotnet_x64:
	@echo "Building .NET wrapper (x64)..."
ifeq ($(OS),Windows_NT)
	@if not exist build-dotnet-x64 mkdir build-dotnet-x64
	@cd build-dotnet-x64 && cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_CLI_WRAPPER=ON ..
	@cmake --build build-dotnet-x64 --config Release
else
	@echo "C++/CLI wrapper requires Windows with MSVC"
	@exit 1
endif
	@echo "x64 artifacts in build-dotnet-x64/bin/x64/Release/"

# Build ARM64 wrapper
subvision_dotnet_arm64:
	@echo "Building .NET wrapper (ARM64)..."
ifeq ($(OS),Windows_NT)
	@if not exist build-dotnet-arm64 mkdir build-dotnet-arm64
	@cd build-dotnet-arm64 && cmake -G "Visual Studio 17 2022" -A ARM64 -DBUILD_CLI_WRAPPER=ON ..
	@cmake --build build-dotnet-arm64 --config Release
else
	@echo "C++/CLI wrapper requires Windows with MSVC"
	@exit 1
endif
	@echo "ARM64 artifacts in build-dotnet-arm64/bin/arm64/Release/"

# Build all architectures
subvision_dotnet_all: subvision_dotnet_x64 subvision_dotnet_arm64
	@echo "All .NET wrapper architectures built successfully."

# Package into NuGet
subvision_nuget: subvision_dotnet_all
	@echo "Creating NuGet package..."
ifeq ($(OS),Windows_NT)
	@if not exist nupkg mkdir nupkg
	@if not exist nupkg\runtimes\win-x64\native mkdir nupkg\runtimes\win-x64\native
	@if not exist nupkg\runtimes\win-arm64\native mkdir nupkg\runtimes\win-arm64\native
	@copy build-dotnet-x64\bin\x64\Release\subvision-x64.dll nupkg\runtimes\win-x64\native\
	@copy build-dotnet-x64\bin\x64\Release\opencv_world4110.dll nupkg\runtimes\win-x64\native\
	@copy build-dotnet-arm64\bin\arm64\Release\subvision-arm64.dll nupkg\runtimes\win-arm64\native\
	@copy build-dotnet-arm64\bin\arm64\Release\opencv_world4110.dll nupkg\runtimes\win-arm64\native\
	@copy Subvision.nuspec nupkg\
	@copy Subvision.targets nupkg\
	nuget pack nupkg\Subvision.nuspec -OutputDirectory nupkg
endif
	@echo "NuGet package created in nupkg/"

# Clean dotnet builds
clean_dotnet:
	@echo "Cleaning .NET build directories..."
ifeq ($(OS),Windows_NT)
	@if exist build-dotnet-x64 rmdir /s /q build-dotnet-x64
	@if exist build-dotnet-arm64 rmdir /s /q build-dotnet-arm64
	@if exist nupkg rmdir /s /q nupkg
endif
	@echo "Clean complete."

# Aide
help:
	@echo "Makefile pour compiler Subvision avec Emscripten via Docker"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  all                    : Compile le projet complet (Subvision et Subvision ES6)"
	@echo "  subvision              : Compile l'application Subvision complète"
	@echo "  subvision_es6          : Compile l'application Subvision en mode ES6"
	@echo "  subvision_dotnet       : Build .NET wrapper (x64, alias for subvision_dotnet_x64)"
	@echo "  subvision_dotnet_x64   : Build .NET wrapper for x64"
	@echo "  subvision_dotnet_arm64 : Build .NET wrapper for ARM64"
	@echo "  subvision_dotnet_all   : Build .NET wrapper for all architectures"
	@echo "  subvision_nuget        : Build all + create NuGet package"
	@echo "  clean_dotnet           : Clean all .NET build directories"
	@echo "  help                   : Affiche cette aide"

