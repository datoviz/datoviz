# -------------------------------------------------------------------------------------------------
# C examples — built against the internal build tree
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

# Detect whether we're being built as part of the main datoviz tree.
if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT TARGET datoviz_scene)
    message(FATAL_ERROR
        "examples/c must be built as part of the datoviz source tree "
        "(add_subdirectory from the top-level CMakeLists.txt).")
endif()

# Helper: add one grouped C example executable.
#
#   dvz_add_example(group name source)
#
# The CMake target is namespaced as example_c_<group>_<name>, while the generated executable keeps
# the short <name> filename under build/examples/c/<group>/.
function(dvz_add_example group name source)
    set(target "example_c_${group}_${name}")
    add_executable(${target} ${source})

    if(TARGET datoviz_gui)
        target_sources(${target} PRIVATE tools/example_gui_controls.c)
    endif()

    target_link_libraries(${target}
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_drp2
        datoviz_scene
    )

    if(TARGET datoviz_app)
        target_link_libraries(${target} PRIVATE datoviz_app)
    endif()

    if(TARGET datoviz_gui)
        target_link_libraries(${target} PRIVATE datoviz_gui)
    endif()
    if(TARGET datoviz_cimgui)
        target_link_libraries(${target} PRIVATE datoviz_cimgui)
    endif()

    if(TARGET datoviz_vklite)
        target_link_libraries(${target} PRIVATE datoviz_vk datoviz_vklite datoviz_volk)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(${target} PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(${target}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    get_property(_dvz_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)
    if(_dvz_compile_definitions)
        target_compile_definitions(${target} PRIVATE ${_dvz_compile_definitions})
    endif()

    if(TARGET datoviz_vklite)
        target_compile_definitions(${target} PRIVATE DVZ_DRP2_HAS_VKLITE=1)
    else()
        target_compile_definitions(${target} PRIVATE DVZ_DRP2_HAS_VKLITE=0)
    endif()

    if(APPLE)
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH  "@executable_path;@loader_path"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    endif()

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${name}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/c/${group}"
    )

    if(NOT TARGET ${name})
        add_custom_target(${name} DEPENDS ${target})
    endif()

    set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${target})
endfunction()

# -------------------------------------------------------------------------------------------------
# Examples
# -------------------------------------------------------------------------------------------------

if(TARGET datoviz_app AND DVZ_HAS_GLFW AND TARGET datoviz_gui)
    dvz_add_example(visuals point visuals/point.c)
    dvz_add_example(visuals pixel visuals/pixel.c)
    dvz_add_example(visuals marker visuals/marker.c)
    dvz_add_example(visuals primitive visuals/primitive.c)
    dvz_add_example(visuals segment visuals/segment.c)
    dvz_add_example(visuals path visuals/path.c)
    dvz_add_example(visuals mesh visuals/mesh.c)
    dvz_add_example(visuals image visuals/image.c)
    dvz_add_example(visuals volume visuals/volume.c)
    dvz_add_example(visuals sphere visuals/sphere.c)
    dvz_add_example(visuals text visuals/text.c)

    dvz_add_example(techniques depth_cue techniques/depth_cue.c)
    dvz_add_example(techniques depth_peel techniques/depth_peel.c)
    dvz_add_example(techniques edl techniques/edl.c)
    dvz_add_example(techniques wboit techniques/wboit.c)
    dvz_add_example(techniques pick_hover techniques/pick_hover.c)
    dvz_add_example(techniques image_probe techniques/image_probe.c)
    dvz_add_example(techniques linked_panels techniques/linked_panels.c)
    dvz_add_example(techniques multi_panel techniques/multi_panel.c)
    dvz_add_example(techniques scatter_axes techniques/scatter_axes.c)
    dvz_add_example(techniques gui_viewport techniques/gui_viewport.c)
    dvz_add_example(techniques gui_multi_viewport techniques/gui_multi_viewport.c)

    dvz_add_example(showcase lidar showcase/lidar.c)
    dvz_add_example(showcase protein showcase/protein.c)
    dvz_add_example(showcase brain showcase/brain.c)
    dvz_add_example(showcase ibl_brain showcase/ibl_brain.c)
    dvz_add_example(showcase spatial_omics showcase/spatial_omics.c)
    dvz_add_example(showcase labels showcase/labels.c)

    dvz_add_example(tools hosted_glfw_smoke tools/hosted_glfw_smoke.c)
    dvz_add_example(tools replay_dvzr tools/replay_dvzr.c)
endif()

if(TARGET datoviz_drp2)
    dvz_add_example(tools raw_triangle tools/raw_triangle.c)
    dvz_add_example(tools raw_triangle_drp2 tools/raw_triangle_drp2.c)
    if(TARGET datoviz_scene)
        dvz_add_example(tools export_primitive_wgsl tools/export_primitive_wgsl.c)
        dvz_add_example(tools export_point_wgsl tools/export_point_wgsl.c)
        dvz_add_example(tools export_image_wgsl tools/export_image_wgsl.c)
    endif()
endif()

if(TARGET datoviz_app)
    dvz_add_example(tools record_dvzr tools/record_dvzr.c)
endif()
