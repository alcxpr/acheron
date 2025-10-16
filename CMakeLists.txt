cmake_minimum_required(VERSION 4.0)
project(acheron)

set(CMAKE_CXX_STANDARD 23)

add_executable(acheron lib/config.cc)

target_include_directories(acheron PRIVATE include)
