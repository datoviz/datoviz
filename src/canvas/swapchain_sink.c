/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas swapchain sink (stub)                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include <stdlib.h>

#include "_assertions.h"
#include "_log.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzCanvas* canvas;
} DvzCanvasSwapchainState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzCanvasSwapchainState* canvas_swapchain_state(DvzStreamSink* sink)
{
    ANN(sink);
    DvzCanvasSwapchainState* state = (DvzCanvasSwapchainState*)sink->backend_data;
    if (!state)
    {
        state = (DvzCanvasSwapchainState*)calloc(1, sizeof(DvzCanvasSwapchainState));
        ANN(state);
        sink->backend_data = state;
    }
    return state;
}



/*************************************************************************************************/
/*  Backend callbacks                                                                            */
/*************************************************************************************************/

static bool canvas_swapchain_probe(const void* config)
{
    (void)config;
    return true;
}



static int canvas_swapchain_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    DvzCanvasSwapchainState* state = canvas_swapchain_state(sink);
    ANN(state);
    state->canvas = (DvzCanvas*)config;
    if (!state->canvas)
    {
        log_error("swapchain sink requires a valid canvas handle");
        return -1;
    }
    return 0;
}



static int canvas_swapchain_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    (void)sink;
    (void)frame;
    return 0;
}



static int canvas_swapchain_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    (void)sink;
    (void)wait_value;
    return 0;
}



static int canvas_swapchain_stop(DvzStreamSink* sink)
{
    (void)sink;
    return 0;
}



static int canvas_swapchain_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    (void)sink;
    (void)frame;
    return 0;
}



static void canvas_swapchain_destroy(DvzStreamSink* sink)
{
    if (!sink || !sink->backend_data)
    {
        return;
    }
    free(sink->backend_data);
    sink->backend_data = NULL;
}



/*************************************************************************************************/
/*  Backend descriptor                                                                           */
/*************************************************************************************************/

static const DvzStreamSinkBackend CANVAS_SWAPCHAIN_SINK = {
    .name = "canvas_swapchain",
    .probe = canvas_swapchain_probe,
    .create = canvas_swapchain_create,
    .start = canvas_swapchain_start,
    .submit = canvas_swapchain_submit,
    .stop = canvas_swapchain_stop,
    .update = canvas_swapchain_update,
    .destroy = canvas_swapchain_destroy,
};



/**
 * Expose the swapchain backend descriptor so it can be registered with the stream registry.
 *
 * @returns backend descriptor
 */
const DvzStreamSinkBackend* dvz_canvas_swapchain_sink_backend(void)
{
    return &CANVAS_SWAPCHAIN_SINK;
}
