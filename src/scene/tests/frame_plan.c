/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan tests                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_scene_resource_key.h"
#include "_scene.h"
#include "_visual_pipeline.h"
#include "core/frame_trace_internal.h"
#include "_technique.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/

static bool _test_frame_plan_upload_texture_2d(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset, uint64_t byte_size,
    const char* data_tag, const void* data, uint32_t width, uint32_t height,
    uint32_t alloc_width, uint32_t alloc_height, uint32_t origin_x, uint32_t origin_y)
{
    DvzFramePlanUploadDesc upload = dvz_frame_plan_upload_desc();
    upload.resource_id = resource_id;
    upload.byte_offset = byte_offset;
    upload.byte_size = byte_size;
    upload.data_tag = data_tag;
    upload.data = data;
    upload.texture_width = width;
    upload.texture_height = height;
    upload.texture_depth = 1;
    upload.texture_alloc_width = alloc_width;
    upload.texture_alloc_height = alloc_height;
    upload.texture_alloc_depth = alloc_width != 0 || alloc_height != 0 ? 1 : 0;
    upload.texture_origin_x = origin_x;
    upload.texture_origin_y = origin_y;
    return dvz_frame_plan_upload_ex(plan, &upload);
}


int test_scene_capabilities_diagnostics(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzCapabilitySnapshot copy = dvz_capability_snapshot();
    AT(caps.struct_size == DVZ_STRUCT_SIZE(DvzCapabilitySnapshot));
    AT(caps.flags == 0);
    AT(caps.max_buffer_size > 0);
    AT(caps.max_texture_dimension_2d > 0);
    AT(caps.supports_readback);
    AT(caps.max_readback_size == caps.max_buffer_size);
    AT(caps.min_texture_copy_bytes_per_row_alignment > 0);
    AT(caps.render_target_format_r32uint);
    AT(caps.render_target_format_rg32uint);
    AT(caps.query_profile_u32_r32);
    AT(caps.query_profile_u64_rg32);
    AT(caps.query_profile_u64_2xr32);

    dvz_capability_snapshot_copy(&copy, &caps);
    AT(copy.max_buffer_size == caps.max_buffer_size);
    AT(copy.max_vertex_buffers == caps.max_vertex_buffers);
    AT(copy.query_profile_u64_rg32 == caps.query_profile_u64_rg32);

    DvzCapabilitySnapshot invalid = dvz_capability_snapshot();
    invalid.struct_size = 0;
    copy.max_buffer_size = 123;
    AT_EXPECTED_ERROR_STRICT(
        suite,
        (dvz_capability_snapshot_copy(&copy, &invalid), copy.max_buffer_size == 123));

    invalid = dvz_capability_snapshot();
    invalid.flags = 0x01u;
    AT_EXPECTED_ERROR_STRICT(suite, !dvz_capability_snapshot_valid(&invalid));

    DvzScene* scene = dvz_scene();
    ANN(scene);
    caps.max_texture_dimension_2d = 2048;
    AT(dvz_scene_set_capabilities(scene, &caps) == DVZ_OK);
    AT(scene->caps.max_texture_dimension_2d == 2048);
    invalid = dvz_capability_snapshot();
    invalid.max_texture_dimension_2d = 8192;
    invalid.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_scene_set_capabilities(scene, &invalid) == DVZ_ERROR &&
            scene->caps.max_texture_dimension_2d == 2048);
    dvz_scene_destroy(scene);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_diagnostic_report_add(&report, "unsupported visual family"));
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "unsupported visual family") == 0);
    AT(dvz_diagnostic_report_get(&report, 1) == NULL);
    return 0;
}


