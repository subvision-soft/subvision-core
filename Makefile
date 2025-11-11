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
			src/sheet_detection.cpp

# Options de compilation emscripten
EMCC_FLAGS = -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 \
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

# Aide
help:
	@echo "Makefile pour compiler Subvision avec Emscripten via Docker"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  all               : Compile le projet complet (Subvision et Subvision ES6)"
	@echo "  subvision         : Compile l'application Subvision complète"
	@echo "  subvision_es6     : Compile l'application Subvision en mode ES6"
	@echo "  help              : Affiche cette aide"
