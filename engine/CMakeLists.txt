# Executable Name

set(executable_name "stlv_engine")

# Subdirectories

add_subdirectory(lib/core)
add_subdirectory(lib/glfw)

# Source Files

set(src_files

   src/init_core.cpp
)

if(OS STREQUAL "linux")
    set(src_files ${src_files})
elseif(OS STREQUAL "darwin")
    set(src_files ${src_files})
elseif(OS STREQUAL "windows")
    set(src_files ${src_files})
endif()

# Create engine library:

add_library(${executable_name} SHARED ${src_files})

set_target_properties(${executable_name} PROPERTIES
    C_VISIBILITY_PRESET hidden
    C_VISIBILITY_INLINE_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_INLINE_PRESET hidden
)

target_include_directories(${executable_name} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${executable_name} PRIVATE
    -DSTLV_LIBRARY_BUILD=1
)
target_compile_definitions(${executable_name} PUBLIC
    "STLV_DEBUG=$<BOOL:${STLV_DEBUG}>"
    STLV_BINARY_PATH="${CMAKE_BINARY_DIR}/"
)

target_link_libraries(${executable_name} PUBLIC
    core
    glfw
)

# Set compiler options for the engine:

if(OS STREQUAL "linux")
    set(engine_common_flags "-std=c++20")
    set(engine_debug_flags "")
    set(engine_release_flags "")
elseif(OS STREQUAL "darwin")
    set(engine_common_flags "-std=c++20")
    set(engine_debug_flags "")
    set(engine_release_flags "")
elseif(OS STREQUAL "windows")
    set(engine_common_flags "/std:c++20")
    set(engine_debug_flags "")
    set(engine_release_flags "")
endif()

stlv_target_append_common_flags(
    ${executable_name}
    engine_common_flags "${engine_common_flags}"
    engine_debug_flags "${engine_debug_flags}"
    engine_release_flags "${engine_release_flags}"
)

message(STATUS "[ENGINE] common flags: ${engine_common_flags}")
message(STATUS "[ENGINE] debug flags: ${engine_debug_flags}")
message(STATUS "[ENGINE] release flags: ${engine_release_flags}")

# TODO: Set the flags!

# ##############################
# # Source Files.

# # # Executable.
# # add_executable(${executable_name} main.cpp ${src_files})
# # target_include_directories(${executable_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# # # Vulkan Library:
# # find_package(Vulkan REQUIRED)
# # if (NOT Vulkan_FOUND)
# #     message(FATAL_ERROR "Vulkan not found. Make sure to install the Vulkan SDK.")
# # endif()
# # target_include_directories(${executable_name} PRIVATE ${Vulkan_INCLUDE_DIRS})
# # target_link_libraries(${executable_name} PRIVATE ${Vulkan_LIBRARIES})

# # # TODO: Add Release and Debug compiler optiontions.
