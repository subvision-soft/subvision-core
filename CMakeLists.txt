cmake_minimum_required(VERSION 3.30.5)

project(subvision_core)

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
    src/sheet_detection.cpp
)

# Créer une bibliothèque statique
add_library(subvision_lib STATIC ${LIB_SOURCES})
target_include_directories(subvision_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

set(OpenCV_LIBS opencv_core
                opencv_imgproc
                opencv_highgui
                opencv_imgcodecs
                opencv_videoio
                opencv_features2d
                opencv_calib3d
                opencv_flann
                opencv_dnn)

# Création d'un répertoire resources si nécessaire
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)

# Activation des tests
option(BUILD_TESTS "Build tests" ON)
option(BUILD_CLI_WRAPPER "Build C++/CLI .NET wrapper" OFF)

# Don't build tests when building CLI wrapper (they conflict with /clr)
if(BUILD_TESTS AND NOT EMSCRIPTEN AND NOT BUILD_CLI_WRAPPER)
    add_subdirectory(test)
endif()

# Configuration C++/CLI pour .NET
if(BUILD_CLI_WRAPPER AND NOT EMSCRIPTEN)
    # C++/CLI requires MSVC
    if(MSVC)
        # Create the C++/CLI wrapper library (only compile the wrapper with /clr)
        add_library(SubvisionNET SHARED cli_wrapper.cpp)
        target_include_directories(SubvisionNET PUBLIC ${CMAKE_SOURCE_DIR}/include)

        # Link against the native library (compiled without /clr)
        target_link_libraries(SubvisionNET PRIVATE subvision_lib ${OpenCV_LIBS})

        # Disable C++ modules and standards that conflict with /clr
        set_target_properties(SubvisionNET PROPERTIES
            CXX_STANDARD 17
            CXX_SCAN_FOR_MODULES OFF
            COMMON_LANGUAGE_RUNTIME ""
            VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
        )

        # Enable C++/CLI only for the wrapper file
        target_compile_options(SubvisionNET PRIVATE
            /clr
            /EHa  # Exception handling for C++/CLI
            /std:c++17  # C++/CLI works best with C++17
        )

        # Set output name
        set_target_properties(SubvisionNET PROPERTIES
            OUTPUT_NAME "SubvisionNET"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    else()
        message(WARNING "C++/CLI wrapper requires MSVC compiler")
    endif()
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