cmake_minimum_required(VERSION 3.16)
project(XFBIN_Merger C CXX)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")

add_library(XFBIN_Merger SHARED ${SOURCES})

add_subdirectory(external/japi)
add_subdirectory(external/nucc)

target_include_directories(${PROJECT_NAME} PRIVATE "src" "external/japi/src/exports")
target_link_libraries(${PROJECT_NAME} PRIVATE nucc-cpp-library)
target_link_libraries(${PROJECT_NAME} PRIVATE d3dcompiler_47)