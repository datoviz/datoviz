# -------------------------------------------------------------------------------------------------
# File I/O
# -------------------------------------------------------------------------------------------------

file(GLOB THREAD_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_thread OBJECT ${THREAD_SRC})

target_include_directories(datoviz_thread
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_thread PUBLIC ${COMPILE_DEFINITIONS})
