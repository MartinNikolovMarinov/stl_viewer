cmake_minimum_required(VERSION 3.20)
project(STL_VIEWER VERSION 0.1.0 LANGUAGES CXX)

set(target_main main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

set(CORE_ASSERT_ENABLED ON CACHE BOOL "Enable Asserts in corelib" FORCE)
add_subdirectory(lib/core)

if(APPLE)
    # Create executable
    add_executable(${target_main}
        main.cpp
        src/platform_mac.mm
        src/vulkan_renderer.cpp
    )

    # Correct paths for MoltenVK
    set(MOLTENVK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/MoltenVK/sdk-1.3.296.0)
    set(MOLTENVK_LIB ${MOLTENVK_DIR}/libMoltenVK.dylib)

    target_include_directories(${target_main} PRIVATE
        ${MOLTENVK_DIR}/include
    )

    target_link_libraries(${target_main} PRIVATE
        ${MOLTENVK_LIB}
        "-framework Cocoa"
        "-framework QuartzCore"
    )

    # Update rpaths to point to the directory where libMoltenVK.dylib is located
    set_target_properties(${target_main} PROPERTIES
        BUILD_RPATH "@executable_path/../lib/MoltenVK/sdk-1.3.296.0"
        INSTALL_RPATH "@executable_path/../lib/MoltenVK/sdk-1.3.296.0"
    )
elseif(WIN32)
    # Create executable
    add_executable(${target_main}
        main.cpp
        src/platform_win32.cpp
        src/vulkan_renderer.cpp
    )

    # Link to Vulkan from system
    find_package(Vulkan REQUIRED)
    target_link_libraries(${target_main} PRIVATE Vulkan::Vulkan)
else()
    option(USE_WAYLAND "Use Wayland instead of X11" OFF)

    if(USE_WAYLAND)
        # Create executable
        add_executable(${target_main}
            main.cpp
            src/platform_wayland.cpp
            src/vulkan_renderer.cpp
        )
        find_package(PkgConfig REQUIRED)
        find_package(Vulkan REQUIRED)
        pkg_check_modules(WAYLAND REQUIRED wayland-client)
        target_include_directories(${target_main} PRIVATE ${WAYLAND_INCLUDE_DIRS})
        target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES} ${WAYLAND_LIBRARIES})
    else()
        # Create executable
        add_executable(${target_main}
            main.cpp

            src/app_error.cpp
            src/platform.cpp

            src/platform_x11.cpp
            src/vulkan_renderer.cpp
        )
        find_package(X11 REQUIRED)
        find_package(Vulkan REQUIRED)
        target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES} ${X11_LIBRARIES})
        target_compile_definitions(${target_main} PRIVATE -DUSE_X11)
    endif()
endif()

target_include_directories(${target_main} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${target_main} PUBLIC core)

# ---------------- Packaging with CPack ----------------
# include(CPack)
# set(CPACK_GENERATOR "ZIP")
# set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
# set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
# set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
