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

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../_scene.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/arcball.h"
#include "datoviz/scene/panzoom.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
#include "_log.h"
#include "datoviz/canvas.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "datoviz/window.h"
#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
#include "datoviz/app.h"
#endif

bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _captured_log_contains(const TstSuite* suite, const char* needle)
{
    ANN(suite);
    ANN(needle);
    for (uint32_t i = 0; i < tst_log_capture_count(suite); i++)
    {
        const TstLogRecord* rec = tst_log_capture_get(suite, i);
        if (rec != NULL && strstr(rec->message, needle) != NULL)
            return true;
    }
    return false;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
typedef struct SceneCanvasDrawState SceneCanvasDrawState;

struct SceneCanvasDrawState
{
    DvzFramePlanEmitter* emitter;
    DvzDrp2Runtime* runtime;
    DvzCapabilitySnapshot caps;
    DvzFramePlanEmitConfig emit_cfg;
    uint32_t callback_count;
    bool attach_ok;
    bool emit_ok;
    bool direct_target_ok;
    bool execute_ok;
};


/**
 * Probe whether the current runtime can create a Vulkan instance for scene runtime tests.
 *
 * @return true when the runtime can create a Vulkan instance, false otherwise
 */
static bool _scene_vklite_runtime_available(void)
{
    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        log_warn("scene vklite runtime test skipped because Vulkan instance creation failed");
        return false;
    }
    dvz_instance_destroy(instance);
    return true;
}


/**
 * Draw one scene frame through DRP2 and copy its runtime target into the canvas frame.
 *
 * @param canvas the canvas
 * @param frame the borrowed stream frame
 * @param user_data callback state
 */
static void _scene_canvas_drp2_draw(
    DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    SceneCanvasDrawState* state = (SceneCanvasDrawState*)user_data;
    ANN(state);

    DvzFramePlan* plan = dvz_frame_plan("figure.canvas.offscreen", state->callback_count);
    if (plan == NULL)
        return;

    state->attach_ok = dvz_drp2_runtime_attach_frame_target(state->runtime, 1, frame);
    state->emit_ok = dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position") &&
                     dvz_frame_plan_render(
                         plan, "panel.0", "target.panel.0.color", false) &&
                     dvz_frame_plan_render_visual(plan, "visual.point.0");

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = NULL;
    if (state->attach_ok && state->emit_ok)
    {
        stream = dvz_frame_plan_emitter_emit_drp2(
            state->emitter, plan, &state->caps, &report, &state->emit_cfg);
        state->emit_ok = stream != NULL && dvz_diagnostic_report_count(&report) == 0;
        state->direct_target_ok = state->emit_ok;
        for (uint32_t i = 0; state->direct_target_ok && i < dvz_drp2_stream_count(stream); i++)
        {
            state->direct_target_ok =
                dvz_drp2_command_type(dvz_drp2_stream_get(stream, i)) !=
                DVZ_DRP2_COMMAND_CREATE_TEXTURE;
        }
    }
    if (state->emit_ok)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(state->runtime, stream);
        state->execute_ok = result.ok && result.code == DVZ_DRP2_VALIDATION_OK;
    }

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    state->callback_count++;
}
#endif


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



/**
 * Count WRITE_BUFFER commands in a DRP2 stream.
 *
 * @param stream the command stream
 * @return number of WRITE_BUFFER commands
 */
static uint32_t _stream_write_buffer_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
            count++;
    }
    return count;
}



/**
 * Count WRITE_BUFFER commands matching a byte range in a DRP2 stream.
 *
 * @param stream the command stream
 * @param offset expected byte offset
 * @param size expected byte size
 * @return number of matching WRITE_BUFFER commands
 */
static uint32_t _stream_write_buffer_range_count(
    const DvzDrp2CommandStream* stream, uint64_t offset, uint64_t size)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.offset == offset &&
            cmd->u.write_buffer.size == size)
        {
            count++;
        }
    }
    return count;
}



/**
 * Count DRAW commands in a DRP2 stream.
 *
 * @param stream the command stream
 * @return number of DRAW commands
 */
static uint32_t _stream_draw_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            count++;
    }
    return count;
}



/**
 * Count SET_VERTEX_BUFFER commands in a DRP2 stream.
 *
 * @param stream the command stream
 * @return number of SET_VERTEX_BUFFER commands
 */
static uint32_t _stream_set_vertex_buffer_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER)
            count++;
    }
    return count;
}



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

int test_panzoom_create_reset(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);
    ANN(pz);
    AT(pz->zoom[0] == 1.0f);
    AT(pz->zoom[1] == 1.0f);
    AT(pz->pan[0] == 0.0f);
    AT(pz->pan[1] == 0.0f);

    /* Modify state then reset. */
    dvz_panzoom_pan(pz, (vec2){0.5f, -0.3f});
    dvz_panzoom_zoom(pz, (vec2){2.0f, 2.0f});
    dvz_panzoom_reset(pz);

    AT(pz->pan[0] == 0.0f);
    AT(pz->pan[1] == 0.0f);
    AT(pz->zoom[0] == 1.0f);
    AT(pz->zoom[1] == 1.0f);

    dvz_panzoom_destroy(pz);
    return 0;
}



int test_panzoom_pan_shift(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);

    /* Shift by half the viewport width → pan[0] should move by 1.0 NDC unit (at zoom=1). */
    dvz_panzoom_pan_shift(pz, (vec2){400.0f, 0.0f}, (vec2){0, 0});
    /* shift[0] = 2 * 400 / 800 = 1.0; pan[0] = pan_center[0] + 1.0 / zoom[0] = 1.0 */
    AT(fabsf(pz->pan[0] - 1.0f) < 1e-5f);
    AT(fabsf(pz->pan[1]) < 1e-5f);

    dvz_panzoom_destroy(pz);
    return 0;
}



int test_panzoom_zoom_wheel(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 800.0f, 0);

    /* Positive wheel delta → zoom in → zoom factor > 1. */
    dvz_panzoom_zoom_wheel(pz, (vec2){0.0f, 1.0f}, (vec2){400.0f, 400.0f});
    AT(pz->zoom[0] > 1.0f);
    AT(pz->zoom[1] > 1.0f);

    /* Negative delta on a fresh panzoom → zoom out → zoom factor < 1. */
    DvzPanzoom* pz2 = dvz_panzoom(800.0f, 800.0f, 0);
    dvz_panzoom_zoom_wheel(pz2, (vec2){0.0f, -1.0f}, (vec2){400.0f, 400.0f});
    AT(pz2->zoom[0] < 1.0f);
    AT(pz2->zoom[1] < 1.0f);

    dvz_panzoom_destroy(pz);
    dvz_panzoom_destroy(pz2);
    return 0;
}



