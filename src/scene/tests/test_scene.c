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
#include <stdlib.h>
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



/**
 * Return the RGBA pixel pointer at integer coordinates.
 *
 * @param rgba the captured RGBA buffer
 * @param width the image width in pixels
 * @param height the image height in pixels
 * @param x the x pixel coordinate
 * @param y the y pixel coordinate
 * @return the pointer to the first RGBA byte
 */
static const uint8_t* _pixel_at(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
    ANN(rgba);
    ASSERT(x < width);
    ASSERT(y < height);
    return &rgba[4 * ((uint64_t)y * width + x)];
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



static uint32_t _stream_set_index_buffer_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            count++;
    }
    return count;
}



static uint32_t _stream_draw_indexed_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            count++;
    }
    return count;
}



static uint32_t _stream_create_buffer_size_count(
    const DvzDrp2CommandStream* stream, uint64_t size)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_CREATE_BUFFER && cmd->u.create_buffer.size == size)
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

    /* Positive wheel delta should zoom in regardless of platform. */
    dvz_panzoom_zoom_wheel(pz, (vec2){0.0f, 1.0f}, (vec2){400.0f, 400.0f});
    AT(pz->zoom[0] > 1.0f);
    AT(pz->zoom[1] > 1.0f);

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



int test_frame_plan_texture_upload_json_includes_region(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzFramePlan* plan = dvz_frame_plan("figure.texture", 11);
    ANN(plan);

    AT(dvz_frame_plan_upload(plan, "tex.image.rgba", 0, 8, "image.rgba.patch"));
    AT(dvz_frame_plan_upload_set_texture_extent(plan, 2, 1));
    AT(dvz_frame_plan_upload_set_texture_region(plan, 1, 2));

    char* json = dvz_frame_plan_json(plan);
    ANN(json);
    AT(strstr(json, "\"texture\": { \"origin_x\": 1, \"origin_y\": 2, \"width\": 2, \"height\": 1 }") != NULL);

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



static int _scene_path_emit_executes(uint32_t vertex_count)
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    float* positions = dvz_calloc(vertex_count * 3, sizeof(float));
    uint8_t (*colors)[4] = dvz_calloc(vertex_count, 4);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        positions[i * 3 + 0] = (float)i / (float)vertex_count - 0.5f;
        positions[i * 3 + 1] = (i % 2 == 0) ? -0.4f : 0.4f;
        positions[i * 3 + 2] = 0.0f;
        colors[i][0] = 255;
        colors[i][1] = (uint8_t)(255 * i / vertex_count);
        colors[i][2] = 64;
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



static int _scene_mesh_emit_executes(void)
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
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_mesh(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(uint16_t),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
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



int test_scene_mesh_indexed_default_color_emits_draw_indexed(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_mesh(scene, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(uint16_t),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(visual->mesh_default_color);
    bool found_color_attr = false;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        found_color_attr = found_color_attr || strcmp(visual->attrs[i].name, "color") == 0;
    AT(found_color_attr);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint16") == 0;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzPrimitiveShadingState)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_indexed_primitive_emits_draw_indexed(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(uint16_t),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    if (stream == NULL && dvz_diagnostic_report_count(&report) > 0)
        log_error("%s", dvz_diagnostic_report_get(&report, 0));
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    bool found_set_index = false;
    bool found_draw_indexed = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_SET_INDEX_BUFFER)
            found_set_index = strcmp(cmd->u.set_index_buffer.index_format, "uint16") == 0;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW_INDEXED)
            found_draw_indexed = cmd->u.draw_indexed.index_count == 6;
    }
    AT(found_set_index);
    AT(found_draw_indexed);
    AT(_stream_set_vertex_buffer_count(stream) == 3);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(DvzPrimitiveShadingState)) == 1);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_shared_index_buffer_emits_one_upload(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual0 = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* visual1 = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual0);
    ANN(visual1);

    float positions0[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.0f, 0.0f},
        {-0.2f, -0.8f, 0.0f}, {-0.2f, 0.0f, 0.0f},
    };
    float positions1[4][3] = {
        {0.2f, 0.0f, 0.0f}, {0.2f, 0.8f, 0.0f},
        {0.8f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors0[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    DvzColor colors1[4] = {
        {0, 0, 255, 255}, {0, 0, 255, 255}, {0, 0, 255, 255}, {0, 0, 255, 255},
    };
    uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(uint16_t),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual0, "position", positions0, 4) == 0);
    AT(dvz_visual_set_data(visual0, "color", colors0, 4) == 0);
    AT(dvz_visual_set_data(visual1, "position", positions1, 4) == 0);
    AT(dvz_visual_set_data(visual1, "color", colors1, 4) == 0);
    AT(dvz_visual_set_buffer(visual0, "index", index_buffer));
    AT(dvz_visual_set_buffer(visual1, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual0, NULL) == 0);
    AT(dvz_panel_add_visual(panel, visual1, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    AT(_stream_create_buffer_size_count(stream, sizeof(indices)) == 1);
    AT(_stream_write_buffer_range_count(stream, 0, sizeof(indices)) == 1);
    AT(_stream_set_index_buffer_count(stream) == 2);
    AT(_stream_draw_indexed_count(stream) == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_mesh_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_mesh_emit_executes();
}



int test_scene_path_glsl_executes(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return _scene_path_emit_executes(4);
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



int test_scene_json_includes_field_dirty_metadata(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    uint8_t base[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    uint8_t patch[2] = {1, 2};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 1}));

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"dirty\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    AT(strstr(json, "\"field_state\":{\"pending\":true,\"full\":false,\"region\":{\"x\":1,\"y\":2,\"z\":0,\"width\":2,\"height\":1,\"depth\":1}}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_json_includes_buffer_binding_metadata(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f},
    };
    DvzColor colors[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };
    uint16_t indices[3] = {0, 1, 2};

    DvzSceneBuffer* buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){.usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(uint16_t)});
    ANN(buffer);
    AT(dvz_scene_buffer_set_data(buffer, indices, sizeof(indices)));
    AT(dvz_visual_set_data(visual, "position", positions, 3) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 3) == 0);
    AT(dvz_visual_set_buffer(visual, "index", buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"buffers\":[") != NULL);
    AT(strstr(json, "\"id\":\"b0\"") != NULL);
    AT(strstr(json, "\"usage\":2") != NULL);
    AT(strstr(json, "\"stride\":2") != NULL);
    AT(strstr(json, "\"byte_size\":6") != NULL);
    AT(strstr(json, "\"dirty\":{\"pending\":true}") != NULL);
    AT(strstr(json, "\"buffer\":{\"id\":\"b0\",\"slot\":\"index\"}") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_z_layer_orders_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* Two point visuals on one panel: behind=3 verts (z=-1), front=5 verts (z=+1).
     * Add front first, behind second, so insertion order ≠ z order. After phase 1
     * both visuals draw inside one render pass; the behind visual (z=-1) must draw
     * before the front visual (z=+1). */
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

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char* json = dvz_drp2_stream_json(stream, "z_layer_order");
    ANN(json);

    /* Both draws appear in the single render pass (pass_id 10001).
     * The behind visual (3 verts, z=-1) must appear before the front visual (5 verts, z=+1). */
    const char* draw3 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 3");
    const char* draw5 = strstr(json, "\"cmd\": \"Draw\", \"pass_id\": 10001, \"vertex_count\": 5");
    AT(draw3 != NULL);
    AT(draw5 != NULL);
    AT(draw3 < draw5);  /* behind drawn first */

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_background_color_creates_fixed_quad(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    /* Initially no visuals. */
    AT(panel->visual_count == 0);
    AT(panel->background_visual == NULL);

    /* First call: creates a hidden background visual at z_layer=-1, FIXED. */
    dvz_panel_set_background_color(panel, 0.1f, 0.2f, 0.3f, 1.0f);
    AT(panel->visual_count == 1);
    ANN(panel->background_visual);
    AT(panel->visuals[0].visual == panel->background_visual);
    AT(panel->visuals[0].z_layer == -1);
    AT(panel->visuals[0].controller_mode == DVZ_CONTROLLER_FIXED);

    /* Second call with a different color: updates in place, no new visual. */
    DvzVisual* before = panel->background_visual;
    dvz_panel_set_background_color(panel, 0.9f, 0.8f, 0.7f, 1.0f);
    AT(panel->visual_count == 1);
    AT(panel->background_visual == before);

    /* A regular visual added afterwards has default attach (z=0, APPLY) and lands
     * in front of the background per stable z-sort. */
    float pos[3 * 3] = {0};
    DvzColor col[3]  = {0};
    float sz[3]      = {0};
    DvzVisual* v = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v, "size", sz, 3) == 0);
    AT(dvz_panel_add_visual(panel, v, NULL) == 0);
    AT(panel->visual_count == 2);
    AT(panel->visuals[1].z_layer == 0);
    AT(panel->visuals[1].controller_mode == DVZ_CONTROLLER_APPLY);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_scale_colormap_colorbar_core(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 100, 100, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Depth",
                   .unit = "um",
                   .format =
                       (DvzFormatDesc){
                           .precision = 2,
                           .show_unit = true,
                           .unit = "um",
                           .suffix = " depth",
                       },
               });
    ANN(scale);
    AT(scene->scale_count == 1);
    AT(scale->scene == scene);
    AT(scale->kind == DVZ_SCALE_CONTINUOUS);
    AT(strcmp(scale->label, "Depth") == 0);
    AT(strcmp(scale->unit, "um") == 0);
    AT(scale->format.precision == 2);
    AT(strcmp(scale->format.unit, "um") == 0);
    AT(strcmp(scale->format.suffix, " depth") == 0);

    dvz_scale_set_domain(scale, -600.0, 0.0);
    dvz_scale_set_view_range(scale, -300.0, -50.0);
    AT(scale->has_domain);
    AT(scale->domain_min == -600.0);
    AT(scale->domain_max == 0.0);
    AT(scale->has_view_range);
    AT(scale->view_min == -300.0);
    AT(scale->view_max == -50.0);

    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_MAGMA);
    ANN(colormap);
    AT(scene->colormap_count == 1);
    AT(colormap->builtin == DVZ_BUILTIN_COLORMAP_MAGMA);

    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 255, 255}},
    };
    dvz_colormap_set_center(colormap, 0.5);
    dvz_colormap_set_stops(colormap, stops, 2);
    AT(colormap->has_center);
    AT(colormap->center == 0.5);
    AT(colormap->stop_count == 2);
    AT(colormap->stops[1].rgba[0] == 255);

    dvz_scale_set_colormap(scale, colormap);
    AT(scale->colormap == colormap);

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale, &(DvzColorbarDesc){
                          .orientation = DVZ_COLORBAR_ORIENTATION_HORIZONTAL,
                          .anchor = DVZ_SCENE_ANCHOR_PANEL_BOTTOM,
                          .title = "Depth map",
                      });
    ANN(colorbar);
    AT(scene->colorbar_count == 1);
    AT(panel->colorbar_count == 1);
    AT(panel->colorbars[0] == colorbar);
    AT(colorbar->scene == scene);
    AT(colorbar->panel == panel);
    AT(colorbar->scale == scale);
    AT(colorbar->orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL);
    AT(colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM);
    AT(strcmp(colorbar->title, "Depth map") == 0);

    dvz_colorbar_set_format(
        colorbar, &(DvzFormatDesc){
                       .precision = 0,
                       .unit = "um",
                   });
    AT(colorbar->has_format);
    AT(colorbar->format.precision == 0);
    AT(strcmp(colorbar->format.unit, "um") == 0);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_colorbar_rejects_cross_scene_scale(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzFigure* figure = dvz_figure(scene0, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzScale* foreign_scale = dvz_scale(scene1, NULL);
    ANN(foreign_scale);

    DvzColorbar* colorbar = dvz_colorbar(panel, foreign_scale, NULL);
    AT(colorbar == NULL);
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}



int test_scene_image_visual_binds_colormap_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Intensity",
                   .unit = "a.u.",
               });
    ANN(scale);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);

    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);
    AT(image->scale == scale);
    AT(strcmp(image->scale_slot, "colormap") == 0);

    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    float positions[4 * 3] = {
        -1.0f, +1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        +1.0f, +1.0f, 0.0f,
        +1.0f, -1.0f, 0.0f,
    };
    float texcoords[4 * 2] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };
    static const uint8_t pixels[4 * 4 * 4] = {0};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 16, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    dvz_drp2_stream_destroy(stream);

    char* json = dvz_scene_json(scene);
    ANN(json);
    AT(strstr(json, "\"scale\":{\"id\":\"s0\",\"slot\":\"colormap\"}") != NULL);
    AT(strstr(json, "\"field\":{\"id\":\"f0\",\"slot\":\"field\"}") != NULL);
    AT(strstr(json, "\"fields\":[{\"id\":\"f0\"") != NULL);
    dvz_scene_json_destroy(json);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_visual_scale_rejects_cross_scene_scale(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzScale* foreign_scale = dvz_scale(scene1, NULL);
    ANN(foreign_scale);
    DvzVisual* image = dvz_image(scene0, 0);
    ANN(image);

    AT(dvz_visual_set_scale(image, "colormap", foreign_scale) != 0);
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}



