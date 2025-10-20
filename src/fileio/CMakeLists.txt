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

target_compile_definitions(datoviz_fileio PUBLIC ${COMPILE_DEFINITIONS})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Enable the SIMD code paths used by fpng.
    target_compile_options(datoviz_fileio PRIVATE -msse4.1 -mpclmul)
endif()
