cmake_minimum_required(VERSION 3.20)
project(datoviz VERSION 0.4 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optional: default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable testing (for ctest)
enable_testing()

# Global include for public headers
# Allow includes like "datoviz_common.h" (no prefix) and "datoviz/*.h"
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    # ${PROJECT_SOURCE_DIR}/include/datoviz
)

# Add subdirectories
add_subdirectory(testing)
add_subdirectory(src)

# Install headers and library
install(DIRECTORY include/ DESTINATION include)