int test_scene_visual_buffer_rejects_cross_scene_buffer(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzSceneBuffer* foreign_buffer = dvz_scene_buffer(
        scene1, &(DvzSceneBufferDesc){.usage = DVZ_SCENE_BUFFER_USAGE_INDEX, .stride = sizeof(uint16_t)});
    ANN(foreign_buffer);
    DvzVisual* visual = dvz_primitive(scene0, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    AT(!dvz_visual_set_buffer(visual, "index", foreign_buffer));
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}



int test_scene_image_scalar_texture_uses_bound_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* visual = dvz_image(scene, 0);
    ANN(visual);
    AT(dvz_visual_set_scale(visual, "colormap", scale) == 0);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    float pixels[4 * 4];
    for (uint32_t i = 0; i < 16; i++)
        pixels[i] = 1.0f;

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_texture_f32(visual, pixels, 4, 4) == 0);
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(visual->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)visual->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_r16_float_field_uses_bound_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    uint16_t values[16];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 0x3c00u; /* half-float 1.0 */

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(uint16_t), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(image->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)image->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_r16_snorm_field_uses_bound_scale(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, -1.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    int16_t values[16];
    for (uint32_t i = 0; i < 16; i++)
        values[i] = 32767; /* SNORM 1.0 */

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R16_SNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(int16_t), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    ANN(stream);
    AT(image->texture.rgba != NULL);
    uint8_t* rgba = (uint8_t*)image->texture.rgba;
    AT(rgba[0] == 255);
    AT(rgba[1] == 0);
    AT(rgba[2] == 0);
    AT(rgba[3] == 255);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_visual_field_rejects_cross_scene_field(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene0 = dvz_scene();
    DvzScene* scene1 = dvz_scene();
    ANN(scene0);
    ANN(scene1);

    DvzVisual* image = dvz_image(scene0, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene1, &(DvzSampledFieldDesc){
                    .dim = DVZ_FIELD_DIM_2D,
                    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                    .width = 2,
                    .height = 2,
                    .depth = 1,
                });
    ANN(field);

    AT(!dvz_visual_set_field(image, "field", field));
    AT(_captured_log_contains(suite, "different scene"));

    dvz_scene_destroy(scene1);
    dvz_scene_destroy(scene0);
    return 0;
}



int test_scene_sampled_field_update_region(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    uint8_t base[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));

    uint8_t patch[4] = {1, 2, 3, 4};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 1, .z = 0, .width = 2, .height = 2, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 2}));

    uint8_t* data = (uint8_t*)field->data;
    AT(data[5] == 1);
    AT(data[6] == 2);
    AT(data[9] == 3);
    AT(data[10] == 4);
    AT(field->dirty);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_sampled_field_rejects_unsupported_format(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RG32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_VECTOR_2,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    AT(field == NULL);
    AT(_captured_log_contains(suite, "unsupported sampled field format"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_visual_rejects_3d_field(TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 4,
               });
    ANN(field);

    AT(!dvz_visual_set_field(image, "field", field));
    AT(_captured_log_contains(suite, "require a 2D sampled field"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_sampled_field_update_region_rejects_out_of_bounds(
    TstSuite* suite, TstItem* item)
{
    tst_log_capture_begin(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R8_UINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    uint8_t base[16] = {0};
    uint8_t patch[4] = {1, 2, 3, 4};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = base, .bytes_per_row = 4, .rows_per_image = 4}));
    AT(!dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 3, .y = 3, .z = 0, .width = 2, .height = 2, .depth = 1},
        &(DvzFieldDataView){.data = patch, .bytes_per_row = 2, .rows_per_image = 2}));
    AT(_captured_log_contains(suite, "update region exceeds field dimensions"));

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_sampled_field_destroy_clears_visual_binding(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);

    uint8_t rgba[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = rgba, .bytes_per_row = 8, .rows_per_image = 2}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(image->field == field);
    AT(strcmp(image->field_slot, "field") == 0);

    AT(dvz_sampled_field_destroy(field));
    AT(image->field == NULL);
    AT(image->field_slot[0] == '\0');

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_shared_field_update_marks_two_visuals_dirty(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 2,
                   .height = 2,
                   .depth = 1,
               });
    ANN(field);
    float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 2 * sizeof(float), .rows_per_image = 2}));
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_visual_set_field(image1, "field", field));

    image0->texture.dirty = false;
    image1->texture.dirty = false;
    field->dirty = false;

    float patch[1] = {1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 1, .z = 0, .width = 1, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch, .bytes_per_row = sizeof(float), .rows_per_image = 1}));
    AT(field->dirty);
    AT(image0->texture.dirty);
    AT(image1->texture.dirty);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_image_field_partial_update_emits_texture_subregion(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f}, { 0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(float), .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    dvz_drp2_stream_destroy(stream0);

    float patch[2] = {1.0f, 1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 1,
        }));

    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);

    uint32_t write_texture_count = 0;
    bool found_region = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        write_texture_count++;
        if (cmd->u.write_texture.origin_x == 1 && cmd->u.write_texture.origin_y == 2 &&
            cmd->u.write_texture.width == 2 && cmd->u.write_texture.height == 1)
        {
            found_region = true;
        }
    }
    AT(write_texture_count == 1);
    AT(found_region);
    AT(!field->dirty);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_shared_field_mixed_full_and_partial_uploads(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale0);
    ANN(scale1);
    dvz_scale_set_domain(scale0, 0.0, 1.0);
    dvz_scale_set_domain(scale1, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale0, colormap);
    dvz_scale_set_colormap(scale1, colormap);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){.data = values, .bytes_per_row = 4 * sizeof(float), .rows_per_image = 4}));

    float positions0[4][3] = {
        {-1.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        { 0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 0.0f},
    };
    float positions1[4][3] = {
        {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", positions0, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "colormap", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", positions1, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "colormap", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);

    DvzDrp2CommandStream* stream0 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream0);
    dvz_drp2_stream_destroy(stream0);

    float patch[2] = {1.0f, 1.0f};
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 1, .y = 2, .z = 0, .width = 2, .height = 1, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 1,
        }));
    dvz_scale_set_view_range(scale0, 0.0, 1.0);

    DvzDrp2CommandStream* stream1 = dvz_figure_emit(figure, &caps, &report);
    ANN(stream1);

    uint32_t write_texture_count = 0;
    bool found_full = false;
    bool found_partial = false;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream1); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream1, i);
        if (cmd->type != DVZ_DRP2_COMMAND_WRITE_TEXTURE)
            continue;
        write_texture_count++;
        if (cmd->u.write_texture.origin_x == 0 && cmd->u.write_texture.origin_y == 0 &&
            cmd->u.write_texture.width == 4 && cmd->u.write_texture.height == 4)
        {
            found_full = true;
        }
        if (cmd->u.write_texture.origin_x == 1 && cmd->u.write_texture.origin_y == 2 &&
            cmd->u.write_texture.width == 2 && cmd->u.write_texture.height == 1)
        {
            found_partial = true;
        }
    }
    AT(write_texture_count == 2);
    AT(found_full);
    AT(found_partial);

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_controller_mode_fixed_emits_separate_mvp(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    /* One panel with a panzoom (APPLY) and a FIXED visual: the converter must allocate
     * two MVP UBOs, one per controller_mode. APPLY gets the panzoom MVP, FIXED gets
     * identity, and writes never overwrite each other. */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    /* Note: we don't actually run a panzoom here — the controller_mode flag alone
     * determines whether the converter writes identity or the controller MVP. */

    float pos[3 * 3] = {0};
    DvzColor col[3] = {0};
    float sz[3] = {0};

    DvzVisual* v_apply = dvz_point(scene, 0);
    DvzVisual* v_fixed = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v_apply, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v_apply, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v_apply, "size", sz, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "color", col, 3) == 0);
    AT(dvz_visual_set_data(v_fixed, "size", sz, 3) == 0);

    AT(dvz_panel_add_visual(panel, v_apply, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v_fixed,
                            &(DvzVisualAttachDesc){.controller_mode = DVZ_CONTROLLER_FIXED}) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    char* json = dvz_drp2_stream_json(stream, "controller_mode_test");
    ANN(json);

    /* Two distinct MVP UBOs (size 208 = sizeof(DvzMVP) + std140 padding) must be
     * created — one keyed by ("_mvp_buf_<panel>_apply") for the APPLY visual and
     * one keyed by ("_mvp_buf_<panel>_fixed") for the FIXED visual. The cache key
     * split prevents the FIXED visual's identity MVP from clobbering the APPLY
     * visual's controller MVP. */
    uint32_t mvp_buffers = 0;
    const char* p = json;
    while ((p = strstr(p, "\"size\": 208, \"usage\": [\"COPY_DST\"")) != NULL)
    {
        mvp_buffers++;
        p += 1;
    }
    AT(mvp_buffers == 2);

    dvz_drp2_stream_json_destroy(json);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_panel_one_pass_per_panel(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

    float pos[3 * 3] = {0};
    DvzColor col[3]  = {0};
    float sz[3]      = {0};

    DvzVisual* v0 = dvz_point(scene, 0);
    DvzVisual* v1 = dvz_point(scene, 0);
    DvzVisual* v2 = dvz_point(scene, 0);
    AT(dvz_visual_set_data(v0, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v0, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v0, "size",     sz,  3) == 0);
    AT(dvz_visual_set_data(v1, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v1, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v1, "size",     sz,  3) == 0);
    AT(dvz_visual_set_data(v2, "position", pos, 3) == 0);
    AT(dvz_visual_set_data(v2, "color",    col, 3) == 0);
    AT(dvz_visual_set_data(v2, "size",     sz,  3) == 0);
    AT(dvz_panel_add_visual(panel, v0, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v1, NULL) == 0);
    AT(dvz_panel_add_visual(panel, v2, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups    = 4;
    caps.max_buffer_size    = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    /* Exactly one BeginRenderPass and three Draws in that pass. */
    uint32_t pass_count = 0, draw_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            pass_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
    }
    AT(pass_count == 1);
    AT(draw_count == 3);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state(
    TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0, 0, 0.5f, 1});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0, 0.5f, 1});

    float pos_l[3] = {-0.5f, 0.0f, 0.0f};
    float pos_r[3] = {0.5f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 6.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_l, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);

    DvzVisualAttachDesc fixed = {.controller_mode = DVZ_CONTROLLER_FIXED};
    AT(dvz_panel_add_visual(left, vl, &fixed) == 0);
    AT(dvz_panel_add_visual(right, vr, &fixed) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, draw_count = 0, pipeline_count = 0, bind_group_count = 0;
    uint32_t viewport_count = 0, scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            pass_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_DRAW)
            draw_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_PIPELINE)
            pipeline_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_BIND_GROUP)
            bind_group_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
            viewport_count++;
        if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
            scissor_count++;
    }
    AT(pass_count == 1);
    AT(draw_count == 2);
    AT(pipeline_count == 1);
    AT(bind_group_count == 1);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_multi_panel_glsl_emits_viewport_scissor_commands(
    TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 128, 64, 0);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});

    float pos_l[3] = {-0.6f, 0.0f, 0.0f};
    float pos_r[3] = {0.6f, 0.0f, 0.0f};
    DvzColor col = {255, 255, 255, 255};
    float sz = 5.0f;

    DvzVisual* vl = dvz_point(scene, 0);
    DvzVisual* vr = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vl, "position", pos_l, 1) == 0);
    AT(dvz_visual_set_data(vl, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vl, "size", &sz, 1) == 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color", &col, 1) == 0);
    AT(dvz_visual_set_data(vr, "size", &sz, 1) == 0);
    AT(dvz_panel_add_visual(left, vl, NULL) == 0);
    AT(dvz_panel_add_visual(right, vr, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_vertex_buffers = 16;
    caps.max_bind_groups = 4;
    caps.max_buffer_size = 256 * 1024 * 1024;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);

    uint32_t pass_count = 0, viewport_count = 0, scissor_count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            pass_count++;
            AC(cmd->u.begin_render_pass.viewport[0], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[1], 0.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[2], 1.0f, 1e-6f);
            AC(cmd->u.begin_render_pass.viewport[3], 1.0f, 1e-6f);
            AT(cmd->u.begin_render_pass.clear);
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_VIEWPORT)
        {
            if (viewport_count == 0)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 1.0f, 1e-6f);
            }
            else if (viewport_count == 1)
            {
                AC(cmd->u.set_viewport.viewport[0], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[1], 0.0f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[2], 0.5f, 1e-6f);
                AC(cmd->u.set_viewport.viewport[3], 1.0f, 1e-6f);
            }
            viewport_count++;
        }
        else if (cmd->type == DVZ_DRP2_COMMAND_SET_SCISSOR)
        {
            if (scissor_count == 0)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[3], 1.0f, 1e-6f);
            }
            else if (scissor_count == 1)
            {
                AC(cmd->u.set_scissor.scissor[0], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[1], 0.0f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[2], 0.5f, 1e-6f);
                AC(cmd->u.set_scissor.scissor[3], 1.0f, 1e-6f);
            }
            scissor_count++;
        }
    }

    AT(pass_count == 1);
    AT(viewport_count == 2);
    AT(scissor_count == 2);

    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_panel_three_visuals_all_drawn(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    AT(panel != NULL);

    /* Three non-overlapping points: red (left), green (center), blue (right). */
    float pos_r[3] = {-0.6f, 0.0f, 0.0f};
    float pos_g[3] = { 0.0f, 0.0f, 0.0f};
    float pos_b[3] = { 0.6f, 0.0f, 0.0f};
    DvzColor red   = {220, 20, 20, 255};
    DvzColor green = {20, 220, 20, 255};
    DvzColor blue  = {20, 20, 220, 255};
    float size = 10.0f;

    DvzVisual* vr = dvz_point(scene, 0);
    DvzVisual* vg = dvz_point(scene, 0);
    DvzVisual* vb = dvz_point(scene, 0);
    AT(dvz_visual_set_data(vr, "position", pos_r, 1) == 0);
    AT(dvz_visual_set_data(vr, "color",    &red,  1) == 0);
    AT(dvz_visual_set_data(vr, "size",     &size, 1) == 0);
    AT(dvz_visual_set_data(vg, "position", pos_g, 1) == 0);
    AT(dvz_visual_set_data(vg, "color",    &green, 1) == 0);
    AT(dvz_visual_set_data(vg, "size",     &size, 1) == 0);
    AT(dvz_visual_set_data(vb, "position", pos_b, 1) == 0);
    AT(dvz_visual_set_data(vb, "color",    &blue, 1) == 0);
    AT(dvz_visual_set_data(vb, "size",     &size, 1) == 0);
    AT(dvz_panel_add_visual(panel, vr, NULL) == 0);
    AT(dvz_panel_add_visual(panel, vg, NULL) == 0);
    AT(dvz_panel_add_visual(panel, vb, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn("test_app_offscreen_panel_three_visuals_all_drawn skipped: GPU context creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    AT(win != NULL);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    uint32_t red_count = 0, green_count = 0, blue_count = 0;
    for (uint32_t frame = 0; frame < 3; frame++)
    {
        dvz_app_run(app, 1);

        uint32_t width = 0, height = 0;
        uint8_t* rgba = NULL;
        AT(dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) == 0);
        ANN(rgba);

        red_count = green_count = blue_count = 0;
        for (uint32_t i = 0; i < width * height; i++)
        {
            uint8_t* px = &rgba[4 * i];
            if (px[0] > 150 && px[0] > px[1] + 80 && px[0] > px[2] + 80)
                red_count++;
            if (px[1] > 150 && px[1] > px[0] + 80 && px[1] > px[2] + 80)
                green_count++;
            if (px[2] > 150 && px[2] > px[0] + 80 && px[2] > px[1] + 80)
                blue_count++;
        }
        dvz_free(rgba);
        if (red_count > 0 && green_count > 0 && blue_count > 0)
            break;
    }
    AT(red_count > 0);
    AT(green_count > 0);
    AT(blue_count > 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



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



int test_scene_path_rejects_size_attribute(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzVisual* visual = dvz_path(scene, 0);
    ANN(visual);

    float sz[1] = {10.0f};
    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_data(visual, "size", sz, 1) == -1);
    AT(_captured_log_contains(suite, "unsupported path visual attribute 'size'"));

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



int test_scene_rejects_scale_binding_while_emitted_stream_is_live(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    DvzScale* scale = dvz_scale(scene, NULL);
    ANN(scale);

    float positions[4][3] = {
        {-0.5f, -0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    uint8_t pixels[4 * 4 * 4] = {0};
    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);

    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_sampled_field_set_data(
           field, &(DvzFieldDataView){.data = pixels, .bytes_per_row = 16, .rows_per_image = 4}));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit(figure, &caps, &report);
    AT(stream != NULL);

    tst_log_capture_begin(suite);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_visual_set_scale(image, "colormap", scale) == -1);
    AT(_captured_log_contains(suite, "destroy the stream first"));

    dvz_drp2_stream_destroy(stream);
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



int test_scene_path_emit(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);
    DvzVisual* visual = dvz_path(scene, 0);
    AT(visual != NULL);

    float positions[4][3] = {
        {-0.75f, -0.25f, 0.0f},
        {-0.25f, 0.25f, 0.0f},
        {0.25f, -0.25f, 0.0f},
        {0.75f, 0.25f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255},
        {255, 255, 0, 255},
        {0, 255, 255, 255},
        {0, 128, 255, 255},
    };

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
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

    bool found_pipeline = false;
    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd != NULL && cmd->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            found_pipeline = true;
            AT(cmd->u.create_render_pipeline.topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
            AT(cmd->u.create_render_pipeline.binding_count == 2);
            AT(cmd->u.create_render_pipeline.attr_count == 2);
            break;
        }
    }
    AT(found_pipeline);

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
int test_app_offscreen_image_field_partial_update_changes_region(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale);
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap(scene, NULL);
    ANN(colormap);
    DvzColormapStop stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 2);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    ANN(image);
    float positions[4][3] = {
        {-0.95f, -0.95f, 0.0f}, {-0.95f, 0.95f, 0.0f},
        {0.95f, -0.95f, 0.0f},  {0.95f, 0.95f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    AT(dvz_visual_set_data(image, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(image, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image, "colormap", scale) == 0);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));
    AT(dvz_visual_set_field(image, "field", field));
    AT(dvz_panel_add_visual(panel, image, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_image_field_partial_update_changes_region skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 64, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);
    AT(width0 == 64);
    AT(height0 == 64);

    float patch[8];
    for (uint32_t i = 0; i < 8; i++)
        patch[i] = 1.0f;
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 2, .y = 0, .z = 0, .width = 2, .height = 4, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 4,
        }));

    dvz_app_run(app, 1);

    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);
    AT(width1 == 64);
    AT(height1 == 64);

    const uint8_t* left0 = _pixel_at(rgba0, width0, height0, 16, 32);
    const uint8_t* right0 = _pixel_at(rgba0, width0, height0, 48, 32);
    const uint8_t* left1 = _pixel_at(rgba1, width1, height1, 16, 32);
    const uint8_t* right1 = _pixel_at(rgba1, width1, height1, 48, 32);

    AT(left0[2] > 180);
    AT(right0[2] > 180);
    AT((int)left1[0] - (int)left0[0] < 40);
    AT(abs((int)left1[2] - (int)left0[2]) < 40);
    AT(right1[0] > 180);
    AT(right1[2] < 80);

    dvz_free(rgba1);
    dvz_free(rgba0);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_scene_indexed_primitive_shading_updates_runtime(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    ANN(visual);

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    DvzColor colors[4] = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(uint16_t),
               });
    ANN(index_buffer);
    AT(dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)));

    AT(dvz_visual_set_data(visual, "position", positions, 4) == 0);
    AT(dvz_visual_set_data(visual, "color", colors, 4) == 0);
    AT(dvz_visual_set_data(visual, "normal", normals, 4) == 0);
    AT(dvz_visual_set_buffer(visual, "index", index_buffer));
    AT(dvz_panel_add_visual(panel, visual, NULL) == 0);
    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 0.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    DvzDrp2Runtime* runtime = NULL;
    if (ctx != NULL)
    {
        DvzDrp2RuntimeConfig runtime_cfg =
            dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
        runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
        ANN(runtime);
    }

    DvzDrp2CommandStream* stream0 = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream0);
    AT(_stream_set_vertex_buffer_count(stream0) == 3);
    AT(_stream_write_buffer_range_count(stream0, 0, sizeof(DvzPrimitiveShadingState)) == 1);
    if (runtime != NULL)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream0);
        AT(result.ok);
        AT(dvz_gpu_ctx_error_count(ctx) == 0);
    }
    dvz_drp2_stream_destroy(stream0);
    stream0 = NULL;

    AT(dvz_visual_set_primitive_shading(
           visual,
           &(DvzPrimitiveShadingDesc){
               .light_direction = {0.0f, 0.0f, 1.0f},
               .ambient = 1.0f,
               .diffuse = 0.0f,
           }) == 0);

    DvzDrp2CommandStream* stream1 = dvz_figure_emit_ex(figure, &caps, &report, &emit_cfg);
    ANN(stream1);
    AT(_stream_set_vertex_buffer_count(stream1) == 3);
    AT(_stream_write_buffer_range_count(stream1, 0, sizeof(DvzPrimitiveShadingState)) == 1);

    if (runtime != NULL)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream1);
        AT(result.ok);
        AT(dvz_gpu_ctx_error_count(ctx) == 0);
    }

    if (runtime != NULL)
    {
        dvz_drp2_runtime_destroy(runtime);
        runtime = NULL;
    }
    if (ctx != NULL)
    {
        dvz_gpu_ctx_destroy(ctx);
        ctx = NULL;
    }

    dvz_drp2_stream_destroy(stream1);
    dvz_scene_destroy(scene);
    return 0;
}
#endif



