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

add_library(datoviz_example_support STATIC
    example_common.c
    example_controller_preview.c
    example_random.c
    example_style.c
    runner/scenario_helpers.c
    runner/scenario_runner.c
)

if(TARGET datoviz_gui)
    target_sources(datoviz_example_support PRIVATE example_gui_controls.c)
endif()

if(TARGET datoviz_app)
    target_sources(datoviz_example_support PRIVATE example_debug.c)
endif()

if(TARGET datoviz_app AND TARGET datoviz_gui)
    target_sources(datoviz_example_support PRIVATE example_tuner.c)
endif()

target_include_directories(datoviz_example_support
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/app
    ${PROJECT_SOURCE_DIR}/src/scene
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/cglm/include
    ${PROJECT_SOURCE_DIR}/external/volk
)

get_property(_dvz_example_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)
if(_dvz_example_compile_definitions)
    target_compile_definitions(
        datoviz_example_support PRIVATE ${_dvz_example_compile_definitions})
endif()

if(TARGET datoviz_vklite)
    target_compile_definitions(datoviz_example_support PRIVATE DVZ_DRP2_HAS_VKLITE=1)
else()
    target_compile_definitions(datoviz_example_support PRIVATE DVZ_DRP2_HAS_VKLITE=0)
endif()

set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS datoviz_example_support)

