#pragma once
#include <iostream>

static const char* cmakeTemplate = R"(
cmake_minimum_required(VERSION 3.15)
project(dlang_compiler CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(WITH_WINMM)

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dlang/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dlang/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dlang/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dlang/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dependencies/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dependencies/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dependencies/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dependencies/*.c"
)

add_executable(dlang_compiler 
    "${CMAKE_CURRENT_SOURCE_DIR}/entry.cpp" 
    ${SOURCES}
)

target_include_directories(dlang_compiler PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dlang"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dlang/utils"
    "${CMAKE_CURRENT_SOURCE_DIR}/dlang/dependencies/soloud"
)

target_link_libraries(dlang_compiler PRIVATE winmm)
)";