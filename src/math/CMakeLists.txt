# -------------------------------------------------------------------------------------------------
# Math library
# -------------------------------------------------------------------------------------------------

file(GLOB MATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_math OBJECT ${MATH_SRC})

target_include_directories(datoviz_math
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external/cglm/include
)

target_compile_definitions(datoviz_math PUBLIC ${COMPILE_DEFINITIONS})

# Ensure pthread headers from PThreads4W (Windows) or system Threads are visible
if(DVZ_THREADING_TARGET)
    target_link_libraries(datoviz_math PUBLIC ${DVZ_THREADING_TARGET})
endif()

if(WIN32)
    target_compile_definitions(datoviz_math PRIVATE DVZ_SHARED)
endif()