int test_panzoom_double_click_resets(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);
    dvz_panzoom_zoom(pz, (vec2){3.0f, 3.0f});
    dvz_panzoom_pan(pz, (vec2){0.5f, 0.5f});

    DvzPointerEvent ev = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    bool consumed = dvz_panzoom_pointer(pz, &ev);
    AT(consumed);
    AT(pz->zoom[0] == 1.0f);
    AT(pz->pan[0] == 0.0f);

    dvz_panzoom_destroy(pz);
    return 0;
}



int test_panzoom_mvp_identity(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    dvz_panzoom_mvp(pz, &mvp);

    /* At identity panzoom the view matrix should place the camera looking down Z. */
    /* view[3][2] (translation z) should be -2 for lookat from (0,0,2) to (0,0,0). */
    AT(fabsf(mvp.view[3][2] - (-2.0f)) < 1e-4f);

    /* At identity panzoom proj maps NDC [-1,1] → [-1,1]. */
    /* For ortho(-1,1,-1,1,-10,10): proj[0][0] = 1, proj[1][1] = 1. */
    AT(fabsf(mvp.proj[0][0] - 1.0f) < 1e-4f);
    AT(fabsf(mvp.proj[1][1] - 1.0f) < 1e-4f);

    dvz_panzoom_destroy(pz);
    return 0;
}



/*************************************************************************************************/
/*  Arcball tests                                                                                */
/*************************************************************************************************/

int test_arcball_create_reset(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 600.0f, 0);
    ANN(arc);

    /* At construction the accumulated matrix should be identity (init angles all zero). */
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}



int test_arcball_rotate_produces_nonidentity_model(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 800.0f, 0);

    /* Simulate a drag: current position != press position → rotation quaternion not identity. */
    dvz_arcball_rotate(arc, (vec2){0.5f, 0.0f}, (vec2){0.0f, 0.0f});

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) != 0);

    dvz_arcball_destroy(arc);
    return 0;
}



int test_arcball_end_commits_rotation(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 800.0f, 0);

    dvz_arcball_rotate(arc, (vec2){0.5f, 0.0f}, (vec2){0.0f, 0.0f});
    mat4 model_before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_before);

    dvz_arcball_end(arc);

    /* After end(), in-flight rotation is identity; mat has been updated.
       dvz_arcball_model should give the same result as before. */
    mat4 model_after = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_after);
    AT(memcmp(model_before, model_after, sizeof(mat4)) == 0);

    /* The in-flight rotation is now identity — another rotate from same positions gives same. */
    versor id = GLM_QUAT_IDENTITY_INIT;
    AT(memcmp(arc->rotation, id, sizeof(versor)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}



int test_arcball_double_click_resets(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 600.0f, 0);

    /* Apply a rotation then commit it. */
    dvz_arcball_rotate(arc, (vec2){0.5f, 0.3f}, (vec2){-0.2f, -0.1f});
    dvz_arcball_end(arc);

    /* Double-click resets. */
    DvzPointerEvent ev = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    bool consumed = dvz_arcball_pointer(arc, &ev);
    AT(consumed);

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}



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



int test_frame_plan_emit_drp2_rejects_unsupported_shader_format(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.unsupported_shader", 17);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = false;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &cfg);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "unsupported shader format") == 0);

    dvz_frame_plan_destroy(plan);
    return 0;
}



