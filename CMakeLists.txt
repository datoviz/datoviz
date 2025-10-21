# -------------------------------------------------------------------------------------------------
# Meta CMake file
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(datoviz VERSION 0.4 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Allocator & sanitizer options
option(DVZ_ENABLE_ASAN_IN_DEBUG "Enable sanitizer instrumentation in Debug builds" ON)
option(DVZ_ENABLE_COVERAGE "Enable code coverage instrumentation for supported compilers" OFF)
set(DVZ_SANITIZER "" CACHE STRING "Optional sanitizer override for Debug builds (asan, msan, tsan, or a comma-separated list)")
option(DVZ_USE_MIMALLOC_RELEASE_DEFAULT "Use mimalloc as the default allocator in Release builds" ON)

# Third-party dependencies
set(DVZ_WITH_MIMALLOC OFF)
set(DVZ_EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/external")
set(DVZ_MIMALLOC_DIR "${DVZ_EXTERNAL_DIR}/mimalloc")

if(EXISTS "${DVZ_MIMALLOC_DIR}/CMakeLists.txt")
    set(DVZ_WITH_MIMALLOC ON)
    set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(MI_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(MI_INSTALL_TOPLEVEL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${DVZ_MIMALLOC_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/external/mimalloc" EXCLUDE_FROM_ALL)
endif()

# Optional: default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable testing (for ctest)
# enable_testing()

# Global include for public headers
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Prepare a shared list of targets so subdirectories can register themselves.
set_property(GLOBAL PROPERTY DVZ_ALL_TARGETS "")

# Add subdirectories
add_subdirectory(src)
add_subdirectory(testing)

if(DVZ_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage instrumentation enabled")
        get_property(_dvz_targets GLOBAL PROPERTY DVZ_ALL_TARGETS)
        list(REMOVE_DUPLICATES _dvz_targets)

        set(_dvz_coverage_compile_flags -O0 -g --coverage)
        set(_dvz_coverage_link_flags --coverage)

        foreach(_dvz_target ${_dvz_targets})
            if(TARGET ${_dvz_target})
                get_target_property(_dvz_target_type ${_dvz_target} TYPE)
                if(_dvz_target_type AND NOT _dvz_target_type STREQUAL "INTERFACE_LIBRARY")
                    target_compile_options(${_dvz_target} PRIVATE ${_dvz_coverage_compile_flags})
                    if(NOT _dvz_target_type STREQUAL "OBJECT_LIBRARY")
                        target_link_options(${_dvz_target} PRIVATE ${_dvz_coverage_link_flags})
                    endif()
                endif()
            endif()
        endforeach()
    else()
        message(WARNING "DVZ_ENABLE_COVERAGE is ON but coverage flags are only supported with GCC or Clang")
    endif()
endif()

# Install headers and library
install(DIRECTORY include/ DESTINATION include)
