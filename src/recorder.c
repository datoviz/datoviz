/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Recorder                                                                                     */
/*************************************************************************************************/

#include "recorder.h"
#include "board.h"
#include "canvas.h"
#include "renderer.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void
_process_command(DvzRecorderCommand* record, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx)
{
    // NOTE: This function is called inside the presenter event loop.
    ANN(record);
    ANN(rd);
    ANN(cmds);
    ASSERT(img_idx < cmds->count);

    DvzCanvas* canvas = NULL;
    DvzBoard* board = NULL;

    ASSERT(
        record->object_type == DVZ_REQUEST_OBJECT_CANVAS ||
        record->object_type == DVZ_REQUEST_OBJECT_BOARD);
    bool is_canvas = record->object_type == DVZ_REQUEST_OBJECT_CANVAS;

    if (is_canvas)
    {
        canvas = dvz_renderer_canvas(rd, record->canvas_or_board_id);
        ANN(canvas);
    }
    else
    {
        board = dvz_renderer_board(rd, record->canvas_or_board_id);
        ANN(board);
    }
    ASSERT(canvas != NULL || board != NULL);

    DvzPipe* pipe = NULL;
    DvzDat* dat_indirect = NULL;

    switch (record->type)
    {

    case DVZ_RECORDER_BEGIN:
    {
        log_debug("recorder: begin (#%d)", img_idx);
        dvz_cmd_reset(cmds, img_idx);
        if (is_canvas)
            dvz_canvas_begin(canvas, cmds, img_idx);
        else
            dvz_board_begin(board, cmds, img_idx);
        break;
    }

    case DVZ_RECORDER_VIEWPORT:
    {
        float x = record->contents.v.offset[0];
        float y = record->contents.v.offset[1];
        float w = record->contents.v.shape[0];
        float h = record->contents.v.shape[1];

        // NOTE: ensure the scale is set.
        float scale = is_canvas ? canvas->scale : 1.0;
        scale = scale == 0 ? 1 : scale;

        log_debug(
            "recorder: viewport %0.0fx%0.0f -> %0.0fx%0.0f (#%d) (scale: %.2f)", //
            x, y, w, h, img_idx, scale);

        // Take DPI scaling into account. canvas->scale is set by the presenter in _create_canvas()
        vec2 offset = {x * scale, y * scale};
        vec2 shape = {w * scale, h * scale};

        if (is_canvas)
            dvz_canvas_viewport(canvas, cmds, img_idx, offset, shape);
        else
            dvz_board_viewport(board, cmds, img_idx, offset, shape);
        break;
    }

    case DVZ_RECORDER_DRAW:
    {
        uint32_t first_vertex = record->contents.draw.first_vertex;
        uint32_t vertex_count = record->contents.draw.vertex_count;
        uint32_t first_instance = record->contents.draw.first_instance;
        uint32_t instance_count = record->contents.draw.instance_count;

        log_debug(
            "recorder: draw direct from vertex #%d for %d vertices, %d instances from idx %d "
            "(#%d)", //
            first_vertex, vertex_count, instance_count, first_instance, img_idx);

        // NOTE: this function lazily create the pipe if needed, this is when the graphics pipeline
        // is created on the Vulkan side.

        pipe = dvz_renderer_pipe(rd, record->contents.draw.pipe_id);
        ANN(pipe);

        if (!dvz_pipe_complete(pipe))
        {
            log_error("cannot draw pipe with incomplete descriptor bindings");
            break;
        }

        dvz_pipe_draw(
            pipe, cmds, img_idx, first_vertex, vertex_count, first_instance, instance_count);
        break;
    }

    case DVZ_RECORDER_DRAW_INDEXED:
    {
        uint32_t first_index = record->contents.draw_indexed.first_index;
        uint32_t index_count = record->contents.draw_indexed.index_count;
        uint32_t vertex_offset = record->contents.draw_indexed.vertex_offset;
        uint32_t first_instance = record->contents.draw_indexed.first_instance;
        uint32_t instance_count = record->contents.draw_indexed.instance_count;

        log_debug(
            "recorder: draw indexed from index #%d for %d indices (#%d)", //
            first_index, index_count, img_idx);

        pipe = dvz_renderer_pipe(rd, record->contents.draw_indexed.pipe_id);
        ANN(pipe);

        if (!dvz_pipe_complete(pipe))
        {
            log_error("cannot draw pipe with incomplete descriptor bindings");
            break;
        }

        dvz_pipe_draw_indexed(
            pipe, cmds, img_idx, first_index, vertex_offset, index_count, first_instance,
            instance_count);
        break;
    }

    case DVZ_RECORDER_DRAW_INDIRECT:
    {
        pipe = dvz_renderer_pipe(rd, record->contents.draw_indirect.pipe_id);
        ANN(pipe);

        if (!dvz_pipe_complete(pipe))
        {
            log_error("cannot draw pipe with incomplete descriptor bindings");
            break;
        }

        uint32_t draw_count = record->contents.draw_indirect.draw_count;

        dat_indirect = dvz_renderer_dat(rd, record->contents.draw_indirect.dat_indirect_id);
        ANN(dat_indirect);

        dvz_pipe_draw_indirect(pipe, cmds, img_idx, dat_indirect, draw_count);
        break;
    }

    case DVZ_RECORDER_DRAW_INDEXED_INDIRECT:
    {
        pipe = dvz_renderer_pipe(rd, record->contents.draw_indirect.pipe_id);
        ANN(pipe);

        if (!dvz_pipe_complete(pipe))
        {
            log_error("cannot draw pipe with incomplete descriptor bindings");
            break;
        }

        uint32_t draw_count = record->contents.draw_indirect.draw_count;

        dat_indirect = dvz_renderer_dat(rd, record->contents.draw_indirect.dat_indirect_id);
        ANN(dat_indirect);

        dvz_pipe_draw_indexed_indirect(pipe, cmds, img_idx, dat_indirect, draw_count);
        break;
    }

    case DVZ_RECORDER_END:
    {
        log_debug("recorder: end (#%d)", img_idx);
        if (is_canvas)
            dvz_canvas_end(canvas, cmds, img_idx);
        else
            dvz_board_end(board, cmds, img_idx);
        break;
    }

    default:
    {
        log_error("unknown record command with type %d", record->type);
        break;
    }
    }
}



