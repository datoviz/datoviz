/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Recorder                                                                                     */
/*************************************************************************************************/

#include "recorder.h"
#include "canvas.h"
#include "renderer.h"

// HACK: we need the scale push constant offset common to all visuals.
#include "scene/visual.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

#define GET_CANVAS                                                                                \
    ANN(recorder);                                                                                \
    ANN(rd);                                                                                      \
    ANN(cmds);                                                                                    \
    ASSERT(record->object_type == DVZ_REQUEST_OBJECT_CANVAS);                                     \
    DvzCanvas* canvas = dvz_renderer_canvas(rd, record->canvas_id);                               \
    ANN(canvas);

#define GET_PIPE(pipe_id)                                                                         \
    DvzPipe* pipe = dvz_renderer_pipe(rd, pipe_id);                                               \
    ANN(pipe);                                                                                    \
    if (!dvz_pipe_complete(pipe))                                                                 \
    {                                                                                             \
        log_error("cannot draw pipe with incomplete descriptor bindings");                        \
        return;                                                                                   \
    }

#define PUSH_CANVAS_SCALE                                                                         \
    float scale = canvas->scale;                                                                  \
    scale = scale == 0 ? 1 : scale;                                                               \
    if ((canvas->flags & DVZ_CANVAS_FLAGS_PUSH_SCALE) != 0)                                       \
    {                                                                                             \
        dvz_cmd_push(                                                                             \
            cmds, img_idx, pipe->descriptors.dslots, DVZ_SHADER_VERTEX | DVZ_SHADER_FRAGMENT,     \
            DVZ_PUSH_SCALE_OFFSET, DVZ_PUSH_SCALE_SIZE, &scale);                                  \
    }



static void _process_begin(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    dvz_cmd_reset(cmds, img_idx);
    log_debug("recorder: begin (#%d)", img_idx);
    dvz_canvas_begin(canvas, cmds, img_idx);
}

static void _process_viewport(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    float x = record->contents.v.offset[0];
    float y = record->contents.v.offset[1];
    float w = record->contents.v.shape[0];
    float h = record->contents.v.shape[1];

    // NOTE: ensure the scale is set.
    float scale = canvas->scale;
    scale = scale == 0 ? 1 : scale;

    log_debug(
        "recorder: viewport %0.0fx%0.0f -> %0.0fx%0.0f (#%d) (scale: %.2f)", //
        x, y, w, h, img_idx, scale);

    // Take DPI scaling into account. canvas->scale is set by the presenter in _create_canvas()
    vec2 offset = {x * scale, y * scale};
    vec2 shape = {w * scale, h * scale};

    dvz_canvas_viewport(canvas, cmds, img_idx, offset, shape);
}

static void _process_push(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    DvzRecorderPush* p = &record->contents.p;
    ANN(p);
    ASSERT(p->size > 0);
    ANN(p->data);

    log_debug("recorder: push constant offset=%d, size=%d", p->offset, p->size);

    GET_PIPE(p->pipe_id)

    dvz_cmd_push(
        cmds, img_idx, pipe->descriptors.dslots, (VkShaderStageFlagBits)p->shader_stages, //
        p->offset, p->size, p->data);

    // NOTE: the data was copied by the requester, now we can free it.

    recorder->to_free = p->data;
}

static void _process_draw(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

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

    GET_PIPE(record->contents.draw.pipe_id)

    // HACK: push the canvas scale to the GPU shaders.
    PUSH_CANVAS_SCALE

    dvz_pipe_draw(pipe, cmds, img_idx, first_vertex, vertex_count, first_instance, instance_count);
}

static void _process_draw_indexed(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    uint32_t first_index = record->contents.draw_indexed.first_index;
    uint32_t index_count = record->contents.draw_indexed.index_count;
    uint32_t vertex_offset = record->contents.draw_indexed.vertex_offset;
    uint32_t first_instance = record->contents.draw_indexed.first_instance;
    uint32_t instance_count = record->contents.draw_indexed.instance_count;

    log_debug(
        "recorder: draw indexed from index #%d for %d indices (#%d)", //
        first_index, index_count, img_idx);

    GET_PIPE(record->contents.draw_indexed.pipe_id)

    // HACK: push the canvas scale to the GPU shaders.
    PUSH_CANVAS_SCALE

    dvz_pipe_draw_indexed(
        pipe, cmds, img_idx, first_index, vertex_offset, index_count, first_instance,
        instance_count);
}

static void _process_draw_indirect(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    GET_PIPE(record->contents.draw_indirect.pipe_id)

    uint32_t draw_count = record->contents.draw_indirect.draw_count;

    DvzDat* dat_indirect = dvz_renderer_dat(rd, record->contents.draw_indirect.dat_indirect_id);
    ANN(dat_indirect);

    // HACK: push the canvas scale to the GPU shaders.
    PUSH_CANVAS_SCALE

    dvz_pipe_draw_indirect(pipe, cmds, img_idx, dat_indirect, draw_count);
}

