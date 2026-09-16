set(NATIVE_SOURCES
    src/utils.cpp
    src/image_processing.cpp
    src/target_detection.cpp
    src/impact_detection.cpp
    src/sheet_detection.cpp
    src/logging.cpp
)

add_library(subvision_lib STATIC ${NATIVE_SOURCES})
target_include_directories(subvision_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(subvision_lib PUBLIC ${OpenCV_LIBS})

if (BUILD_TESTS AND NOT EMSCRIPTEN AND NOT BUILD_CLI_WRAPPER)
    add_subdirectory(tests)
endif ()
