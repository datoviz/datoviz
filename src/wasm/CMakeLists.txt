# -------------------------------------------------------------------------------------------------
# WASM scene bridge
# -------------------------------------------------------------------------------------------------

if(NOT EMSCRIPTEN)
    return()
endif()

add_executable(datoviz_wasm_scene scene_bridge.c)

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
    "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free','_dvz_wasm_scene_create','_dvz_wasm_scene_set_canvas_format','_dvz_wasm_scene_destroy','_dvz_wasm_scene_resize','_dvz_wasm_scene_set_points','_dvz_wasm_scene_set_primitive','_dvz_wasm_scene_emit','_dvz_wasm_scene_payload_ptr','_dvz_wasm_scene_payload_size','_dvz_wasm_scene_diagnostic_count','_dvz_wasm_scene_diagnostic','_dvz_wasm_scene_pointer','_dvz_wasm_scene_wheel']"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPU8','HEAPF32']"
)
