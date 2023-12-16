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
option(STLV_BACKEND_VULKAN "Use Vulkan as backend." OFF)
option(STLV_BACKEND_OPENGL "Use OpenGL as backend." OFF)
option(STLV_USE_GLFW "Use GLFW as windowing library." ON)

# Includes:

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/cmake"
)

include(CompilerOptions)
include(DetectOS)

# Default compiler options macro:

macro (stlv_target_set_default_flags target)

    if(OS STREQUAL "linux")
        set(common_flags "-std=c++20")
        set(debug_flags "")
        set(release_flags "")
    elseif(OS STREQUAL "darwin")
        set(common_flags "-std=c++20")
        set(debug_flags "")
        set(release_flags "")
    elseif(OS STREQUAL "windows")
        set(common_flags "/std:c++20")
        set(debug_flags "")
        set(release_flags "")
    endif()

    stlv_target_append_common_flags(
        ${executable_name}
        common_flags "${common_flags}"
        debug_flags "${debug_flags}"
        release_flags "${release_flags}"
    )

    if(STLV_DEBUG)
        target_compile_options(${executable_name} PRIVATE ${common_flags} ${debug_flags})
    else()
        target_compile_options(${executable_name} PRIVATE ${common_flags} ${release_flags})
    endif()

endmacro()

# Subdirectories:

add_subdirectory(engine)
add_subdirectory(testbed)
