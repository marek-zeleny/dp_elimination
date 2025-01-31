cmake_minimum_required(VERSION 3.14)

project(dp)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "No build type specified, setting to Release by default")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JSON_Install OFF CACHE INTERNAL "")

add_subdirectory(external/sylvan/)
add_subdirectory(external/CLI11/)
add_subdirectory(external/catch2/)
add_subdirectory(external/simple-logger/)
add_subdirectory(external/json/)

add_library(dp_lib STATIC
        lib/io/cnf_reader.cpp
        lib/io/cnf_writer.cpp
        lib/data_structures/sylvan_zdd_cnf.cpp
        lib/data_structures/vector_cnf.cpp
        lib/data_structures/watched_literals.cpp
        lib/algorithms/unit_propagation.cpp
)
target_include_directories(dp_lib PUBLIC lib/)
target_link_libraries(dp_lib PUBLIC sylvan PUBLIC simple_logger PUBLIC nlohmann_json::nlohmann_json)

add_executable(dp
        app/main.cpp
        app/args_parser.cpp
)
target_include_directories(dp PRIVATE app/)
target_link_libraries(dp PRIVATE dp_lib PRIVATE CLI11::CLI11)

add_executable(tests
        tests/main.cpp
        tests/test_lru_cache.cpp
        tests/test_cnf_reader.cpp
        tests/test_cnf_writer.cpp
        tests/test_watched_literals.cpp
        tests/test_sylvan_zdd_cnf.cpp
        tests/test_unit_propagation.cpp
        tests/test_dp_elimination.cpp
        tests/test_heuristics.cpp
)
target_link_libraries(tests PRIVATE dp_lib PRIVATE Catch2::Catch2)
