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
    add_executable(${target} ${source} example_common.c example_style.c runner/scenario_runner.c)

    if(TARGET datoviz_gui)
        target_sources(${target} PRIVATE example_gui_controls.c)
    endif()

    if(TARGET datoviz_app)
        target_sources(${target} PRIVATE example_debug.c)
    endif()

    target_link_libraries(${target}
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_controller
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
        ${PROJECT_SOURCE_DIR}/src/app
        ${PROJECT_SOURCE_DIR}/src/scene
        ${CMAKE_CURRENT_SOURCE_DIR}
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
    dvz_add_example(features scene_basic features/scene_basic.c)
    dvz_add_example(features panel_single features/panel_single.c)
    dvz_add_example(features panel_grid features/panel_grid.c)
    dvz_add_example(features panel_multi features/panel_multi.c)
    dvz_add_example(features panel_linked features/panel_linked.c)
    dvz_add_example(features axes_2d features/axes_2d.c)
    dvz_add_example(features axis_labels features/axis_labels.c)
    dvz_add_example(features sampled_field_2d features/sampled_field_2d.c)
    dvz_add_example(features sampled_field_3d features/sampled_field_3d.c)
    dvz_add_example(features text_block features/text_block.c)
    dvz_add_example(features overlay_card features/overlay_card.c)
    dvz_add_example(features controller_arcball features/controller_arcball.c)
    dvz_add_example(features controller_fly features/controller_fly.c)
    dvz_add_example(features controller_turntable features/controller_turntable.c)
    dvz_add_example(features mesh_texture features/mesh_texture.c)
    dvz_add_example(features material_mesh features/material_mesh.c)
    dvz_add_example(features lighting features/lighting.c)
    dvz_add_example(features datetime_axis features/datetime_axis.c)
    dvz_add_example(features scalebar features/scalebar.c)
    dvz_add_example(features scalebar_units features/scalebar_units.c)
    dvz_add_example(features colorbar features/colorbar.c)
    dvz_add_example(features colormap_scale features/colormap_scale.c)
    dvz_add_example(features annotation_readout features/annotation_readout.c)
    dvz_add_example(features image_probe features/image_probe.c)
    dvz_add_example(features update_visual_data features/update_visual_data.c)
    dvz_add_example(features update_partial features/update_partial.c)
    dvz_add_example(features visibility features/visibility.c)
    dvz_add_example(features depth_test features/depth_test.c)
    dvz_add_example(features alpha_blending features/alpha_blending.c)
    dvz_add_example(features panel_background features/panel_background.c)
    dvz_add_example(features panzoom_attachment features/panzoom_attachment.c)
    dvz_add_example(features pick_point features/pick_point.c)
    dvz_add_example(features pick_hover features/pick_hover.c)
    dvz_add_example(features pick_marker features/pick_marker.c)
    dvz_add_example(features probe_labels features/probe_labels.c)
    dvz_add_example(features selection features/selection.c)
    dvz_add_example(features timer_animation features/timer_animation.c)
    dvz_add_example(features marker_symbols features/marker_symbols.c)

    dvz_add_example(workflows panel_linked_axes workflows/panel_linked_axes.c)
    dvz_add_example(workflows linked_probe_colorbar workflows/linked_probe_colorbar.c)
    dvz_add_example(workflows scalebar_measurement workflows/scalebar_measurement.c)

    dvz_add_example(visuals point visuals/point.c)
    dvz_add_example(visuals pixel visuals/pixel.c)
    dvz_add_example(visuals marker visuals/marker.c)
    dvz_add_example(visuals primitive visuals/primitive.c)
    dvz_add_example(visuals segment visuals/segment.c)
    dvz_add_example(visuals vector visuals/vector.c)
    dvz_add_example(visuals path visuals/path.c)
    dvz_add_example(visuals image visuals/image.c)
    dvz_add_example(visuals mesh visuals/mesh.c)
    dvz_add_example(visuals sphere visuals/sphere.c)
    dvz_add_example(visuals splat visuals/splat.c)
    dvz_add_example(visuals text visuals/text.c)
    dvz_add_example(visuals glyph visuals/glyph.c)
    dvz_add_example(visuals labels visuals/labels.c)
    dvz_add_example(visuals volume visuals/volume.c)

    dvz_add_example(composites polygon composites/polygon.c)
    dvz_add_example(composites graph composites/graph.c)

    dvz_add_example(showcases gpu_particle_smoke showcases/gpu_particle_smoke.c)
    dvz_add_example(showcases brain_volume_mesh showcases/brain_volume_mesh.c)
    dvz_add_example(showcases point_cloud showcases/point_cloud.c)
    dvz_add_example(showcases textured_planet showcases/textured_planet.c)
    dvz_add_example(showcases wind_field showcases/wind_field.c)

    dvz_add_example(scientific protein scientific/protein.c)

    dvz_add_example(lab axis_lattice_smoke lab/axis_lattice_smoke.c)
    dvz_add_example(lab point_stress lab/point_stress.c)
    dvz_add_example(lab protein_viewer lab/protein_viewer.c)
    dvz_add_example(lab record_dvzr lab/record_dvzr.c)
    dvz_add_example(lab replay_dvzr lab/replay_dvzr.c)
    dvz_add_example(lab scalebar_2d_3d lab/scalebar_2d_3d.c)
    dvz_add_example(lab text_msdf_diagnostics lab/text_msdf_diagnostics.c)
    dvz_add_example(lab text_msdf_lab lab/text_msdf_lab.c)
endif()
