# Executable Name

set(executable_name "stlv_engine")

# Subdirectories

add_subdirectory(lib/core)
add_subdirectory(lib/glfw)

message(STATUS "[ENGINE] -------------------------------------------")

# Source Files

set(src_files

   src/init_core.cpp

   src/application/app_engine.cpp
   src/application/logger.cpp
   src/application/events.cpp
   src/application/timer.cpp
   src/application/input.cpp

   src/renderer/renderer_frontend.cpp
)

if (STLV_USE_GLFW)
    set(src_files ${src_files} src/platform/glfw_platform.cpp)
else()
    # At some point I might deside to use a native platform layer for some/all platforms.
    message(FATAL_ERROR "GLFW can't be disabled yet.")
endif()

if (STLV_BACKEND_VULKAN)
    set(src_files ${src_files}
        src/renderer/backends/vulkan/vulkan_backend.cpp
        src/renderer/backends/vulkan/vulkan_utils.cpp
        src/renderer/backends/vulkan/vulkan_device.cpp
        # src/renderer/backends/swapchain_vulkan.cpp
        # src/renderer/backends/image_vulkan.cpp
        # src/renderer/backends/renderpass_vulkan.cpp
        # src/renderer/backends/command_buffer_vulkan.cpp
        # src/renderer/backends/frame_buffer_vulkan.cpp
        # src/renderer/backends/sync_objects_vulkan.cpp
    )
else()
    message(FATAL_ERROR "Only Vulkan is supported at the moment.")
endif()

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

stlv_target_set_default_flags(${executable_name})

if(NOT STLV_BACKEND_OPENGL AND NOT STLV_BACKEND_VULKAN)

    message(FATAL_ERROR "No backend selected. Please select a backend.")

elseif(STLV_BACKEND_VULKAN)

    target_compile_definitions(${executable_name} PUBLIC
        STLV_BACKEND_VULKAN=1
    )

    # Vulkan Library:
    find_package(Vulkan REQUIRED)
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan not found.")
    endif()
    target_include_directories(${executable_name} PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(${executable_name} PRIVATE ${Vulkan_LIBRARIES})

    message(STATUS "[ENGINE] Backend: Vulkan")
    message(STATUS "[ENGINE] Backedn Location: ${Vulkan_LIBRARY}")

endif()

message(STATUS "[ENGINE] -------------------------------------------")
