# this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details

# @brief: create an Acheron executable
# @usage: acheron_executable(name SOURCES source1.cpp source2.cpp... LIBS lib1 lib2... FLAGS flag1 flag2...)
function(acheron_executable name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;FLAGS" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})

    set_target_properties(${name} PROPERTIES
            CXX_STANDARD 26
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    # include root project's include directory
    target_include_directories(${name} PRIVATE ${ACHERON_INCLUDE_DIR})
    
    # link the libraries if provided
    if (ARG_LIBS)
        target_link_libraries(${name} PRIVATE ${ARG_LIBS})
    endif()

    # add compile/link flags if provided
    if (ARG_FLAGS)
        target_compile_options(${name} PRIVATE ${ARG_FLAGS})
        target_link_options(${name} PRIVATE ${ARG_FLAGS})
    endif()

    target_compile_features(${name} PUBLIC cxx_std_26)
endfunction()

# @brief: create an Acheron test
# @usage: acheron_test(name SOURCES test1.cpp test2.cpp... LIBS lib1 lib2... FLAGS flag1 flag2...)
function(acheron_test name)
    if (NOT ACHERON_BUILD_TESTS)
        return()
    endif()

    find_package(GTest REQUIRED)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;FLAGS" ${ARGN})

    # create the test executable and link it with GTest as
    # the test framework by default
    acheron_executable(${name} SOURCES ${ARG_SOURCES} LIBS ${ARG_LIBS} FLAGS ${ARG_FLAGS})
    if(TARGET GTest::gtest_main)
        target_link_libraries(${name} PRIVATE GTest::gtest_main)
    elseif(TARGET gtest_main)
        target_link_libraries(${name} PRIVATE gtest_main)
    endif()

    # add to CTest
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)

    # set test properties
    set_tests_properties(${name} PROPERTIES
            TIMEOUT 300  # 5 minutes max per test
            LABELS "unit")
endfunction()

# @brief: create a benchmark executable
# @usage: acheron_benchmark(name SOURCES bench1.cpp bench2.cpp... LIBS lib1 lib2... FLAGS flag1 flag2...)
function(acheron_benchmark name)
    if (NOT ACHERON_BUILD_BENCHMARKS)
        return()
    endif()

    find_package(benchmark REQUIRED)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;FLAGS" ${ARGN})

    acheron_executable(${name} SOURCES ${ARG_SOURCES} LIBS ${ARG_LIBS} FLAGS ${ARG_FLAGS})
    # link with GBench if found
    if(TARGET benchmark::benchmark)
        target_link_libraries(${name} PRIVATE benchmark::benchmark)
    elseif(TARGET benchmark)
        target_link_libraries(${name} PRIVATE benchmark)
    endif()
    # benchmarks should be optimized even in debug builds
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${name} PRIVATE -O3)
    endif()
endfunction()
