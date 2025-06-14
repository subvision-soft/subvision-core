# Configuration des tests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)

# Pour les systèmes Windows: Évite l'avertissement d'overriding crt
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Ajout des fichiers sources pour les tests
set(TEST_SOURCES
    ImpactDetectionTest.cpp
    EllipseDetectionTest.cpp
)

# Création de l'exécutable de test
add_executable(
    subvision_tests
    ${TEST_SOURCES}
)

# Liaison avec les bibliothèques nécessaires
target_link_libraries(
    subvision_tests
    PRIVATE
    subvision_lib
    gtest_main
    ${OpenCV_LIBS}
)

# Ajout du répertoire de ressources pour les tests
add_custom_command(TARGET subvision_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:subvision_tests>/resources
)

# Découverte et enregistrement des tests
include(GoogleTest)
gtest_discover_tests(subvision_tests)
