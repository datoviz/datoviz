# -------------------------------------------------------------------------------------------------
# Math library
# -------------------------------------------------------------------------------------------------

# Math module (OBJECT library)
file(GLOB DS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_ds OBJECT ${DS_SRC})

target_include_directories(datoviz_ds
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
)

# Propagate global compile definitions (OS/Compiler flags)
target_compile_definitions(datoviz_ds PUBLIC ${COMPILE_DEFINITIONS})
