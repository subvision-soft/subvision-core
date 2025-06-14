cmake_minimum_required(VERSION 3.31)
project(subvision_cv)

set(CMAKE_CXX_STANDARD 23)

# Vérifier si on compile avec Emscripten
if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_NAME='SubvisionCV'")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")

    # OpenCV doit être configuré pour Emscripten
    set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv_emscripten_build")
else()
    set(ENV{OPENCV_DIR} "C:\\tools\\opencv\\build")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

# Définir les fichiers sources de la bibliothèque
set(LIB_SOURCES 
    src/utils.cpp
    src/image_processing.cpp
    src/target_detection.cpp
    src/impact_detection.cpp
    src/encoding.cpp
)

# Créer une bibliothèque statique
add_library(subvision_lib STATIC ${LIB_SOURCES})
target_include_directories(subvision_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Définir l'exécutable principal
add_executable(subvision_cv main.cpp)

set(OpenCV_LIBS opencv_core
                opencv_imgproc
                opencv_highgui
                opencv_imgcodecs
                opencv_videoio
                opencv_features2d
                opencv_calib3d
                opencv_flann
                opencv_dnn)

# Lier la bibliothèque à l'exécutable principal
target_link_libraries(subvision_cv subvision_lib ${OpenCV_LIBS})

# Création d'un répertoire resources si nécessaire
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)

# Activation des tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS AND NOT EMSCRIPTEN)
    add_subdirectory(test)
endif()

# Configuration WebAssembly pour Emscripten
if(EMSCRIPTEN)
    # Création de la cible WebAssembly
    add_executable(subvision_wasm emscripten_binding.cpp)
    target_link_libraries(subvision_wasm subvision_lib ${OpenCV_LIBS})

    # Options spécifiques pour la cible WebAssembly
    set_target_properties(subvision_wasm PROPERTIES
        LINK_FLAGS "-s EXPORT_ES6=1 -s MODULARIZE=1 -s ENVIRONMENT=web,worker -s USE_ES6_IMPORT_META=0 -s EXPORTED_FUNCTIONS=['_malloc','_free'] -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString']")

    # Générer un fichier HTML de test
    configure_file(${CMAKE_SOURCE_DIR}/web/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
endif()