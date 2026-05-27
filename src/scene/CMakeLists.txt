# -------------------------------------------------------------------------------------------------
# Scene
# -------------------------------------------------------------------------------------------------

file(GLOB SCENE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
file(GLOB SCENE_QUERY_SRC "${CMAKE_CURRENT_SOURCE_DIR}/query/*.c*")
file(GLOB SCENE_VISUAL_QUERY_SRC "${CMAKE_CURRENT_SOURCE_DIR}/visuals/*/*.c*")
list(APPEND SCENE_SRC ${SCENE_QUERY_SRC} ${SCENE_VISUAL_QUERY_SRC})

# Include precompiled SPIR-V shaders if glslc was found.
if(DVZ_HAS_PRECOMPILED_SHADERS AND DVZ_SHADERS_C)
    list(APPEND SCENE_SRC "${DVZ_SHADERS_C}")
endif()
if(DVZ_HAS_GLSL_SHADERS AND DVZ_GLSL_SHADERS_C)
    list(APPEND SCENE_SRC "${DVZ_GLSL_SHADERS_C}")
endif()
if(DVZ_HAS_WGSL_SHADERS AND DVZ_WGSL_SHADERS_C)
    list(APPEND SCENE_SRC "${DVZ_WGSL_SHADERS_C}")
endif()
if(DVZ_HAS_EMBEDDED_FONTS AND DVZ_FONTS_C)
    list(APPEND SCENE_SRC "${DVZ_FONTS_C}")
endif()

add_library(datoviz_scene OBJECT ${SCENE_SRC})

if(DVZ_HAS_PRECOMPILED_SHADERS AND TARGET datoviz_shaders)
    add_dependencies(datoviz_scene datoviz_shaders)
endif()
if(DVZ_HAS_GLSL_SHADERS AND TARGET datoviz_glsl_shaders)
    add_dependencies(datoviz_scene datoviz_glsl_shaders)
endif()
if(DVZ_HAS_WGSL_SHADERS AND TARGET datoviz_wgsl_shaders)
    add_dependencies(datoviz_scene datoviz_wgsl_shaders)
endif()
if(DVZ_HAS_EMBEDDED_FONTS AND TARGET datoviz_fonts)
    add_dependencies(datoviz_scene datoviz_fonts)
endif()

target_include_directories(datoviz_scene
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/cimgui
    ${PROJECT_SOURCE_DIR}/external/cglm/include
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_scene PUBLIC ${DVZ_COMPILE_DEFINITIONS})
target_compile_definitions(
    datoviz_scene PUBLIC DVZ_HAS_PRECOMPILED_SHADERS=${DVZ_HAS_PRECOMPILED_SHADERS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_GLSL_SHADERS=${DVZ_HAS_GLSL_SHADERS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_WGSL_SHADERS=${DVZ_HAS_WGSL_SHADERS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_EMBEDDED_FONTS=${DVZ_HAS_EMBEDDED_FONTS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_FREETYPE=${DVZ_HAS_FREETYPE})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_MSDF_ATLAS=${DVZ_HAS_MSDF_ATLAS})
if(TARGET datoviz_geom)
    target_link_libraries(datoviz_scene PUBLIC datoviz_geom)
endif()
if(TARGET datoviz_controller)
    target_link_libraries(datoviz_scene PUBLIC datoviz_controller)
endif()
if(TARGET datoviz_vklite)
    target_compile_definitions(datoviz_scene PUBLIC DVZ_DRP2_HAS_VKLITE=1)
else()
    target_compile_definitions(datoviz_scene PUBLIC DVZ_DRP2_HAS_VKLITE=0)
endif()
if(WIN32)
    target_compile_definitions(datoviz_scene PRIVATE DVZ_SHARED)
endif()
