cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)


project(blaze VERSION 1.0.0)

# Disable PCH, it causes pugixml 1.2 headers to be used
set(GEODE_DISABLE_PRECOMPILED_HEADERS ON)

option(BLAZE_USE_TRACY "Use tracy" OFF)
option(BLAZE_DEBUG "Debug mode" OFF)
option(BLAZE_RELEASE "Release build" OFF)

# source files
file(GLOB_RECURSE SOURCES
	src/*.cpp
    external/*.c
    external/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE external/)
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# make spng use miniz instead of zlib
target_compile_definitions(${PROJECT_NAME} PRIVATE SPNG_USE_MINIZ=1 SPNG_STATIC=1)

CPMAddPackage("gh:dankmeme01/asp2#8e25afa")
CPMAddPackage("gh:simdutf/simdutf#83c0e74")
CPMAddPackage(
    NAME libdeflate
    GIT_REPOSITORY "https://github.com/ebiggers/libdeflate.git"
    GIT_TAG "v1.20"
    OPTIONS "LIBDEFLATE_BUILD_SHARED_LIB OFF" "LIBDEFLATE_BUILD_GZIP OFF"
)
CPMAddPackage("gh:Prevter/sinaps#1663e86")
CPMAddPackage("gh:zeux/pugixml@1.15")
CPMAddPackage("gh:dankmeme01/fast_float#b7b17e6")

if (BLAZE_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BLAZE_DEBUG=1)
    target_compile_definitions(asp PRIVATE _HAS_ITERATOR_DEBUGGING=0)
    target_compile_definitions(simdutf PRIVATE _HAS_ITERATOR_DEBUGGING=0)
endif()

if (BLAZE_RELEASE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# silly thing
if (ANDROID)
    if (ANDROID_ABI STREQUAL "arm64-v8a")
        target_compile_options(${PROJECT_NAME} PRIVATE "-march=armv8-a+crc")
    endif()
endif()

if (BLAZE_USE_TRACY)
    # taken from https://github.com/cgytrus/AlgebraDash/blob/main/CMakeLists.txt
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)

    set(TRACY_ONLY_LOCALHOST OFF CACHE BOOL "" FORCE)

    set(TRACY_NO_CALLSTACK OFF CACHE BOOL "" FORCE)
    set(TRACY_CALLSTACK OFF CACHE BOOL "" FORCE)

    CPMAddPackage("gh:dankmeme01/tracy#95a3b94")
    target_include_directories(${PROJECT_NAME} PRIVATE ${Tracy_SOURCE_DIR}/public/tracy)
    target_link_libraries(${PROJECT_NAME} Tracy::TracyClient)

    target_compile_definitions(${PROJECT_NAME} PRIVATE BLAZE_TRACY=1)

    add_compile_options("-Wno-incompatible-pointer-types")
endif()

target_link_libraries(${PROJECT_NAME} asp simdutf libdeflate_static sinaps fast_float pugixml-static)
