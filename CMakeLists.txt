cmake_minimum_required(VERSION 3.3)

project(stl_viewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# Cmake subdirectories:

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/core)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/fmt)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)

# Source Files.

file(GLOB_RECURSE src_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

# Executable.
add_executable(${PROJECT_NAME} main.cpp ${src_files})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core Library:
target_link_libraries(${PROJECT_NAME} PRIVATE core)

# Fmt Library:
target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include/GLFW)

# GLFW Library:
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
