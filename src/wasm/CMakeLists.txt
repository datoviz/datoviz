# -------------------------------------------------------------------------------------------------
# WASM scene bridge
# -------------------------------------------------------------------------------------------------

if(NOT EMSCRIPTEN)
    return()
endif()

add_executable(datoviz_wasm_scene scene_api.c)

target_link_libraries(datoviz_wasm_scene PRIVATE datoviz)

target_include_directories(datoviz_wasm_scene
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external/cglm/include
)

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
    "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free','_dvz_wasm_api_scene','_dvz_wasm_api_scene_destroy','_dvz_wasm_api_figure','_dvz_wasm_api_panel_full','_dvz_wasm_api_visual','_dvz_wasm_api_controller','_dvz_wasm_api_panel_add_visual','_dvz_wasm_api_panel_bind_controller','_dvz_wasm_api_panel_set_camera','_dvz_wasm_api_arcball_initial','_dvz_wasm_api_visual_set_f32','_dvz_wasm_api_visual_set_rgba8','_dvz_wasm_api_visual_set_u32','_dvz_wasm_api_visual_set_texture_rgba8','_dvz_wasm_api_set_canvas_format','_dvz_wasm_api_set_capabilities','_dvz_wasm_api_resize','_dvz_wasm_api_pointer','_dvz_wasm_api_wheel','_dvz_wasm_api_emit','_dvz_wasm_api_payload_ptr','_dvz_wasm_api_payload_size','_dvz_wasm_api_emit_packets','_dvz_wasm_api_packet_status','_dvz_wasm_api_packet_ptr','_dvz_wasm_api_packet_size','_dvz_wasm_api_packet_arena_ptr','_dvz_wasm_api_packet_arena_size','_dvz_wasm_api_resource_version','_dvz_wasm_api_frame_index','_dvz_wasm_api_diagnostic_count','_dvz_wasm_api_diagnostic']"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPU8','HEAPF32','HEAPU32']"
)
