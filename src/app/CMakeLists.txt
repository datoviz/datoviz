# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

file(GLOB APP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_app OBJECT ${APP_SRC})

target_include_directories(datoviz_app
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_link_libraries(datoviz_app
    PUBLIC
    datoviz_scene
    datoviz_drp2
    datoviz_canvas
    datoviz_vklite
    datoviz_vk
)

target_compile_definitions(datoviz_app PUBLIC
    ${DVZ_COMPILE_DEFINITIONS}
    DVZ_DRP2_HAS_VKLITE=1
    DVZ_HAS_APP=1
)

if(WIN32)
    target_compile_definitions(datoviz_app PRIVATE DVZ_SHARED)
endif()
