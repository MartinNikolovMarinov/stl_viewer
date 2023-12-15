cmake_minimum_required(VERSION 3.2)

project(core VERSION 0.2.0)

# Global CMake options:

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# BEGIN Options

option(CORE_LIBRARY_SHARED "Build core as a shared library." OFF)
option(CORE_ASSERT_ENABLED "Enable asserts." ON)
option(CORE_BUILD_TESTS "Build core tests." OFF)
option(CORE_RUN_COMPILETIME_TESTS "Run compile-time tests." OFF)
option(CORE_USE_CPPCHECK "Use cppcheck. [WARNING] This makes compilation incredibly slow!" OFF)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CORE_DEBUG OFF)
else()
    set(CORE_DEBUG ON)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
   CMAKE_SYSTEM_NAME STREQUAL "CYGWIN" OR
   CMAKE_SYSTEM_NAME STREQUAL "MSYS" OR
   CMAKE_SYSTEM_NAME STREQUAL "WindowsCE")
    set(OS "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OS "darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS "linux")
else()
    set(OS "unsupported")
    message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}!")
endif()

if(CORE_USE_CPPCHECK)
    # IMPORTANT: This makes compilation incredibly slow!
    find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
    if(CMAKE_CXX_CPPCHECK)
        list(APPEND CMAKE_CXX_CPPCHECK
                "--enable=all"
                "--inconclusive"
                "--inline-suppr"
                "--std=c++20"
                "--check-level=exhaustive"
                "--output-file=${CMAKE_BINARY_DIR}/cppcheck.out.txt"
                "--suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.txt")
    else()
        message(WARNING "[CORE] cppcheck not found!")
    endif()
endif()

# END Options

# BEGIN Print selected options

message(STATUS "[CORE] ---------------------------------------------")
message(STATUS "[CORE] Version:           ${PROJECT_VERSION}")
message(STATUS "[CORE] Platform:          ${OS}")
message(STATUS "[CORE] Arch:              ${CMAKE_HOST_SYSTEM_PROCESSOR}") # IMPORTANT: Do not relly on CMAKE_HOST_SYSTEM_PROCESSOR for anything serious !
message(STATUS "[CORE] C++ Version:       ${CMAKE_CXX_STANDARD}")
message(STATUS "[CORE] Compiler:          ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "[CORE] Debug:             ${CORE_DEBUG}")
message(STATUS "[CORE] Assert:            ${CORE_ASSERT_ENABLED}")
message(STATUS "[CORE] Shared:            ${CORE_LIBRARY_SHARED}")
message(STATUS "[CORE] Build Tests:       ${CORE_BUILD_TESTS}")
message(STATUS "[CORE] Run Compile Tests: ${CORE_RUN_COMPILETIME_TESTS}")
message(STATUS "[CORE] Use CppCheck:      ${CORE_USE_CPPCHECK}")

# END Print selected options

# BEGIN Declare Source Files

set(src_files
    src/core_hash.cpp
    src/core_intrinsics.cpp
    src/core_mem.cpp
    src/core_rnd.cpp
    src/core_utils.cpp

    src/allocators/core_alloc_bump.cpp
    src/allocators/core_alloc_stats_std.cpp
    src/allocators/core_alloc_std.cpp

    src/math/core_projections.cpp
)

if(OS STREQUAL "linux")
    set(src_files ${src_files}
        src/plt/unix/unix_threading.cpp
        src/plt/unix/unix_time.cpp
        src/plt/unix/unix_plt_error.cpp
        src/plt/unix/unix_pages.cpp
        src/plt/unix/unix_fs.cpp
    )
elseif(OS STREQUAL "darwin")
    set(src_files ${src_files}
        src/plt/unix/unix_threading.cpp
        src/plt/unix/unix_time.cpp
        src/plt/unix/unix_plt_error.cpp
        src/plt/unix/unix_pages.cpp
        src/plt/unix/unix_fs.cpp
    )
elseif(OS STREQUAL "windows")
    set(src_files ${src_files}
        src/plt/win/win_threading.cpp
        src/plt/win/win_time.cpp
        src/plt/win/win_plt_error.cpp
        src/plt/win/win_pages.cpp
        src/plt/win/win_fs.cpp
    )
endif()

# END Declare Source Files

# BEGIN Add library

if(CORE_LIBRARY_SHARED)
    add_library(${PROJECT_NAME} SHARED ${src_files})
    # Set symbols visibility to be hidden by default:
    set_target_properties(${PROJECT_NAME} PROPERTIES
        C_VISIBILITY_PRESET hidden
        C_VISIBILITY_INLINE_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_INLINE_PRESET hidden
    )
else()
    add_library(${PROJECT_NAME} STATIC ${src_files})
endif()

# END Add library

# BEGIN Set Configure File

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/core_config.h.in core_config.h)