int test_frame_plan_emit_drp2_rejects_small_caps(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.small_caps", 18);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    dvz_capability_snapshot_default(&caps);
    caps.max_buffer_size = 8;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(dvz_diagnostic_report_get(&report, 0), "upload buffer exceeds max_buffer_size") == 0);

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_texture_dimension_2d = 3;
    stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_texture_dimension_2d is too small for fixture render target") == 0);

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_vertex_buffers = 0;
    stream = dvz_frame_plan_emit_drp2(plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_vertex_buffers is too small for fixture render pipeline") == 0);

    DvzFramePlan* texture_plan = dvz_frame_plan("figure.convert.small_bind_group_caps", 19);
    ANN(texture_plan);
    AT(dvz_frame_plan_upload(texture_plan, "tex.image.rgba", 0, 16, "image.rgba"));
    AT(dvz_frame_plan_render(texture_plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(texture_plan, "visual.image.0"));

    dvz_diagnostic_report_init(&report);
    dvz_capability_snapshot_default(&caps);
    caps.max_bind_groups = 0;
    stream = dvz_frame_plan_emit_drp2(texture_plan, &caps, &report);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) == 1);
    AT(strcmp(
           dvz_diagnostic_report_get(&report, 0),
           "max_bind_groups is too small for fixture bind groups") == 0);

    dvz_frame_plan_destroy(texture_plan);
    dvz_frame_plan_destroy(plan);
    return 0;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_frame_plan_emit_drp2_static_render_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene GLSL DRP2 execution test skipped because GPU context creation failed");
        return 0;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.convert.glsl.execute", 16);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(plan, "visual.point.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg);
    ANN(stream);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_frame_plan_emit_drp2_readback_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene readback DRP2 execution test skipped because GPU context creation failed");
        return 0;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.readback.glsl.execute", 20);
    ANN(plan);
    AT(dvz_frame_plan_upload(plan, "buf.point.position", 0, 16, "point.position"));
    AT(dvz_frame_plan_render(plan, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(plan, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(plan, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(plan, "buf.pick.readback", "request.pick.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_frame_plan_emit_drp2_ex(plan, &caps, &report, &emit_cfg);
    ANN(stream);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);

    uint8_t downloaded[4] = {0};
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, 12, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_destroy(plan);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_frame_plan_emitter_runtime_two_frames_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "scene runtime-mode DRP2 execution test skipped because GPU context creation failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.point.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.point.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "scene dynamic runtime-mode DRP2 execution test skipped because GPU context creation "
            "failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.dynamic.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.dynamic.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.color", 0, 16, "point.color.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.color", 0, 16, "point.color.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "scene texture runtime-mode DRP2 execution test skipped because GPU context creation "
            "failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "tex.image.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "tex.image.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes(
    TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn(
            "scene compute runtime-mode DRP2 execution test skipped because GPU context creation "
            "failed");
        return 0;
    }

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.compute.glsl.execute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.compute.glsl.execute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.compute.input", 0, 36, "compute.input.0"));
    AT(dvz_frame_plan_compute(frame0, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame0, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame0, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.compute.input", 0, 36, "compute.input.1"));
    AT(dvz_frame_plan_compute(frame1, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame1, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame1, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    uint8_t downloaded[4] = {0};
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(emitter, "_rb");
    AT(rb_id != 0);
    AT(_dvz_drp2_runtime_vklite_download_buffer(runtime, rb_id, 0, 4, downloaded));
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}


int test_scene_drp2_offscreen_canvas_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("scene offscreen canvas test skipped because GPU context creation failed");
        return 0;
    }

    DvzWindowHost* host = dvz_window_host();
    ANN(host);

    DvzWindowConfig window_cfg = dvz_window_default_config();
    window_cfg.title = "scene-drp2-offscreen-canvas";
    window_cfg.width = 64;
    window_cfg.height = 64;
    DvzWindow* window = dvz_window_create(host, DVZ_BACKEND_OFFSCREEN, &window_cfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_OFFSCREEN)
    {
        log_warn("scene offscreen canvas test skipped because headless window creation failed");
        dvz_window_host_destroy(host);
        dvz_gpu_ctx_destroy(ctx);
        return 0;
    }

    DvzCanvasConfig canvas_cfg = dvz_canvas_default_config();
    canvas_cfg.window = window;
    canvas_cfg.device = dvz_gpu_ctx_device(ctx);
    canvas_cfg.render_mode = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    canvas_cfg.timing_history = 2;
    DvzCanvas* canvas = dvz_canvas_create(&canvas_cfg);
    ANN(canvas);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    SceneCanvasDrawState state = {0};
    state.emitter = emitter;
    state.runtime = runtime;
    state.emit_cfg = dvz_frame_plan_emit_config();
    state.emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    state.emit_cfg.external_color_target = true;
    state.emit_cfg.color_target_id = 1;
    state.emit_cfg.fullscreen_triangle = true;
    dvz_capability_snapshot_default(&state.caps);

    dvz_canvas_set_draw_callback(canvas, _scene_canvas_drp2_draw, &state);
    AT(dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY);
    AT(state.callback_count == 1);
    AT(state.attach_ok);
    AT(state.emit_ok);
    AT(state.direct_target_ok);
    AT(state.execute_ok);
    AT(dvz_canvas_submit(canvas) == 0);
    AT(dvz_canvas_offscreen_runtime_state(canvas) == DVZ_CANVAS_OFFSCREEN_STATE_READY);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200 && pixel[2] > 200 && pixel[3] > 200)
        {
            bright_count++;
        }
    }
    AT(bright_count > (width * height) / 2);
    dvz_free(rgba);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_frame_plan_emitter_destroy(emitter);
    dvz_drp2_runtime_destroy(runtime);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



int test_scene_point_emit_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_scene_point_emit_glsl_executes skipped: GPU context creation failed");
        return 0;
    }

    /* Build scene */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3]     = {10.0f, 20.0f, 15.0f};

    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Emit with GLSL */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    /* Execute on GPU */
    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



static int _scene_primitive_emit_executes(DvzPrimitiveTopology topology, uint32_t vertex_count)
{
    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_primitive(scene, topology, 0);
    AT(visual != NULL);

    /* Build vertex_count positions on a unit triangle / strip path; details don't matter. */
    float* positions = dvz_calloc(vertex_count * 3, sizeof(float));
    uint8_t (*colors)[4] = dvz_calloc(vertex_count, 4);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        positions[i * 3 + 0] = (float)i / (float)vertex_count - 0.5f;
        positions[i * 3 + 1] = (i % 2 == 0) ? -0.4f : 0.4f;
        positions[i * 3 + 2] = 0.0f;
        colors[i][0] = (uint8_t)(255 * i / vertex_count);
        colors[i][1] = 128;
        colors[i][2] = (uint8_t)(255 - 255 * i / vertex_count);
        colors[i][3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, vertex_count) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, vertex_count) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    dvz_free(positions);
    dvz_free(colors);
    return 0;
}



int test_scene_primitive_triangle_list_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
}



int test_scene_primitive_line_strip_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_primitive_emit_executes(DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP, 4);
}



int test_scene_image_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* TRIANGLE_STRIP: TL, BL, TR, BR */
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f},
        {-0.5f,  0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.5f,  0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    /* 4x4 RGBA8 checker pattern. */
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t y = 0; y < 4; y++)
    {
        for (uint32_t x = 0; x < 4; x++)
        {
            uint32_t i = (y * 4 + x) * 4;
            uint8_t v = ((x ^ y) & 1) ? 255 : 0;
            pixels[i+0] = v; pixels[i+1] = v; pixels[i+2] = v; pixels[i+3] = 255;
        }
    }

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Build scene */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f};
    uint8_t colors[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 20.0f, 15.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Create app and offscreen window */
    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    /* Run two frames */
    dvz_app_run(app, 2);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



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
#endif



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



int test_frame_plan_emitter_runtime_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.point.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.point.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.pickable.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 18);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 0)) ==
       DVZ_DRP2_COMMAND_HELLO_RENDERER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 16)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 10);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 8)) ==
       DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 9)) ==
       DVZ_DRP2_COMMAND_QUEUE_SUBMIT);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.dynamic", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.dynamic", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_upload(frame0, "buf.dynamic.color", 0, 16, "point.color.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.position", 0, 16, "point.position.1"));
    AT(dvz_frame_plan_upload(frame1, "buf.dynamic.color", 0, 16, "point.color.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.dynamic.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 21);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 14)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 12);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 5)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 6)) ==
       DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_dynamic_grow_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.grow", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.grow", 1);
    ANN(frame0);
    ANN(frame1);

    AT(dvz_frame_plan_upload(frame0, "buf.grow.position", 0, 16, "point.position.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame0, "visual.grow.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.grow.position", 0, 256, "point.position.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.color", false));
    AT(dvz_frame_plan_render_visual(frame1, "visual.grow.0"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}



int test_frame_plan_emitter_runtime_texture_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.texture", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.texture", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "tex.image.rgba", 0, 16, "image.rgba.0"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "tex.image.rgba", 0, 16, "image.rgba.1"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.image.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 21);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_SAMPLER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 5)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 9)) ==
       DVZ_DRP2_COMMAND_CREATE_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 10);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_TEXTURE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 4)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}


