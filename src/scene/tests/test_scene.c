/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
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



int test_frame_plan_emit_drp2_static_render(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert", 10);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream) == 16);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) == DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 6)) ==
       DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateRenderPipeline\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateTexture\"") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);
    AT(strstr(json, "\"command_buffer_ids\": [4]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_emit_drp2_readback(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.readback.convert", 11);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) ==
       DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_readback");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CopyTextureToBuffer\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"MAP_READ\"]") != NULL);
    AT(strstr(json, "\"readbacks\": [ { \"buffer_id\": 12, \"offset\": 0, \"size\": 4 } ]") !=
       NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_scene(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "scene";

    TEST_SIMPLE(test_scene_capabilities_diagnostics);
    TEST_SIMPLE(test_frame_plan_static_render);
    TEST_SIMPLE(test_frame_plan_dynamic_update);
    TEST_SIMPLE(test_frame_plan_readbacks);
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render);
    TEST_SIMPLE(test_frame_plan_emit_drp2_readback);

    return 0;
}
