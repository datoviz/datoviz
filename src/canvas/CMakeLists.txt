file(GLOB CANVAS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_canvas OBJECT ${CANVAS_SRC})

target_include_directories(datoviz_canvas
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/window
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_canvas PUBLIC ${COMPILE_DEFINITIONS})
