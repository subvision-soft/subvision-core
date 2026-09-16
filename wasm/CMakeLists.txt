set(WASM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/build_wasm")

set(WASM_COMMON_LINK_FLAGS
    "-s MODULARIZE=1"
    "-s ENVIRONMENT=web,worker"
    "-s USE_ES6_IMPORT_META=0"
    "-s EXPORTED_FUNCTIONS=['_malloc','_free']"
    "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString']"
    "--bind"
)

add_executable(subvision_wasm emscripten_binding.cpp)
target_link_libraries(subvision_wasm PRIVATE subvision_lib ${OpenCV_LIBS})
target_link_options(subvision_wasm PRIVATE ${WASM_COMMON_LINK_FLAGS})
set_target_properties(subvision_wasm PROPERTIES
    OUTPUT_NAME "subvision"
    RUNTIME_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
)

add_executable(subvision_wasm_es6 emscripten_binding.cpp)
target_link_libraries(subvision_wasm_es6 PRIVATE subvision_lib ${OpenCV_LIBS})
target_link_options(subvision_wasm_es6 PRIVATE ${WASM_COMMON_LINK_FLAGS} "-s EXPORT_ES6=1")
set_target_properties(subvision_wasm_es6 PROPERTIES
    OUTPUT_NAME "subvision"
    SUFFIX ".mjs"
    RUNTIME_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/web/index.html ${WASM_OUTPUT_DIR}/index.html COPYONLY)