static void _process_draw_indexed_indirect(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    GET_PIPE(record->contents.draw_indirect.pipe_id)

    uint32_t draw_count = record->contents.draw_indirect.draw_count;

    DvzDat* dat_indirect = dvz_renderer_dat(rd, record->contents.draw_indirect.dat_indirect_id);
    ANN(dat_indirect);

    // HACK: push the canvas scale to the GPU shaders.
    PUSH_CANVAS_SCALE

    dvz_pipe_draw_indexed_indirect(pipe, cmds, img_idx, dat_indirect, draw_count);
}

static void _process_end(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data)
{
    GET_CANVAS

    log_debug("recorder: end (#%d)", img_idx);
    dvz_canvas_end(canvas, cmds, img_idx);
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
    recorder->commands =
        (DvzRecorderCommand*)calloc(recorder->capacity, sizeof(DvzRecorderCommand));

    // Clear the recorder initially, and make sure it is set as dirty. This way, the command buffer
    // will be recorded at the very first frame.
    dvz_recorder_clear(recorder);

    // Register the default recorder callbacks. The caller can customize these.
    dvz_recorder_register(recorder, DVZ_RECORDER_BEGIN, _process_begin, NULL);
    dvz_recorder_register(recorder, DVZ_RECORDER_DRAW, _process_draw, NULL);
    dvz_recorder_register(recorder, DVZ_RECORDER_DRAW_INDEXED, _process_draw_indexed, NULL);
    dvz_recorder_register(recorder, DVZ_RECORDER_DRAW_INDIRECT, _process_draw_indirect, NULL);
    dvz_recorder_register(
        recorder, DVZ_RECORDER_DRAW_INDEXED_INDIRECT, _process_draw_indexed_indirect, NULL);
    dvz_recorder_register(recorder, DVZ_RECORDER_VIEWPORT, _process_viewport, NULL);
    dvz_recorder_register(recorder, DVZ_RECORDER_PUSH, _process_push, NULL);
    dvz_recorder_register(recorder, DVZ_RECORDER_END, _process_end, NULL);

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
    ASSERT(rc.canvas_id != 0);
    log_debug("append recorder command");

    if (recorder->count >= recorder->capacity)
    {
        recorder->capacity *= 2;
        REALLOC(
            DvzRecorderCommand*, recorder->commands,
            recorder->capacity * sizeof(DvzRecorderCommand))
    }
    ASSERT(recorder->count < recorder->capacity);
    recorder->commands[recorder->count++] = rc;
}



void dvz_recorder_register(
    DvzRecorder* recorder, DvzRecorderCommandType ctype, DvzRecorderCallback cb, void* user_data)
{
    ANN(recorder);
    ASSERT(0 < (int)ctype);
    ASSERT((int)ctype < DVZ_RECORDER_COUNT);
    if (cb == NULL)
    {
        log_debug("registering empty recorder callback for record type %d", (int)ctype);
    }
    log_trace("register callback for recorder command type %d", (int)ctype);
    recorder->callbacks[(uint32_t)ctype] = cb;
    recorder->callback_user_data[(uint32_t)ctype] = user_data;
}



void dvz_recorder_set(DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx)
{
    ANN(recorder);
    ASSERT(img_idx < DVZ_MAX_SWAPCHAIN_IMAGES);

    // this function updates the command buffer for the given swapchain image index, only if needed
    if (_has_cache(recorder) && !recorder->dirty[img_idx])
        return;

    // Go through all record commands and update the command buffer
    for (uint32_t i = 0; i < recorder->count; i++)
    {
        DvzRecorderCommand* record = &recorder->commands[i];
        // Get the index which is the record type enum number.
        uint32_t cb_idx = (uint32_t)record->type;
        if (cb_idx >= DVZ_RECORDER_COUNT)
        {
            log_error("unknown record type %d, skipping record #%d", cb_idx, i);
            continue;
        }

        // This index is used to fetch the right callback (only one per record type).
        ASSERT(cb_idx < DVZ_RECORDER_COUNT);
        DvzRecorderCallback cb = recorder->callbacks[cb_idx];
        // Same for the callback user data.
        void* user_data = recorder->callback_user_data[cb_idx];
        if (cb == NULL)
        {
            log_warn(
                "no recorder callback registered for type %d, skipping record #%d", cb_idx, i);
            continue;
        }

        ANN(cb);
        // We call the recorder callback for the record type.
        cb(recorder, rd, cmds, img_idx, record, user_data);
    }

    recorder->dirty[img_idx] = false;


    // HACK: push constant data value once all command buffers have been recorded.
    bool zeros[DVZ_MAX_SWAPCHAIN_IMAGES] = {0};
    if (recorder->to_free != NULL &&
        memcmp(recorder->dirty, zeros, DVZ_MAX_SWAPCHAIN_IMAGES * sizeof(bool)) == 0)
    {
        log_trace("free push constant copy after finished recording the command buffer");
        FREE(recorder->to_free);
    }
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
