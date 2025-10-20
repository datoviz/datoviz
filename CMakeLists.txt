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
option(DVZ_ENABLE_ASAN_IN_DEBUG "Enable AddressSanitizer instrumentation in Debug builds" ON)
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

# Add subdirectories
add_subdirectory(src)
add_subdirectory(testing)

# Install headers and library
install(DIRECTORY include/ DESTINATION include)
