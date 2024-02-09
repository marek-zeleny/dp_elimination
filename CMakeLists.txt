cmake_minimum_required(VERSION 3.14)

project(dp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/sylvan/)
add_subdirectory(external/catch2/)

add_executable(dp
    src/main.cpp
    src/args_parser.cpp
    src/logging.cpp
    src/io/cnf_reader.cpp
    src/io/cnf_writer.cpp
    src/data_structures/sylvan_zdd_cnf.cpp
    src/data_structures/watched_literals.cpp
)
target_include_directories(dp PRIVATE src/)
target_link_libraries(dp PRIVATE sylvan)

add_executable(tests
    tests/test_lru_cache.cpp
)
target_include_directories(tests PRIVATE src/)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
