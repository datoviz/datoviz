# -------------------------------------------------------------------------------------------------
# WASM scene bridge
# -------------------------------------------------------------------------------------------------

if(NOT EMSCRIPTEN)
    return()
endif()

add_executable(datoviz_wasm_scene
    scene_api.c
    scene_api_common.c
    scene_api_input.c
    scene_api_packets.c
    scene_api_query.c
    scene_api_scenario.c
    scene_api_wrappers.c
    ${PROJECT_SOURCE_DIR}/examples/c/runner/scenario_helpers.c
    ${PROJECT_SOURCE_DIR}/examples/c/example_common.c
    ${PROJECT_SOURCE_DIR}/examples/c/example_controller_preview.c
    ${PROJECT_SOURCE_DIR}/examples/c/example_random.c
    ${PROJECT_SOURCE_DIR}/examples/c/example_style.c
    ${PROJECT_SOURCE_DIR}/examples/c/composites/polygon.c
    ${PROJECT_SOURCE_DIR}/examples/c/composites/graph.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/annotation_readout.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/animation_tracks.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/alpha_blending.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/axes_2d.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/axis_labels.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/bezier_curve_path.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/bars_bands.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/basic_scene.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/builtin_shapes_2d.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/builtin_shapes_3d.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/bounds_overlay.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/camera_manual.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/colorbar.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/colormap_scale.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/compute_buffer_animation.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/coordinate_system.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/controller_arcball.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/controller_fly.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/controller_turntable.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/guide_lines.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/guide_spans.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/image_probe.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/isolines.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/legend_categorical.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/lighting.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/material_mesh.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/mesh_texture.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/obj_loading.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/orientation_gizmo.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/overlay_card.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panel_background.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panel_grid.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panel_linked.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panel_multi.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panel_single.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panel_view2d.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/panzoom.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/path_join.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/picking.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/reference_grid.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/selection_mesh_instances.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/selection_pixel.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/selection_sphere.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/scalebar.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/scalebar_units.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/sampled_field_update.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/text_block.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/technique_depth_test.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/timer_animation.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/update_partial.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/update_visual_data.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/visibility.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/visual_transform.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/linked_probe_colorbar.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/gpu_particle_smoke.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/panel_linked_axes.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/protein.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/scalebar_measurement.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/scientific_plotting.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/surface_grid.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/choropleth.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/textured_planet.c
    ${PROJECT_SOURCE_DIR}/examples/c/showcases/wind_field.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/glyph.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/image.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/image_rgba.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/labels.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/marker.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/mesh.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/path.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/pixel.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/point.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/primitive.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/segment.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/sphere.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/text.c
    ${PROJECT_SOURCE_DIR}/examples/c/visuals/vector.c
)

target_link_libraries(datoviz_wasm_scene PRIVATE datoviz)

target_include_directories(datoviz_wasm_scene
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/scene
    ${PROJECT_SOURCE_DIR}/src/scene/core
    ${PROJECT_SOURCE_DIR}/src/scene/visuals
    ${PROJECT_SOURCE_DIR}/examples/c
)

target_link_libraries(datoviz_wasm_scene PRIVATE dvz_cglm)

target_compile_definitions(datoviz_wasm_scene PRIVATE DVZ_EXAMPLE_NO_APP=1 DVZ_EXAMPLE_NO_MAIN=1)

set_target_properties(datoviz_wasm_scene PROPERTIES
    OUTPUT_NAME "datoviz_wasm_scene"
    SUFFIX ".mjs"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/wasm"
)

