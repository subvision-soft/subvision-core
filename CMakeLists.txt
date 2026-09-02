cmake_minimum_required(VERSION 3.27.7)

project(subvision_core)

set(CMAKE_CXX_STANDARD 23)

# Vérifier si on compile avec Emscripten
if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_NAME='Subvision'")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")

    # OpenCV doit être configuré pour Emscripten
    set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv_emscripten_build")
else ()
    set(ENV{OPENCV_DIR} "C:\\tools\\opencv\\build")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
endif ()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

# Définir les fichiers sources de la bibliothèque
set(LIB_SOURCES
        src/utils.cpp
        src/image_processing.cpp
        src/target_detection.cpp
        src/impact_detection.cpp
        src/sheet_detection.cpp
        src/logging.cpp
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
if (BUILD_TESTS AND NOT EMSCRIPTEN AND NOT BUILD_CLI_WRAPPER)
    add_subdirectory(test)
endif ()

# Configuration C++/CLI pour .NET
if (BUILD_CLI_WRAPPER AND NOT EMSCRIPTEN)
    # C++/CLI requires MSVC
    if (MSVC)
        # --- Architecture detection (x64, ARM64, x86) ---
        if (CMAKE_GENERATOR_PLATFORM)
            string(TOUPPER "${CMAKE_GENERATOR_PLATFORM}" _PLAT)
        elseif (CMAKE_VS_PLATFORM_NAME)
            string(TOUPPER "${CMAKE_VS_PLATFORM_NAME}" _PLAT)
        else ()
            string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" _PLAT)
        endif ()

        if (_PLAT MATCHES "ARM64")
            set(ARCH_SUFFIX "arm64")
        elseif (_PLAT MATCHES "X64" OR _PLAT MATCHES "AMD64" OR CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ARCH_SUFFIX "x64")
        else ()
            set(ARCH_SUFFIX "x86")
        endif ()
        message(STATUS "Building C++/CLI wrapper for architecture: ${ARCH_SUFFIX}")

        # Create the C++/CLI wrapper library (only compile the wrapper with /clr)
        add_library(Subvision SHARED cli_wrapper.cpp)
        target_include_directories(Subvision PUBLIC ${CMAKE_SOURCE_DIR}/include)

        # Link against the native library (compiled without /clr)
        target_link_libraries(Subvision PRIVATE subvision_lib ${OpenCV_LIBS})

        # Disable C++ modules and standards that conflict with /clr
        set_target_properties(Subvision PROPERTIES
                CXX_STANDARD 17
                CXX_SCAN_FOR_MODULES OFF
                COMMON_LANGUAGE_RUNTIME ""
                VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
        )

        # Enable C++/CLI only for the wrapper file
        target_compile_options(Subvision PRIVATE
                /EHa  # Exception handling for C++/CLI (clr added via COMMON_LANGUAGE_RUNTIME)
        )

        # NOTE: C++/CLI (/clr) requires dynamic CRT (/MD). Static CRT (/MT) is NOT
        # compatible. The VC++ Redistributable is required on target machines, but it
        # is already installed on the vast majority of Windows machines.

        # Set output name
        set_target_properties(Subvision PROPERTIES
                OUTPUT_NAME "SubvisionNET"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARCH_SUFFIX}"
        )

        # --- Post-build: copy OpenCV DLLs next to the wrapper for distribution ---
        # OpenCV_DIR is set by find_package(OpenCV).
        set(_OPENCV_BIN_DIR "")
        
        # 1. Try OpenCV_DIR / ARCH / RUNTIME / bin (typical for OpenCV root)
        if (DEFINED OpenCV_ARCH AND DEFINED OpenCV_RUNTIME)
            set(_TEST_DIR "${OpenCV_DIR}/${OpenCV_ARCH}/${OpenCV_RUNTIME}/bin")
            if (EXISTS "${_TEST_DIR}")
                set(_OPENCV_BIN_DIR "${_TEST_DIR}")
            endif ()
        endif ()

        # 2. Try OpenCV_DIR / ../bin (typical if OpenCV_DIR is in the lib folder)
        if (NOT _OPENCV_BIN_DIR OR NOT EXISTS "${_OPENCV_BIN_DIR}")
            get_filename_component(_TEST_DIR "${OpenCV_DIR}/../bin" ABSOLUTE)
            if (EXISTS "${_TEST_DIR}")
                set(_OPENCV_BIN_DIR "${_TEST_DIR}")
            endif ()
        endif ()

        # 3. Fallback to standard choco path if everything else fails
        if (NOT _OPENCV_BIN_DIR OR NOT EXISTS "${_OPENCV_BIN_DIR}")
            set(_OPENCV_BIN_DIR "C:/tools/opencv/build/x64/vc16/bin")
        endif ()

        file(GLOB OPENCV_DLLS "${_OPENCV_BIN_DIR}/opencv_*.dll")
        if (OPENCV_DLLS)
            message(STATUS "Found OpenCV DLLs in: ${_OPENCV_BIN_DIR}")
            add_custom_command(TARGET Subvision POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E echo "Copying OpenCV DLLs to output directory..."
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OPENCV_DLLS}
                    "$<TARGET_FILE_DIR:Subvision>/"
                    COMMAND ${CMAKE_COMMAND} -E echo "OpenCV DLLs copied successfully."
                    COMMENT "Bundling OpenCV DLLs with Subvision wrapper"
                    VERBATIM
            )
        else ()
            message(WARNING "Could not find OpenCV DLLs (opencv_*.dll) in ${_OPENCV_BIN_DIR}. "
                    "You will need to manually place the OpenCV binaries next to SubvisionNET.dll")
        endif ()
    else ()
        message(WARNING "C++/CLI wrapper requires MSVC compiler")
    endif ()
endif ()

# Configuration WebAssembly pour Emscripten
if (EMSCRIPTEN)
    # Création de la cible WebAssembly
    add_executable(subvision_wasm emscripten_binding.cpp)
    target_link_libraries(subvision_wasm subvision_lib ${OpenCV_LIBS})

    # Options spécifiques pour la cible WebAssembly
    set_target_properties(subvision_wasm PROPERTIES
            LINK_FLAGS "-s EXPORT_ES6=1 -s MODULARIZE=1 -s ENVIRONMENT=web,worker -s USE_ES6_IMPORT_META=0 -s EXPORTED_FUNCTIONS=['_malloc','_free'] -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString']")

    # Générer un fichier HTML de test
    configure_file(${CMAKE_SOURCE_DIR}/web/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
endif ()