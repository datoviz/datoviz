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
#include "../_frame_plan.h"
#include "../_frame_plan_emit.h"
#include "../_scene_resource_key.h"
#include "../_scene.h"
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

int test_scene_capabilities_diagnostics(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzCapabilitySnapshot caps = {0};
    DvzCapabilitySnapshot copy = {0};
    dvz_capability_snapshot_default(&caps);
    AT(caps.max_buffer_size > 0);
    AT(caps.max_texture_dimension_2d > 0);

    dvz_capability_snapshot_copy(&copy, &caps);
    AT(copy.max_buffer_size == caps.max_buffer_size);
    AT(copy.max_vertex_buffers == caps.max_vertex_buffers);

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
int test_frame_plan_static_render(TstSuite* suite, TstItem* item)
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
    AT(strstr(json, "\"frame_plan_schema\": \"0.1\"") != NULL);
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


int test_frame_plan_render_pass_roles(TstSuite* suite, TstItem* item)
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
    AT(dvz_frame_plan_render_pass_role(opaque) == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE);
    AT(
        dvz_frame_plan_render_pass_role(accum) ==
        DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION);
    AT(dvz_frame_plan_render_pass_role(resolve) == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE);

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"pass_role\": \"opaque\"") != NULL);
    AT(strstr(json, "\"pass_role\": \"transparent_accumulation\"") != NULL);
    AT(strstr(json, "\"pass_role\": \"wboit_resolve\"") != NULL);

    dvz_frame_plan_json_destroy(json);
    dvz_frame_plan_destroy(plan);
    return 0;
}


int test_frame_plan_clear(TstSuite* suite, TstItem* item)
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


int test_frame_plan_growth_json(TstSuite* suite, TstItem* item)
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


int test_frame_plan_json_escapes_labels(TstSuite* suite, TstItem* item)
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
int test_scene_resource_keys(TstSuite* suite, TstItem* item)
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

    char visual_id[DVZ_SCENE_LABEL_SIZE] = {0};
    char index_id[DVZ_SCENE_LABEL_SIZE] = {0};
    _scene_resource_key_split_visual(key, visual_id, sizeof(visual_id), index_id, sizeof(index_id));
    AT(strcmp(visual_id, "v3") == 0);
    AT(strcmp(index_id, "b7") == 0);

    char tiny[4] = {0};
    AT(!_scene_resource_key_visual_indexed(123, 456, tiny, sizeof(tiny)));
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
int test_frame_plan_render_visual_metadata(TstSuite* suite, TstItem* item)
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
    dvz_capability_snapshot_default(&caps);
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

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
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
int test_frame_plan_render_visual_metadata_diagnostic(TstSuite* suite, TstItem* item)
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
    dvz_capability_snapshot_default(&caps);
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


int test_frame_plan_dynamic_update(TstSuite* suite, TstItem* item)
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


int test_frame_plan_texture_upload_json_includes_region(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture", 11);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "tex.image.rgba", 0, 8, "image.rgba.patch"));
    AT(dvz_frame_plan_upload_set_texture_extent(plan, 2, 1));
    AT(dvz_frame_plan_upload_set_texture_allocation_extent(plan, 4, 4));
    AT(dvz_frame_plan_upload_set_texture_region(plan, 1, 2));

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


int test_frame_plan_readbacks(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.readback", 9);
    ANN(plan);

    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));
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
    AT(strstr(json, "\"src_resource_id\": \"target.panel.0.picking\"") != NULL);
    AT(strstr(json, "\"request_id\": \"request.pick.0\"") != NULL);
    AT(strstr(json, "\"request_id\": \"request.export.0\"") != NULL);

    dvz_frame_plan_json_destroy(json);
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
int test_frame_plan_graph_static_multipass(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.graph", 12);
    ANN(plan);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource depth = {0};
    dvz_strlcpy(depth.id, "panel0.depth.opaque", sizeof(depth.id));
    depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
    depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    AT(dvz_frame_plan_graph_resource(plan, &depth));

    DvzFrameGraphResource accum = {0};
    dvz_strlcpy(accum.id, "panel0.wboit.accum", sizeof(accum.id));
    accum.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    accum.extent_kind = DVZ_FRAME_GRAPH_EXTENT_PANEL;
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


/**
 * Ensure the internal FramePlan graph reports read-before-write mistakes.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_frame_plan_graph_validation_read_before_write(TstSuite* suite, TstItem* item)
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
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_read(&pass, "tex.unwritten", DVZ_FRAME_GRAPH_ACCESS_SAMPLED));
    AT(dvz_frame_plan_graph_pass(plan, &pass));

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    AT(!dvz_frame_plan_graph_validate(plan, &report));
    AT(dvz_diagnostic_report_count(&report) >= 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "FramePlan graph pass reads resource 'tex.unwritten' before any write") == 0);

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
int test_frame_plan_graph_validation_missing_usage(TstSuite* suite, TstItem* item)
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
 * Register scene frameplan tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_frame_plan(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TEST_SIMPLE(test_scene_capabilities_diagnostics);
    TEST_SIMPLE(test_frame_plan_static_render);
    TEST_SIMPLE(test_frame_plan_render_pass_roles);
    TEST_SIMPLE(test_frame_plan_clear);
    TEST_SIMPLE(test_frame_plan_growth_json);
    TEST_SIMPLE(test_frame_plan_json_escapes_labels);
    TEST_SIMPLE(test_scene_resource_keys);
    TEST_SIMPLE(test_frame_plan_render_visual_metadata);
    TEST_SIMPLE(test_frame_plan_render_visual_metadata_diagnostic);
    TEST_SIMPLE(test_frame_plan_dynamic_update);
    TEST_SIMPLE(test_frame_plan_texture_upload_json_includes_region);
    TEST_SIMPLE(test_frame_plan_readbacks);
    TEST_SIMPLE(test_frame_plan_graph_static_multipass);
    TEST_SIMPLE(test_frame_plan_graph_validation_read_before_write);
    TEST_SIMPLE(test_frame_plan_graph_validation_missing_usage);

    return 0;
}
