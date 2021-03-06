
project(uniject C)
cmake_minimum_required(VERSION 3.3)

set(PROJECT_DESCRIPTION "Library and command line tool for injecting .NET assemblies into Unity games at runtime.")
message(STATUS "Running CMake for ${PROJECT_NAME} (${PROJECT_DESCRIPTION})")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake")

include(PrecompiledHeader)

set(CMAKE_C_STANDARD 89)

# Default build type to debug
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, defaulting to Debug")
	set(CMAKE_BUILD_TYPE "Debug")
endif()

# Architecture Detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(UNIJECT_X64 ON CACHE BOOL "")
	set(UNIJECT_BITS 64 CACHE INTERNAL "")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(UNIJECT_X86 ON CACHE BOOL "")
	set(UNIJECT_BITS 32 CACHE INTERNAL "")
else()
	message(FATAL_ERROR "Unrecognized processor architecture detected!") 
endif()

# Project-wide include folders
include_directories(include)

# Project-wide preprocessor definitions
add_compile_definitions(UNICODE=1)
add_compile_definitions(_UNICODE=1)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
add_compile_definitions(_TIME_BITS=64)
add_compile_definitions(_FILE_OFFSET_BITS=64)

set(UNIJECT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
set(UNIJECT_SOURCE_DIR "${UNIJECT_ROOTDIR}/src" CACHE INTERNAL "")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_NDEBUG=1")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -D_NDEBUG=1")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_NDEBUG=1")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(src)
#add_subdirectory(tests)
