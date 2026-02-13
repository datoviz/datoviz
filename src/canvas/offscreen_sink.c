/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas offscreen sink                                                                        */
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

typedef struct DvzCanvasOffscreenSinkState
{
    DvzStreamFrame last_frame;
    bool has_frame;
} DvzCanvasOffscreenSinkState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool canvas_offscreen_probe(const void* config)
{
    return config != NULL;
}



static int canvas_offscreen_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    const DvzCanvas* canvas = (const DvzCanvas*)config;
    if (!canvas)
    {
        log_error("offscreen sink requires a valid canvas handle");
        return -1;
    }

    DvzCanvasOffscreenSinkState* state =
        (DvzCanvasOffscreenSinkState*)dvz_calloc(1, sizeof(DvzCanvasOffscreenSinkState));
    ANN(state);
    state->last_frame.memory_fd = -1;
    state->last_frame.wait_semaphore_fd = -1;
    state->has_frame = false;
    sink->backend_data = state;
    return 0;
}



static int canvas_offscreen_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    DvzCanvasOffscreenSinkState* state = (DvzCanvasOffscreenSinkState*)sink->backend_data;
    ANN(state);
    if (frame->extent.width == 0 || frame->extent.height == 0)
    {
        log_error("offscreen sink cannot start with a zero-sized frame extent");
        return -1;
    }
    state->last_frame = *frame;
    state->has_frame = true;
    return 0;
}



static int canvas_offscreen_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    (void)wait_value;
    DvzCanvasOffscreenSinkState* state = (DvzCanvasOffscreenSinkState*)sink->backend_data;
    ANN(state);
    if (!state->has_frame)
    {
        log_error("offscreen sink submit called before start");
        return -1;
    }
    return 0;
}



static int canvas_offscreen_stop(DvzStreamSink* sink)
{
    ANN(sink);
    DvzCanvasOffscreenSinkState* state = (DvzCanvasOffscreenSinkState*)sink->backend_data;
    ANN(state);
    state->has_frame = false;
    return 0;
}



static int canvas_offscreen_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    DvzCanvasOffscreenSinkState* state = (DvzCanvasOffscreenSinkState*)sink->backend_data;
    ANN(state);
    state->last_frame = *frame;
    state->has_frame = true;
    return 0;
}



static void canvas_offscreen_destroy(DvzStreamSink* sink)
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

static const DvzStreamSinkBackend CANVAS_OFFSCREEN_SINK = {
    .name = "canvas_offscreen",
    .probe = canvas_offscreen_probe,
    .create = canvas_offscreen_create,
    .start = canvas_offscreen_start,
    .submit = canvas_offscreen_submit,
    .stop = canvas_offscreen_stop,
    .update = canvas_offscreen_update,
    .destroy = canvas_offscreen_destroy,
};



/**
 * Expose the offscreen sink backend used by no-present canvas mode.
 *
 * @returns backend descriptor
 */
const DvzStreamSinkBackend* dvz_canvas_offscreen_sink_backend(void)
{
    return &CANVAS_OFFSCREEN_SINK;
}
