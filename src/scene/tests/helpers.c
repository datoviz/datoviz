/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene test helpers                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "frame_plan/emit.h"
#include "../../drp2/_stream.h"
#include "datoviz/canvas.h"
#include "datoviz/drp2.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "helpers.h"
#include "testing.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/*************************************************************************************************/

bool _captured_log_contains(const TstContext* suite, const char* needle)
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
const uint8_t* _pixel_at(
    const uint8_t* rgba, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
    ANN(rgba);
    ASSERT(x < width);
    ASSERT(y < height);
    return &rgba[4 * ((uint64_t)y * width + x)];
}


/**
 * Probe whether the current runtime can create a Vulkan instance for scene runtime tests.
 *
 * @return true when the runtime can create a Vulkan instance, false otherwise
 */
bool _scene_vklite_runtime_available(void)
{
    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    if (instance == NULL)
    {
        log_warn("scene vklite runtime unavailable because Vulkan instance creation failed");
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
void _scene_canvas_drp2_draw(
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


/**
 * Read a text fixture file into an owned NUL-terminated string.
 *
 * @param path the fixture path
 * @return the owned fixture contents, or NULL on failure
 */
char* _read_text_fixture(const char* path)
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
int _assert_stream_matches_fixture(
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
uint32_t _stream_write_buffer_count(const DvzDrp2CommandStream* stream)
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
 * Count visual-data WRITE_BUFFER commands in a DRP2 stream.
 *
 * @param stream the command stream
 * @return number of WRITE_BUFFER commands excluding per-panel common uniform uploads
 */
uint32_t _stream_visual_write_buffer_count(const DvzDrp2CommandStream* stream)
{
    ANN(stream);

    uint32_t count = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* cmd = dvz_drp2_stream_get(stream, i);
        if (cmd->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
            cmd->u.write_buffer.size != sizeof(DvzMVP) &&
            cmd->u.write_buffer.size != sizeof(DvzSceneViewportUniform))
        {
            count++;
        }
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
uint32_t _stream_write_buffer_range_count(
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
uint32_t _stream_draw_count(const DvzDrp2CommandStream* stream)
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
uint32_t _stream_set_vertex_buffer_count(const DvzDrp2CommandStream* stream)
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


uint32_t _stream_set_index_buffer_count(const DvzDrp2CommandStream* stream)
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


uint32_t _stream_draw_indexed_count(const DvzDrp2CommandStream* stream)
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


uint32_t _stream_create_buffer_size_count(
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
