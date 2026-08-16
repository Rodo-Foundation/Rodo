cmake_minimum_required(VERSION 3.10)
project(rodo_core C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_library(rodo SHARED
    src/rodo.c
    src/rodo_format.c
    src/rodo_varint.c
    src/rodo_symbol.c
    src/rodo_dict.c
    src/rodo_platform.c
)

target_include_directories(rodo PUBLIC include)

enable_testing()
add_executable(test_basic tests/test_basic.c)
target_link_libraries(test_basic rodo)
add_test(NAME test_basic COMMAND test_basic)

add_executable(test_types tests/test_types.c)
target_link_libraries(test_types rodo)
add_test(NAME test_types COMMAND test_types)

add_executable(test_persistence tests/test_persistence.c)
target_link_libraries(test_persistence rodo)
add_test(NAME test_persistence COMMAND test_persistence)

add_executable(test_concurrency tests/test_concurrency.c)
target_link_libraries(test_concurrency rodo)
add_test(NAME test_concurrency COMMAND test_concurrency)