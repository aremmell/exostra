# minimum acceptable CMake version.
cmake_minimum_required(VERSION 3.27)

# define the project.
project(
    twm
    VERSION 0.0.1
    LANGUAGES CXX
)

# set the C++ standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    twmtests
    src/main.cpp
)

target_include_directories(
    twmtests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# TODO:
# - auto-bump version in whatever library config file(s) we end up using.
# - create static/shared libraries.
