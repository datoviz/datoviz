# -------------------------------------------------------------------------------------------------
# WASM scene bridge
# -------------------------------------------------------------------------------------------------

if(NOT EMSCRIPTEN)
    return()
endif()

add_executable(datoviz_wasm_scene
    scene_api.c
    ${PROJECT_SOURCE_DIR}/examples/c/example_style.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/basic_scene.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/image_probe.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/picking.c
    ${PROJECT_SOURCE_DIR}/examples/c/features/timer_animation.c
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
    ${PROJECT_SOURCE_DIR}/external/cglm/include
)

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
    "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free','_dvz_wasm_api_scene','_dvz_wasm_api_scene_destroy','_dvz_wasm_api_scenario_count','_dvz_wasm_api_scenario_id','_dvz_wasm_api_scenario_title','_dvz_wasm_api_scenario_width','_dvz_wasm_api_scenario_height','_dvz_wasm_api_scenario_fps','_dvz_wasm_api_scenario_requirements','_dvz_wasm_api_scenario_create','_dvz_wasm_api_scenario_figure','_dvz_wasm_api_scenario_frame','_dvz_wasm_api_scenario_post_frame','_dvz_wasm_api_scenario_pointer','_dvz_wasm_api_scenario_wheel','_dvz_wasm_api_figure','_dvz_wasm_api_panel_full','_dvz_wasm_api_visual','_dvz_wasm_api_buffer','_dvz_wasm_api_buffer_set_data','_dvz_wasm_api_controller','_dvz_wasm_api_panel_add_visual','_dvz_wasm_api_panel_bind_controller','_dvz_wasm_api_panel_set_camera','_dvz_wasm_api_panel_set_domain','_dvz_wasm_api_panel_axis','_dvz_wasm_api_axis_set_visible','_dvz_wasm_api_axis_set_grid','_dvz_wasm_api_axis_set_label','_dvz_wasm_api_axis_set_plot_margins','_dvz_wasm_api_arcball_initial','_dvz_wasm_api_visual_set_f32','_dvz_wasm_api_visual_set_rgba8','_dvz_wasm_api_visual_set_u32','_dvz_wasm_api_visual_set_strings','_dvz_wasm_api_visual_set_attr_buffer','_dvz_wasm_api_visual_set_texture_rgba8','_dvz_wasm_api_visual_set_labels_s32','_dvz_wasm_api_visual_set_material','_dvz_wasm_api_visual_set_segment_caps','_dvz_wasm_api_visual_set_path_caps','_dvz_wasm_api_visual_set_path_join','_dvz_wasm_api_set_canvas_format','_dvz_wasm_api_set_capabilities','_dvz_wasm_api_resize','_dvz_wasm_api_pointer','_dvz_wasm_api_wheel','_dvz_wasm_api_emit','_dvz_wasm_api_payload_ptr','_dvz_wasm_api_payload_size','_dvz_wasm_api_emit_packets','_dvz_wasm_api_release_packets','_dvz_wasm_api_query_pending_count','_dvz_wasm_api_query_active','_dvz_wasm_api_query_readback_size','_dvz_wasm_api_emit_query_packets','_dvz_wasm_api_query_resolve','_dvz_wasm_api_packet_status','_dvz_wasm_api_packet_ptr','_dvz_wasm_api_packet_size','_dvz_wasm_api_packet_arena_ptr','_dvz_wasm_api_packet_arena_size','_dvz_wasm_api_resource_version','_dvz_wasm_api_frame_index','_dvz_wasm_api_diagnostic_count','_dvz_wasm_api_diagnostic']"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPU8','HEAPF32','HEAPU32']"
)
