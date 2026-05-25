# -------------------------------------------------------------------------------------------------
# Geometry utilities
# -------------------------------------------------------------------------------------------------

file(GLOB GEOM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_geom OBJECT ${GEOM_SRC})

target_include_directories(datoviz_geom
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external/cglm/include
    ${PROJECT_SOURCE_DIR}/external
)

target_compile_definitions(datoviz_geom PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(WIN32)
    target_compile_definitions(datoviz_geom PRIVATE DVZ_SHARED)
endif()
