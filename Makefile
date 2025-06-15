# Makefile pour compiler le projet SubvisionCV avec Emscripten via Docker

# Variables
DOCKER_IMAGE = sha256:5873603785987880cdf1c82c97764b776e2ece6868e017497e2bda5f8474a1c5
OUTPUT_DIR = build_wasm
SRC_DIR = .

# Sources pour la bibliothèque statique
LIB_SOURCES = src/utils.cpp \
			src/image_processing.cpp \
			src/target_detection.cpp \
			src/impact_detection.cpp \
			src/sheet_detection.cpp \
			src/encoding.cpp

# Options de compilation
EMCC_FLAGS = -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 \
			-s EXPORT_ES6=1 -s MODULARIZE=1 -s ENVIRONMENT=web,worker \
			-s DISABLE_EXCEPTION_CATCHING=2 \
			-s USE_ES6_IMPORT_META=0 -s NO_EXIT_RUNTIME=1 \
			-s EXPORTED_FUNCTIONS=['_malloc','_free'] \
			-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString'] \
			-s EXPORT_NAME='SubvisionCV' -s ASSERTIONS=1

# Cibles
.PHONY: all clean serve build_docker

all: $(OUTPUT_DIR)/subvision_wasm.js $(OUTPUT_DIR)/index.html

# Création du répertoire de sortie
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Compilation de la bibliothèque et du binding WebAssembly
$(OUTPUT_DIR)/subvision_wasm.js: $(OUTPUT_DIR)
	docker run --rm -v $${PWD}:/src -w /src $(DOCKER_IMAGE) bash -c "emcc $(LIB_SOURCES) emscripten_binding.cpp \
		-I./include \
		\`pkg-config --cflags --libs opencv4\` \
		-o $(OUTPUT_DIR)/subvision_wasm.js \
		$(EMCC_FLAGS) \
		--bind"

# Copie du fichier HTML de test
$(OUTPUT_DIR)/index.html: $(OUTPUT_DIR) web/index.html
	cp web/index.html $(OUTPUT_DIR)/

# Lance un serveur web pour tester l'application
serve: all
	cd $(OUTPUT_DIR) && python3 -m http.server 8080
	@echo "Serveur démarré sur http://localhost:8080"

# Nettoie les fichiers de sortie
clean:
	rm -rf $(OUTPUT_DIR)

# Construit l'image Docker si elle n'existe pas déjà
build_docker:
	docker pull $(DOCKER_IMAGE) || (echo "Image Docker non disponible. Veuillez consulter le README.md pour les instructions de compilation d'OpenCV avec Emscripten.")


# Compilation de l'application complète SubvisionCV
subvision: $(OUTPUT_DIR)
	@echo "Compilation de SubvisionCV..."
	docker run --rm -v $${PWD}:/src -w /src $(DOCKER_IMAGE) bash -c "emcc $(LIB_SOURCES) emscripten_binding.cpp \
		-I./include \
		\`pkg-config --cflags --libs opencv4\` \
		-o $(OUTPUT_DIR)/subvision_wasm.js \
		$(EMCC_FLAGS) \
		--bind"
	cp web/index.html $(OUTPUT_DIR)/
	@echo "SubvisionCV compilé avec succès. Les fichiers sont dans $(OUTPUT_DIR)/"

# Lancer l'application SubvisionCV
run_subvision: subvision
	cd $(OUTPUT_DIR) && python3 -m http.server 8080
	@echo "Application SubvisionCV disponible sur http://localhost:8080"

# Aide
help:
	@echo "Makefile pour compiler SubvisionCV avec Emscripten via Docker"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  all               : Compile le projet (cible par défaut)"
	@echo "  serve             : Compile et lance un serveur web pour tester l'application"
	@echo "  clean             : Supprime les fichiers générés"
	@echo "  build_docker      : Vérifie/télécharge l'image Docker nécessaire"
	@echo "  subvision         : Compile l'application SubvisionCV complète"
	@echo "  run_subvision     : Compile et exécute l'application SubvisionCV"
	@echo "  help              : Affiche cette aide"
