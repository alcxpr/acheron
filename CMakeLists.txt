cmake_minimum_required(VERSION 3.15)
project(acheron)

add_library(acheron INTERFACE)
add_library(Acheron::Acheron ALIAS acheron)

target_include_directories(acheron INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(acheron INTERFACE cxx_std_23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
