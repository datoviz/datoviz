file(GLOB WINDOW_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_window OBJECT ${WINDOW_SRC})

target_include_directories(datoviz_window
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_window PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(WIN32)
    target_compile_definitions(datoviz_window PRIVATE DVZ_SHARED)
endif()

if(DVZ_HAS_GLFW AND TARGET glfw::glfw)
    target_link_libraries(datoviz_window PUBLIC glfw::glfw)
endif()
