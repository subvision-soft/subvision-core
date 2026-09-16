cmake_minimum_required(VERSION 3.27.7)

project(subvision_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

option(BUILD_TESTS "Build native tests" ON)
option(BUILD_CLI_WRAPPER "Build C++/CLI .NET wrapper" OFF)
option(BUILD_WASM "Build WebAssembly targets" ${EMSCRIPTEN})

find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    highgui
    imgcodecs
    videoio
    features2d
    calib3d
    flann
    dnn
)

add_subdirectory(native)

if (BUILD_CLI_WRAPPER AND NOT EMSCRIPTEN)
    add_subdirectory(cli)
endif ()

if (BUILD_WASM AND EMSCRIPTEN)
    add_subdirectory(wasm)
elseif (BUILD_WASM AND NOT EMSCRIPTEN)
    message(WARNING "BUILD_WASM is ON but EMSCRIPTEN toolchain is not active. Skipping WASM target.")
endif ()
