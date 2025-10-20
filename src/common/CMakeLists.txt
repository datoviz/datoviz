# -------------------------------------------------------------------------------------------------
# Common library
# -------------------------------------------------------------------------------------------------

# Common module (OBJECT library)
file(GLOB COMMON_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_library(datoviz_common OBJECT ${COMMON_SRC})

target_include_directories(datoviz_common
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Propagate global compile definitions (OS/Compiler flags)
target_compile_definitions(datoviz_common PUBLIC ${COMPILE_DEFINITIONS})
