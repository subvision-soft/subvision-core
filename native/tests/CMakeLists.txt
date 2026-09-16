find_package(GTest CONFIG REQUIRED)

enable_testing()

add_executable(subvision_tests
    ImpactDetectionTest.cpp
    EllipseDetectionTest.cpp
)

target_link_libraries(subvision_tests PRIVATE
    subvision_lib
    GTest::gtest_main
)

add_custom_command(TARGET subvision_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:subvision_tests>/resources
)

include(GoogleTest)
gtest_discover_tests(subvision_tests)
