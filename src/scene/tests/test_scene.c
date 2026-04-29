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
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Read a text fixture file into an owned NUL-terminated string.
 *
 * @param path the fixture path
 * @return the owned fixture contents, or NULL on failure
 */
static char* _read_text_fixture(const char* path)
{
    ANN(path);

    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }
    long size = ftell(file);
    if (size < 0)
    {
        fclose(file);
        return NULL;
    }
    rewind(file);

    char* out = (char*)dvz_malloc((uint64_t)size + 1);
    if (out == NULL)
    {
        fclose(file);
        return NULL;
    }

    size_t nread = fread(out, 1, (size_t)size, file);
    fclose(file);
    if (nread != (size_t)size)
    {
        dvz_free(out);
        return NULL;
    }
    out[size] = '\0';
    return out;
}



/**
 * Assert that a DRP2 stream serializes exactly like a committed fixture.
 *
 * @param stream the command stream
 * @param name the fixture name to use during serialization
 * @param path the committed fixture path
 * @return 0 on success, 1 on mismatch
 */
static int _assert_stream_matches_fixture(
    DvzDrp2CommandStream* stream, const char* name, const char* path)
{
    ANN(stream);
    ANN(name);
    ANN(path);

    char* json = dvz_drp2_stream_json(stream, name);
    ANN(json);

    char* fixture = _read_text_fixture(path);
    ANN(fixture);

    int out = strcmp(json, fixture) == 0 ? 0 : 1;
    dvz_free(fixture);
    dvz_drp2_stream_json_destroy(json);
    return out;
}



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
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 16);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) == DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 6)) ==
       DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateRenderPipeline\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateTexture\"") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);
    AT(strstr(json, "\"command_buffer_ids\": [4]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_static_render_from_c",
           "spec/drp2/fixtures/positive/scene_static_render_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_emit_drp2_static_render_glsl(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.glsl", 15);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &cfg);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 16);

    char* json = dvz_drp2_stream_json(stream, "scene_static_render_glsl_from_c");
    ANN(json);
    AT(strstr(json, "\"format\": \"glsl\"") != NULL);
    AT(strstr(json, "#version 450\\nvoid main()") != NULL);
    AT(strstr(json, "\"format\": \"wgsl\"") == NULL);

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
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 15)) ==
       DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_readback_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CopyTextureToBuffer\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"MAP_READ\"]") != NULL);
    AT(strstr(json, "\"readbacks\": [ { \"buffer_id\": 12, \"offset\": 0, \"size\": 4 } ]") !=
       NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_readback_from_c",
           "spec/drp2/fixtures/positive/scene_readback_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_emit_drp2_dynamic_uploads(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.dynamic.convert", 12);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.dynamic.position", 0, 16, "point.position.update"));
    AT(dvz_frame_plan_upload(plan, "buf.dynamic.color", 0, 16, "point.color.update"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.dynamic.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 3)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 5)) == DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 13)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_dynamic_uploads_from_c");
    ANN(json);
    AT(strstr(json, "\"id\": 20") != NULL);
    AT(strstr(json, "\"id\": 21") != NULL);
    AT(strstr(json, "\"buffer_id\": 20") != NULL);
    AT(strstr(json, "\"buffer_id\": 21") != NULL);
    AT(strstr(json, "\"cmd\": \"Draw\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_dynamic_uploads_from_c",
           "spec/drp2/fixtures/positive/scene_dynamic_uploads_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_emit_drp2_texture_sampling(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture.convert", 13);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "tex.image.rgba", 0, 16, "image.rgba"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.image.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 19);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 3)) == DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 5)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 9)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 14)) == DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 18)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_texture_sampling_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"CreateSampler\"") != NULL);
    AT(strstr(json, "\"cmd\": \"CreateBindGroupLayout\"") != NULL);
    AT(strstr(json, "\"cmd\": \"SetBindGroup\"") != NULL);
    AT(strstr(json, "\"usage\": [\"COPY_DST\", \"TEXTURE_BINDING\"]") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_texture_sampling_from_c",
           "spec/drp2/fixtures/positive/scene_texture_sampling_from_c.json") == 0);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_emit_drp2_compute_assisted(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.compute.convert", 14);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.compute.input", 0, 36, "compute.input"));
    AT(dvz_frame_plan_compute(plan, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(plan, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(plan, "buf.compute.output"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.compute.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    ANN(stream);
    AT(dvz_diagnostic_report_count(&report) == 0);
    DvzDrp2ValidationResult validation = dvz_drp2_validate_stream(stream);
    AT(validation.ok);
    AT(dvz_drp2_stream_count(stream) == 26);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 2)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 4)) == DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 7)) ==
       DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 14)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 17)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 24)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream, 25)) == DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    char* json = dvz_drp2_stream_json(stream, "scene_compute_assisted_from_c");
    ANN(json);
    AT(strstr(json, "\"cmd\": \"BeginComputePass\"") != NULL);
    AT(strstr(json, "\"cmd\": \"DispatchWorkgroups\"") != NULL);
    AT(strstr(json, "\"usage\": [\"VERTEX\", \"STORAGE\"]") != NULL);
    AT(strstr(json, "\"binding_type\": \"storage_buffer\"") != NULL);

    dvz_drp2_stream_json_destroy(json);
    AT(_assert_stream_matches_fixture(
           stream, "scene_compute_assisted_from_c",
           "spec/drp2/fixtures/positive/scene_compute_assisted_from_c.json") == 0);
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
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render_glsl);
    TEST_SIMPLE(test_frame_plan_emit_drp2_readback);
    TEST_SIMPLE(test_frame_plan_emit_drp2_dynamic_uploads);
    TEST_SIMPLE(test_frame_plan_emit_drp2_texture_sampling);
    TEST_SIMPLE(test_frame_plan_emit_drp2_compute_assisted);

    return 0;
}
