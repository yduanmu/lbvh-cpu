cmake_minimum_required(VERSION 3.20)

project(LBVH_CPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

add_executable(lbvh_cpu
	src/lbvh.cpp
	src/lbvh/comp_zorder.cpp
	src/lbvh/cons_bvh.cpp
	src/lbvh/cons_radix.cpp
	src/lbvh/sort_zorder.cpp
	src/util/normalize.cpp
)
target_include_directories(lbvh_cpu PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_normalize
	src/lbvh.cpp
	src/util/normalize.cpp
)
target_include_directories(test_normalize PRIVATE ${CMAKE_SOURCE_DIR}/include)
