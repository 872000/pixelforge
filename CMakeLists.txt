cmake_minimum_required(VERSION 3.16)
project(pixelforge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

# Core library: image type, hand-rolled BMP/PPM codecs, pixel ops.
add_library(pixelforge_lib
    src/bmp.cpp
    src/ppm.cpp
    src/ops.cpp
)
target_include_directories(pixelforge_lib PUBLIC src)

add_executable(pixelforge src/cli_main.cpp)
target_link_libraries(pixelforge PRIVATE pixelforge_lib)

# Self-contained test runner (no external framework).
add_executable(pixelforge_tests tests/test_main.cpp)
target_link_libraries(pixelforge_tests PRIVATE pixelforge_lib)

enable_testing()
add_test(NAME pixelforge_tests COMMAND pixelforge_tests)
