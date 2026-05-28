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
#include "_frame_plan.h"
#include "_frame_plan_emit.h"
#include "_scene_resource_key.h"
#include "_scene.h"
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

int test_scene_capabilities_diagnostics(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzCapabilitySnapshot caps = {0};
    DvzCapabilitySnapshot copy = {0};
    dvz_capability_snapshot_default(&caps);
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
 * Ensure runtime scene rendering follows graph pass order instead of render node order.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
static int test_frame_plan_runtime_uses_graph_pass_order(TstContext* suite, const TstCase* item)
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

    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.1", "rt", false, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f},
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE));
    AT(dvz_frame_plan_render_visual(plan, "point"));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));
    AT(dvz_frame_plan_render_panel_role(
        plan, "panel.0", "rt", false, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f},
        DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE));
    AT(dvz_frame_plan_render_visual(plan, "point"));
    AT(dvz_frame_plan_render_visual_metadata(plan, &metadata));

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
    pass0.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass0, &color));
    AT(dvz_frame_plan_graph_pass(plan, &pass0));

    color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    DvzFrameGraphPass pass1 = {0};
    dvz_strlcpy(pass1.id, "panel.1.opaque", sizeof(pass1.id));
    dvz_strlcpy(pass1.panel_id, "panel.1", sizeof(pass1.panel_id));
    dvz_strlcpy(pass1.work_label, "opaque", sizeof(pass1.work_label));
    pass1.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    AT(dvz_frame_graph_pass_color_attachment(&pass1, &color));
    AT(dvz_frame_plan_graph_pass(plan, &pass1));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = 100;
    emit_cfg.target_height = 100;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);

    uint32_t viewport_count = 0;
    float viewport_x[2] = {0};
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_SET_VIEWPORT && viewport_count < 2)
            viewport_x[viewport_count++] = command->u.set_viewport.viewport[0];
    }
    AT(viewport_count == 2);
    AC(viewport_x[0], 0.0f, 1e-6f);
    AC(viewport_x[1], 50.0f, 1e-6f);

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
    dvz_capability_snapshot_default(&caps);
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


int test_frame_plan_readbacks(TstContext* suite, const TstCase* item)
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
    AT(strstr(json, "\"bytes_per_row\": 4") != NULL);
    AT(strstr(json, "\"rows_per_image\": 1") != NULL);
    AT(strstr(json, "\"request_id\": \"request.pick.0\"") != NULL);
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

    DvzFramePlanCopyDesc desc = {
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

    DvzFigure figure = {0};
    AT(!_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NONE, &figure, "graph"));
    AT(_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, &figure, "graph"));
    figure.has_last_frame_plan_trace = true;
    figure.last_frame_plan_trace = "graph";
    AT(!_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, &figure, "graph"));
    AT(_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, &figure, "graph2"));
    AT(_scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_FULL, &figure, "graph"));
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
    rt.format = VK_FORMAT_R8G8B8A8_UNORM;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                     DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    AT(dvz_frame_plan_graph_resource(plan, &rt));

    DvzFrameGraphResource opaque_depth = {0};
    dvz_strlcpy(opaque_depth.id, "panel0.depth.opaque", sizeof(opaque_depth.id));
    opaque_depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    opaque_depth.format = VK_FORMAT_D32_SFLOAT;
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
        resource.format = VK_FORMAT_R16G16B16A16_SFLOAT;
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
    ANN(front_accum);
    AT(strcmp(front_accum->id, "panel0.peel.front_accum") == 0);
    AT(front_accum->format == VK_FORMAT_R16G16B16A16_SFLOAT);
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
    AT(depth->format == VK_FORMAT_D32_SFLOAT);
    AT(depth->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT);
    AT(depth->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);
    AT(strcmp(normal->id, "panel0.gbuffer.normal") == 0);
    AT(normal->format == VK_FORMAT_R16G16B16A16_SFLOAT);
    AT(normal->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT);
    AT(normal->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED);
    AT(strcmp(object->id, "panel0.gbuffer.object_id") == 0);
    AT(object->format == VK_FORMAT_R32_UINT);
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
    TST_CASE(test_frame_plan_json_escapes_labels);
    TST_CASE(test_scene_resource_keys);
    TST_CASE(test_frame_plan_render_visual_metadata);
    TST_CASE(test_frame_plan_runtime_uses_graph_pass_order);
    TST_CASE(test_frame_plan_render_visual_metadata_diagnostic);
    TST_CASE(test_frame_plan_draw_resource_validation_rejects_short_position);
    TST_CASE(test_frame_plan_dynamic_update);
    TST_CASE(test_frame_plan_texture_upload_json_includes_region);
    TST_CASE(test_frame_plan_readbacks);
    TST_CASE(test_frame_plan_query_readback_copy_metadata);
    TST_CASE(test_frame_plan_graph_static_multipass);
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
