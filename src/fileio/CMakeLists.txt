# -------------------------------------------------------------------------------------------------
# File I/O
# -------------------------------------------------------------------------------------------------

file(GLOB FILEIO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
list(APPEND FILEIO_SRC "${PROJECT_SOURCE_DIR}/external/fpng.cpp")
add_library(datoviz_fileio OBJECT ${FILEIO_SRC})

target_include_directories(datoviz_fileio
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(
    datoviz_fileio PUBLIC
    ${COMPILE_DEFINITIONS} "FPNG_USE_UNALIGNED_LOADS=0")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Enable the SIMD code paths used by fpng.
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _dvz_cpu_arch)
    if(_dvz_cpu_arch MATCHES "amd64|x86_64|i.86")
        target_compile_options(datoviz_fileio PRIVATE -msse4.1 -mpclmul)
    else()
        message(STATUS "Skipping SSE4.1/PCLMUL flags on non-x86 architecture (${CMAKE_SYSTEM_PROCESSOR}).")
    endif()
endif()
