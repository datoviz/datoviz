/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge packet export and diagnostics                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_api_internal.h"



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_set_canvas_format(uint32_t scene_handle, uint32_t color_format)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (
        color_format != DVZ_FORMAT_R8G8B8A8_UNORM &&
        color_format != DVZ_FORMAT_B8G8R8A8_UNORM &&
        color_format != DVZ_FORMAT_R16G16B16A16_SFLOAT)
        return _fail(scene, "unsupported WASM canvas format");
    _clear_payload(scene);
    scene->color_format = color_format;
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_set_capabilities(
    uint32_t scene_handle, uint32_t max_texture_dimension_2d, uint32_t max_bind_groups,
    uint32_t max_vertex_buffers, uint32_t max_buffer_size,
    uint32_t min_texture_copy_bytes_per_row_alignment, uint32_t max_sample_count,
    uint32_t supports_color_blending)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (max_texture_dimension_2d == 0 || max_bind_groups == 0 || max_vertex_buffers == 0 ||
        max_buffer_size == 0 || min_texture_copy_bytes_per_row_alignment == 0 ||
        max_sample_count == 0)
    {
        return _fail(scene, "invalid WASM capability snapshot");
    }
    _clear_payload(scene);
    scene->caps.max_texture_dimension_2d = max_texture_dimension_2d;
    scene->caps.max_bind_groups = max_bind_groups;
    scene->caps.max_vertex_buffers = max_vertex_buffers;
    scene->caps.max_buffer_size = max_buffer_size;
    scene->caps.max_color_sample_count = max_sample_count;
    scene->caps.max_depth_sample_count = max_sample_count;
    scene->caps.min_texture_copy_bytes_per_row_alignment =
        min_texture_copy_bytes_per_row_alignment;
    scene->caps.supports_color_blending = supports_color_blending != 0;
    return 0;
}



static DvzFramePlanEmitConfig _wasm_emit_config(const DvzWasmApiScene* scene)
{
    ANN(scene);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.external_color_target = true;
    emit_cfg.color_target_id = 0;
    emit_cfg.color_target_format = scene->color_format;
    emit_cfg.target_width = scene->width;
    emit_cfg.target_height = scene->height;
    emit_cfg.device_scale_x = scene->device_scale > 0.0f ? scene->device_scale : 1.0f;
    emit_cfg.device_scale_y = scene->device_scale > 0.0f ? scene->device_scale : 1.0f;
    return emit_cfg;
}



static int
_emit_frame_artifact(uint32_t scene_handle, uint32_t figure_handle, const char* failure_message)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
    {
        int ret = _fail(scene, failure_message);
        if (scene != NULL)
            scene->packet_status = -1;
        return ret;
    }
    _clear_payload(scene);

    DvzFramePlanEmitConfig emit_cfg = _wasm_emit_config(scene);
    uint64_t next_frame_index = scene->frame_index + 1;
    uint64_t next_resource_version = scene->resource_version + 1;
    scene->frame_artifact = _scene_emit_frame_artifact(
        figure->figure, &scene->caps, &scene->report, &emit_cfg, next_resource_version,
        next_frame_index);
    if (scene->frame_artifact == NULL)
    {
        if (dvz_diagnostic_report_count(&scene->report) == 0)
            (void)dvz_diagnostic_report_add(&scene->report, "WASM scene frame emission failed");
        scene->packet_status = -1;
        return -1;
    }
    if (dvz_diagnostic_report_count(&scene->report) > 0)
    {
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
        scene->packet_status = -1;
        return -1;
    }

    scene->frame_index = next_frame_index;
    scene->resource_version = next_resource_version;
    if (dvz_scene_frame_artifact_status(scene->frame_artifact) !=
        DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK)
    {
        (void)_fail(scene, "WASM DRP2 packet encoding failed");
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
        scene->packet_status = -2;
        return -1;
    }
    scene->packet_status = 0;
    return 0;
}



