cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(blaze VERSION 1.0.0)

option(BLAZE_USE_TRACY "Use tracy" OFF)
option(BLAZE_DEBUG "Debug mode" OFF)

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

if (BLAZE_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BLAZE_DEBUG=1)
endif()

CPMAddPackage("gh:dankmeme01/asp2#7024af9")
CPMAddPackage(
    NAME Boost
    VERSION 1.84.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
    URL_HASH SHA256=2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e
    OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES container" # escape with \\\;
)
CPMAddPackage("gh:simdutf/simdutf#83c0e74")
CPMAddPackage(
    NAME libdeflate
    GIT_REPOSITORY "https://github.com/ebiggers/libdeflate.git"
    GIT_TAG "v1.20"
    OPTIONS "LIBDEFLATE_BUILD_SHARED_LIB OFF" "LIBDEFLATE_BUILD_GZIP OFF"
)

# Suppress some warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(boost_container PRIVATE "-Wno-incompatible-pointer-types")
endif()

# silly thing
if (ANDROID)
    if (${GEODE_TARGET_PLATFORM} STREQUAL "Android64")
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

target_link_libraries(${PROJECT_NAME} asp Boost::container simdutf libdeflate_static)
