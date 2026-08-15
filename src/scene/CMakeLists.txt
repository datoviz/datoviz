# -------------------------------------------------------------------------------------------------
# Scene
# -------------------------------------------------------------------------------------------------

file(GLOB SCENE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
file(GLOB SCENE_QUERY_SRC "${CMAKE_CURRENT_SOURCE_DIR}/query/*.c*")
file(GLOB SCENE_SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.c*")
file(GLOB SCENE_VISUAL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/visuals/*.c*")
file(GLOB SCENE_VISUAL_QUERY_SRC "${CMAKE_CURRENT_SOURCE_DIR}/visuals/*/*.c*")
file(GLOB SCENE_ANNOTATION_SRC "${CMAKE_CURRENT_SOURCE_DIR}/annotation/*.c*")
file(GLOB SCENE_COLOR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/color/*.c*")
file(GLOB SCENE_CORE_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/core/*.c*")
file(GLOB SCENE_DOMAIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/domain/*.c*")
file(GLOB SCENE_FRAME_PLAN_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/frame_plan/*.c*")
file(GLOB SCENE_FRAME_PLAN_GRAPH_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/frame_plan/graph/*.c*")
file(GLOB SCENE_INTERACTION_SRC "${CMAKE_CURRENT_SOURCE_DIR}/interaction/*.c*")
file(GLOB SCENE_PLOT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/plot/*.c*")
file(GLOB SCENE_RENDER_CONTRACT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/render_contract/*.c*")
file(GLOB SCENE_RUNTIME_SRC "${CMAKE_CURRENT_SOURCE_DIR}/runtime/*.c*")
file(GLOB SCENE_SCENE_EMIT_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scene_emit/*.c*")
file(GLOB SCENE_TECHNIQUE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/techniques/*.c*")
file(GLOB SCENE_TEXT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/text/*.c*")

find_package(Python COMPONENTS Interpreter REQUIRED)

set(DVZ_TEXT_DEFAULT_MSDF_ATLAS_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/text/text_default_msdf_atlas.inc")
set(DVZ_TEXT_DEFAULT_MSDF_ATLAS_GENERATED
    "${CMAKE_CURRENT_BINARY_DIR}/text_default_msdf_atlas_generated.inc")

add_custom_command(
    OUTPUT ${DVZ_TEXT_DEFAULT_MSDF_ATLAS_GENERATED}
    COMMAND ${Python_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tools/convert_msdf_atlas_payloads.py
    --input
    ${DVZ_TEXT_DEFAULT_MSDF_ATLAS_SRC}
    --output
    ${DVZ_TEXT_DEFAULT_MSDF_ATLAS_GENERATED}
    DEPENDS
    ${DVZ_TEXT_DEFAULT_MSDF_ATLAS_SRC}
    ${PROJECT_SOURCE_DIR}/tools/convert_msdf_atlas_payloads.py
    COMMENT "Generating default MSDF atlas byte arrays"
    VERBATIM)

add_custom_target(datoviz_text_default_msdf_atlas
    DEPENDS ${DVZ_TEXT_DEFAULT_MSDF_ATLAS_GENERATED})
list(
    APPEND
    SCENE_SRC
    ${SCENE_QUERY_SRC}
    ${SCENE_SHADER_SRC}
    ${SCENE_VISUAL_SRC}
    ${SCENE_VISUAL_QUERY_SRC}
    ${SCENE_ANNOTATION_SRC}
    ${SCENE_COLOR_SRC}
    ${SCENE_CORE_SRC}
    ${SCENE_DOMAIN_SRC}
    ${SCENE_FRAME_PLAN_SRC}
    ${SCENE_FRAME_PLAN_GRAPH_SRC}
    ${SCENE_INTERACTION_SRC}
    ${SCENE_PLOT_SRC}
    ${SCENE_RENDER_CONTRACT_SRC}
    ${SCENE_RUNTIME_SRC}
    ${SCENE_SCENE_EMIT_SRC}
    ${SCENE_TECHNIQUE_SRC}
    ${SCENE_TEXT_SRC})

# Always include the SPIR-V resource registry so the public lookup symbol remains available. The
# registry is empty when glslc was not found.
if(DVZ_SHADERS_C)
    list(APPEND SCENE_SRC "${DVZ_SHADERS_C}")
endif()
if(DVZ_HAS_GLSL_SHADERS AND DVZ_GLSL_SHADERS_C)
    list(APPEND SCENE_SRC "${DVZ_GLSL_SHADERS_C}")
endif()
if(DVZ_HAS_WGSL_SHADERS AND DVZ_WGSL_SHADERS_C)
    list(APPEND SCENE_SRC "${DVZ_WGSL_SHADERS_C}")
endif()

add_library(datoviz_scene OBJECT ${SCENE_SRC})

# The canonical atlas generator is a developer tool. It deliberately compiles the pure CPU product
# layer directly and remains excluded from ordinary library builds.
add_executable(datoviz_text_atlas_generate EXCLUDE_FROM_ALL
    ${PROJECT_SOURCE_DIR}/tools/text_atlas_generate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/text_atlas_product.cpp)
target_compile_features(datoviz_text_atlas_generate PRIVATE cxx_std_17)
target_compile_definitions(
    datoviz_text_atlas_generate PRIVATE
    DVZ_HAS_MSDF_ATLAS=${DVZ_HAS_MSDF_ATLAS}
    DVZ_TEXT_ATLAS_GENERATOR_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
    DVZ_TEXT_ATLAS_GENERATOR_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}")
target_include_directories(datoviz_text_atlas_generate PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    datoviz_text_atlas_generate PRIVATE datoviz_common ${DVZ_TEXT_ATLAS_LIBS})
set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS datoviz_text_atlas_generate)

if(TARGET datoviz_shaders)
    add_dependencies(datoviz_scene datoviz_shaders)
endif()
if(DVZ_HAS_GLSL_SHADERS AND TARGET datoviz_glsl_shaders)
    add_dependencies(datoviz_scene datoviz_glsl_shaders)
endif()
if(DVZ_HAS_WGSL_SHADERS AND TARGET datoviz_wgsl_shaders)
    add_dependencies(datoviz_scene datoviz_wgsl_shaders)
endif()
add_dependencies(datoviz_scene datoviz_text_default_msdf_atlas)

target_include_directories(datoviz_scene
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/cimgui
    ${PROJECT_SOURCE_DIR}/external/volk
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/annotation
    ${CMAKE_CURRENT_SOURCE_DIR}/color
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/frame_plan
    ${CMAKE_CURRENT_SOURCE_DIR}/frame_plan/graph
    ${CMAKE_CURRENT_SOURCE_DIR}/interaction
    ${CMAKE_CURRENT_SOURCE_DIR}/plot
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_emit
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/techniques
    ${CMAKE_CURRENT_SOURCE_DIR}/text
    ${CMAKE_CURRENT_SOURCE_DIR}/visuals
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(datoviz_scene PUBLIC ${DVZ_COMPILE_DEFINITIONS})
target_link_libraries(datoviz_scene PUBLIC dvz_cglm)
target_compile_definitions(
    datoviz_scene PUBLIC DVZ_HAS_PRECOMPILED_SHADERS=${DVZ_HAS_PRECOMPILED_SHADERS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_GLSL_SHADERS=${DVZ_HAS_GLSL_SHADERS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_WGSL_SHADERS=${DVZ_HAS_WGSL_SHADERS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_EMBEDDED_FONTS=${DVZ_HAS_EMBEDDED_FONTS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_FREETYPE=${DVZ_HAS_FREETYPE})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_MSDF_ATLAS=${DVZ_HAS_MSDF_ATLAS})
target_compile_definitions(datoviz_scene PUBLIC DVZ_HAS_MSDF_SVG=${DVZ_HAS_MSDF_SVG})
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