static inline bool _has_cache(DvzRecorder* recorder)
{
    ANN(recorder);
    return !(recorder->flags & DVZ_RECORDER_FLAGS_DISABLE_CACHE);
}



/*************************************************************************************************/
/*  Recorder functions                                                                           */
/*************************************************************************************************/

DvzRecorder* dvz_recorder(int flags)
{
    DvzRecorder* recorder = (DvzRecorder*)calloc(1, sizeof(DvzRecorder));
    recorder->flags = flags;
    recorder->capacity = DVZ_RECORDER_COMMAND_COUNT;
    recorder->commands = calloc(recorder->capacity, sizeof(DvzRecorderCommand));

    // Clear the recorder initially, and make sure it is set as dirty. This way, the command buffer
    // will be recorded at the very first frame.
    dvz_recorder_clear(recorder);

    return recorder;
}



void dvz_recorder_clear(DvzRecorder* recorder)
{
    ANN(recorder);
    log_debug("clear recorder commands");
    recorder->count = 0;
    dvz_recorder_set_dirty(recorder);
}



void dvz_recorder_append(DvzRecorder* recorder, DvzRecorderCommand rc)
{
    ANN(recorder);
    ASSERT(rc.canvas_or_board_id != 0);
    log_debug("append recorder command");

    if (recorder->count >= recorder->capacity)
    {
        recorder->capacity *= 2;
        REALLOC(recorder->commands, recorder->capacity * sizeof(DvzRecorderCommand))
    }
    ASSERT(recorder->count < recorder->capacity);
    recorder->commands[recorder->count++] = rc;
}



void dvz_recorder_set(DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx)
{
    ANN(recorder);

    // this function updates the command buffer for the given swapchain image index, only if needed
    if (_has_cache(recorder) && !recorder->dirty[img_idx])
        return;

    // Go through all record commands and update the command buffer
    for (uint32_t i = 0; i < recorder->count; i++)
    {
        _process_command(&recorder->commands[i], rd, cmds, img_idx);
    }

    recorder->dirty[img_idx] = false;
}



void dvz_recorder_cache(DvzRecorder* recorder, bool activate)
{
    ANN(recorder);
    if (activate)
        // Clear DISABLE_CACHE flag
        recorder->flags &= ~(DVZ_RECORDER_FLAGS_DISABLE_CACHE << 0);
    else
        // Set DISABLE_CACHE flag
        recorder->flags |= (DVZ_RECORDER_FLAGS_DISABLE_CACHE << 0);
    ASSERT(_has_cache(recorder) == activate);
    log_debug("set recorder cache to %d", activate);
}



bool dvz_recorder_is_dirty(DvzRecorder* recorder, uint32_t img_idx)
{
    ANN(recorder);
    return !_has_cache(recorder) || recorder->dirty[img_idx];
}



void dvz_recorder_set_dirty(DvzRecorder* recorder)
{
    ANN(recorder);

    // Reset dirty to true for all swapchain image indices.
    if (_has_cache(recorder))
        memset(recorder->dirty, 1, sizeof(recorder->dirty));
}



uint32_t dvz_recorder_size(DvzRecorder* recorder)
{
    ANN(recorder);
    return (recorder->count);
}



void dvz_recorder_destroy(DvzRecorder* recorder)
{
    ANN(recorder);
    FREE(recorder->commands);
    FREE(recorder);
}