# Helper: add one grouped C example executable.
#
#   dvz_add_example(group name source)
#
# The CMake target is namespaced as example_c_<group>_<name>, while the generated executable keeps
# the short <name> filename under build/examples/c/<group>/.
function(dvz_add_example group name source)
    set(target "example_c_${group}_${name}")
    add_executable(${target} ${source})

    target_link_libraries(${target}
        PRIVATE
        datoviz_example_support
        datoviz_common
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

    if(APPLE OR UNIX)
        set(_dvz_example_runtime_rpath "@executable_path;@loader_path")
        if(NOT APPLE)
            set(_dvz_example_runtime_rpath "$ORIGIN")
        endif()
        if(DVZ_SHADERC_LIBRARY)
            get_filename_component(_dvz_shaderc_library_dir "${DVZ_SHADERC_LIBRARY}" DIRECTORY)
            list(APPEND _dvz_example_runtime_rpath "${_dvz_shaderc_library_dir}")
        endif()
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH  "${_dvz_example_runtime_rpath}"
            INSTALL_RPATH "${_dvz_example_runtime_rpath}"
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
    dvz_add_example(features basic_scene features/basic_scene.c)
    dvz_add_example(features panel_single features/panel_single.c)
    dvz_add_example(features panel_grid features/panel_grid.c)
    dvz_add_example(features panel_multi features/panel_multi.c)
    dvz_add_example(features panel_linked features/panel_linked.c)
    dvz_add_example(features coordinate_system features/coordinate_system.c)
    dvz_add_example(features axes_2d features/axes_2d.c)
    dvz_add_example(features axis_labels features/axis_labels.c)
    dvz_add_example(features sampled_field_update features/sampled_field_update.c)
    dvz_add_example(features text_block features/text_block.c)
    dvz_add_example(features overlay_card features/overlay_card.c)
    dvz_add_example(features camera_manual features/camera_manual.c)
    dvz_add_example(features controller_arcball features/controller_arcball.c)
    dvz_add_example(features orientation_gizmo features/orientation_gizmo.c)
    dvz_add_example(features reference_grid features/reference_grid.c)
    dvz_add_example(features bounds_overlay features/bounds_overlay.c)
    dvz_add_example(features controller_fly features/controller_fly.c)
    dvz_add_example(features controller_turntable features/controller_turntable.c)
    dvz_add_example(features mesh_texture features/mesh_texture.c)
    dvz_add_example(features material_mesh features/material_mesh.c)
    dvz_add_example(features lighting features/lighting.c)
    dvz_add_example(features user_scale features/user_scale.c)
    dvz_add_example(features gui_controls features/gui_controls.c)
    dvz_add_example(features gui_viewport features/gui_viewport.c)
    dvz_add_example(features gui_cimgui features/gui_cimgui.c)
    dvz_add_example(features animation_tracks features/animation_tracks.c)
    dvz_add_example(features compute_buffer_animation features/compute_buffer_animation.c)
    dvz_add_example(features technique_edl features/technique_edl.c)
    dvz_add_example(features technique_ssao features/technique_ssao.c)
    dvz_add_example(features technique_msaa features/technique_msaa.c)
    dvz_add_example(features technique_depth_cue features/technique_depth_cue.c)
    dvz_add_example(features technique_transparency features/technique_transparency.c)
    dvz_add_example(features volume_occlusion features/volume_occlusion.c)
    dvz_add_example(features guide_lines features/guide_lines.c)
    dvz_add_example(features guide_spans features/guide_spans.c)
    dvz_add_example(features bars_bands features/bars_bands.c)
    dvz_add_example(features view_size_policies features/view_size_policies.c)
    dvz_add_example(features input_events features/input_events.c)
    dvz_add_example(features visual_transform features/visual_transform.c)
    dvz_add_example(features panel_view2d features/panel_view2d.c)
    dvz_add_example(features json_export features/json_export.c)
    dvz_add_example(features bezier_curve_path features/bezier_curve_path.c)
    dvz_add_example(features path_join features/path_join.c)
    dvz_add_example(features isolines features/isolines.c)
    dvz_add_example(features builtin_shapes_2d features/builtin_shapes_2d.c)
    dvz_add_example(features builtin_shapes_3d features/builtin_shapes_3d.c)
    dvz_add_example(features obj_loading features/obj_loading.c)
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
    dvz_add_example(features technique_depth_test features/technique_depth_test.c)
    dvz_add_example(features alpha_blending features/alpha_blending.c)
    dvz_add_example(features panel_background features/panel_background.c)
    dvz_add_example(features panzoom features/panzoom.c)
    dvz_add_example(features picking features/picking.c)
    dvz_add_example(features selection_pixel features/selection_pixel.c)
    dvz_add_example(features selection_sphere features/selection_sphere.c)
    dvz_add_example(features selection_mesh_instances features/selection_mesh_instances.c)
    dvz_add_example(features probe_labels features/probe_labels.c)
    dvz_add_example(features timer_animation features/timer_animation.c)
    dvz_add_example(features marker_symbols features/marker_symbols.c)
    dvz_add_example(features legend_categorical features/legend_categorical.c)
    dvz_add_example(runtime app_glfw runtime/app_glfw.c)
    dvz_add_example(runtime multi_window runtime/multi_window.c)
    dvz_add_example(runtime offscreen_capture runtime/offscreen_capture.c)
    dvz_add_example(runtime record_replay runtime/record_replay.c)
    dvz_add_example(runtime video_export runtime/video_export.c)

    dvz_add_example(visuals point visuals/point.c)
    dvz_add_example(visuals pixel visuals/pixel.c)
    dvz_add_example(visuals marker visuals/marker.c)
    dvz_add_example(visuals primitive visuals/primitive.c)
    dvz_add_example(visuals segment visuals/segment.c)
    dvz_add_example(visuals vector visuals/vector.c)
    dvz_add_example(visuals path visuals/path.c)
    dvz_add_example(visuals image visuals/image.c)
    dvz_add_example(visuals image_rgba visuals/image_rgba.c)
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
    dvz_add_example(showcases brain_volume showcases/brain_volume.c)
    dvz_add_example(showcases embedding_atlas showcases/embedding_atlas.c)
    dvz_add_example(showcases lipid_brain_atlas showcases/lipid_brain_atlas.c)
    dvz_add_example(showcases synthetic_mouse showcases/synthetic_mouse.c)
    dvz_add_example(showcases spherical_harmonics showcases/spherical_harmonics.c)
    dvz_add_example(showcases surface_grid showcases/surface_grid.c)
    dvz_add_example(showcases point_cloud showcases/point_cloud.c)
    dvz_add_example(showcases textured_planet showcases/textured_planet.c)
    dvz_add_example(showcases wind_field showcases/wind_field.c)
    dvz_add_example(showcases panel_linked_axes showcases/panel_linked_axes.c)
    dvz_add_example(showcases linked_probe_colorbar showcases/linked_probe_colorbar.c)
    dvz_add_example(showcases scientific_plotting showcases/scientific_plotting.c)
    dvz_add_example(showcases scalebar_measurement showcases/scalebar_measurement.c)
    dvz_add_example(showcases choropleth showcases/choropleth.c)
    dvz_add_example(showcases protein showcases/protein.c)

    dvz_add_example(advanced raw_triangle_vklite advanced/raw_triangle_vklite.c)
    dvz_add_example(advanced raw_triangle_drp2 advanced/raw_triangle_drp2.c)
    dvz_add_example(advanced external_surface_glfw advanced/external_surface_glfw.c)

    dvz_add_example(start scatter start/scatter.c)

    dvz_add_example(lab axis_lattice_smoke lab/axis_lattice_smoke.c)
    dvz_add_example(lab point_stress lab/point_stress.c)
    dvz_add_example(lab protein_viewer lab/protein_viewer.c)
    dvz_add_example(lab record_dvzr lab/record_dvzr.c)
    dvz_add_example(lab replay_dvzr lab/replay_dvzr.c)
    dvz_add_example(lab scalebar_2d_3d lab/scalebar_2d_3d.c)
    dvz_add_example(lab text_msdf_diagnostics lab/text_msdf_diagnostics.c)
    dvz_add_example(lab text_msdf_lab lab/text_msdf_lab.c)
endif()
