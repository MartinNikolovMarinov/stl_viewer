cmake_minimum_required(VERSION 3.3)

project(stl_viewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# Force cmake to put all libraries and executables in the same directory, so that special compilers can find them.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Cmake subdirectories:

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/core)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)

# Source Files.

file(GLOB_RECURSE src_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

# Executable.
add_executable(${PROJECT_NAME} main.cpp ${src_files})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core Library:
target_link_libraries(${PROJECT_NAME} PUBLIC core)

# GLFW Library:
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

# Vulkan Library:
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found. Make sure to install the Vulkan SDK.")
endif()
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

# TODO: Add Release and Debug compiler optiontions.
