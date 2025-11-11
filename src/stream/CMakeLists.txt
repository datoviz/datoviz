# -------------------------------------------------------------------------------------------------
# Frame stream module
# -------------------------------------------------------------------------------------------------

file(GLOB STREAM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(datoviz_stream OBJECT
    ${STREAM_SRC}
)

target_include_directories(datoviz_stream
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_stream
    PUBLIC
        ${DVZ_COMPILE_DEFINITIONS}
)
