cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(blaze VERSION 1.0.0)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

set(GEODE_CODEGEN_EXTRA_ARGS "--skip-pugixml" CACHE STRING "" FORCE)
set(GEODE_NO_PUGIXML_HEADER ON CACHE BOOL "" FORCE)
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)


CPMAddPackage("gh:dankmeme01/geobuild#main")
include("${CMAKE_CURRENT_BINARY_DIR}/geobuild-gen.cmake")