static int _emit(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (_emit_frame_artifact(scene_handle, figure_handle, "invalid WASM emit request") != 0)
        return -1;
    scene = _scene(scene_handle);
    ANN(scene);
    scene->json = dvz_scene_frame_artifact_json(scene->frame_artifact, "wasm_api_scene");
    if (scene->json == NULL)
    {
        scene->packet_status = -3;
        return _fail(scene, "WASM frame artifact JSON serialization failed");
    }
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit(scene_handle, figure_handle);
}



static int _emit_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit_frame_artifact(scene_handle, figure_handle, "invalid WASM packet emit request");
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    return _emit_packets(scene_handle, figure_handle);
}


EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_runtime_reset(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL)
        return -1;
    _clear_payload(scene);
    if (!_scene_runtime_emitter_reset(scene->scene))
        return _fail(scene, "WASM runtime emitter reset failed");
    return 0;
}


EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_release_packets(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    if (scene->frame_artifact != NULL)
    {
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
    }
    scene->packet_status = 0;
    return 0;
}
EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_payload_ptr(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->json != NULL ? (uint32_t)(uintptr_t)scene->json : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_payload_size(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->json != NULL ? (uint32_t)strlen(scene->json) : 0;
}



static bool _valid_packet_kind(uint32_t kind)
{
    return kind >= DVZ_DRP2_PACKET_SETUP && kind <= DVZ_DRP2_PACKET_FRAME;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_packet_status(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL)
        return -1;
    return scene->packet_status;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_ptr(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !_valid_packet_kind(kind))
        return 0;
    const void* ptr = NULL;
    (void)dvz_scene_frame_artifact_get_packet(
        scene->frame_artifact, (DvzDrp2PacketKind)kind, &ptr, NULL, NULL, NULL);
    return ptr != NULL ? (uint32_t)(uintptr_t)ptr : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_size(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    uint64_t size = 0;
    if (scene != NULL && _valid_packet_kind(kind))
    {
        (void)dvz_scene_frame_artifact_get_packet(
            scene->frame_artifact, (DvzDrp2PacketKind)kind, NULL, &size, NULL, NULL);
    }
    return size <= UINT32_MAX ? (uint32_t)size : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_arena_ptr(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || !_valid_packet_kind(kind))
        return 0;
    const void* ptr = NULL;
    (void)dvz_scene_frame_artifact_get_packet(
        scene->frame_artifact, (DvzDrp2PacketKind)kind, NULL, NULL, &ptr, NULL);
    return ptr != NULL ? (uint32_t)(uintptr_t)ptr : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_packet_arena_size(uint32_t scene_handle, uint32_t kind)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    uint64_t size = 0;
    if (scene != NULL && _valid_packet_kind(kind))
    {
        (void)dvz_scene_frame_artifact_get_packet(
            scene->frame_artifact, (DvzDrp2PacketKind)kind, NULL, NULL, NULL, &size);
    }
    return size <= UINT32_MAX ? (uint32_t)size : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_resource_version(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene != NULL && scene->frame_artifact != NULL)
    {
        uint64_t resource_version =
            dvz_scene_frame_artifact_resource_version(scene->frame_artifact);
        return resource_version <= UINT32_MAX ? (uint32_t)resource_version : 0;
    }
    return scene != NULL && scene->resource_version <= UINT32_MAX
               ? (uint32_t)scene->resource_version
               : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_frame_index(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene != NULL && scene->frame_artifact != NULL)
    {
        uint64_t frame_index = dvz_scene_frame_artifact_frame_index(scene->frame_artifact);
        return frame_index <= UINT32_MAX ? (uint32_t)frame_index : 0;
    }
    return scene != NULL && scene->frame_index <= UINT32_MAX ? (uint32_t)scene->frame_index : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_diagnostic_count(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL ? dvz_diagnostic_report_count(&scene->report) : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_diagnostic(uint32_t scene_handle, uint32_t index)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    const char* diagnostic =
        scene != NULL ? dvz_diagnostic_report_get(&scene->report, index) : NULL;
    return diagnostic != NULL ? (uint32_t)(uintptr_t)diagnostic : 0;
}
