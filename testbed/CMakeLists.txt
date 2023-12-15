# Executable Name

set(executable_name "stlv_app")

# Subdirectories

add_subdirectory(app)

# Source Files

set(src_files

   main.cpp
)

if(OS STREQUAL "linux")
    set(src_files ${src_files})
elseif(OS STREQUAL "darwin")
    set(src_files ${src_files})
elseif(OS STREQUAL "windows")
    set(src_files ${src_files})
endif()

# Executable

add_executable(${executable_name} ${src_files})

target_compile_definitions(${executable_name} PRIVATE
    "STLV_HOT_RELOADING=$<BOOL:${STLV_HOT_RELOADING}>"
)

# Set compiler options for the engine:

stlv_target_set_default_flags(${executable_name})

# Link Libraries

if(STLV_HOT_RELOADING)
    target_link_libraries(${executable_name} PRIVATE stlv_engine dl)
    target_link_directories(${executable_name} PRIVATE ${CMAKE_BINARY_DIR})
else()
    target_link_libraries(${executable_name} PRIVATE stlv_engine stlv_app_hot)
endif()
