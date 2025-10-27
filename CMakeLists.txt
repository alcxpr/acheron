# this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details

cmake_minimum_required(VERSION 3.15)
project(acheron LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ACHERON_INCLUDE_DIR "${CMAKE_CURRENT_DIR}/include")

option(ACHERON_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ACHERON_BUILD_EXAMPLES "Build examples" OFF)
option(ACHERON_BUILD_TESTS "Build tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(acheron.utils)

# the library and its configuration
add_library(acheron INTERFACE)
add_library(Acheron::Acheron ALIAS acheron)

target_include_directories(acheron INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(acheron INTERFACE cxx_std_23)

# <acheron/diagnostics.hpp> requires stdc++exp for <stacktrace>
# in debug mode
target_link_libraries(acheron INTERFACE
        $<$<NOT:$<CONFIG:Release>>:stdc++exp>
)

# test, bench and examples
if (ACHERON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
