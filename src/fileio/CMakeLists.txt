# -------------------------------------------------------------------------------------------------
# File I/O
# -------------------------------------------------------------------------------------------------

file(GLOB FILEIO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_fileio OBJECT ${FILEIO_SRC})

target_include_directories(datoviz_fileio
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_fileio PUBLIC ${COMPILE_DEFINITIONS})
