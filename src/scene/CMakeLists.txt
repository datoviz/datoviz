# -------------------------------------------------------------------------------------------------
# Scene
# -------------------------------------------------------------------------------------------------

file(GLOB SCENE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_scene OBJECT ${SCENE_SRC})

target_include_directories(datoviz_scene
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_scene PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(TARGET datoviz_vklite)
    target_compile_definitions(datoviz_scene PUBLIC DVZ_DRP2_HAS_VKLITE=1)
else()
    target_compile_definitions(datoviz_scene PUBLIC DVZ_DRP2_HAS_VKLITE=0)
endif()
if(WIN32)
    target_compile_definitions(datoviz_scene PRIVATE DVZ_SHARED)
endif()
