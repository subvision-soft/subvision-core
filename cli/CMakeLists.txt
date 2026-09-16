if (MSVC)
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

    add_library(Subvision SHARED src/cli_wrapper.cpp)
    target_link_libraries(Subvision PRIVATE subvision_lib ${OpenCV_LIBS})

    set_target_properties(Subvision PROPERTIES
        CXX_STANDARD 17
        CXX_SCAN_FOR_MODULES OFF
        COMMON_LANGUAGE_RUNTIME ""
        VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
        OUTPUT_NAME "SubvisionNET"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARCH_SUFFIX}"
    )

    target_compile_options(Subvision PRIVATE /EHa)

    set(_OPENCV_BIN_DIR "")
    if (DEFINED OpenCV_ARCH AND DEFINED OpenCV_RUNTIME)
        set(_TEST_DIR "${OpenCV_DIR}/${OpenCV_ARCH}/${OpenCV_RUNTIME}/bin")
        if (EXISTS "${_TEST_DIR}")
            set(_OPENCV_BIN_DIR "${_TEST_DIR}")
        endif ()
    endif ()

    if (NOT _OPENCV_BIN_DIR OR NOT EXISTS "${_OPENCV_BIN_DIR}")
        get_filename_component(_TEST_DIR "${OpenCV_DIR}/../bin" ABSOLUTE)
        if (EXISTS "${_TEST_DIR}")
            set(_OPENCV_BIN_DIR "${_TEST_DIR}")
        endif ()
    endif ()

    if (NOT _OPENCV_BIN_DIR OR NOT EXISTS "${_OPENCV_BIN_DIR}")
        set(_OPENCV_BIN_DIR "C:/tools/opencv/build/x64/vc16/bin")
    endif ()

    file(GLOB OPENCV_DLLS "${_OPENCV_BIN_DIR}/opencv_*.dll")
    if (OPENCV_DLLS)
        add_custom_command(TARGET Subvision POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENCV_DLLS}
            "$<TARGET_FILE_DIR:Subvision>/"
            COMMENT "Bundling OpenCV DLLs with Subvision wrapper"
            VERBATIM
        )
    else ()
        message(WARNING "Could not find OpenCV DLLs (opencv_*.dll) in ${_OPENCV_BIN_DIR}. You may need to place OpenCV binaries next to SubvisionNET.dll")
    endif ()
else ()
    message(WARNING "C++/CLI wrapper requires MSVC compiler")
endif ()
