# -------------------------------------------------------------------------------------------------
# Math library
# -------------------------------------------------------------------------------------------------

# Math module (OBJECT library)
file(GLOB MATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_library(datoviz_math OBJECT ${MATH_SRC})

target_include_directories(datoviz_math
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
)

# Propagate global compile definitions (OS/Compiler flags)
target_compile_definitions(datoviz_math PUBLIC ${COMPILE_DEFINITIONS})