/*************************************************************************************************/
/*  Tests                                                                                        */
int test_frame_plan_static_render(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.main", 7);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 48, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    AT(dvz_frame_plan_node_count(plan) == 2);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 0)) == DVZ_FRAME_PLAN_NODE_UPLOAD);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 1)) == DVZ_FRAME_PLAN_NODE_RENDER);
    AT(dvz_frame_plan_node_get(plan, 2) == NULL);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"frame_plan_schema\": \"0.2\"") != NULL);
    AT(strstr(json, "\"figure_id\": \"figure.main\"") != NULL);
    AT(strstr(json, "\"frame_index\": 7") != NULL);
    AT(strstr(json, "\"type\": \"upload\"") != NULL);
    AT(strstr(json, "\"resource_id\": \"buf.point.position\"") != NULL);
    AT(strstr(json, "\"type\": \"render\"") != NULL);
    AT(strstr(json, "\"visuals\": [\"visual.point.0\"]") != NULL);
    AT(strstr(json, "\"picking\": false") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_render_pass_roles(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.wboit", 2);
    ANN(plan);

    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.0", "target.panel.0.color", false, (DvzPanelDesc){0, 0, 1, 1},
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE));
    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.0", "target.panel.0.wboit_accum", false, (DvzPanelDesc){0, 0, 1, 1},
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION));
    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.0", "target.panel.0.color", false, (DvzPanelDesc){0, 0, 1, 1},
        DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE));

    const DvzFramePlanNode* opaque = dvz_frame_plan_node_get(plan, 0);
    const DvzFramePlanNode* accum = dvz_frame_plan_node_get(plan, 1);
    const DvzFramePlanNode* resolve = dvz_frame_plan_node_get(plan, 2);
    AT(_frame_plan_render_pass_role(opaque) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(
        _frame_plan_render_pass_role(accum) ==
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(_frame_plan_render_pass_role(resolve) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"pass_role\": \"opaque\"") != NULL);
    AT(strstr(json, "\"pass_role\": \"transparent_accumulation\"") != NULL);
    AT(strstr(json, "\"pass_role\": \"wboit_resolve\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_clear(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.clear", 3);
    ANN(plan);

    AT(dvz_frame_plan_clear(plan, "panel.empty", "target.clear"));
    AT(dvz_frame_plan_node_count(plan) == 1);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 0)) == DVZ_FRAME_PLAN_NODE_CLEAR);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"type\": \"clear\"") != NULL);
    AT(strstr(json, "\"panel_id\": \"panel.empty\"") != NULL);
    AT(strstr(json, "\"render_target_id\": \"target.clear\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_growth_json(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.growth", 9);
    ANN(plan);

    char resource_id[DVZ_SCENE_LABEL_SIZE] = {0};
    char data_tag[DVZ_SCENE_LABEL_SIZE] = {0};
    for (uint32_t i = 0; i < 80; i++)
    {
        dvz_snprintf(resource_id, sizeof(resource_id), "buf.growth.%03" PRIu32, i);
        dvz_snprintf(data_tag, sizeof(data_tag), "growth.payload.%03" PRIu32, i);
        AT(dvz_frame_plan_upload(plan, resource_id, i * 16, 16, data_tag));
    }

    AT(dvz_frame_plan_node_count(plan) == 80);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 79)) ==
       DVZ_FRAME_PLAN_NODE_UPLOAD);
    AT(dvz_frame_plan_node_get(plan, 80) == NULL);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"figure_id\": \"figure.growth\"") != NULL);
    AT(strstr(json, "\"resource_id\": \"buf.growth.079\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_render_visual_growth(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    AT(sizeof(DvzFramePlanNode) <= 16 * 1024);

    DvzFramePlan* plan = dvz_frame_plan("figure.visual-growth", 10);
    ANN(plan);
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));

    char visual_id[DVZ_SCENE_LABEL_SIZE] = {0};
    for (uint32_t i = 0; i < 9; i++)
    {
        dvz_snprintf(visual_id, sizeof(visual_id), "visual.growth.%02" PRIu32, i);
        AT(dvz_frame_plan_render_visual(plan, visual_id));

        DvzFramePlanVisualMeta metadata = {0};
        metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
        metadata.item_range_first = i;
        metadata.item_range_count = 100 + i;
        AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    }
    AT(dvz_frame_plan_render_metadata_complete(plan));

    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == 9);
    AT(render->u.render.visual_capacity >= 9);
    AT(render->u.render.visual_capacity <= DVZ_SCENE_MAX_RENDER_VISUALS);
    for (uint32_t i = 0; i < 9; i++)
    {
        dvz_snprintf(visual_id, sizeof(visual_id), "visual.growth.%02" PRIu32, i);
        AT(strcmp(render->u.render.visuals[i], visual_id) == 0);
        AT(render->u.render.visual_metadata[i].has_metadata);
        AT(render->u.render.visual_metadata[i].item_range_first == i);
        AT(render->u.render.visual_metadata[i].item_range_count == 100 + i);
    }

    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_render_visual_capacity_limit(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.visual-limit", 11);
    ANN(plan);
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));

    char visual_id[DVZ_SCENE_LABEL_SIZE] = {0};
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_RENDER_VISUALS; i++)
    {
        dvz_snprintf(visual_id, sizeof(visual_id), "visual.limit.%03" PRIu32, i);
        AT(dvz_frame_plan_render_visual(plan, visual_id));
        DvzFramePlanVisualMeta metadata = {.visual_index = i};
        AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    }

    const DvzFramePlanNode* render = dvz_frame_plan_node_get(plan, 0);
    ANN(render);
    AT(render->u.render.visual_count == DVZ_SCENE_MAX_RENDER_VISUALS);
    AT(render->u.render.visual_capacity == DVZ_SCENE_MAX_RENDER_VISUALS);
    AT(dvz_frame_plan_render_metadata_complete(plan));
    AT(strcmp(render->u.render.visuals[0], "visual.limit.000") == 0);
    AT(render->u.render.visual_metadata[0].visual_index == 0);
    AT(render->u.render.visual_metadata[DVZ_SCENE_MAX_RENDER_VISUALS - 1].visual_index ==
       DVZ_SCENE_MAX_RENDER_VISUALS - 1);

    AT(!dvz_frame_plan_render_visual(plan, "visual.limit.overflow"));
    AT(render->u.render.visual_count == DVZ_SCENE_MAX_RENDER_VISUALS);
    AT(render->u.render.visual_capacity == DVZ_SCENE_MAX_RENDER_VISUALS);

    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_json_escapes_labels(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("fig\"escape\\test", 1);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf\"x\\y", 0, 16, "tag\nline"));
    AT(dvz_frame_plan_compute(plan, "shader\tkey", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(plan, "read\"id"));
    AT(dvz_frame_plan_compute_write(plan, "write\\id"));

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"figure_id\": \"fig\\\"escape\\\\test\"") != NULL);
    AT(strstr(json, "\"resource_id\": \"buf\\\"x\\\\y\"") != NULL);
    AT(strstr(json, "\"data_tag\": \"tag\\nline\"") != NULL);
    AT(strstr(json, "\"shader_key\": \"shader\\tkey\"") != NULL);
    AT(strstr(json, "\"read\\\"id\"") != NULL);
    AT(strstr(json, "\"write\\\\id\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure scene resource key helpers preserve the current string conventions.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_resource_keys(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    char key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(_scene_resource_key_visual(3, key, sizeof(key)));
    AT(strcmp(key, "v3") == 0);
    AT(_scene_resource_key_buffer(7, key, sizeof(key)));
    AT(strcmp(key, "b7") == 0);
    AT(_scene_resource_key_visual_attr(3, "position", key, sizeof(key)));
    AT(strcmp(key, "v3_position") == 0);
    AT(_scene_resource_key_visual_texture(3, key, sizeof(key)));
    AT(strcmp(key, "v3_texture") == 0);
    AT(_scene_resource_key_visual_indexed(3, 7, key, sizeof(key)));
    AT(strcmp(key, "v3#index=b7") == 0);
    char indexed_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(_scene_resource_key_visual_indexed(3, 7, indexed_key, sizeof(indexed_key)));
    AT(_scene_resource_key_panel_graph("figure_0_p1", "scene_occlusion.depth", key, sizeof(key)));
    AT(strcmp(key, "figure_0_p1.scene_occlusion.depth") == 0);
    AT(_scene_resource_id_has_suffix(key, ".scene_occlusion.depth"));
    AT(!_scene_resource_id_has_suffix(key, ".volume_occlusion.depth"));
    AT(_scene_resource_id_has_depth_marker(key));

    char visual_id[DVZ_SCENE_LABEL_SIZE] = {0};
    char index_id[DVZ_SCENE_LABEL_SIZE] = {0};
    _scene_resource_key_split_visual(
        indexed_key, visual_id, sizeof(visual_id), index_id, sizeof(index_id));
    AT(strcmp(visual_id, "v3") == 0);
    AT(strcmp(index_id, "b7") == 0);

    char tiny[4] = {0};
    AT(!_scene_resource_key_visual_indexed(123, 456, tiny, sizeof(tiny)));
    AT(tiny[0] == '\0');
    AT(!_scene_resource_key_panel_graph("figure_0_p1", "scene_occlusion.depth", tiny, sizeof(tiny)));
    AT(tiny[0] == '\0');
    return 0;
}


/**
 * Ensure typed FramePlan metadata, not visual-id parsing, drives retained visual emission.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_visual_metadata(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata", 1);
    ANN(plan);

    DvzFramePlanUploadMeta upload_meta = {0};
    upload_meta.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
    upload_meta.visual_type = DVZ_VISUAL_TYPE_POINT;
    upload_meta.visual_index = 0;
    upload_meta.buffer_index = UINT32_MAX;

    AT(dvz_frame_plan_upload(plan, "opaque-position", 0, 3 * 3 * sizeof(float), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_upload(plan, "opaque-color", 0, 3 * sizeof(DvzColor), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_upload(plan, "opaque-size", 0, 3 * sizeof(float), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "opaque-debug-id"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "opaque-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "opaque-color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "opaque-size", sizeof(metadata.size_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream) > 0);
    uint64_t pos_id = _resource_lookup_id(&emitter->resources, "opaque-position");
    AT(pos_id != 0);
    AT(_resource_role(&emitter->resources, pos_id) == DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure WGSL fallback point emission uses typed metadata labels, not upload order.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_visual_metadata_wgsl_uses_typed_labels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.wgsl.labels", 1);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "typed-color", 0, 3 * sizeof(DvzColor), ""));
    AT(dvz_frame_plan_upload(plan, "typed-size", 0, 3 * sizeof(float), ""));
    AT(dvz_frame_plan_upload(plan, "typed-position", 0, 3 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "typed-point"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "typed-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "typed-color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "typed-size", sizeof(metadata.size_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_count == 3);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 6);
            AT(command->u.draw.instance_count == 3);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure WGSL fallback splat emission uses typed metadata labels, not upload roles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_splat_metadata_wgsl_uses_typed_labels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.wgsl.splat.labels", 1);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "typed-splat-color", 0, 3 * sizeof(DvzColor), ""));
    AT(dvz_frame_plan_upload(plan, "typed-splat-angle", 0, 3 * sizeof(float), ""));
    AT(dvz_frame_plan_upload(plan, "typed-splat-position", 0, 3 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_upload(plan, "typed-splat-sigma", 0, 3 * 2 * sizeof(float), ""));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "typed-splat"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_SPLAT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "typed-splat-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "typed-splat-color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.sigma_id, "typed-splat-sigma", sizeof(metadata.sigma_id));
    dvz_strlcpy(metadata.angle_id, "typed-splat-angle", sizeof(metadata.angle_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            if (command->u.create_render_pipeline.binding_count != 4)
                continue;
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_step_modes[0] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[1] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[2] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
            AT(command->u.create_render_pipeline.binding_step_modes[3] ==
               DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 6);
            AT(command->u.draw.instance_count == 3);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure WGSL fallback primitive emission uses typed metadata labels, not upload roles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_primitive_metadata_wgsl_uses_typed_labels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.wgsl.primitive.labels", 1);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "typed-primitive-color", 0, 3 * sizeof(DvzColor), ""));
    AT(dvz_frame_plan_upload(plan, "typed-primitive-position", 0, 3 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "typed-primitive"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_PRIMITIVE;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    dvz_strlcpy(metadata.position_id, "typed-primitive-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "typed-primitive-color", sizeof(metadata.color_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            AT(command->u.create_render_pipeline.binding_count == 2);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 3);
            AT(command->u.draw.instance_count == 1);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure WGSL fallback image emission uses typed metadata labels, not upload roles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_image_metadata_wgsl_uses_typed_labels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.wgsl.image.labels", 1);
    ANN(plan);

    static const uint8_t pixels[2 * 2 * 4] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 255, 255,
    };

    AT(dvz_frame_plan_upload(plan, "typed-image-uv", 0, 4 * 2 * sizeof(float), ""));
    AT(_test_frame_plan_upload_texture_2d(
        plan, "typed-image-texture", 0, sizeof(pixels), "", pixels, 2, 2, 0, 0, 0, 0));
    AT(dvz_frame_plan_upload(plan, "typed-image-position", 0, 4 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "typed-img-quad"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_IMAGE;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "typed-image-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.texcoords_id, "typed-image-uv", sizeof(metadata.texcoords_id));
    dvz_strlcpy(metadata.texture_id, "typed-image-texture", sizeof(metadata.texture_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            if (command->u.create_render_pipeline.binding_count != 2)
                continue;
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology ==
               DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 4);
            AT(command->u.draw.instance_count == 1);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure WGSL fallback textured mesh emission uses typed metadata labels, not upload roles.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_textured_mesh_metadata_wgsl_uses_typed_labels(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.wgsl.textured_mesh.labels", 1);
    ANN(plan);

    static const uint8_t pixels[2 * 2 * 4] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 255, 255,
    };

    AT(_test_frame_plan_upload_texture_2d(
        plan, "typed-mesh-texture", 0, sizeof(pixels), "", pixels, 2, 2, 0, 0, 0, 0));
    AT(dvz_frame_plan_upload(plan, "typed-mesh-normal", 0, 3 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_upload(plan, "typed-mesh-color", 0, 3 * sizeof(DvzColor), ""));
    AT(dvz_frame_plan_upload(plan, "typed-mesh-uv", 0, 3 * 2 * sizeof(float), ""));
    AT(dvz_frame_plan_upload(plan, "typed-mesh-position", 0, 3 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_upload(plan, "typed-mesh-material", 0, sizeof(DvzSceneMaterialParams), ""));
    plan->nodes[plan->count - 1].u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                                          DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                                          DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "typed-mesh"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_MESH;
    metadata.desc_kind = DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    dvz_strlcpy(metadata.position_id, "typed-mesh-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "typed-mesh-color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.normal_id, "typed-mesh-normal", sizeof(metadata.normal_id));
    dvz_strlcpy(metadata.texcoords_id, "typed-mesh-uv", sizeof(metadata.texcoords_id));
    dvz_strlcpy(metadata.texture_id, "typed-mesh-texture", sizeof(metadata.texture_id));
    dvz_strlcpy(metadata.material_id, "typed-mesh-material", sizeof(metadata.material_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    caps = dvz_capability_snapshot();
    caps.shader_format_wgsl = true;
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    bool found_pipeline = false;
    bool found_draw = false;
    const uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            if (command->u.create_render_pipeline.binding_count != 4)
                continue;
            found_pipeline = true;
            AT(command->u.create_render_pipeline.topology == DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        }
        else if (command->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
            AT(command->u.draw.vertex_count == 3);
            AT(command->u.draw.instance_count == 1);
        }
    }
    AT(found_pipeline);
    AT(found_draw);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure runtime lowering does not infer unresolved graph work from legacy pass roles or labels.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_frame_plan_runtime_ignores_unresolved_graph_work(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.runtime_order", 2);
    ANN(plan);

    DvzFramePlanUploadMeta upload_meta = {0};
    upload_meta.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
    upload_meta.visual_type = DVZ_VISUAL_TYPE_POINT;
    upload_meta.visual_index = 0;
    upload_meta.buffer_index = UINT32_MAX;

    AT(dvz_frame_plan_upload(plan, "point-position", 0, 3 * 3 * sizeof(float), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_upload(plan, "point-color", 0, 3 * sizeof(DvzColor), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_upload(plan, "point-size", 0, 3 * sizeof(float), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "point-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "point-color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "point-size", sizeof(metadata.size_id));

    uint32_t panel1_node_index = plan->count;
    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.1", "rt", false, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f},
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE));
    AT(dvz_frame_plan_render_visual(plan, "point"));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    uint32_t panel0_node_index = plan->count;
    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.0", "rt", false, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f},
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE));
    AT(dvz_frame_plan_render_visual(plan, "point"));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    plan->nodes[panel1_node_index].u.render.has_composition_pass = true;
    plan->nodes[panel1_node_index].u.render.composition_pass_id = (DvzFramePlanPassId){1};
    plan->nodes[panel1_node_index].u.render.has_graph_pass_index = true;
    plan->nodes[panel1_node_index].u.render.graph_pass_index = 1;
    plan->nodes[panel0_node_index].u.render.has_composition_pass = true;
    plan->nodes[panel0_node_index].u.render.composition_pass_id = (DvzFramePlanPassId){1};
    plan->nodes[panel0_node_index].u.render.has_graph_pass_index = true;
    plan->nodes[panel0_node_index].u.render.graph_pass_index = 0;

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphAttachment color = {0};
    dvz_strlcpy(color.resource_id, "rt", sizeof(color.resource_id));
    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass pass0 = {0};
    dvz_strlcpy(pass0.id, "panel.0.opaque", sizeof(pass0.id));
    dvz_strlcpy(pass0.panel_id, "panel.0", sizeof(pass0.panel_id));
    dvz_strlcpy(pass0.work_label, "opaque", sizeof(pass0.work_label));
    pass0.has_composition_pass = true;
    pass0.composition_pass_id = (DvzFramePlanPassId){1};
    pass0.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass0, &color));
    AT(dvz_frame_plan_graph_pass(plan, &pass0));

    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    DvzFrameGraphPass pass1 = {0};
    dvz_strlcpy(pass1.id, "panel.1.opaque", sizeof(pass1.id));
    dvz_strlcpy(pass1.panel_id, "panel.1", sizeof(pass1.panel_id));
    dvz_strlcpy(pass1.work_label, "opaque", sizeof(pass1.work_label));
    pass1.has_composition_pass = true;
    pass1.composition_pass_id = (DvzFramePlanPassId){1};
    pass1.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass1, &color));
    AT(dvz_frame_plan_graph_pass(plan, &pass1));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 100;
    emit_cfg.target_height = 100;
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint32_t viewport_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
            viewport_count++;
    }
    AT(viewport_count == 0);

    _test_scene_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure render metadata completeness is a FramePlan invariant, not query-local policy.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_metadata_complete(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.complete", 2);
    ANN(plan);
    AT(dvz_frame_plan_render_metadata_complete(plan));

    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));
    AT(!dvz_frame_plan_render_metadata_complete(plan));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "size", sizeof(metadata.size_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    AT(dvz_frame_plan_render_metadata_complete(plan));

    AT(dvz_frame_plan_render(plan, "panel.1", "target.panel.1.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.compat.0"));
    AT(!dvz_frame_plan_render_metadata_complete(plan));

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure malformed typed FramePlan metadata reports a focused diagnostic.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_render_visual_metadata_diagnostic(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.metadata.invalid", 2);
    ANN(plan);

    DvzFramePlanUploadMeta upload_meta = {0};
    upload_meta.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    upload_meta.visual_type = DVZ_VISUAL_TYPE_POINT;
    upload_meta.visual_index = 0;
    upload_meta.buffer_index = UINT32_MAX;

    AT(dvz_frame_plan_upload(plan, "only-position", 0, 3 * 3 * sizeof(float), ""));
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "opaque-debug-id"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "only-position", sizeof(metadata.position_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "typed point metadata missing color/size resource") == 0);

    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure draw emission rejects resources whose logical count is shorter than the draw count.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_draw_resource_validation_rejects_short_position(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.draw_resource.invalid", 3);
    ANN(plan);

    DvzFramePlanUploadMeta upload_meta = {0};
    upload_meta.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
    upload_meta.visual_type = DVZ_VISUAL_TYPE_POINT;
    upload_meta.visual_index = 0;
    upload_meta.buffer_index = UINT32_MAX;

    AT(dvz_frame_plan_upload(plan, "point-position", 0, 3 * 3 * sizeof(float), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    upload_meta.logical_item_count = 2;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_upload(plan, "point-color", 0, 3 * sizeof(DvzColor), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    upload_meta.logical_item_count = 3;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_upload(plan, "point-size", 0, 3 * sizeof(float), ""));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
    upload_meta.logical_item_count = 3;
    AT(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "point-debug-id"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    dvz_strlcpy(metadata.position_id, "point-position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "point-color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "point-size", sizeof(metadata.size_id));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    caps = dvz_capability_snapshot();
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) >= 1);
    const char* message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "visual=point") != NULL);
    AT(strstr(message, "role=position") != NULL);
    AT(strstr(message, "draw_count=3") != NULL);
    AT(strstr(message, "logical_count=2") != NULL);

    dvz_frame_plan_emitter_destroy(emitter);

    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    dvz_diagnostic_report_init(&report);
    emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    stream = dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) >= 1);
    message = dvz_diagnostic_report_get(&report, 0);
    ANN(message);
    AT(strstr(message, "visual=point") != NULL);
    AT(strstr(message, "role=position") != NULL);
    AT(strstr(message, "draw_count=3") != NULL);
    AT(strstr(message, "logical_count=2") != NULL);

    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_dynamic_update(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.dynamic", 8);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.dynamic.position", 64, 32, "point.position.update"));
    AT(dvz_frame_plan_compute(plan, "normalize_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(plan, "buf.dynamic.position"));
    AT(dvz_frame_plan_compute_write(plan, "buf.dynamic.normalized"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.dynamic.0"));

    AT(dvz_frame_plan_node_count(plan) == 3);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 1)) == DVZ_FRAME_PLAN_NODE_COMPUTE);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"shader_key\": \"normalize_positions\"") != NULL);
    AT(strstr(json, "\"dispatch\": { \"x\": 1, \"y\": 1, \"z\": 1 }") != NULL);
    AT(strstr(json, "\"reads\": [\"buf.dynamic.position\"]") != NULL);
    AT(strstr(json, "\"writes\": [\"buf.dynamic.normalized\"]") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_texture_upload_json_includes_region(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture", 11);
    ANN(plan);

    AT(_test_frame_plan_upload_texture_2d(
        plan, "tex.image.rgba", 0, 8, "image.rgba.patch", NULL, 2, 1, 4, 4, 1, 2));

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"origin_x\": 1") != NULL);
    AT(strstr(json, "\"width\": 2") != NULL);
    AT(strstr(json, "\"alloc_width\": 4") != NULL);
    AT(strstr(json, "\"alloc_height\": 4") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_texture_upload_json_includes_color_role(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture.color_role", 11);
    ANN(plan);

    DvzFramePlanUploadMeta metadata = {0};
    metadata.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D;
    metadata.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE;
    metadata.color_role = DVZ_COLOR_ROLE_SRGB_COLOR;
    metadata.visual_index = UINT32_MAX;
    metadata.buffer_index = UINT32_MAX;

    AT(_test_frame_plan_upload_texture_2d(
        plan, "tex.image.rgba", 0, 16, "image.rgba", NULL, 2, 2, 0, 0, 0, 0));
    AT(dvz_frame_plan_upload_metadata(plan, &metadata));

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"color_role\": \"srgb_color\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_readbacks(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.readback", 9);
    ANN(plan);

    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.query", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.query", "buf.query.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.query.readback", "request.query.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.color", "buf.offscreen.readback", 1024));
    AT(dvz_frame_plan_readback(plan, "buf.offscreen.readback", "request.export.0"));

    AT(dvz_frame_plan_node_count(plan) == 5);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 0)) == DVZ_FRAME_PLAN_NODE_RENDER);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 1)) == DVZ_FRAME_PLAN_NODE_COPY);
    AT(dvz_frame_plan_node_type(dvz_frame_plan_node_get(plan, 2)) == DVZ_FRAME_PLAN_NODE_READBACK);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"picking\": true") != NULL);
    AT(strstr(json, "\"type\": \"copy\"") != NULL);
    AT(strstr(json, "\"src_resource_id\": \"target.panel.0.query\"") != NULL);
    AT(strstr(json, "\"bytes_per_row\": 4") != NULL);
    AT(strstr(json, "\"rows_per_image\": 1") != NULL);
    AT(strstr(json, "\"request_id\": \"request.query.0\"") != NULL);
    AT(strstr(json, "\"request_id\": \"request.export.0\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_query_readback_copy_metadata(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.query.readback", 10);
    ANN(plan);

    DvzFramePlanCopyDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzFramePlanCopyDesc),
        .src_resource_id = "target.panel.0.query.identity",
        .dst_resource_id = "buf.query.readback",
        .src_attachment_index = 1,
        .src_origin = {2, 3, 0},
        .extent = {2, 1, 1},
        .format = 98,
        .bytes_per_texel = 8,
        .bytes_per_row = 16,
        .rows_per_image = 1,
        .dst_offset = 32,
        .byte_size = 16,
        .request_id = 1234,
    };
    AT(dvz_frame_plan_copy_ex(plan, &desc));
    AT(dvz_frame_plan_readback(plan, "buf.query.readback", "request.query.1234"));

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"src_attachment_index\": 1") != NULL);
    AT(strstr(json, "\"src_origin\": { \"x\": 2, \"y\": 3, \"z\": 0 }") != NULL);
    AT(strstr(json, "\"extent\": { \"width\": 2, \"height\": 1, \"depth\": 1 }") != NULL);
    AT(strstr(json, "\"format\": 98") != NULL);
    AT(strstr(json, "\"bytes_per_texel\": 8") != NULL);
    AT(strstr(json, "\"bytes_per_row\": 16") != NULL);
    AT(strstr(json, "\"dst_offset\": 32") != NULL);
    AT(strstr(json, "\"byte_size\": 16") != NULL);
    AT(strstr(json, "\"request_id\": 1234") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_buffer_to_texture_copy(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.buffer_to_texture", 11);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.cuda.shared", 0, 36, "cuda.shared"));
    plan->nodes[plan->count - 1].u.upload.buffer_usage =
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    DvzFramePlanCopyDesc copy = dvz_frame_plan_copy_desc();
    copy.direction = DVZ_FRAME_PLAN_COPY_BUFFER_TO_TEXTURE;
    copy.src_resource_id = "buf.cuda.shared";
    copy.dst_resource_id = "tex.cuda.normal";
    copy.src_offset = 8;
    copy.extent[0] = 2;
    copy.extent[1] = 2;
    copy.extent[2] = 1;
    copy.format = DVZ_FORMAT_R8G8B8A8_UNORM;
    copy.bytes_per_texel = 4;
    copy.bytes_per_row = 8;
    copy.rows_per_image = 2;
    copy.byte_size = 16;
    AT(dvz_frame_plan_copy_ex(plan, &copy));
    AT(dvz_frame_plan_upload(plan, "buf.cuda.shared.second", 0, 16, "cuda.shared.second"));
    plan->nodes[plan->count - 1].u.upload.buffer_usage =
        DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    copy.src_resource_id = "buf.cuda.shared.second";
    copy.dst_resource_id = "tex.cuda.normal.second";
    copy.src_offset = 0;
    AT(dvz_frame_plan_copy_ex(plan, &copy));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.image.0"));

    char* plan_json = dvz_frame_plan_json(plan);
    ANN(plan_json);
    AT(strstr(plan_json, "\"direction\": \"buffer_to_texture\"") != NULL);
    AT(strstr(plan_json, "\"src_offset\": 8") != NULL);
    AT(strstr(plan_json, "\"dst_origin\": { \"x\": 0, \"y\": 0, \"z\": 0 }") != NULL);
    dvz_frame_plan_json_destroy(plan_json);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    char* stream_json = dvz_drp2_stream_json(stream, "scene_buffer_to_texture_from_c");
    ANN(stream_json);
    AT(strstr(stream_json, "\"cmd\": \"CreateTexture\"") != NULL);
    AT(strstr(stream_json, "\"cmd\": \"CopyBufferToTexture\"") != NULL);
    AT(strstr(stream_json, "\"src_offset\": 8") != NULL);
    uint32_t copy_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        if (dvz_drp2_stream_get(stream, i)->type == DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE)
            copy_count++;
    }
    AT(copy_count == 2);
    AT(dvz_drp2_validate_stream(stream).ok);
    dvz_drp2_stream_json_destroy(stream_json);
    dvz_drp2_stream_destroy(stream);

    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.abi", 1);
    ANN(plan);

    DvzFramePlanCopyDesc copy = dvz_frame_plan_copy_desc();
    copy.struct_size = 0;
    AT(!dvz_frame_plan_copy_ex(plan, &copy));

    copy = dvz_frame_plan_copy_desc();
    copy.flags = 1;
    AT(!dvz_frame_plan_copy_ex(plan, &copy));

    AT(dvz_frame_plan_upload(plan, "buf.position", 0, 48, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.struct_size = DVZ_STRUCT_SIZE(DvzFramePlanEmitConfig) - 1;
    AT(dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg) == NULL);
    AT(dvz_diagnostic_report_count(&report) >= 1);

    dvz_diagnostic_report_init(&report);
    emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.flags = 1;
    AT(dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg) == NULL);
    AT(dvz_diagnostic_report_count(&report) >= 1);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the internal FramePlan graph records typed resources, passes, and attachments.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_static_multipass(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph", 12);
    ANN(plan);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.width = 128;
    rt.height = 96;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, "panel0.depth.opaque", sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    depth.width = 128;
    depth.height = 96;
    depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &depth));

    DvzFrameGraphResource accum = {0};
    dvz_strlcpy(accum.id, "panel0.wboit.accum", sizeof(accum.id));
    accum.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    accum.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    accum.width = 128;
    accum.height = 96;
    accum.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    accum.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &accum));

    DvzFrameGraphAttachment rt_clear = {0};
    dvz_strlcpy(rt_clear.resource_id, "rt", sizeof(rt_clear.resource_id));
    rt_clear.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    rt_clear.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    rt_clear.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_clear = {0};
    dvz_strlcpy(depth_clear.resource_id, "panel0.depth.opaque", sizeof(depth_clear.resource_id));
    depth_clear.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    depth_clear.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_clear.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    depth_clear.clear_depth = 1.0f;

    DvzFrameGraphAttachment accum_clear = {0};
    dvz_strlcpy(accum_clear.resource_id, "panel0.wboit.accum", sizeof(accum_clear.resource_id));
    accum_clear.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    accum_clear.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    accum_clear.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, "panel0.opaque", sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, "panel.0", sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "draws", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    opaque.has_viewport = true;
    opaque.viewport = (DvzFrameGraphRect){0.0f, 0.0f, 128.0f, 96.0f};
    opaque.has_scissor = true;
    opaque.scissor = (DvzFrameGraphRect){0.0f, 0.0f, 128.0f, 96.0f};
    AT(dvz_frame_graph_pass_color_attachment(&opaque, &rt_clear));
    AT(dvz_frame_graph_pass_depth_attachment(&opaque, &depth_clear));
    AT(dvz_frame_plan_graph_pass(plan, &opaque));

    DvzFrameGraphPass accum_pass = {0};
    dvz_strlcpy(accum_pass.id, "panel0.wboit.accum", sizeof(accum_pass.id));
    dvz_strlcpy(accum_pass.panel_id, "panel.0", sizeof(accum_pass.panel_id));
    dvz_strlcpy(accum_pass.work_label, "draws", sizeof(accum_pass.work_label));
    accum_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&accum_pass, &accum_clear));
    AT(dvz_frame_plan_graph_pass(plan, &accum_pass));

    DvzFrameGraphAttachment rt_load = {0};
    dvz_strlcpy(rt_load.resource_id, "rt", sizeof(rt_load.resource_id));
    rt_load.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    rt_load.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    rt_load.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, "panel0.wboit.resolve", sizeof(resolve.id));
    dvz_strlcpy(resolve.panel_id, "panel.0", sizeof(resolve.panel_id));
    dvz_strlcpy(resolve.work_label, "draws", sizeof(resolve.work_label));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(
        &resolve, "panel0.wboit.accum", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_color_attachment(&resolve, &rt_load));
    AT(dvz_frame_plan_graph_pass(plan, &resolve));

    AT(dvz_frame_plan_graph_resource_count(plan) == 3);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3);
    AT(dvz_frame_plan_graph_resource_get(plan, 2) != NULL);
    AT(dvz_frame_plan_graph_pass_get(plan, 2) != NULL);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"graph\"") != NULL);
    AT(strstr(json, "\"resources\"") != NULL);
    AT(strstr(json, "\"passes\"") != NULL);
    AT(strstr(json, "\"id\": \"panel0.wboit.resolve\"") != NULL);
    AT(strstr(json, "\"viewport\": { \"x\": 0") != NULL);
    AT(strstr(json, "\"scissor\": { \"x\": 0") != NULL);
    AT(strstr(json, "\"resource_id\": \"panel0.wboit.accum\"") != NULL);
    AT(strstr(json, "\"load_op\": \"load\"") != NULL);
    AT(strstr(json, "\"usage\": \"sampled\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}



static DvzFramePlan* _product_test_plan(uint32_t resource_samples)
{
    DvzFramePlan* plan = dvz_frame_plan("figure.products", 40);
    if (plan == NULL)
        return NULL;

    DvzFrameGraphResource resource = {0};
    dvz_strlcpy(resource.id, "product.resource", sizeof(resource.id));
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    resource.width = 64;
    resource.height = 48;
    resource.sample_count = resource_samples;
    resource.format = DVZ_FORMAT_R8G8B8A8_UNORM;
    resource.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!dvz_frame_plan_graph_resource(plan, &resource))
        goto error;

    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(attachment.resource_id, resource.id, sizeof(attachment.resource_id));
    attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    DvzFrameGraphPass producer = {0};
    dvz_strlcpy(producer.id, "panel0.producer", sizeof(producer.id));
    dvz_strlcpy(producer.panel_id, "panel.0", sizeof(producer.panel_id));
    producer.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_color_attachment(&producer, &attachment) ||
        !dvz_frame_plan_graph_pass(plan, &producer))
        goto error;

    DvzFrameGraphPass consumer = {0};
    dvz_strlcpy(consumer.id, "panel0.consumer", sizeof(consumer.id));
    dvz_strlcpy(consumer.panel_id, "panel.0", sizeof(consumer.panel_id));
    consumer.kind = DVZ_FRAME_GRAPH_PASS_COMPUTE;
    if (!dvz_frame_graph_pass_read(&consumer, resource.id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_plan_graph_pass(plan, &consumer))
        goto error;

    DvzRenderProductContract product = {0};
    product.id = (DvzRenderProductId){1};
    dvz_strlcpy(
        product.diagnostic_label, "scene_color@1", sizeof(product.diagnostic_label));
    product.version = 1;
    product.kind = DVZ_RENDER_PRODUCT_SCENE_COLOR;
    product.domain = DVZ_RENDER_PRODUCT_DOMAIN_PANEL;
    dvz_strlcpy(product.panel_id, "panel.0", sizeof(product.panel_id));
    dvz_strlcpy(product.view_id, "view.0", sizeof(product.view_id));
    dvz_strlcpy(product.camera_id, "camera.0", sizeof(product.camera_id));
    dvz_strlcpy(product.projection_id, "projection.0", sizeof(product.projection_id));
    product.extent_policy = DVZ_RENDER_PRODUCT_EXTENT_PANEL_RELATIVE;
    product.rounding_policy = DVZ_RENDER_PRODUCT_ROUND_OUTWARD;
    product.width = 64;
    product.height = 48;
    product.render_scale = 1.0f;
    product.local_to_target[0] = 1.0f;
    product.local_to_target[1] = 1.0f;
    product.format_class = DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR;
    product.concrete_format = resource.format;
    product.sample_domain = resource_samples > 1 ? DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE
                                                : DVZ_RENDER_PRODUCT_SAMPLES_SINGLE;
    product.sample_count = resource_samples;
    product.resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_NONE;
    product.coordinate_space = DVZ_RENDER_PRODUCT_COORDINATES_PANEL_LOCAL;
    product.encoding = DVZ_RENDER_PRODUCT_ENCODING_LINEAR_SCENE_COLOR;
    product.alpha = DVZ_RENDER_PRODUCT_ALPHA_PREMULTIPLIED;
    product.validity = DVZ_RENDER_PRODUCT_VALIDITY_FULL_EXTENT;
    product.required_usage_flags = resource.usage_flags;
    product.lifetime = resource.lifetime;
    product.resource_index = 0;
    product.producer_pass_index = 0;
    if (!dvz_frame_plan_product(plan, &product) ||
        !dvz_frame_plan_product_consumer(
            plan, product.id, 1, DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT))
        goto error;
    return plan;

error:
    dvz_frame_plan_destroy(plan);
    return NULL;
}



static bool _report_contains(const DvzDiagnosticReport* report, const char* text)
{
    ANN(report);
    ANN(text);
    for (uint32_t i = 0; i < dvz_diagnostic_report_count(report); i++)
    {
        const char* message = dvz_diagnostic_report_get(report, i);
        if (message != NULL && strstr(message, text) != NULL)
            return true;
    }
    return false;
}



int test_frame_plan_products_schema(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_frame_plan_product_count(plan) == 1);
    const DvzRenderProductContract* product = dvz_frame_plan_product_get(plan, 0);
    ANN(product);
    AT(product->kind == DVZ_RENDER_PRODUCT_SCENE_COLOR);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"frame_plan_schema\": \"0.2\"") != NULL);
    AT(strstr(json, "\"products\": [") != NULL);
    AT(strstr(json, "\"kind\": \"scene_color\"") != NULL);
    AT(strstr(json, "\"id\": 1") != NULL);
    AT(strstr(json, "\"label\": \"scene_color@1\"") != NULL);
    AT(strstr(json, "\"resource_index\": 0") != NULL);
    AT(strstr(json, "\"resource_label\": \"product.resource\"") != NULL);
    AT(strstr(json, "\"producer_pass_index\": 0") != NULL);
    AT(strstr(json, "\"producer_pass_label\": \"panel0.producer\"") != NULL);
    AT(strstr(json, "\"pass_label\": \"panel0.consumer\"") != NULL);
    dvz_frame_plan_json_destroy(json);

    char* ascii = dvz_frame_plan_graph_ascii(plan, DVZ_FRAME_PLAN_ASCII_COMPACT);
    ANN(ascii);
    AT(strstr(ascii, "Graph products=1") != NULL);
    AT(strstr(ascii, "#1 scene_color@1 v1") != NULL);
    dvz_frame_plan_graph_ascii_destroy(ascii);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_color_successor(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);

    DvzFrameGraphResource resource = plan->graph_resources[0];
    dvz_strlcpy(resource.id, "product.successor", sizeof(resource.id));
    AT(dvz_frame_plan_graph_resource(plan, &resource));
    DvzFrameGraphAttachment attachment = plan->graph_passes[0].color_attachments[0];
    dvz_strlcpy(attachment.resource_id, resource.id, sizeof(attachment.resource_id));
    DvzFrameGraphPass transform = {0};
    dvz_strlcpy(transform.id, "panel0.transform", sizeof(transform.id));
    dvz_strlcpy(transform.panel_id, "panel.0", sizeof(transform.panel_id));
    transform.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(
        &transform, plan->graph_resources[plan->products[0].resource_index].id,
        DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_color_attachment(&transform, &attachment));
    AT(dvz_frame_plan_graph_pass(plan, &transform));
    AT(dvz_frame_plan_product_consumer(
        plan, plan->products[0].id, 2,
        DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT));

    DvzRenderProductContract successor = plan->products[0];
    successor.id = (DvzRenderProductId){2};
    dvz_strlcpy(
        successor.diagnostic_label, "scene_color@2", sizeof(successor.diagnostic_label));
    successor.version = 2;
    successor.resource_index = 1;
    successor.source_product_id = plan->products[0].id;
    successor.producer_pass_index = 2;
    AT(dvz_frame_plan_product(plan, &successor));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_cross_panel(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    dvz_strlcpy(
        plan->graph_passes[1].panel_id, "panel.1", sizeof(plan->graph_passes[1].panel_id));
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "crosses panel scope at consumer"));
    plan->products[0].domain = DVZ_RENDER_PRODUCT_DOMAIN_QUERY;
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "crosses panel scope at consumer"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_implicit_samples(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(4);
    ANN(plan);
    plan->products[0].sample_domain = DVZ_RENDER_PRODUCT_SAMPLES_SINGLE;
    plan->products[0].sample_count = 1;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "sample count does not match resource"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_undefined_background(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    plan->products[0].validity = DVZ_RENDER_PRODUCT_VALIDITY_BACKGROUND_VALUE;
    plan->products[0].has_background_value = false;
    plan->product_uses[0].validity_requirement =
        DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_BACKGROUND_VALUE;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "lacks its declared background value"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_format_inference(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    plan->products[0].format_class = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
    plan->products[0].encoding = DVZ_RENDER_PRODUCT_ENCODING_LINEAR_VIEW_DEPTH;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "kind is incompatible"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_incoherent_surface_record(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    DvzRenderProductContract* depth = &plan->products[0];
    depth->id = (DvzRenderProductId){10};
    plan->product_uses[0].product_id = depth->id;
    dvz_strlcpy(
        depth->diagnostic_label, "surface_depth@1", sizeof(depth->diagnostic_label));
    depth->kind = DVZ_RENDER_PRODUCT_SURFACE_DEPTH;
    depth->format_class = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
    depth->concrete_format = DVZ_FORMAT_R32_SFLOAT;
    plan->graph_resources[0].format = depth->concrete_format;
    depth->encoding = DVZ_RENDER_PRODUCT_ENCODING_LINEAR_VIEW_DEPTH;
    depth->alpha = DVZ_RENDER_PRODUCT_ALPHA_NONE;
    depth->validity = DVZ_RENDER_PRODUCT_VALIDITY_EXPLICIT_COVERAGE;
    plan->product_uses[0].validity_requirement =
        DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_EXPLICIT_COVERAGE;
    depth->surface_record_id = (DvzSurfaceRecordId){1};

    const DvzRenderProductKind kinds[2] = {
        DVZ_RENDER_PRODUCT_SURFACE_NORMAL, DVZ_RENDER_PRODUCT_SURFACE_COVERAGE};
    const DvzRenderProductFormatClass formats[2] = {
        DVZ_RENDER_PRODUCT_FORMAT_NORMAL_FLOAT, DVZ_RENDER_PRODUCT_FORMAT_COVERAGE};
    const DvzRenderProductEncoding encodings[2] = {
        DVZ_RENDER_PRODUCT_ENCODING_VIEW_NORMAL, DVZ_RENDER_PRODUCT_ENCODING_COVERAGE};
    const uint32_t concrete_formats[2] = {
        DVZ_FORMAT_R16G16B16A16_SFLOAT, DVZ_FORMAT_R8_UNORM};
    const char* ids[2] = {"surface_normal@1", "surface_coverage@1"};
    const char* resources[2] = {"surface.normal", "surface.coverage"};
    for (uint32_t i = 0; i < 2; i++)
    {
        DvzFrameGraphResource resource = plan->graph_resources[0];
        dvz_strlcpy(resource.id, resources[i], sizeof(resource.id));
        resource.format = concrete_formats[i];
        AT(dvz_frame_plan_graph_resource(plan, &resource));
        DvzFrameGraphAttachment attachment = plan->graph_passes[0].color_attachments[0];
        dvz_strlcpy(attachment.resource_id, resources[i], sizeof(attachment.resource_id));
        AT(dvz_frame_graph_pass_color_attachment(&plan->graph_passes[0], &attachment));
        AT(dvz_frame_graph_pass_read(
            &plan->graph_passes[1], resources[i], DVZ_FRAME_GRAPH_ACCESS_SAMPLED));

        DvzRenderProductContract product = *depth;
        product.id = (DvzRenderProductId){11 + i};
        dvz_strlcpy(product.diagnostic_label, ids[i], sizeof(product.diagnostic_label));
        product.kind = kinds[i];
        product.format_class = formats[i];
        product.concrete_format = concrete_formats[i];
        product.encoding = encodings[i];
        product.coverage = i == 1 ? DVZ_RENDER_PRODUCT_COVERAGE_BINARY
                                  : DVZ_RENDER_PRODUCT_COVERAGE_NONE;
        product.resource_index = i + 1;
        AT(dvz_frame_plan_product(plan, &product));
        AT(dvz_frame_plan_product_consumer(
            plan, product.id, 1,
            i == 1 ? DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT
                   : DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_EXPLICIT_COVERAGE));
    }
    plan->products[0].validity_product_id = plan->products[2].id;
    plan->products[1].validity_product_id = plan->products[2].id;
    plan->products[2].validity = DVZ_RENDER_PRODUCT_VALIDITY_FULL_EXTENT;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));

    plan->product_use_count--;
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "omits its explicit coverage use"));
    AT(dvz_frame_plan_product_consumer(
        plan, plan->products[2].id, 1,
        DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT));

    plan->products[1].surface_record_id = (DvzSurfaceRecordId){2};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "mixes incompatible surface records"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_incompatible_concrete_format(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    plan->products[0].kind = DVZ_RENDER_PRODUCT_OBJECT_ID;
    plan->products[0].format_class = DVZ_RENDER_PRODUCT_FORMAT_UINT_ID;
    plan->products[0].encoding = DVZ_RENDER_PRODUCT_ENCODING_INTEGER_ID;
    plan->products[0].alpha = DVZ_RENDER_PRODUCT_ALPHA_NONE;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "concrete format is incompatible"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_omitted_reader(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    DvzFrameGraphPass reader = plan->graph_passes[1];
    dvz_strlcpy(reader.id, "panel0.untyped-reader", sizeof(reader.id));
    AT(dvz_frame_plan_graph_pass(plan, &reader));
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "omits actual reader"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_intervening_writer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    DvzFrameGraphPass consumer = plan->graph_passes[1];
    DvzFrameGraphPass writer = plan->graph_passes[0];
    dvz_strlcpy(writer.id, "panel0.intervening-writer", sizeof(writer.id));
    plan->graph_passes[1] = writer;
    AT(dvz_frame_plan_graph_pass(plan, &consumer));
    plan->product_uses[0].pass_index = 2;
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "overwritten before consumer"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_alias_overlap(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    DvzRenderProductContract alias = plan->products[0];
    alias.id = (DvzRenderProductId){2};
    dvz_strlcpy(alias.diagnostic_label, "alias@1", sizeof(alias.diagnostic_label));
    AT(dvz_frame_plan_product(plan, &alias));
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "overlapping physical aliases"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



static DvzFramePlan* _product_explicit_resolve_test_plan(void)
{
    DvzFramePlan* plan = _product_test_plan(4);
    if (plan == NULL)
        return NULL;
    DvzFrameGraphPass consumer = plan->graph_passes[1];

    DvzFrameGraphResource resolved_resource = plan->graph_resources[0];
    dvz_strlcpy(resolved_resource.id, "product.resolved", sizeof(resolved_resource.id));
    resolved_resource.sample_count = 1;
    resolved_resource.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    if (!dvz_frame_plan_graph_resource(plan, &resolved_resource))
        goto error;

    DvzFrameGraphPass resolver = {0};
    dvz_strlcpy(resolver.id, "panel0.resolve", sizeof(resolver.id));
    dvz_strlcpy(resolver.panel_id, "panel.0", sizeof(resolver.panel_id));
    resolver.kind = DVZ_FRAME_GRAPH_PASS_COMPUTE;
    if (!dvz_frame_graph_pass_read(
            &resolver, plan->graph_resources[0].id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_write(
            &resolver, resolved_resource.id, DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE))
        goto error;
    plan->graph_passes[1] = resolver;
    dvz_strlcpy(
        consumer.reads[0].resource_id, resolved_resource.id,
        sizeof(consumer.reads[0].resource_id));
    if (!dvz_frame_plan_graph_pass(plan, &consumer))
        goto error;
    plan->product_uses[0].pass_index = 1;

    DvzRenderProductContract resolved = plan->products[0];
    resolved.id = (DvzRenderProductId){2};
    dvz_strlcpy(
        resolved.diagnostic_label, "scene_color_resolved@2",
        sizeof(resolved.diagnostic_label));
    resolved.version = 2;
    resolved.resource_index = 1;
    resolved.source_product_id = plan->products[0].id;
    resolved.producer_pass_index = 1;
    resolved.sample_domain = DVZ_RENDER_PRODUCT_SAMPLES_RESOLVED;
    resolved.sample_count = 1;
    resolved.resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR;
    resolved.required_usage_flags = resolved_resource.usage_flags;
    if (!dvz_frame_plan_product(plan, &resolved) ||
        !dvz_frame_plan_product_consumer(
            plan, resolved.id, 2, DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT))
        goto error;
    return plan;

error:
    dvz_frame_plan_destroy(plan);
    return NULL;
}



int test_frame_plan_products_explicit_shader_resolve(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_explicit_resolve_test_plan();
    ANN(plan);
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    plan->products[1].resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_WINNING_ID;
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "invalid sample-domain or resolve contract"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_attachment_resolve(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(4);
    ANN(plan);

    DvzFrameGraphResource resolved_resource = plan->graph_resources[0];
    dvz_strlcpy(resolved_resource.id, "product.attachment-resolved", sizeof(resolved_resource.id));
    resolved_resource.sample_count = 1;
    AT(dvz_frame_plan_graph_resource(plan, &resolved_resource));
    dvz_strlcpy(
        plan->graph_passes[0].color_attachments[0].resolve_resource_id, resolved_resource.id,
        sizeof(plan->graph_passes[0].color_attachments[0].resolve_resource_id));
    dvz_strlcpy(
        plan->graph_passes[1].reads[0].resource_id, resolved_resource.id,
        sizeof(plan->graph_passes[1].reads[0].resource_id));
    plan->product_use_count = 0;

    DvzRenderProductContract resolved = plan->products[0];
    resolved.id = (DvzRenderProductId){2};
    dvz_strlcpy(
        resolved.diagnostic_label, "scene_color_attachment_resolved@2",
        sizeof(resolved.diagnostic_label));
    resolved.version = 2;
    resolved.resource_index = 1;
    resolved.source_product_id = plan->products[0].id;
    resolved.sample_domain = DVZ_RENDER_PRODUCT_SAMPLES_RESOLVED;
    resolved.sample_count = 1;
    resolved.resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR;
    AT(dvz_frame_plan_product(plan, &resolved));
    AT(dvz_frame_plan_product_consumer(
        plan, resolved.id, 1, DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    plan->products[1].resolve_policy = DVZ_RENDER_PRODUCT_RESOLVE_WINNING_NORMAL;
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "attachment resolve for a non-color policy"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_consumer_growth(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    for (uint32_t i = 2; i < 42; i++)
    {
        DvzFrameGraphPass consumer = plan->graph_passes[1];
        dvz_snprintf(consumer.id, sizeof(consumer.id), "panel0.consumer.%" PRIu32, i);
        AT(dvz_frame_plan_graph_pass(plan, &consumer));
        AT(dvz_frame_plan_product_consumer(
            plan, plan->products[0].id, i,
            DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT));
    }
    AT(plan->product_use_count == 41);
    AT(plan->product_use_capacity >= 41);
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_products_reject_orphan_use(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzFramePlan* plan = _product_test_plan(1);
    ANN(plan);
    plan->product_uses[0].product_id = (DvzRenderProductId){999};
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(_report_contains(&report, "references unknown product id 999"));
    dvz_frame_plan_destroy(plan);
    return 0;
}



/**
 * Ensure the FramePlan graph terminal view includes passes, resources, and dependencies.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_ascii(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.ascii", 31);
    ANN(plan);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.width = 128;
    rt.height = 96;
    rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, "panel0.depth", sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    depth.width = 128;
    depth.height = 96;
    depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &depth));

    DvzFrameGraphResource ssao = {0};
    dvz_strlcpy(ssao.id, "panel0.ssao", sizeof(ssao.id));
    ssao.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    ssao.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    ssao.width = 128;
    ssao.height = 96;
    ssao.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                       DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    ssao.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &ssao));

    DvzFrameGraphAttachment color = {0};
    dvz_strlcpy(color.resource_id, "rt", sizeof(color.resource_id));
    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_attachment = {0};
    dvz_strlcpy(depth_attachment.resource_id, "panel0.depth", sizeof(depth_attachment.resource_id));
    depth_attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    depth_attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    depth_attachment.clear_depth = 1.0f;

    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, "panel0.opaque", sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, "panel0", sizeof(opaque.panel_id));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    opaque.has_viewport = true;
    opaque.viewport = (DvzFrameGraphRect){0.0f, 0.0f, 128.0f, 96.0f};
    AT(dvz_frame_graph_pass_color_attachment(&opaque, &color));
    AT(dvz_frame_graph_pass_depth_attachment(&opaque, &depth_attachment));
    AT(dvz_frame_plan_graph_pass(plan, &opaque));

    DvzFrameGraphAttachment ssao_color = {0};
    dvz_strlcpy(ssao_color.resource_id, "panel0.ssao", sizeof(ssao_color.resource_id));
    ssao_color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    ssao_color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    ssao_color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass ssao_pass = {0};
    dvz_strlcpy(ssao_pass.id, "panel0.ssao", sizeof(ssao_pass.id));
    dvz_strlcpy(ssao_pass.panel_id, "panel0", sizeof(ssao_pass.panel_id));
    ssao_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(&ssao_pass, "panel0.depth", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_color_attachment(&ssao_pass, &ssao_color));
    AT(dvz_frame_plan_graph_pass(plan, &ssao_pass));

    char* text = dvz_frame_plan_graph_ascii(plan, DVZ_FRAME_PLAN_ASCII_VERBOSE);
    ANN(text);
    AT(strstr(text, "FramePlan figure=figure.ascii frame=31") != NULL);
    AT(strstr(text, "Flow:") != NULL);
    AT(strstr(text, "[render #0]") != NULL);
    AT(strstr(
           text,
           "depth_attachment_write sampled (panel0.depth) "
           "──▶ [render #1 panel0.ssao]") != NULL);
    AT(strstr(text, "id: panel0.opaque") != NULL);
    AT(strstr(text, "depth[clear/store]") != NULL);
    AT(strstr(text, "(panel0.depth) texture panel per_frame usage=depth,sampled") != NULL);
    AT(strstr(
           text,
           "[render #0 panel0.opaque] -> [render #1 panel0.ssao] via (panel0.depth) "
           "depth_attachment_write -> sampled") != NULL);
    dvz_frame_plan_graph_ascii_destroy(text);

    char* ascii = dvz_frame_plan_graph_ascii(plan, DVZ_FRAME_PLAN_ASCII_ASCII_ONLY);
    ANN(ascii);
    AT(strstr(ascii, "->") != NULL);
    dvz_frame_plan_graph_ascii_destroy(ascii);

    dvz_frame_plan_destroy(plan);
    return 0;
}



/**
 * Ensure FramePlan graph trace environment parsing and change filtering stay deterministic.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_trace_env(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    AT(_scene_frame_plan_trace_mode_from_env(NULL) == DVZ_FRAME_PLAN_TRACE_NONE);
    AT(_scene_frame_plan_trace_mode_from_env("0") == DVZ_FRAME_PLAN_TRACE_NONE);
    AT(_scene_frame_plan_trace_mode_from_env("off") == DVZ_FRAME_PLAN_TRACE_NONE);
    AT(_scene_frame_plan_trace_mode_from_env("1") == DVZ_FRAME_PLAN_TRACE_NORMAL);
    AT(_scene_frame_plan_trace_mode_from_env("normal") == DVZ_FRAME_PLAN_TRACE_NORMAL);
    AT(_scene_frame_plan_trace_mode_from_env("ascii") == DVZ_FRAME_PLAN_TRACE_NORMAL);
    AT(_scene_frame_plan_trace_mode_from_env("full") == DVZ_FRAME_PLAN_TRACE_FULL);
    AT(_scene_frame_plan_trace_mode_from_env("ascii-full") == DVZ_FRAME_PLAN_TRACE_FULL);

    AT(
        (_scene_frame_plan_trace_flags_from_env("ascii") &
         DVZ_FRAME_PLAN_ASCII_ASCII_ONLY) != 0);
    AT(
        (_scene_frame_plan_trace_flags_from_env("full") &
         DVZ_FRAME_PLAN_ASCII_ASCII_ONLY) == 0);
    AT(
        (_scene_frame_plan_trace_flags_from_env("full-ascii") &
         DVZ_FRAME_PLAN_ASCII_ASCII_ONLY) != 0);

    DvzFigure* figure = (DvzFigure*)dvz_calloc(1, sizeof(DvzFigure));
    ANN(figure);
    AT(!_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NONE, figure, "graph"));
    AT(_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, figure, "graph"));
    figure->has_last_frame_plan_trace = true;
    figure->last_frame_plan_trace = "graph";
    AT(!_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, figure, "graph"));
    AT(_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, figure, "graph2"));
    AT(_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_FULL, figure, "graph"));
    dvz_free(figure);
    return 0;
}



/**
 * Ensure the internal FramePlan graph reports read-before-write mistakes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_read_before_write(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.invalid", 13);
    ANN(plan);

    DvzFrameGraphResource resource = {0};
    dvz_strlcpy(resource.id, "tex.unwritten", sizeof(resource.id));
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &resource));

    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, "panel0.resolve", sizeof(pass.id));
    pass.kind = DVZ_FRAME_GRAPH_PASS_COMPUTE;
    AT(dvz_frame_graph_pass_read(&pass, "tex.unwritten", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph pass 'panel0.resolve' has no producer for resource 'tex.unwritten'") ==
       0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure graph validation reports consumers that appear before their producer pass.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_topological_order(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.order", 20);
    ANN(plan);

    DvzFrameGraphResource resource = {0};
    dvz_strlcpy(resource.id, "tex.future", sizeof(resource.id));
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    resource.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &resource));

    DvzFrameGraphPass consumer = {0};
    dvz_strlcpy(consumer.id, "panel0.resolve", sizeof(consumer.id));
    consumer.kind = DVZ_FRAME_GRAPH_PASS_COMPUTE;
    AT(dvz_frame_graph_pass_read(&consumer, "tex.future", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_plan_graph_pass(plan, &consumer));

    DvzFrameGraphAttachment color = {0};
    dvz_strlcpy(color.resource_id, "tex.future", sizeof(color.resource_id));
    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass producer = {0};
    dvz_strlcpy(producer.id, "panel0.producer", sizeof(producer.id));
    producer.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&producer, &color));
    AT(dvz_frame_plan_graph_pass(plan, &producer));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph pass 'panel0.resolve' reads resource 'tex.future' before producer "
           "pass 'panel0.producer'; graph passes must be topological") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the internal FramePlan graph rejects attachments on non-render passes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_pass_kind(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.pass_kind", 17);
    ANN(plan);

    DvzFrameGraphResource resource = {0};
    dvz_strlcpy(resource.id, "color.fixed", sizeof(resource.id));
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIXED;
    resource.width = 64;
    resource.height = 64;
    resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &resource));

    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(attachment.resource_id, "color.fixed", sizeof(attachment.resource_id));
    attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, "panel0.compute", sizeof(pass.id));
    pass.kind = DVZ_FRAME_GRAPH_PASS_COMPUTE;
    AT(dvz_frame_graph_pass_color_attachment(&pass, &attachment));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph pass 'panel0.compute' has render attachments but is not a render "
           "pass") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure graph dependencies expose ordered WBOIT-style producers and consumers.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_dependencies_dump(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.dependencies", 18);
    ANN(plan);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, "panel0.depth", sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &depth));

    DvzFrameGraphResource accum = {0};
    dvz_strlcpy(accum.id, "panel0.wboit.accum", sizeof(accum.id));
    accum.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    accum.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    accum.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    accum.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &accum));

    DvzFrameGraphResource weight = {0};
    dvz_strlcpy(weight.id, "panel0.wboit.weight", sizeof(weight.id));
    weight.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    weight.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    weight.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                         DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    weight.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &weight));

    DvzFrameGraphAttachment color = {0};
    dvz_strlcpy(color.resource_id, "rt", sizeof(color.resource_id));
    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_write = {0};
    dvz_strlcpy(depth_write.resource_id, "panel0.depth", sizeof(depth_write.resource_id));
    depth_write.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    depth_write.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_write.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, "panel0.opaque", sizeof(opaque.id));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&opaque, &color));
    AT(dvz_frame_graph_pass_depth_attachment(&opaque, &depth_write));
    AT(dvz_frame_plan_graph_pass(plan, &opaque));

    DvzFrameGraphAttachment accum_color = {0};
    dvz_strlcpy(accum_color.resource_id, "panel0.wboit.accum", sizeof(accum_color.resource_id));
    accum_color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    accum_color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    accum_color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment weight_color = {0};
    dvz_strlcpy(weight_color.resource_id, "panel0.wboit.weight", sizeof(weight_color.resource_id));
    weight_color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    weight_color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    weight_color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_read = {0};
    dvz_strlcpy(depth_read.resource_id, "panel0.depth", sizeof(depth_read.resource_id));
    depth_read.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    depth_read.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_read.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ;

    DvzFrameGraphPass accum_pass = {0};
    dvz_strlcpy(accum_pass.id, "panel0.wboit.accum", sizeof(accum_pass.id));
    accum_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&accum_pass, &accum_color));
    AT(dvz_frame_graph_pass_color_attachment(&accum_pass, &weight_color));
    AT(dvz_frame_graph_pass_depth_attachment(&accum_pass, &depth_read));
    AT(dvz_frame_plan_graph_pass(plan, &accum_pass));

    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, "panel0.wboit.resolve", sizeof(resolve.id));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(
        &resolve, "panel0.wboit.accum", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_read(
        &resolve, "panel0.wboit.weight", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_color_attachment(&resolve, &color));
    AT(dvz_frame_plan_graph_pass(plan, &resolve));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    AT(dvz_frame_plan_graph_dependency_count(plan) == 4);
    DvzFrameGraphDependency dep = {0};
    AT(dvz_frame_plan_graph_dependency_get(plan, 0, &dep));
    AT(strcmp(dep.producer_pass_id, "panel0.opaque") == 0);
    AT(strcmp(dep.consumer_pass_id, "panel0.wboit.accum") == 0);
    AT(strcmp(dep.resource_id, "panel0.depth") == 0);
    AT(dep.consumer_usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ);
    AT(dvz_frame_plan_graph_dependency_get(plan, 1, &dep));
    AT(strcmp(dep.resource_id, "panel0.wboit.accum") == 0);
    AT(strcmp(dep.consumer_pass_id, "panel0.wboit.resolve") == 0);
    AT(dvz_frame_plan_graph_dependency_get(plan, 2, &dep));
    AT(strcmp(dep.resource_id, "panel0.wboit.weight") == 0);
    AT(strcmp(dep.consumer_pass_id, "panel0.wboit.resolve") == 0);
    AT(dvz_frame_plan_graph_dependency_get(plan, 3, &dep));
    AT(strcmp(dep.resource_id, "rt") == 0);
    AT(strcmp(dep.consumer_pass_id, "panel0.wboit.resolve") == 0);

    char* dump = dvz_frame_plan_graph_dump(plan);
    ANN(dump);
    AT(strstr(dump, "\"passes\"") != NULL);
    AT(strstr(dump, "\"dependencies\"") != NULL);
    AT(strstr(dump, "\"producer\": \"panel0.opaque\"") != NULL);
    AT(strstr(dump, "\"consumer\": \"panel0.wboit.resolve\"") != NULL);
    dvz_frame_plan_json_destroy(dump);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the FramePlan graph can describe a depth-peeling-shaped pass/resource graph.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_depth_peeling_shape(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.depth_peel", 20);
    ANN(plan);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.format = DVZ_FORMAT_R8G8B8A8_UNORM;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                     DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource opaque_depth = {0};
    dvz_strlcpy(opaque_depth.id, "panel0.depth.opaque", sizeof(opaque_depth.id));
    opaque_depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    opaque_depth.format = DVZ_FORMAT_D32_SFLOAT;
    opaque_depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    opaque_depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    opaque_depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &opaque_depth));

    const char* color_ids[4] = {
        "panel0.peel.front_accum",
        "panel0.peel.back_accum",
        "panel0.peel.depth_minmax_ping",
        "panel0.peel.depth_minmax_pong",
    };
    for (uint32_t i = 0; i < 4; i++)
    {
        DvzFrameGraphResource resource = {0};
        dvz_strlcpy(resource.id, color_ids[i], sizeof(resource.id));
        resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        resource.format = i < 2 ? DVZ_FORMAT_R16G16B16A16_SFLOAT : DVZ_FORMAT_R32G32_SFLOAT;
        resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                               DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        AT(dvz_frame_plan_graph_resource(plan, &resource));
    }

    DvzFrameGraphAttachment rt_clear = {0};
    dvz_strlcpy(rt_clear.resource_id, "rt", sizeof(rt_clear.resource_id));
    rt_clear.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    rt_clear.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    rt_clear.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_write = {0};
    dvz_strlcpy(depth_write.resource_id, "panel0.depth.opaque", sizeof(depth_write.resource_id));
    depth_write.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    depth_write.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_write.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    depth_write.clear_depth = 1.0f;

    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, "panel0.opaque", sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, "panel.0", sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&opaque, &rt_clear));
    AT(dvz_frame_graph_pass_depth_attachment(&opaque, &depth_write));
    AT(dvz_frame_plan_graph_pass(plan, &opaque));

    DvzFrameGraphAttachment depth_read = {0};
    dvz_strlcpy(depth_read.resource_id, "panel0.depth.opaque", sizeof(depth_read.resource_id));
    depth_read.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    depth_read.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
    depth_read.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ;

    DvzFrameGraphPass init = {0};
    dvz_strlcpy(init.id, "panel0.peel.init", sizeof(init.id));
    dvz_strlcpy(init.panel_id, "panel.0", sizeof(init.panel_id));
    dvz_strlcpy(init.work_label, "depth_peel_init", sizeof(init.work_label));
    init.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzFrameGraphAttachment attachment = {0};
        dvz_strlcpy(attachment.resource_id, color_ids[i], sizeof(attachment.resource_id));
        attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
        attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
        attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
        AT(dvz_frame_graph_pass_color_attachment(&init, &attachment));
    }
    AT(dvz_frame_graph_pass_depth_attachment(&init, &depth_read));
    AT(dvz_frame_plan_graph_pass(plan, &init));

    for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        const bool even = (iter_idx % 2) == 0;
        const char* read_depth = even ? color_ids[2] : color_ids[3];
        const char* write_depth = even ? color_ids[3] : color_ids[2];

        DvzFrameGraphPass iter = {0};
        dvz_snprintf(iter.id, sizeof(iter.id), "panel0.peel.iter.%u", iter_idx);
        dvz_strlcpy(iter.panel_id, "panel.0", sizeof(iter.panel_id));
        dvz_strlcpy(iter.work_label, "depth_peel_iter", sizeof(iter.work_label));
        iter.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
        AT(dvz_frame_graph_pass_read(&iter, read_depth, DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
        for (uint32_t i = 0; i < 3; i++)
        {
            DvzFrameGraphAttachment attachment = {0};
            const char* resource_id = i == 0 ? color_ids[0] : i == 1 ? color_ids[1] : write_depth;
            dvz_strlcpy(attachment.resource_id, resource_id, sizeof(attachment.resource_id));
            attachment.load_op = i < 2 ? DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD :
                                         DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
            attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
            attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
            AT(dvz_frame_graph_pass_color_attachment(&iter, &attachment));
        }
        AT(dvz_frame_graph_pass_depth_attachment(&iter, &depth_read));
        AT(dvz_frame_plan_graph_pass(plan, &iter));
    }

    DvzFrameGraphAttachment rt_load = {0};
    dvz_strlcpy(rt_load.resource_id, "rt", sizeof(rt_load.resource_id));
    rt_load.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    rt_load.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    rt_load.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass composite = {0};
    dvz_strlcpy(composite.id, "panel0.peel.composite", sizeof(composite.id));
    dvz_strlcpy(composite.panel_id, "panel.0", sizeof(composite.panel_id));
    dvz_strlcpy(composite.work_label, "depth_peel_composite", sizeof(composite.work_label));
    composite.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(
        &composite, "panel0.peel.front_accum", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_read(
        &composite, "panel0.peel.back_accum", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_graph_pass_color_attachment(&composite, &rt_load));
    AT(dvz_frame_plan_graph_pass(plan, &composite));

    AT(dvz_frame_plan_graph_resource_count(plan) == 6);
    AT(dvz_frame_plan_graph_pass_count(plan) == 3 + DVZ_SCENE_DEPTH_PEEL_ITERATIONS);

    const DvzFrameGraphResource* front_accum = dvz_frame_plan_graph_resource_get(plan, 2);
    const DvzFrameGraphResource* depth_minmax_ping = dvz_frame_plan_graph_resource_get(plan, 4);
    ANN(front_accum);
    ANN(depth_minmax_ping);
    AT(strcmp(front_accum->id, "panel0.peel.front_accum") == 0);
    AT(front_accum->format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(strcmp(depth_minmax_ping->id, "panel0.peel.depth_minmax_ping") == 0);
    AT(depth_minmax_ping->format == DVZ_FORMAT_R32G32_SFLOAT);
    AT(
        (front_accum->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0);
    AT((front_accum->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED) != 0);

    const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, 2);
    ANN(pass);
    AT(strcmp(pass->id, "panel0.peel.iter.0") == 0);
    AT(pass->read_count == 1);
    AT(strcmp(pass->reads[0].resource_id, "panel0.peel.depth_minmax_ping") == 0);
    AT(pass->color_attachment_count == 3);
    AT(pass->has_depth_attachment);

    for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
    {
        const DvzFrameGraphPass* iter = dvz_frame_plan_graph_pass_get(plan, 2 + iter_idx);
        ANN(iter);
        char iter_id[DVZ_SCENE_LABEL_SIZE];
        dvz_snprintf(iter_id, sizeof(iter_id), "panel0.peel.iter.%u", iter_idx);
        AT(strcmp(iter->id, iter_id) == 0);
        AT(iter->read_count == 1);

        const bool even = (iter_idx % 2) == 0;
        const char* read_depth = even ? color_ids[2] : color_ids[3];
        const char* write_depth = even ? color_ids[3] : color_ids[2];
        AT(strcmp(iter->reads[0].resource_id, read_depth) == 0);
        AT(iter->color_attachment_count == 3);
        AT(strcmp(iter->color_attachments[0].resource_id, color_ids[0]) == 0);
        AT(iter->color_attachments[0].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);
        AT(strcmp(iter->color_attachments[1].resource_id, color_ids[1]) == 0);
        AT(iter->color_attachments[1].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD);
        AT(strcmp(iter->color_attachments[2].resource_id, write_depth) == 0);
        AT(iter->color_attachments[2].load_op == DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR);
        AT(iter->has_depth_attachment);
        AT(strcmp(iter->depth_attachment.resource_id, "panel0.depth.opaque") == 0);
        AT(iter->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);
    }

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    AT(dvz_frame_plan_graph_dependency_count(plan) >= 6);
    DvzFrameGraphDependency dep = {0};
    bool init_depth_dep = false;
    bool iter_depth_dep = false;
    bool iter_minmax_dep = false;
    bool iter_chain_deps[DVZ_SCENE_DEPTH_PEEL_ITERATIONS] = {0};
    for (uint32_t i = 0; i < dvz_frame_plan_graph_dependency_count(plan); i++)
    {
        AT(dvz_frame_plan_graph_dependency_get(plan, i, &dep));
        if (
            strcmp(dep.producer_pass_id, "panel0.opaque") == 0 &&
            strcmp(dep.consumer_pass_id, "panel0.peel.init") == 0 &&
            strcmp(dep.resource_id, "panel0.depth.opaque") == 0 &&
            dep.consumer_usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ)
            init_depth_dep = true;
        if (
            strcmp(dep.producer_pass_id, "panel0.opaque") == 0 &&
            strcmp(dep.consumer_pass_id, "panel0.peel.iter.0") == 0 &&
            strcmp(dep.resource_id, "panel0.depth.opaque") == 0)
            iter_depth_dep = true;
        if (
            strcmp(dep.producer_pass_id, "panel0.peel.init") == 0 &&
            strcmp(dep.consumer_pass_id, "panel0.peel.iter.0") == 0 &&
            strcmp(dep.resource_id, "panel0.peel.depth_minmax_ping") == 0)
            iter_minmax_dep = true;
        for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
        {
            char producer_id[DVZ_SCENE_LABEL_SIZE];
            char consumer_id[DVZ_SCENE_LABEL_SIZE];
            if (iter_idx == 0)
                dvz_strlcpy(producer_id, "panel0.peel.init", sizeof(producer_id));
            else
                dvz_snprintf(
                    producer_id, sizeof(producer_id), "panel0.peel.iter.%u", iter_idx - 1);
            dvz_snprintf(consumer_id, sizeof(consumer_id), "panel0.peel.iter.%u", iter_idx);

            const bool even = (iter_idx % 2) == 0;
            const char* resource_id = even ? color_ids[2] : color_ids[3];
            if (
                strcmp(dep.producer_pass_id, producer_id) == 0 &&
                strcmp(dep.consumer_pass_id, consumer_id) == 0 &&
                strcmp(dep.resource_id, resource_id) == 0 &&
                dep.consumer_usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED)
                iter_chain_deps[iter_idx] = true;
        }
    }
    AT(init_depth_dep);
    AT(iter_depth_dep);
    AT(iter_minmax_dep);
    for (uint32_t iter_idx = 0; iter_idx < DVZ_SCENE_DEPTH_PEEL_ITERATIONS; iter_idx++)
        AT(iter_chain_deps[iter_idx]);

    char* dump = dvz_frame_plan_graph_dump(plan);
    ANN(dump);
    AT(strstr(dump, "\"id\": \"panel0.peel.iter.0\"") != NULL);
    AT(strstr(dump, "\"resource_id\": \"panel0.peel.depth_minmax_pong\"") != NULL);
    AT(strstr(dump, "\"consumer\": \"panel0.peel.composite\"") != NULL);
    dvz_frame_plan_json_destroy(dump);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"id\": \"panel0.peel.front_accum\"") != NULL);
    AT(strstr(json, "\"work\": \"depth_peel_composite\"") != NULL);
    AT(strstr(json, "\"usage\": \"sampled\"") != NULL);
    AT(strstr(json, "\"access\": \"read\"") != NULL);
    dvz_frame_plan_json_destroy(json);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the internal G-buffer technique declares shared depth, normal, and object-id targets.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_gbuffer_shape(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.gbuffer", 8);
    ANN(plan);

    DvzSceneGBufferPlan gbuffer = {0};
    _scene_technique_gbuffer_plan_init(&gbuffer);
    gbuffer.enabled = true;
    gbuffer.needs_depth = true;
    gbuffer.needs_normal = true;
    gbuffer.needs_object_id = true;
    gbuffer.producer_count = 1;

    AT(_scene_technique_emit_gbuffer_frame_graph(plan, "panel0", &gbuffer));
    AT(dvz_frame_plan_graph_resource_count(plan) == 3);
    AT(dvz_frame_plan_graph_pass_count(plan) == 1);

    const DvzFrameGraphResource* depth = dvz_frame_plan_graph_resource_get(plan, 0);
    const DvzFrameGraphResource* normal = dvz_frame_plan_graph_resource_get(plan, 1);
    const DvzFrameGraphResource* object = dvz_frame_plan_graph_resource_get(plan, 2);
    ANN(depth);
    ANN(normal);
    ANN(object);
    AT(strcmp(depth->id, "panel0.gbuffer.depth") == 0);
    AT(depth->format == DVZ_FORMAT_D32_SFLOAT);
    AT(depth->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT);
    AT(depth->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);
    AT(strcmp(normal->id, "panel0.gbuffer.normal") == 0);
    AT(normal->format == DVZ_FORMAT_R16G16B16A16_SFLOAT);
    AT(normal->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT);
    AT(normal->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);
    AT(strcmp(object->id, "panel0.gbuffer.object_id") == 0);
    AT(object->format == DVZ_FORMAT_R32_UINT);
    AT(object->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT);
    AT(object->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);

    const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, 0);
    ANN(pass);
    AT(strcmp(pass->id, "panel0.gbuffer") == 0);
    AT(strcmp(pass->work_label, "gbuffer") == 0);
    AT(pass->color_attachment_count == 2);
    AT(pass->has_depth_attachment);
    AT(strcmp(pass->color_attachments[0].resource_id, "panel0.gbuffer.normal") == 0);
    AT(strcmp(pass->color_attachments[1].resource_id, "panel0.gbuffer.object_id") == 0);
    AT(strcmp(pass->depth_attachment.resource_id, "panel0.gbuffer.depth") == 0);
    AT(pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) == 0);

    char* dump = dvz_frame_plan_graph_dump(plan);
    ANN(dump);
    AT(strstr(dump, "\"resource_id\": \"panel0.gbuffer.normal\"") != NULL);
    dvz_frame_plan_json_destroy(dump);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"work\": \"gbuffer\"") != NULL);
    AT(strstr(json, "\"id\": \"panel0.gbuffer.object_id\"") != NULL);
    dvz_frame_plan_json_destroy(json);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure graph validation reports ambiguous same-pass producers by pass and resource.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_ambiguous_producer(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.ambiguous", 19);
    ANN(plan);

    DvzFrameGraphResource color_resource = {0};
    dvz_strlcpy(color_resource.id, "color", sizeof(color_resource.id));
    color_resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    color_resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    color_resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    color_resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &color_resource));

    DvzFrameGraphAttachment color = {0};
    dvz_strlcpy(color.resource_id, "color", sizeof(color.resource_id));
    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, "panel0.bad", sizeof(pass.id));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass, &color));
    AT(dvz_frame_graph_pass_write(&pass, "color", DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph pass 'panel0.bad' has ambiguous producer declarations for resource "
           "'color'") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the internal FramePlan graph rejects attachment usage mismatches.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_missing_usage(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.usage", 14);
    ANN(plan);

    DvzFrameGraphResource resource = {0};
    dvz_strlcpy(resource.id, "tex.sampled.only", sizeof(resource.id));
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &resource));

    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(attachment.resource_id, "tex.sampled.only", sizeof(attachment.resource_id));
    attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, "panel0.render", sizeof(pass.id));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass, &attachment));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph resource 'tex.sampled.only' is missing usage for color_attachment") ==
       0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the internal FramePlan graph rejects non-renderable color attachments.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_attachment_kind(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.kind", 15);
    ANN(plan);

    DvzFrameGraphResource resource = {0};
    dvz_strlcpy(resource.id, "buffer.not.renderable", sizeof(resource.id));
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_BUFFER;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIXED;
    resource.width = 64;
    resource.height = 64;
    resource.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &resource));

    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(attachment.resource_id, "buffer.not.renderable", sizeof(attachment.resource_id));
    attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, "panel0.render", sizeof(pass.id));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass, &attachment));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph color attachment resource 'buffer.not.renderable' is not renderable") ==
       0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Ensure the internal FramePlan graph rejects mismatched render attachment extents.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_attachment_extent(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph.extent", 16);
    ANN(plan);

    DvzFrameGraphResource color = {0};
    dvz_strlcpy(color.id, "color.fixed", sizeof(color.id));
    color.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    color.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIXED;
    color.width = 64;
    color.height = 64;
    color.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    color.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &color));

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, "depth.fixed.small", sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIXED;
    depth.width = 32;
    depth.height = 64;
    depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &depth));

    DvzFrameGraphAttachment color_attachment = {0};
    dvz_strlcpy(color_attachment.resource_id, "color.fixed", sizeof(color_attachment.resource_id));
    color_attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    color_attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    color_attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;

    DvzFrameGraphAttachment depth_attachment = {0};
    dvz_strlcpy(
        depth_attachment.resource_id, "depth.fixed.small", sizeof(depth_attachment.resource_id));
    depth_attachment.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    depth_attachment.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    depth_attachment.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    depth_attachment.clear_depth = 1.0f;

    DvzFrameGraphPass pass = {0};
    dvz_strlcpy(pass.id, "panel0.render", sizeof(pass.id));
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass, &color_attachment));
    AT(dvz_frame_graph_pass_depth_attachment(&pass, &depth_attachment));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph depth attachment resource 'depth.fixed.small' extent does not match "
           "color attachment 'color.fixed'") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}


/**
 * Register scene frameplan tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_frame_plan(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("frame-plan");

    TST_CASE(test_scene_capabilities_diagnostics);
    TST_CASE(test_frame_plan_static_render);
    TST_CASE(test_frame_plan_render_pass_roles);
    TST_CASE(test_frame_plan_clear);
    TST_CASE(test_frame_plan_growth_json);
    TST_CASE(test_frame_plan_render_visual_growth);
    TST_CASE(test_frame_plan_render_visual_capacity_limit);
    TST_CASE(test_frame_plan_json_escapes_labels);
    TST_CASE(test_scene_resource_keys);
    TST_CASE(test_frame_plan_render_visual_metadata);
    TST_CASE(test_frame_plan_render_visual_metadata_wgsl_uses_typed_labels);
    TST_CASE(test_frame_plan_render_splat_metadata_wgsl_uses_typed_labels);
    TST_CASE(test_frame_plan_render_primitive_metadata_wgsl_uses_typed_labels);
    TST_CASE(test_frame_plan_render_image_metadata_wgsl_uses_typed_labels);
    TST_CASE(test_frame_plan_render_textured_mesh_metadata_wgsl_uses_typed_labels);
    TST_CASE(test_frame_plan_render_metadata_complete);
    TST_CASE(test_frame_plan_runtime_ignores_unresolved_graph_work);
    TST_CASE(test_frame_plan_render_visual_metadata_diagnostic);
    TST_CASE(test_frame_plan_draw_resource_validation_rejects_short_position);
    TST_CASE(test_frame_plan_dynamic_update);
    TST_CASE(test_frame_plan_texture_upload_json_includes_region);
    TST_CASE(test_frame_plan_texture_upload_json_includes_color_role);
    TST_CASE(test_frame_plan_readbacks);
    TST_CASE(test_frame_plan_query_readback_copy_metadata);
    TST_CASE(test_frame_plan_buffer_to_texture_copy);
    TST_CASE(test_frame_plan_abi_rejects_invalid_structs);
    TST_CASE(test_frame_plan_graph_static_multipass);
    TST_CASE(test_frame_plan_products_schema);
    TST_CASE(test_frame_plan_products_color_successor);
    TST_CASE(test_frame_plan_products_reject_cross_panel);
    TST_CASE(test_frame_plan_products_reject_implicit_samples);
    TST_CASE(test_frame_plan_products_reject_undefined_background);
    TST_CASE(test_frame_plan_products_reject_format_inference);
    TST_CASE(test_frame_plan_products_reject_incoherent_surface_record);
    TST_CASE(test_frame_plan_products_reject_incompatible_concrete_format);
    TST_CASE(test_frame_plan_products_reject_omitted_reader);
    TST_CASE(test_frame_plan_products_reject_intervening_writer);
    TST_CASE(test_frame_plan_products_reject_alias_overlap);
    TST_CASE(test_frame_plan_products_explicit_shader_resolve);
    TST_CASE(test_frame_plan_products_attachment_resolve);
    TST_CASE(test_frame_plan_products_consumer_growth);
    TST_CASE(test_frame_plan_products_reject_orphan_use);
    TST_CASE(test_frame_plan_graph_ascii);
    TST_CASE(test_frame_plan_trace_env);
    TST_CASE(test_frame_plan_graph_dependencies_dump);
    TST_CASE(test_frame_plan_graph_depth_peeling_shape);
    TST_CASE(test_frame_plan_graph_gbuffer_shape);
    TST_CASE(test_frame_plan_graph_validation_read_before_write);
    TST_CASE(test_frame_plan_graph_validation_topological_order);
    TST_CASE(test_frame_plan_graph_validation_ambiguous_producer);
    TST_CASE(test_frame_plan_graph_validation_missing_usage);
    TST_CASE(test_frame_plan_graph_validation_attachment_kind);
    TST_CASE(test_frame_plan_graph_validation_attachment_extent);
    TST_CASE(test_frame_plan_graph_validation_pass_kind);

    return 0;
}