int test_frame_plan_emitter_runtime_compute_two_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    ANN(emitter);

    DvzFramePlan* frame0 = dvz_frame_plan("figure.runtime.compute", 0);
    DvzFramePlan* frame1 = dvz_frame_plan("figure.runtime.compute", 1);
    ANN(frame0);
    ANN(frame1);
    AT(dvz_frame_plan_upload(frame0, "buf.compute.input", 0, 36, "compute.input.0"));
    AT(dvz_frame_plan_compute(frame0, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame0, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame0, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame0, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame0, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame0, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame0, "buf.pick.readback", "request.pick.0"));

    AT(dvz_frame_plan_upload(frame1, "buf.compute.input", 0, 36, "compute.input.1"));
    AT(dvz_frame_plan_compute(frame1, "copy_positions", 1, 1, 1));
    AT(dvz_frame_plan_compute_read(frame1, "buf.compute.input"));
    AT(dvz_frame_plan_compute_write(frame1, "buf.compute.output"));
    AT(dvz_frame_plan_render(frame1, "panel.0", "target.panel.0.picking", true));
    AT(dvz_frame_plan_render_visual(frame1, "visual.compute.0"));
    AT(dvz_frame_plan_copy(frame1, "target.panel.0.picking", "buf.pick.readback", 4));
    AT(dvz_frame_plan_readback(frame1, "buf.pick.readback", "request.pick.1"));

    DvzCapabilitySnapshot caps = {0};
    DvzDiagnosticReport report = {0};
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    dvz_capability_snapshot_default(&caps);
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame0, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream0) == 28);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 2)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 4)) ==
       DVZ_DRP2_COMMAND_CREATE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 7)) ==
       DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 15)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream0, 18)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 =
        dvz_frame_plan_emitter_emit_drp2(emitter, frame1, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(dvz_drp2_stream_count(stream1) == 15);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 0)) ==
       DVZ_DRP2_COMMAND_WRITE_BUFFER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 1)) ==
       DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 4)) ==
       DVZ_DRP2_COMMAND_SET_BIND_GROUP);
    AT(dvz_drp2_command_type(dvz_drp2_stream_get(stream1, 5)) ==
       DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    runtime_cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
    AT(result.ok);
    result = dvz_drp2_runtime_execute(runtime, stream1);
    AT(result.ok);

    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream1);
    dvz_drp2_stream_destroy(stream0);
    dvz_frame_plan_destroy(frame1);
    dvz_frame_plan_destroy(frame0);
    dvz_frame_plan_emitter_destroy(emitter);
    return 0;
}



/*************************************************************************************************/
/*  Scene graph tests                                                                            */
/*************************************************************************************************/

