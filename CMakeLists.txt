cmake_minimum_required(VERSION 3.2)
project(STL_VIEWER VERSION 0.1.0 LANGUAGES CXX)

set(target_main stlv)

# Standard Requirements:

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# Add directories from lib cmake projects

set(CORE_ASSERT_ENABLED ON CACHE BOOL "Enable Asserts in corelib" FORCE)
add_subdirectory(lib/core)

# Include cmake modules from:

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/lib/core/cmake") # TODO2: Is this how I want it to work?
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_local")

include(DetectOS)
include(CompilerOptions)
include(Logger)
include(STLVDefaultFlags)

init_logger("[STLV]")

# ---------------------------------------- Begin Options ---------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(STLV_DEBUG OFF)
else()
    set(STLV_DEBUG ON)
endif()

option(CORE_LIBRARY_SHARED "Build core as a shared library." OFF)
option(CORE_ASSERT_ENABLED "Enable asserts." OFF)
option(USE_EXTERNAL_VULKAN_SDK "Use external Vulkan SDK." ON) # NOTE: This is only relevant for MacOS for now.

# Print Selected Options:

log_info("---------------------------------------------")
log_info("Version:                   ${PROJECT_VERSION}")
log_info("Platform:                  ${OS}")
log_info("Arch:                      ${CMAKE_SYSTEM_PROCESSOR}")
log_info("C++ Version:               ${CMAKE_CXX_STANDARD}")
log_info("Compiler:                  ${CMAKE_CXX_COMPILER_ID}")
log_info("Debug:                     ${STLV_DEBUG}")
log_info("Use External Vulkan SDK:   ${USE_EXTERNAL_VULKAN_SDK}")
log_info("---------------------------------------------")

# ---------------------------------------- End Options -----------------------------------------------------------------

# ---------------------------------------- Begin Declare Source Files --------------------------------------------------

set(sandbox_src) # TODO: add a build option for this

set(stlv_src
    main.cpp

    src/app_error.cpp
    src/app.cpp
    src/user_input.cpp
    src/vulkan_renderer.cpp
    src/vulkan_device_picker.cpp
    src/vulkan_swapchain.cpp
)

if(OS STREQUAL "linux")
    # TODO: make an option for wayland
    set(stlv_src ${stlv_src}
        src/platform_x11.cpp
    )

    set(sandbox_src ${sandbox_src} tools/sandbox/sandbox.cpp)
elseif(OS STREQUAL "darwin")
    set(stlv_src ${stlv_src}
        src/platform_mac.mm
    )

    set(sandbox_src ${sandbox_src} tools/sandbox/sandbox.mm)
elseif(OS STREQUAL "windows")
    set(stlv_src ${stlv_src}
        src/platform_win32.cpp
    )

    set(sandbox_src ${sandbox_src} tools/sandbox/sandbox.cpp)
endif()

# ---------------------------------------- End Declare Source Files ----------------------------------------------------

# ---------------------------------------- Begin Create Executable -----------------------------------------------------

add_executable(${target_main} ${stlv_src} ${sandbox_src})
target_link_libraries(${target_main} PUBLIC
    core # link with corelib
)
target_include_directories(${target_main} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(${target_main} PUBLIC
    "STLV_DEBUG=$<BOOL:${STLV_DEBUG}>"
)

if(OS STREQUAL "linux")
    if(!USE_EXTERNAL_VULKAN_SDK)
        log_fatal("Can't package executable with vulkan SDK on Linux (for now). Use external SDK.")
    endif()

    find_package(X11 REQUIRED)
    find_package(Vulkan REQUIRED)  # Link with Vulkan from system
    target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES} ${X11_LIBRARIES})
    target_compile_definitions(${target_main} PRIVATE -DUSE_X11)

    # TODO: NEEDED FOR WAYLAND
    #
    #     if(USE_WAYLAND)
    #         find_package(PkgConfig REQUIRED)
    #         find_package(Vulkan REQUIRED)
    #         pkg_check_modules(WAYLAND REQUIRED wayland-client)
    #         target_include_directories(${target_main} PRIVATE ${WAYLAND_INCLUDE_DIRS})
    #         target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES} ${WAYLAND_LIBRARIES})
    #     else()

elseif(OS STREQUAL "darwin")

    if(USE_EXTERNAL_VULKAN_SDK)
        find_package(Vulkan REQUIRED)
        target_include_directories(${target_main} PRIVATE ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES})
    else()
        # Link to MoltenVK from lib
        set(MOLTENVK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/MoltenVK/sdk-1.3.296.0)
        set(MOLTENVK_LIB ${MOLTENVK_DIR}/libMoltenVK.dylib)

        target_include_directories(${target_main} PRIVATE
            ${MOLTENVK_DIR}/include
        )

        # Update rpaths to point to the directory where libMoltenVK.dylib is located
        set_target_properties(${target_main} PROPERTIES
            BUILD_RPATH "@executable_path/../lib/MoltenVK/sdk-1.3.296.0"
            INSTALL_RPATH "@executable_path/../lib/MoltenVK/sdk-1.3.296.0"
        )

        target_link_libraries(${target_main} PRIVATE ${MOLTENVK_LIB})
    endif()

    find_library(METAL Metal REQUIRED)
    target_link_libraries(${target_main} PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        ${METAL}
    )

elseif(OS STREQUAL "windows")
    if(!USE_EXTERNAL_VULKAN_SDK)
        log_fatal("Can't package executable with vulkan SDK on Windows (for now). Use external SDK.")
    endif()

    # Link with Vulkan from system
    find_package(Vulkan REQUIRED)
    target_link_libraries(${target_main} PRIVATE Vulkan::Vulkan)
    target_include_directories(${target_main} PRIVATE ${Vulkan_INCLUDE_DIRS})

endif()

# ---------------------------------------- END Create Executable -------------------------------------------------------
