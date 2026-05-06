/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas live-image sink                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzCanvasLiveImageSinkState
{
    DvzCanvasLiveImageSinkConfig cfg;
    DvzStreamFrame frame;
    bool has_frame;
    uint64_t frame_id;
} DvzCanvasLiveImageSinkState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool canvas_live_image_probe(const void* config)
{
    const DvzCanvasLiveImageSinkConfig* cfg = (const DvzCanvasLiveImageSinkConfig*)config;
    return cfg != NULL && cfg->callback != NULL;
}



static int canvas_live_image_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    const DvzCanvasLiveImageSinkConfig* cfg = (const DvzCanvasLiveImageSinkConfig*)config;
    if (!cfg || !cfg->callback)
    {
        log_error("live-image sink requires a callback");
        return -1;
    }

    DvzCanvasLiveImageSinkState* state =
        (DvzCanvasLiveImageSinkState*)dvz_calloc(1, sizeof(DvzCanvasLiveImageSinkState));
    ANN(state);
    state->cfg = *cfg;
    state->frame.memory_fd = -1;
    state->frame.wait_semaphore_fd = -1;
    state->has_frame = false;
    state->frame_id = 0;
    sink->backend_data = state;
    return 0;
}



static int canvas_live_image_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    DvzCanvasLiveImageSinkState* state = (DvzCanvasLiveImageSinkState*)sink->backend_data;
    ANN(state);
    state->frame = *frame;
    state->has_frame = true;
    return 0;
}



static int canvas_live_image_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    DvzCanvasLiveImageSinkState* state = (DvzCanvasLiveImageSinkState*)sink->backend_data;
    ANN(state);
    if (!state->has_frame)
    {
        log_error("live-image sink submit called before start");
        return -1;
    }

    DvzCanvasLiveImageFrame frame = {
        .frame_id = state->frame_id++,
        .wait_value = wait_value,
        .color_format = state->frame.color_format != VK_FORMAT_UNDEFINED ?
                            state->frame.color_format :
                            DVZ_DEFAULT_COLOR_FORMAT,
        .image = state->frame.image,
        .image_view = state->frame.image_view,
        .command_buffer = state->frame.command_buffer,
        .extent = state->frame.extent,
        .handles_dirty = state->frame.handles_dirty,
        .memory_fd = state->frame.memory_fd,
        .wait_semaphore_fd = state->frame.wait_semaphore_fd,
    };
    return state->cfg.callback(&frame, state->cfg.user_data);
}



static int canvas_live_image_stop(DvzStreamSink* sink)
{
    ANN(sink);
    DvzCanvasLiveImageSinkState* state = (DvzCanvasLiveImageSinkState*)sink->backend_data;
    ANN(state);
    state->has_frame = false;
    return 0;
}



static int canvas_live_image_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    DvzCanvasLiveImageSinkState* state = (DvzCanvasLiveImageSinkState*)sink->backend_data;
    ANN(state);
    state->frame = *frame;
    state->has_frame = true;
    return 0;
}



static void canvas_live_image_destroy(DvzStreamSink* sink)
{
    if (!sink || !sink->backend_data)
    {
        return;
    }
    dvz_free(sink->backend_data);
    sink->backend_data = NULL;
}



/*************************************************************************************************/
/*  Backend descriptor                                                                           */
/*************************************************************************************************/

static const DvzStreamSinkBackend CANVAS_LIVE_IMAGE_SINK = {
    .name = "canvas_live_image",
    .probe = canvas_live_image_probe,
    .create = canvas_live_image_create,
    .start = canvas_live_image_start,
    .submit = canvas_live_image_submit,
    .stop = canvas_live_image_stop,
    .update = canvas_live_image_update,
    .destroy = canvas_live_image_destroy,
};



/**
 * Expose the live-image sink backend used by optional canvas frame publication.
 *
 * @returns backend descriptor
 */
const DvzStreamSinkBackend* dvz_canvas_live_image_sink_backend(void)
{
    return &CANVAS_LIVE_IMAGE_SINK;
}