# END Set Configure File

# BEGIN Include Path

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR} # for core_config.h
)

# END Include Path

# BEGIN Definitions

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "CORE_LIBRARY_SHARED=$<BOOL:${CORE_LIBRARY_SHARED}>" # turn CMake Boolean to 1 or 0
    -DCORE_LIBRARY_BUILD=1
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    "CORE_DEBUG=$<BOOL:${CORE_DEBUG}>"
    "CORE_ASSERT_ENABLED=$<BOOL:${CORE_ASSERT_ENABLED}>"
    "CORE_RUN_COMPILETIME_TESTS=$<BOOL:${CORE_RUN_COMPILETIME_TESTS}>"
)

# END Definitions

# BEGIN Compile Options

macro(__core_configure_compiler_settings
      target
      common_flags_varname common_flags
      debug_flags_varname debug_flags
      release_flags_varname release_flags)

    set(local_common_flags "${common_flags}")
    set(local_debug_flags "${debug_flags}")
    set(local_release_flags "${release_flags}")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

        set(local_common_flags ${local_common_flags}
            -std=c++20 -pthread
            -Wall -Wextra -Wfatal-errors -Wconversion -Wpedantic -Werror
            -Wshadow -Wold-style-cast -Wdouble-promotion -Wswitch-enum -Wundef -Wcast-align -Wsign-conversion
            -Wmisleading-indentation -Woverloaded-virtual -Wnon-virtual-dtor
            -Wdisabled-optimization # warn if the compailer disables requestd level of optimization
            -Wno-unknown-pragmas -Wno-unused-function -Wno-variadic-macros
        )
        set(local_debug_flags ${local_debug_flags} -g -O0 -Wnull-dereference)
        set(local_release_flags ${local_release_flags} -O3)

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        set(local_common_flags ${local_common_flags}
            -std:c++20
            /MP
            -Wall -W4 -WX
            -nologo -FC
            /fastfail
            /w14254 # Warns if more than one operator conversion is applied to get from one type to another (useful for finding implicit conversions that might be unintended).
            /w14263 # Warns if a member function hides a base class virtual function (similar to -Woverloaded-virtual in GCC/Clang).
            /w14265 # Warns if a class is derived from two or more classes that are not interfaces.
            /w14287 # Warns for 'unsigned' char, short, int or long types in a conditional expression, which can lead to unexpected behaviors.
            /w14296 # Warns if a constructor is declared without specifying either 'explicit' or 'implicit'.
            /w14311 # Warns if a catch block has been written to catch C++ exceptions by value rather than by reference.
        )
        set(local_debug_flags ${local_debug_flags} -Zi -Od)
        set(local_release_flags ${local_release_flags} -O2)

        if (CORE_DEBUG)
            # DbgHelp.lib is required for stacktrace.
            target_link_options(${target} PRIVATE /INCREMENTAL:NO /OPT:REF)
            target_link_libraries(${target} PUBLIC DbgHelp.lib)
        endif()

    else()
        message(FATAL_ERROR "Unsupported compiler")
        return()
    endif()

    set(${common_flags_varname} ${local_common_flags})
    set(${debug_flags_varname} ${local_debug_flags})
    set(${release_flags_varname} ${local_release_flags})