#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
int test_app_offscreen_shared_field_mixed_runtime_updates(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;

    if (!_scene_vklite_runtime_available())
        return 0;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 96, 64, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzScale* scale0 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    DvzScale* scale1 = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    ANN(scale0);
    ANN(scale1);
    dvz_scale_set_domain(scale0, 0.0, 1.0);
    dvz_scale_set_domain(scale1, 0.0, 1.0);

    DvzColormap* colormap0 = dvz_colormap(scene, NULL);
    DvzColormap* colormap1 = dvz_colormap(scene, NULL);
    ANN(colormap0);
    ANN(colormap1);
    DvzColormapStop base_stops[2] = {
        {.position = 0.0, .rgba = {0, 0, 255, 255}},
        {.position = 1.0, .rgba = {255, 0, 0, 255}},
    };
    dvz_colormap_set_stops(colormap0, base_stops, 2);
    dvz_colormap_set_stops(colormap1, base_stops, 2);
    dvz_scale_set_colormap(scale0, colormap0);
    dvz_scale_set_colormap(scale1, colormap1);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = 4,
                   .height = 4,
                   .depth = 1,
               });
    ANN(field);
    float values[16] = {0};
    AT(dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = 4 * sizeof(float),
                   .rows_per_image = 4,
               }));

    float texcoords[4][2] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
    };
    float left_positions[4][3] = {
        {-1.0f, -0.95f, 0.0f}, {-1.0f, 0.95f, 0.0f},
        {0.0f, -0.95f, 0.0f},  {0.0f, 0.95f, 0.0f},
    };
    float right_positions[4][3] = {
        {0.0f, -0.95f, 0.0f}, {0.0f, 0.95f, 0.0f},
        {1.0f, -0.95f, 0.0f}, {1.0f, 0.95f, 0.0f},
    };

    DvzVisual* image0 = dvz_image(scene, 0);
    DvzVisual* image1 = dvz_image(scene, 0);
    ANN(image0);
    ANN(image1);
    AT(dvz_visual_set_data(image0, "position", left_positions, 4) == 0);
    AT(dvz_visual_set_data(image0, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image0, "colormap", scale0) == 0);
    AT(dvz_visual_set_field(image0, "field", field));
    AT(dvz_panel_add_visual(panel, image0, NULL) == 0);

    AT(dvz_visual_set_data(image1, "position", right_positions, 4) == 0);
    AT(dvz_visual_set_data(image1, "texcoords", texcoords, 4) == 0);
    AT(dvz_visual_set_scale(image1, "colormap", scale1) == 0);
    AT(dvz_visual_set_field(image1, "field", field));
    AT(dvz_panel_add_visual(panel, image1, NULL) == 0);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        log_warn(
            "test_app_offscreen_shared_field_mixed_runtime_updates skipped: GPU context "
            "creation failed");
        dvz_scene_destroy(scene);
        return 0;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, 96, 64);
    ANN(win);
    DvzCanvas* canvas = dvz_app_window_canvas(win);
    ANN(canvas);

    dvz_app_run(app, 1);

    uint32_t width0 = 0, height0 = 0;
    uint8_t* rgba0 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width0, &height0, &rgba0) == 0);
    ANN(rgba0);

    DvzColormapStop updated_stops[2] = {
        {.position = 0.0, .rgba = {0, 255, 0, 255}},
        {.position = 1.0, .rgba = {255, 255, 0, 255}},
    };
    dvz_colormap_set_stops(colormap0, updated_stops, 2);

    float patch[8];
    for (uint32_t i = 0; i < 8; i++)
        patch[i] = 1.0f;
    AT(dvz_sampled_field_update_region(
        field, (DvzFieldRegion){.x = 2, .y = 0, .z = 0, .width = 2, .height = 4, .depth = 1},
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = 2 * sizeof(float),
            .rows_per_image = 4,
        }));

    dvz_app_run(app, 1);

    uint32_t width1 = 0, height1 = 0;
    uint8_t* rgba1 = NULL;
    AT(dvz_canvas_capture_rgba(canvas, &width1, &height1, &rgba1) == 0);
    ANN(rgba1);

    const uint8_t* left_left0 = _pixel_at(rgba0, width0, height0, 24, 32);
    const uint8_t* left_left1 = _pixel_at(rgba1, width1, height1, 24, 32);
    const uint8_t* right_left0 = _pixel_at(rgba0, width0, height0, 60, 32);
    const uint8_t* right_left1 = _pixel_at(rgba1, width1, height1, 60, 32);
    const uint8_t* right_right1 = _pixel_at(rgba1, width1, height1, 84, 32);

    AT(left_left0[2] > 180);
    AT((int)left_left1[1] > (int)left_left0[1] + 40);
    AT((int)left_left1[2] + 40 < (int)left_left0[2]);

    AT(right_left0[2] > 180);
    AT(abs((int)right_left1[0] - (int)right_left0[0]) < 40);
    AT(abs((int)right_left1[1] - (int)right_left0[1]) < 40);
    AT(abs((int)right_left1[2] - (int)right_left0[2]) < 40);

    AT(right_right1[0] > 180);
    AT(right_right1[2] < 120);

    dvz_free(rgba1);
    dvz_free(rgba0);
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
    TEST_SIMPLE(test_frame_plan_texture_upload_json_includes_region);
    TEST_SIMPLE(test_frame_plan_readbacks);
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render);
    TEST_SIMPLE(test_frame_plan_emit_drp2_static_render_glsl);
    TEST_SIMPLE(test_frame_plan_emit_drp2_rejects_unsupported_shader_format);
    TEST_SIMPLE(test_frame_plan_emit_drp2_rejects_small_caps);
    TEST_SIMPLE(test_scene_json);
    TEST_SIMPLE(test_scene_json_includes_field_dirty_metadata);
    TEST_SIMPLE(test_scene_json_includes_buffer_binding_metadata);
    TEST_SIMPLE(test_scene_rejects_cross_scene_visual);
    TEST_SIMPLE(test_scene_z_layer_orders_emit);
    TEST_SIMPLE(test_scene_controller_mode_fixed_emits_separate_mvp);
    TEST_SIMPLE(test_scene_panel_one_pass_per_panel);
    TEST_SIMPLE(test_scene_multi_panel_reuses_fixed_pipeline_and_bind_group_state);
    TEST_SIMPLE(test_scene_multi_panel_glsl_emits_viewport_scissor_commands);
    TEST_SIMPLE(test_scene_background_color_creates_fixed_quad);
    TEST_SIMPLE(test_scene_scale_colormap_colorbar_core);
    TEST_SIMPLE(test_scene_colorbar_rejects_cross_scene_scale);
    TEST_SIMPLE(test_scene_image_visual_binds_colormap_scale);
    TEST_SIMPLE(test_scene_visual_scale_rejects_cross_scene_scale);
    TEST_SIMPLE(test_scene_visual_buffer_rejects_cross_scene_buffer);
    TEST_SIMPLE(test_scene_image_scalar_texture_uses_bound_scale);
    TEST_SIMPLE(test_scene_image_r16_float_field_uses_bound_scale);
    TEST_SIMPLE(test_scene_image_r16_snorm_field_uses_bound_scale);
    TEST_SIMPLE(test_scene_visual_field_rejects_cross_scene_field);
    TEST_SIMPLE(test_scene_sampled_field_update_region);
    TEST_SIMPLE(test_scene_sampled_field_rejects_unsupported_format);
    TEST_SIMPLE(test_scene_image_visual_rejects_3d_field);
    TEST_SIMPLE(test_scene_sampled_field_update_region_rejects_out_of_bounds);
    TEST_SIMPLE(test_scene_sampled_field_destroy_clears_visual_binding);
    TEST_SIMPLE(test_scene_shared_field_update_marks_two_visuals_dirty);
    TEST_SIMPLE(test_scene_image_field_partial_update_emits_texture_subregion);
    TEST_SIMPLE(test_scene_shared_field_mixed_full_and_partial_uploads);
    TEST_SIMPLE(test_scene_rejects_unsupported_point_attribute);
    TEST_SIMPLE(test_scene_point_rejects_texcoords_attribute);
    TEST_SIMPLE(test_scene_primitive_rejects_size_attribute);
    TEST_SIMPLE(test_scene_path_rejects_size_attribute);
    TEST_SIMPLE(test_scene_image_rejects_size_attribute);
    TEST_SIMPLE(test_scene_emit_warns_visual_with_no_position);
    TEST_SIMPLE(test_scene_rejects_mismatched_point_attribute_counts);
    TEST_SIMPLE(test_scene_rejects_range_update_without_full_allocation);
    TEST_SIMPLE(test_scene_rejects_mutation_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_scale_binding_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_range_mutation_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_destroy_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_rejects_visual_destroy_while_emitted_stream_is_live);
    TEST_SIMPLE(test_scene_live_stream_count_tracks_multiple_emits);
    TEST_SIMPLE(test_scene_point_emit);
    TEST_SIMPLE(test_scene_path_emit);
    TEST_SIMPLE(test_scene_image_emit);
    TEST_SIMPLE(test_scene_empty_figure_emit_clear_only);
    TEST_SIMPLE(test_scene_point_emit_has_vertex_layout);
    TEST_SIMPLE(test_scene_second_emit_no_uploads_when_not_dirty);
    TEST_SIMPLE(test_scene_partial_update_uploads_only_range);
    TEST_SIMPLE(test_scene_repeated_partial_updates_across_frames);
    TEST_SIMPLE(test_scene_partial_update_merges_ranges_before_emit);
    TEST_SIMPLE(test_scene_multiple_panels_multiple_point_visuals_emit);
    TEST_SIMPLE(test_scene_mesh_indexed_default_color_emits_draw_indexed);
    TEST_SIMPLE(test_scene_indexed_primitive_emits_draw_indexed);
    TEST_SIMPLE(test_scene_shared_index_buffer_emits_one_upload);
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
    TEST_SIMPLE(test_scene_mesh_glsl_executes);
    TEST_SIMPLE(test_scene_path_glsl_executes);
    TEST_SIMPLE(test_scene_image_glsl_executes);
#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    TEST_SIMPLE(test_app_offscreen);
    TEST_SIMPLE(test_app_offscreen_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_has_nonblank_pixels);
    TEST_SIMPLE(test_app_offscreen_image_field_partial_update_changes_region);
    TEST_SIMPLE(test_scene_indexed_primitive_shading_updates_runtime);
    TEST_SIMPLE(test_app_offscreen_shared_field_mixed_runtime_updates);
    TEST_SIMPLE(test_app_offscreen_image_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_retained_render_second_frame);
    TEST_SIMPLE(test_app_offscreen_two_panel_points_light_both_halves);
    TEST_SIMPLE(test_app_offscreen_panel_three_visuals_all_drawn);
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
