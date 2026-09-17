cmake_minimum_required(VERSION 3.16)

project(type_erasure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generates build/compile_commands.json, which clangd reads for include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# The sinks are the concrete types Logger erases; logger.hpp and log_level.hpp
# are header-only, so they only need to be on the include path.
add_library(sinks STATIC sinks.cpp)

target_include_directories(sinks PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(type_erasure main.cpp)

target_link_libraries(type_erasure PRIVATE sinks)
