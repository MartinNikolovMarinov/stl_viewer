cmake_minimum_required(VERSION 3.3)

project(stl_viewer VERSION 0.0.1)

# Standard requirenments:

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# Force cmake to put all libraries and executables in the same directory, so that special compilers can find them.

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Detect Build Type

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(STLV_DEBUG OFF)
else()
    set(STLV_DEBUG ON)
endif()

# Options:

option(STLV_HOT_RELOADING "Hot reload app code executable." OFF)

# Includes:

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/cmake"
)

include(CompilerOptions)
include(DetectOS)

# Subdirectories:

add_subdirectory(engine)
add_subdirectory(testbed)