target_link_options(datoviz_wasm_scene PRIVATE
    "SHELL:-sMODULARIZE=1"
    "SHELL:-sEXPORT_ES6=1"
    "SHELL:-sENVIRONMENT=web,worker,node"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sSTACK_SIZE=1048576"
    "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free','_dvz_wasm_api_scene','_dvz_wasm_api_scene_destroy','_dvz_wasm_api_scenario_count','_dvz_wasm_api_scenario_id','_dvz_wasm_api_scenario_title','_dvz_wasm_api_scenario_width','_dvz_wasm_api_scenario_height','_dvz_wasm_api_scenario_fps','_dvz_wasm_api_scenario_requirements','_dvz_wasm_api_scenario_create','_dvz_wasm_api_scenario_figure','_dvz_wasm_api_scenario_frame','_dvz_wasm_api_scenario_post_frame','_dvz_wasm_api_scenario_pointer','_dvz_wasm_api_scenario_wheel','_dvz_wasm_api_figure','_dvz_wasm_api_panel_full','_dvz_wasm_api_visual','_dvz_wasm_api_buffer','_dvz_wasm_api_buffer_set_data','_dvz_wasm_api_controller','_dvz_wasm_api_panel_add_visual','_dvz_wasm_api_panel_bind_controller','_dvz_wasm_api_panel_set_camera','_dvz_wasm_api_panel_set_domain','_dvz_wasm_api_panel_axis','_dvz_wasm_api_axis_set_visible','_dvz_wasm_api_axis_set_grid','_dvz_wasm_api_axis_set_label','_dvz_wasm_api_axis_set_plot_margins','_dvz_wasm_api_arcball_initial','_dvz_wasm_api_visual_set_f32','_dvz_wasm_api_visual_set_rgba8','_dvz_wasm_api_visual_set_u32','_dvz_wasm_api_visual_set_strings','_dvz_wasm_api_visual_set_attr_buffer','_dvz_wasm_api_visual_set_texture_rgba8','_dvz_wasm_api_visual_set_labels_s32','_dvz_wasm_api_visual_set_material','_dvz_wasm_api_visual_set_segment_caps','_dvz_wasm_api_visual_set_path_caps','_dvz_wasm_api_visual_set_path_join','_dvz_wasm_api_set_canvas_format','_dvz_wasm_api_set_capabilities','_dvz_wasm_api_resize','_dvz_wasm_api_pointer','_dvz_wasm_api_wheel','_dvz_wasm_api_emit','_dvz_wasm_api_payload_ptr','_dvz_wasm_api_payload_size','_dvz_wasm_api_emit_packets','_dvz_wasm_api_runtime_reset','_dvz_wasm_api_release_packets','_dvz_wasm_api_query_pending_count','_dvz_wasm_api_query_active','_dvz_wasm_api_query_readback_size','_dvz_wasm_api_emit_query_packets','_dvz_wasm_api_query_resolve','_dvz_wasm_api_packet_status','_dvz_wasm_api_packet_ptr','_dvz_wasm_api_packet_size','_dvz_wasm_api_packet_arena_ptr','_dvz_wasm_api_packet_arena_size','_dvz_wasm_api_resource_version','_dvz_wasm_api_frame_index','_dvz_wasm_api_diagnostic_count','_dvz_wasm_api_diagnostic']"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPU8','HEAPF32','HEAPU32']"
)

set(DVZ_WASM_CHOROPLETH_BUNDLE "")
if(EXISTS "${PROJECT_SOURCE_DIR}/data/examples/us_state_choropleth/prepared/metadata.tsv")
    set(DVZ_WASM_CHOROPLETH_BUNDLE
        "${PROJECT_SOURCE_DIR}/data/examples/us_state_choropleth/prepared")
elseif(EXISTS "${PROJECT_SOURCE_DIR}/.cache/datoviz/examples/us_state_choropleth/prepared/metadata.tsv")
    set(DVZ_WASM_CHOROPLETH_BUNDLE
        "${PROJECT_SOURCE_DIR}/.cache/datoviz/examples/us_state_choropleth/prepared")
endif()

if(NOT DVZ_WASM_CHOROPLETH_BUNDLE STREQUAL "")
    target_link_options(datoviz_wasm_scene PRIVATE
        "SHELL:--preload-file ${DVZ_WASM_CHOROPLETH_BUNDLE}@data/examples/us_state_choropleth/prepared"
    )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/data/assets/textures")
    target_link_options(datoviz_wasm_scene PRIVATE
        "SHELL:--preload-file ${PROJECT_SOURCE_DIR}/data/assets/textures@data/assets/textures"
    )
endif()

set(DVZ_WASM_PROTEIN_6M0J_DIR "${PROJECT_SOURCE_DIR}/data/examples/proteins/6m0j/prepared")
if(EXISTS "${DVZ_WASM_PROTEIN_6M0J_DIR}/atom_position.f32" AND
   EXISTS "${DVZ_WASM_PROTEIN_6M0J_DIR}/atom_radius_vdw.f32" AND
   EXISTS "${DVZ_WASM_PROTEIN_6M0J_DIR}/atom_color_element.rgba8")
    target_link_options(datoviz_wasm_scene PRIVATE
        "SHELL:--preload-file ${DVZ_WASM_PROTEIN_6M0J_DIR}/atom_position.f32@data/examples/proteins/6m0j/prepared/atom_position.f32"
        "SHELL:--preload-file ${DVZ_WASM_PROTEIN_6M0J_DIR}/atom_radius_vdw.f32@data/examples/proteins/6m0j/prepared/atom_radius_vdw.f32"
        "SHELL:--preload-file ${DVZ_WASM_PROTEIN_6M0J_DIR}/atom_color_element.rgba8@data/examples/proteins/6m0j/prepared/atom_color_element.rgba8"
    )
elseif(EXISTS "${PROJECT_SOURCE_DIR}/data/examples/proteins/1ubq/prepared")
    set(DVZ_WASM_PROTEIN_1UBQ_DIR "${PROJECT_SOURCE_DIR}/data/examples/proteins/1ubq/prepared")
    target_link_options(datoviz_wasm_scene PRIVATE
        "SHELL:--preload-file ${DVZ_WASM_PROTEIN_1UBQ_DIR}/atom_position.f32@data/examples/proteins/1ubq/prepared/atom_position.f32"
        "SHELL:--preload-file ${DVZ_WASM_PROTEIN_1UBQ_DIR}/atom_radius_vdw.f32@data/examples/proteins/1ubq/prepared/atom_radius_vdw.f32"
        "SHELL:--preload-file ${DVZ_WASM_PROTEIN_1UBQ_DIR}/atom_color_element.rgba8@data/examples/proteins/1ubq/prepared/atom_color_element.rgba8"
    )
endif()