endmacro()

set(mm_core_common_flags "")
set(mm_core_debug_flags "")
set(mm_core_release_flags "")

__core_configure_compiler_settings(
    ${PROJECT_NAME}
    mm_core_common_flags "${mm_core_common_flags}"
    mm_core_debug_flags "${mm_core_debug_flags}"
    mm_core_release_flags "${mm_core_release_flags}"
)

if(CORE_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE ${mm_core_common_flags} ${mm_core_debug_flags})
else()
    target_compile_options(${PROJECT_NAME} PRIVATE ${mm_core_common_flags} ${mm_core_release_flags})
endif()

# END Compile Options

# BEGIN Create Testing Executable

if(CORE_BUILD_TESTS)
    message(STATUS "[CORE] Configuring tests")

    set(TEST_EXECUTABLE ${PROJECT_NAME}_test)

    # Create executable
    add_executable(${TEST_EXECUTABLE}
        ${TEST_EXECUTABLE}.cpp

        tests/t-index_core_init.cpp

        tests/t-algorithms.cpp
        tests/t-arr.cpp
        tests/t-bits.cpp
        tests/t-cmd_flag_parser.cpp
        tests/t-cptr_conv.cpp
        tests/t-cptr.cpp
        tests/t-defer.cpp
        tests/t-expected.cpp
        tests/t-hash_map.cpp
        tests/t-hash.cpp
        tests/t-intrinsics.cpp
        tests/t-ints.cpp
        tests/t-mem.cpp
        tests/t-rnd.cpp
        tests/t-sarr.cpp
        tests/t-str_builder.cpp
        tests/t-traits.cpp
        tests/t-tuple.cpp
        tests/t-unique_ptr.cpp
        tests/t-utf.cpp

        tests/allocators/t-alloc_bump.cpp
        tests/allocators/t-alloc_std_stats.cpp
        tests/allocators/t-alloc_std.cpp

        tests/math/t-math.cpp
        tests/math/t-matrix.cpp
        tests/math/t-transforms.cpp
        tests/math/t-vec.cpp

        tests/plt/t-pages.cpp
        tests/plt/t-plt_err.cpp
        tests/plt/t-stacktrace.cpp
        tests/plt/t-threading.cpp
        tests/plt/t-time.cpp
        tests/plt/t-fs.cpp
    )

    # Link core
    target_link_libraries(${TEST_EXECUTABLE} ${PROJECT_NAME})

    # Definitions
    target_compile_definitions(${TEST_EXECUTABLE} PRIVATE
        -DPATH_TO_TEST_DATA="${CMAKE_CURRENT_BINARY_DIR}"
    )

    # Compile options

    set(mm_test_common_flags "")
    set(mm_test_debug_flags "")
    set(mm_test_release_flags "")

    __core_configure_compiler_settings(
        ${PROJECT_NAME}
        mm_test_common_flags "${mm_test_common_flags}"
        mm_test_debug_flags "${mm_test_debug_flags}"
        mm_test_release_flags "${mm_test_release_flags}"
    )

    if(CORE_DEBUG)
        target_compile_options(${TEST_EXECUTABLE} PUBLIC ${mm_test_common_flags} ${mm_test_debug_flags})
    else()
        target_compile_options(${TEST_EXECUTABLE} PUBLIC ${mm_test_common_flags} ${mm_test_release_flags})
    endif()

    # Add the test
    enable_testing()
    add_test(NAME ${TEST_EXECUTABLE} COMMAND ${TEST_EXECUTABLE})
endif()

# END Create Testing Executable

message(STATUS "[CORE] ---------------------------------------------")
