# Executalbe Name

set(executable_name "stlv_app_hot")

# Source Files

set(src_files
    app.h
    app.cpp
)

# Executable

if(STLV_HOT_RELOADING)
    add_library(${executable_name} SHARED ${src_files})
else()
    add_library(${executable_name} STATIC ${src_files})
endif()

if(STLV_HOT_RELOADING)
    # The following adds -fPIC
    set_property(TARGET ${executable_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_link_libraries(${executable_name} PRIVATE stlv_engine)
