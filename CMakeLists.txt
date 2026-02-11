cmake_minimum_required(VERSION 4.0)
project(XFBIN_Merging VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

option(STATIC_CPP_RUNTIME "Link C/C++ runtime statically" OFF)

if (MSVC AND STATIC_CPP_RUNTIME)
	set(CMAKE_MSVC_RUNTIME_LIBRARY
		"MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif((CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") AND STATIC_CPP_RUNTIME)
	add_link_options(
		-static-libstdc++
		-static-libgcc
	)
endif()

# Disable tail call optimization to improve stack traces + make API calls not bug out in some cases.
if (MSVC)
	add_compile_options(/Oy-)
else()
	add_compile_options(-fno-optimize-sibling-calls)
endif()


add_library(${PROJECT_NAME} SHARED
	src/main.cpp
)

# add_subdirectory(external/nucc)

target_include_directories(${PROJECT_NAME} PRIVATE
	src
	japi
)

# target_link_libraries(${PROJECT_NAME} PRIVATE nucc-cpp-library)
target_link_libraries(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/japi/JAPI.dll
)
