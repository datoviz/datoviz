/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge shared helpers                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_api_internal.h"



DvzCapabilitySnapshot _wasm_capability_snapshot(void)
{
    DvzCapabilitySnapshot caps = {0};
    caps.struct_size = DVZ_STRUCT_SIZE(DvzCapabilitySnapshot);
    caps.flags = 0;
    caps.max_buffer_size = 256 * 1024 * 1024;
    caps.max_texture_dimension_2d = 4096;
    caps.max_bind_groups = 4;
    caps.max_vertex_buffers = 8;
    caps.max_color_attachments = 1;
    caps.max_color_sample_count = 16;
    caps.max_depth_sample_count = 16;
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    caps.supports_readback = true;
    caps.min_texture_copy_bytes_per_row_alignment = 4;
    caps.max_readback_size = caps.max_buffer_size;
    caps.texture_format_r32uint = true;
    caps.texture_format_rg32uint = true;
    caps.render_target_format_r32uint = true;
    caps.render_target_format_rg32uint = true;
    caps.query_profile_u32_r32 = true;
    caps.query_profile_u64_rg32 = true;
    caps.query_profile_u64_2xr32 = true;
    return caps;
}
void _clear_query(DvzWasmApiScene* scene)
{
    if (scene == NULL)
        return;
    if (scene->query_active)
        _scene_query_scratch_destroy(&scene->query_plan.scratch);
    if (scene->frame_artifact != NULL)
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
    scene->frame_artifact = NULL;
    scene->packet_status = 0;
    scene->query_pending = (DvzPendingQueryRequest){0};
    scene->query_build = (DvzSceneQueryBuildContext){0};
    scene->query_plan = (DvzSceneQueryPlan){0};
    scene->query_result = (DvzQueryResult){0};
    scene->query_ops = NULL;
    scene->query_visual_type = DVZ_VISUAL_TYPE_NONE;
    scene->query_family = DVZ_SCENE_VISUAL_FAMILY_NONE;
    scene->query_panel = NULL;
    scene->query_active = false;
}
void _clear_payload(DvzWasmApiScene* scene)
{
    if (scene == NULL)
        return;
    if (scene->json != NULL)
    {
        dvz_drp2_stream_json_destroy(scene->json);
        scene->json = NULL;
    }
    if (scene->frame_artifact != NULL)
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
    scene->frame_artifact = NULL;
    scene->packet_status = 0;
    dvz_diagnostic_report_init(&scene->report);
}
bool _ensure_query_emitter(DvzScene* owner)
{
    ANN(owner);
    DvzSceneRequestExecutor* executor = &owner->query_executor;
    if (executor->emitter != NULL)
        return true;

    executor->emitter = dvz_frame_plan_emitter();
    if (executor->emitter == NULL)
        return false;
    executor->emitter->resources.next_id = DVZ_WASM_QUERY_RESOURCE_ID_BASE;
    executor->emitter->objects.next_id = DVZ_WASM_QUERY_OBJECT_ID_BASE;
    executor->emitter->next_transient_id = DVZ_WASM_QUERY_TRANSIENT_ID_BASE;
    executor->emitter_create_count++;
    return true;
}
int _fail(DvzWasmApiScene* scene, const char* diagnostic)
{
    if (scene != NULL)
    {
        _clear_payload(scene);
        if (diagnostic != NULL)
            (void)dvz_diagnostic_report_add(&scene->report, diagnostic);
    }
    return -1;
}



uint32_t _fail_handle(DvzWasmApiScene* scene, const char* diagnostic)
{
    (void)_fail(scene, diagnostic);
    return 0;
}



int _fail_upload(DvzWasmApiScene* scene, const char* kind, const char* attr, uint32_t item_count)
{
    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
    int ret = snprintf(
        diagnostic, sizeof(diagnostic), "WASM %s visual upload failed: attr=%s item_count=%u",
        kind != NULL ? kind : "data", attr != NULL ? attr : "<null>", item_count);
    if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
        return _fail(scene, "WASM visual upload failed");
    return _fail(scene, diagnostic);
}



bool _remember(DvzWasmApiScene* scene, void* wrapper)
{
    if (scene == NULL || wrapper == NULL || scene->wrapper_count >= DVZ_WASM_API_MAX_WRAPPERS)
        return false;
    scene->wrappers[scene->wrapper_count++] = wrapper;
    return true;
}
void _emit_resize(
    DvzWasmApiScene* scene, uint32_t logical_width, uint32_t logical_height,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float device_scale)
{
    if (scene == NULL || scene->router == NULL)
        return;

    DvzInputResizeEvent resize = {
        .framebuffer_width = framebuffer_width,
        .framebuffer_height = framebuffer_height,
        .window_width = logical_width,
        .window_height = logical_height,
        .content_scale_x = device_scale > 0.0f ? device_scale : 1.0f,
        .content_scale_y = device_scale > 0.0f ? device_scale : 1.0f,
    };
    dvz_input_emit_resize(scene->router, &resize);
}