int test_scene_json(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene*  scene  = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel*  panel  = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    DvzVisual* visual = dvz_point(scene, 0);

    float positions[] = {-0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f};
    dvz_visual_set_data(visual, "position", positions, 2);
    dvz_panel_add_visual(panel, visual, NULL);

    char* json = dvz_scene_json(scene);
    AT(json != NULL);
    AT(strstr(json, "\"figures\"") != NULL);
    AT(strstr(json, "\"fig0\"") != NULL);
    AT(strstr(json, "\"point\"") != NULL);
    AT(strstr(json, "\"position\"") != NULL);
    AT(strstr(json, "\"item_count\":2") != NULL);
    AT(strstr(json, "\"data\":\"") != NULL); /* base64 data present */

    dvz_scene_json_destroy(json);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_z_layer_orders_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* Two point visuals on one panel: behind=3 verts (z=-1), front=5 verts (z=+1).
     * Add front first, behind second, so insertion order ≠ z order. After stable
     * sort by z_layer the first vertex-buffer slot should be the behind-visual's
     * position buffer, and the converter's single Draw should pick its vertex_count.
     * (The converter currently emits one composite Draw per panel; this test verifies
     * the z-sorted visual is the one that drives the draw, which is enough to confirm
     * the sort took effect.) */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    float pos5[5 * 3] = {0};
    float pos3[3 * 3] = {0};
    DvzColor col[5] = {0};
    float sz[5] = {0};

    DvzVisual* v_front  = dvz_point(scene, 0);  /* z=+1, 5 verts */
    DvzVisual* v_behind = dvz_point(scene, 0);  /* z=-1, 3 verts */

    AT(dvz_visual_set_data(v_front, "position", pos5, 5) == 0);
    AT(dvz_visual_set_data(v_front, "color",    col,  5) == 0);
    AT(dvz_visual_set_data(v_front, "size",     sz,   5) == 0);
    AT(dvz_visual_set_data(v_behind, "position", pos3, 3) == 0);
    AT(dvz_visual_set_data(v_behind, "color",    col,  3) == 0);
    AT(dvz_visual_set_data(v_behind, "size",     sz,   3) == 0);

    AT(dvz_panel_add_visual(panel, v_front,  &(DvzVisualAttachDesc){.z_layer = +1}) == 0);
    AT(dvz_panel_add_visual(panel, v_behind, &(DvzVisualAttachDesc){.z_layer = -1}) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char* json = dvz_drp2_stream_json(stream, "z_layer_order");
    ANN(json);

    /* Single Draw per panel — its vertex_count comes from the first sorted visual's
     * position buffer. With z-sort: behind (3 verts) is first → vertex_count == 3.
     * Without z-sort (insertion order): front (5 verts) would lead → vertex_count == 5. */
    const char* draw3 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 3");
    const char* draw5 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 5");
    AT(draw3 != NULL);
    AT(draw5 == NULL);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_rejects_cross_scene_visual(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene_a = dvz_scene();
    DvzScene* scene_b = dvz_scene();
    ANN(scene_a);
    ANN(scene_b);

    DvzFigure* figure = dvz_figure(scene_a, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* foreign = dvz_point(scene_b, 0);
    ANN(foreign);

    AT(dvz_panel_add_visual(panel, foreign, NULL) == -1);

    dvz_scene_destroy(scene_b);
    dvz_scene_destroy(scene_a);
    return 0;
}



int test_scene_rejects_unsupported_point_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float opacity[2] = {0.25f, 0.75f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "opacity", opacity, 2) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'opacity'"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_point_rejects_texcoords_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float uv[2] = {0.0f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "texcoords", uv, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported point visual attribute 'texcoords'"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_primitive_rejects_size_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported primitive visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_rejects_size_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported image visual attribute 'size'"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_emit_warns_visual_with_no_position(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    /* Emit with no position set — should warn but not crash. */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    tst_log_capture_begin(suite);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(_captured_log_contains(suite, "has no 'position' data"));

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_rejects_mismatched_point_attribute_counts(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {
        -0.25f, 0.00f, 0.0f,
         0.25f, 0.00f, 0.0f,
    };
    DvzColor color = {255, 0, 0, 255};

    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "color", &color, 1) == -1);
    AT(_captured_log_contains(suite, "item_count 1 does not match existing attribute 'position'"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_rejects_range_update_without_full_allocation(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float update[3] = {0.5f, 0.0f, 0.0f};

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data_range(visual, "position", update, 0, 1) == -1);
    AT(_captured_log_contains(suite, "range update requires prior full allocation"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_rejects_mutation_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(scene->outstanding_emitted_streams == 1);

    float update[2 * 3] = {-0.5f, 0.1f, 0.0f, 0.5f, 0.1f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "position", update, 2) == -1);
    AT(_captured_log_contains(suite, "cannot mutate scene visual data while an emitted stream is still live"));

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);
    AT(dvz_visual_set_data(visual, "position", update, 2) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_rejects_range_mutation_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[4 * 3] = {-0.75f, 0.0f, 0.0f, -0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f};
    DvzColor colors[4] = {{255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}};
    float sizes[4] = {8.0f, 8.0f, 8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    float update[2 * 3] = {-0.1f, 0.2f, 0.0f, 0.1f, 0.2f, 0.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data_range(visual, "position", update, 1, 2) == -1);
    AT(_captured_log_contains(suite, "cannot mutate scene visual data while an emitted stream is still live"));

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);
    AT(dvz_visual_set_data_range(visual, "position", update, 1, 2) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_rejects_destroy_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 12.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    dvz_scene_destroy(scene);
    AT(tst_expect_error_end(suite) == 0);
    AT(_captured_log_contains(suite, "cannot destroy scene-owned visual data while an emitted stream is still live"));
    AT(scene->outstanding_emitted_streams == 1);

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);
    dvz_scene_destroy(scene);
    return 0;
}



static int
test_scene_rejects_visual_destroy_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(visual->scene == scene);
    AT(visual->attr_count == 3);
    for (uint32_t i = 0; i < visual->attr_count; i++)
        AT(visual->attrs[i].data != NULL);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    dvz_visual_destroy(visual);
    AT(tst_expect_error_end(suite) == 0);
    AT(_captured_log_contains(suite, "cannot destroy scene-owned visual data while an emitted stream is still live"));
    AT(scene->outstanding_emitted_streams == 1);
    AT(visual->scene == scene);
    AT(visual->attr_count == 3);
    for (uint32_t i = 0; i < visual->attr_count; i++)
        AT(visual->attrs[i].data != NULL);

    dvz_drp2_stream_destroy(stream);
    AT(scene->outstanding_emitted_streams == 0);

    dvz_visual_destroy(visual);
    AT(visual->scene == NULL);
    AT(visual->attr_count == 0);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_live_stream_count_tracks_multiple_emits(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    float positions[2 * 3] = {-0.25f, 0.0f, 0.0f, 0.25f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(stream1 != NULL);
    AT(scene->outstanding_emitted_streams == 1);

    float update[2] = {9.0f, 10.0f};
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_range(visual, "size", update, 0, 2) == -1);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(stream2 != NULL);
    AT(scene->outstanding_emitted_streams == 2);

    dvz_drp2_stream_destroy(stream1);
    AT(scene->outstanding_emitted_streams == 1);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data_range(visual, "size", update, 0, 2) == -1);

    dvz_drp2_stream_destroy(stream2);
    AT(scene->outstanding_emitted_streams == 0);
    AT(dvz_visual_set_data_range(visual, "size", update, 0, 2) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_point_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* Build a minimal scene: one figure, one full-frame panel, one point visual. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    AT(figure != NULL);

    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);

    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3]     = {10.0f, 20.0f, 15.0f};

    int rc = dvz_visual_set_data(visual, "position", positions, 3);
    AT(rc == 0);
    rc = dvz_visual_set_data(visual, "color", colors, 3);
    AT(rc == 0);
    rc = dvz_visual_set_data(visual, "size", sizes, 3);
    AT(rc == 0);

    rc = dvz_panel_add_visual(panel, visual, NULL);
    AT(rc == 0);

    /* Emit the DRP2 command stream. */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;
    caps.max_vertex_buffers = 8;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);

    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    AT(dvz_drp2_stream_count(stream) > 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    memset(pixels, 128, sizeof(pixels));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);
    AT(dvz_drp2_stream_count(stream) > 0);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Check that an empty panel emits an explicit clear-only render pass.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_scene_empty_figure_emit_clear_only(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.clear_color[0] = 0.05f;
    emit_cfg.clear_color[1] = 0.06f;
    emit_cfg.clear_color[2] = 0.07f;
    emit_cfg.clear_color[3] = 1.0f;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool found_begin = false;
    bool found_end = false;
    bool found_draw = false;
    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd == NULL)
            continue;
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            found_begin = true;
            AC(cmd->u.begin_render_pass.clear_color[0], 0.05f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[1], 0.06f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[2], 0.07f, 1e-6f);
            AC(cmd->u.begin_render_pass.clear_color[3], 1.0f, 1e-6f);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_END_RENDER_PASS)
        {
            found_end = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
        {
            found_draw = true;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
        {
            found_pipeline = true;
        }
    }
    AT(found_begin);
    AT(found_end);
    AT(!found_draw);
    AT(!found_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/* ---- New regression tests ---- */

int test_scene_point_emit_has_vertex_layout(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f};
    DvzColor colors[3] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    float sizes[3] = {10.0f, 10.0f, 10.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 3) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    /* Find the CREATE_RENDER_PIPELINE command and verify it has vertex layout. */
    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.binding_count > 0);
            AT(cmd->u.create_render_pipeline.attr_count > 0);
            AT(cmd->u.create_render_pipeline.binding_strides[0] > 0);
            break;
        }
    }
    AT(found_pipeline);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Build scene with ONE large yellow point at center. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255}; /* yellow */
    float size = 32.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    /* Count pixels with r>200 && g>200 (yellow-ish from the point). */
    uint32_t yellow_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] > 200)
            yellow_count++;
    }
    AT(yellow_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_image_has_nonblank_pixels(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    /* Large quad covering most of the panel. TRIANGLE_STRIP order: TL, BL, TR, BR */
    float positions[4][3] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };

    /* Solid red 4x4 texture. */
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[i * 4 + 0] = 255;
        pixels[i * 4 + 1] = 0;
        pixels[i * 4 + 2] = 0;
        pixels[i * 4 + 3] = 255;
    }

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_has_nonblank_pixels skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);
    AT(width == 64);
    AT(height == 64);

    /* Count pixels that are red-dominant (from the solid red texture). */
    uint32_t red_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* pixel = &rgba[4 * i];
        if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50)
            red_count++;
    }
    AT(red_count > 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_retained_render_second_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor color = {255, 255, 0, 255};
    float size = 32.0f;
    AT(dvz_visual_set_data(visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(visual, "color", &color, 1) == 0);
    AT(dvz_visual_set_data(visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_retained_render_second_frame skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t yellow_counts[2] = {0, 0};
    for (uint32_t frame = 0; frame < 2; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 64);
        AT(height == 64);

        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* pixel = &rgba[4 * i];
            if (pixel[0] > 200 && pixel[1] > 200)
                yellow_counts[frame]++;
        }
        dvz_free(rgba);
    }
    AT(yellow_counts[0] > 0);
    AT(yellow_counts[1] > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_image_retained_render_second_frame(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_image(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f},
        { 0.9f, -0.9f, 0.0f}, { 0.9f, 0.9f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4];
    for (uint32_t i = 0; i < 4 * 4; i++)
    {
        pixels[i * 4 + 0] = 255; pixels[i * 4 + 1] = 0;
        pixels[i * 4 + 2] = 0;   pixels[i * 4 + 3] = 255;
    }
    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_image_retained_render_second_frame skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Both frames should show red pixels from the retained texture. */
    uint32_t red_counts[2] = {0, 0};
    for (uint32_t frame = 0; frame < 2; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 64);
        AT(height == 64);

        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* pixel = &rgba[4 * i];
            if (pixel[0] > 200 && pixel[1] < 50 && pixel[2] < 50)
                red_counts[frame]++;
        }
        dvz_free(rgba);
    }
    AT(red_counts[0] > 0);
    AT(red_counts[1] > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_two_panel_points_light_both_halves(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    AT(figure != NULL);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(left != NULL);
    AT(right != NULL);

    DvzVisual* left_visual = dvz_point(scene, 0);
    DvzVisual* right_visual = dvz_point(scene, 0);
    AT(left_visual != NULL);
    AT(right_visual != NULL);

    float position[3] = {0.0f, 0.0f, 0.0f};
    DvzColor red = {255, 32, 32, 255};
    DvzColor green = {32, 255, 32, 255};
    float size = 24.0f;

    AT(dvz_visual_set_data(left_visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(left_visual, "color", &red, 1) == 0);
    AT(dvz_visual_set_data(left_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(left, left_visual, NULL) == 0);

    AT(dvz_visual_set_data(right_visual, "position", position, 1) == 0);
    AT(dvz_visual_set_data(right_visual, "color", &green, 1) == 0);
    AT(dvz_visual_set_data(right_visual, "size", &size, 1) == 0);
    AT(dvz_panel_add_visual(right, right_visual, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_two_panel_points_light_both_halves skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    AT(win != NULL);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t red_count = 0;
    uint32_t green_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);
        AT(width == 96);
        AT(height == 64);

        red_count = 0;
        green_count = 0;
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint8_t* pixel = &rgba[4 * (y * width + x)];
                if (pixel[0] > 150 && pixel[0] > pixel[1] + 40)
                    red_count++;
                if (pixel[1] > 150 && pixel[1] > pixel[0] + 40)
                    green_count++;
            }
        }
        dvz_free(rgba);
        if (red_count > 0 && green_count > 0)
            break;
    }
    AT(red_count > 0);
    AT(green_count > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
int test_scene_point_large_count_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_scene_point_large_count_executes skipped: GPU context creation failed");
        return 0;
    }

    /* 1000 points — same as hello_scatter, exercises large buffer upload path. */
    const uint32_t N = 1000;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float* positions = (float*)dvz_malloc(N * 3 * sizeof(float));
    DvzColor* colors = (DvzColor*)dvz_malloc(N * sizeof(DvzColor));
    float* sizes    = (float*)dvz_malloc(N * sizeof(float));
    ANN(positions); ANN(colors); ANN(sizes);

    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i + 0] = -1.0f + 2.0f * (float)i / (float)(N - 1);
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0] = 255;
        colors[i][1] = (uint8_t)(i % 256);
        colors[i][2] = 0;
        colors[i][3] = 255;
        sizes[i] = 4.0f;
    }

    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(sizes);
    dvz_drp2_runtime_destroy(runtime);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
#endif



int test_scene_second_emit_no_uploads_when_not_dirty(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    float positions[] = {-0.5f, 0.0f, 0.0f,  0.5f, 0.0f, 0.0f};
    DvzColor colors[2] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    float sizes[2] = {8.0f, 8.0f};
    AT(dvz_visual_set_data(visual, "position", positions, 2) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 2) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, 2) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    /* First emit — dirty, must produce WRITE_BUFFER commands. */
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);

    uint32_t wb_count1 = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
            wb_count1++;
    }
    AT(wb_count1 > 0);
    dvz_drp2_stream_destroy(stream1);

    /* Second emit — nothing dirty, so no WRITE_BUFFER commands should be emitted. */
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);

    uint32_t wb_count2 = 0;
    bool found_draw = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
            wb_count2++;
        else if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            found_draw = true;
    }
    AT(wb_count2 == 0);
    AT(found_draw);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_partial_update_uploads_only_range(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    const uint32_t N = 20;
    float positions[20 * 3];
    DvzColor colors[20];
    float sizes[20];
    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i]     = (float)i / (float)N * 2.0f - 1.0f;
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0] = 255; colors[i][1] = 0; colors[i][2] = 0; colors[i][3] = 255;
        sizes[i] = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    /* First emit clears dirty flags. */
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);

    /* Partial update: items 5–9 only (first_item=5, item_count=5). */
    float new_pos[5 * 3];
    for (uint32_t i = 0; i < 5; i++)
    {
        new_pos[3 * i]     = 0.5f;
        new_pos[3 * i + 1] = 0.5f;
        new_pos[3 * i + 2] = 0.0f;
    }
    AT(dvz_visual_set_data_range(visual, "position", new_pos, 5, 5) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(stream2 != NULL);

    /* Find the position WRITE_BUFFER and verify it covers only the partial range. */
    /* Position attribute size = 3 floats × 4 bytes = 12 bytes per item. */
    const uint64_t item_size    = 3 * sizeof(float);
    const uint64_t expected_off = 5 * item_size;        /* items 0-4 untouched */
    const uint64_t expected_sz  = 5 * item_size;        /* 5 items updated     */
    bool found_partial = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream2); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream2, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.offset == expected_off &&
            cmd->u.write_buffer.size == expected_sz)
        {
            found_partial = true;
            break;
        }
    }
    AT(found_partial);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_repeated_partial_updates_across_frames(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    const uint32_t N = 20;
    float positions[20 * 3];
    DvzColor colors[20];
    float sizes[20];
    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i]     = (float)i / (float)N * 2.0f - 1.0f;
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0]         = 255;
        colors[i][1]         = 0;
        colors[i][2]         = 0;
        colors[i][3]         = 255;
        sizes[i]             = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);

    const uint64_t item_size = 3 * sizeof(float);

    float frame2_pos[3 * 3] = {
        -0.25f, 0.25f, 0.0f,
        -0.15f, 0.25f, 0.0f,
        -0.05f, 0.25f, 0.0f,
    };
    uint64_t frame2_offset = 2 * item_size;
    uint64_t frame2_size = 3 * item_size;
    AT(dvz_visual_set_data_range(visual, "position", frame2_pos, 2, 3) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, frame2_offset, frame2_size) == 1);
    dvz_drp2_stream_destroy(stream2);

    float frame3_pos[2 * 3] = {
        0.25f, -0.25f, 0.0f,
        0.35f, -0.25f, 0.0f,
    };
    uint64_t frame3_offset = 10 * item_size;
    uint64_t frame3_size = 2 * item_size;
    AT(dvz_visual_set_data_range(visual, "position", frame3_pos, 10, 2) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream3 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream3 != NULL);
    AT(_stream_write_buffer_count(stream3) == 1);
    AT(_stream_write_buffer_range_count(stream3, frame2_offset, frame2_size) == 0);
    AT(_stream_write_buffer_range_count(stream3, frame3_offset, frame3_size) == 1);

    dvz_drp2_stream_destroy(stream3);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_partial_update_merges_ranges_before_emit(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    DvzVisual* visual = dvz_point(scene, 0);
    AT(visual != NULL);

    const uint32_t N = 20;
    float positions[20 * 3];
    DvzColor colors[20];
    float sizes[20];
    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i]     = (float)i / (float)N * 2.0f - 1.0f;
        positions[3 * i + 1] = 0.0f;
        positions[3 * i + 2] = 0.0f;
        colors[i][0]         = 0;
        colors[i][1]         = 255;
        colors[i][2]         = 0;
        colors[i][3]         = 255;
        sizes[i]             = 5.0f;
    }
    AT(dvz_visual_set_data(visual, "position", positions, N) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, N) == 0);
    AT(dvz_visual_set_data(visual, "size", sizes, N) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    dvz_drp2_stream_destroy(stream1);

    float update_a[2 * 3] = {
        -0.75f, 0.1f, 0.0f,
        -0.65f, 0.1f, 0.0f,
    };
    float update_b[3 * 3] = {
        0.15f, 0.1f, 0.0f,
        0.25f, 0.1f, 0.0f,
        0.35f, 0.1f, 0.0f,
    };
    AT(dvz_visual_set_data_range(visual, "position", update_a, 2, 2) == 0);
    AT(dvz_visual_set_data_range(visual, "position", update_b, 8, 3) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);

    const uint64_t item_size = 3 * sizeof(float);
    const uint64_t expected_offset = 2 * item_size;
    const uint64_t expected_size = 9 * item_size;
    AT(_stream_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, expected_offset, expected_size) == 1);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_multiple_panels_multiple_point_visuals_emit(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 128, 64, 0);
    AT(figure != NULL);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    AT(left != NULL);
    AT(right != NULL);

    DvzVisual* visual_a = dvz_point(scene, 0);
    DvzVisual* visual_b = dvz_point(scene, 0);
    AT(visual_a != NULL);
    AT(visual_b != NULL);

    float pos_a[2 * 3] = {
        -0.75f, 0.0f, 0.0f,
        -0.60f, 0.0f, 0.0f,
    };
    float pos_b[3 * 3] = {
        0.15f, 0.0f, 0.0f,
        0.30f, 0.0f, 0.0f,
        0.45f, 0.0f, 0.0f,
    };
    DvzColor color_a[2] = {{255, 0, 0, 255}, {255, 0, 0, 255}};
    DvzColor color_b[3] = {
        {0, 255, 0, 255},
        {0, 255, 0, 255},
        {0, 255, 0, 255},
    };
    float size_a[2] = {5.0f, 5.0f};
    float size_b[3] = {6.0f, 6.0f, 6.0f};

    AT(dvz_visual_set_data(visual_a, "position", pos_a, 2) == 0);
    AT(dvz_visual_set_data(visual_a, "color", color_a, 2) == 0);
    AT(dvz_visual_set_data(visual_a, "size", size_a, 2) == 0);
    AT(dvz_visual_set_data(visual_b, "position", pos_b, 3) == 0);
    AT(dvz_visual_set_data(visual_b, "color", color_b, 3) == 0);
    AT(dvz_visual_set_data(visual_b, "size", size_b, 3) == 0);
    AT(dvz_panel_add_visual(left, visual_a, NULL) == 0);
    AT(dvz_panel_add_visual(right, visual_b, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_wgsl = true;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream1 != NULL);
    AT(_stream_write_buffer_count(stream1) == 6);
    AT(_stream_set_vertex_buffer_count(stream1) == 6);
    AT(_stream_draw_count(stream1) == 2);
    uint32_t begin_render_pass_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd == NULL || cmd->type != DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            continue;
        if (begin_render_pass_count == 0)
        {
            AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 0.5f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(cmd->u.begin_render_pass.clear);
        }
        else if (begin_render_pass_count == 1)
        {
            AC(cmd->u.begin_render_pass.viewport[0], 0.5f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 0.5f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(!cmd->u.begin_render_pass.clear);
        }
        begin_render_pass_count++;
    }
    AT(begin_render_pass_count == 2);
    dvz_drp2_stream_destroy(stream1);

    float size_update[2] = {10.0f, 11.0f};
    AT(dvz_visual_set_data_range(visual_b, "size", size_update, 1, 2) == 0);

    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream2 = dvz_figure_emit(figure, &caps, &report);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream2 != NULL);
    AT(_stream_write_buffer_count(stream2) == 1);
    AT(_stream_write_buffer_range_count(stream2, sizeof(float), 2 * sizeof(float)) == 1);
    AT(_stream_set_vertex_buffer_count(stream2) == 6);
    AT(_stream_draw_count(stream2) == 2);

    dvz_drp2_stream_destroy(stream2);
    dvz_scene_destroy(scene);
    return 0;
}



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_clear_color(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    /* Scene with NO visuals — all pixels should show the clear color. */
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    (void)panel;
    AT(panel != NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_clear_color skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);

    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t width = 0, height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
    ANN(rgba);

    /* Default clear color is (0.05, 0.05, 0.08, 1.0) — very dark, R<20, G<20, B<25.
       All pixels must be dark (no stray bright pixels from missing clear). */
    uint32_t bright_count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        uint8_t* px = &rgba[4 * i];
        if (px[0] > 30 || px[1] > 30 || px[2] > 30)
            bright_count++;
    }
    AT(bright_count == 0);

    dvz_free(rgba);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_capture_rejects_wrong_dimensions(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    (void)panel;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_wrong_dimensions skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Ask for a dimension that doesn't match the 64x64 offscreen canvas. */
    uint8_t buf[128 * 128 * 4];
    tst_log_capture_begin(suite);
    AT(dvz_canvas_capture_rgba_into(canvas, 128, 128, buf, sizeof(buf)) != 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_capture_rejects_undersized_buffer(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    AT(panel != NULL);
    (void)panel;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_capture_rejects_undersized_buffer skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    dvz_app_run(app, 1);

    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    /* Buffer is one byte short of the required 64*64*4 bytes. */
    size_t required = 64 * 64 * 4;
    uint8_t* buf = dvz_malloc(required - 1);
    ANN(buf);
    tst_log_capture_begin(suite);
    AT(dvz_canvas_capture_rgba_into(canvas, 64, 64, buf, required - 1) != 0);
    dvz_free(buf);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_scene(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "scene";

    TEST_SIMPLE(test_panzoom_create_reset);
    TEST_SIMPLE(test_panzoom_pan_shift);
    TEST_SIMPLE(test_panzoom_zoom_wheel);
    TEST_SIMPLE(test_panzoom_double_click_resets);
    TEST_SIMPLE(test_panzoom_mvp_identity);

    TEST_SIMPLE(test_arcball_create_reset);
    TEST_SIMPLE(test_arcball_rotate_produces_nonidentity_model);
    TEST_SIMPLE(test_arcball_end_commits_rotation);
    TEST_SIMPLE(test_arcball_double_click_resets);

    TEST_SIMPLE(test_scene_capabilities_diagnostics);
    TEST_SIMPLE(test_frame_plan_static_render);
    TEST_SIMPLE(test_frame_plan_clear);
    TEST_SIMPLE(test_frame_plan_growth_json);
    TEST_SIMPLE(test_frame_plan_json_escapes_labels);
    TEST_SIMPLE(test_frame_plan_dynamic_update);
    TEST_SIMPLE(test_frame_plan_readbacks);
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render);
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render_glsl);
    TEST_SIMPLE(test_frame_plan_emit_drp2_rejects_unsupported_shader_format);
    TEST_SIMPLE(test_frame_plan_emit_drp2_rejects_small_caps);
    TEST_SIMPLE(test_scene_json);
    TEST_SIMPLE(test_scene_rejects_cross_scene_visual);
    TEST_SIMPLE(test_scene_z_layer_orders_emit);
    TEST_SIMPLE(test_scene_rejects_unsupported_point_attribute);
    TEST_SIMPLE(test_scene_point_rejects_texcoords_attribute);
    TEST_SIMPLE(test_scene_primitive_rejects_size_attribute);
    TEST_SIMPLE(test_scene_image_rejects_size_attribute);
    TEST_SIMPLE(test_scene_emit_warns_visual_with_no_position);
    TEST_SIMPLE(test_scene_rejects_mismatched_point_attribute_counts);
    TEST_SIMPLE(test_scene_rejects_range_update_without_full_allocation);
    TEST_SIMPLE(test_scene_rejects_mutation_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_range_mutation_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_destroy_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_visual_destroy_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_live_stream_count_tracks_multiple_emits);
    TEST_SIMPLE(test_scene_point_emit);
    TEST_SIMPLE(test_scene_image_emit);
    TEST_SIMPLE(test_scene_empty_figure_emit_clear_only);
    TEST_SIMPLE(test_scene_point_emit_has_vertex_layout);
    TEST_SIMPLE(test_scene_second_emit_no_uploads_when_not_dirty);
    TEST_SIMPLE(test_scene_partial_update_uploads_only_range);
    TEST_SIMPLE(test_scene_repeated_partial_updates_across_frames);
    TEST_SIMPLE(test_scene_partial_update_merges_ranges_before_emit);
    TEST_SIMPLE(test_scene_multiple_panels_multiple_point_visuals_emit);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emit_drp2_readback_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_two_frames_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_dynamic_two_frames_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_texture_two_frames_glsl_executes);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes);
    TEST_SIMPLE(test_scene_drp2_offscreen_canvas_frame);
    TEST_SIMPLE(test_scene_point_emit_glsl_executes);
    TEST_SIMPLE(test_scene_primitive_triangle_list_glsl_executes);
    TEST_SIMPLE(test_scene_primitive_line_strip_glsl_executes);
    TEST_SIMPLE(test_scene_image_glsl_executes);
#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    TEST_SIMPLE(test_app_offscreen);
    TEST_SIMPLE(test_app_offscreen_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_two_panel_points_light_both_halves);
    TEST_SIMPLE(test_app_offscreen_clear_color);
    TEST_SIMPLE(test_app_capture_rejects_wrong_dimensions);
    TEST_SIMPLE(test_app_capture_rejects_undersized_buffer);
#endif
    TEST_SIMPLE(test_scene_point_large_count_executes);
#endif
    TEST_SIMPLE(test_frame_plan_emit_drp2_readback);
    TEST_SIMPLE(test_frame_plan_emit_drp2_dynamic_uploads);
    TEST_SIMPLE(test_frame_plan_emit_drp2_texture_sampling);
    TEST_SIMPLE(test_frame_plan_emit_drp2_compute_assisted);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_two_frames);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_dynamic_two_frames);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_dynamic_grow_buffer);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_texture_two_frames);
    TEST_SIMPLE(test_frame_plan_emitter_runtime_compute_two_frames);

    return 0;
}
