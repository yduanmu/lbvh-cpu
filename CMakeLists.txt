cmake_minimum_required(VERSION 3.20)

project(LBVH_CPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-Wall -Wextra -Wpedantic -Werror -O3)

find_package(OpenMP REQUIRED)

# Executables for testing only
add_executable(test
	src/lbvh.cpp
	src/util/normalize.cpp
	src/lbvh/comp_zorder.cpp
)
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test PRIVATE OpenMP::OpenMP_CXX)

# For use with rapidobj only
include(FetchContent)
FetchContent_Declare(rapidobj
	GIT_REPOSITORY  https://github.com/guybrush77/rapidobj.git
    GIT_TAG         origin/master
)
FetchContent_MakeAvailable(rapidobj)
target_link_libraries(test PRIVATE rapidobj)
