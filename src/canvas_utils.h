#ifndef DVZ_CANVAS_UTILS_HEADER
#define DVZ_CANVAS_UTILS_HEADER

#include "../include/datoviz/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

// Enqueue an event.
static void _event_enqueue(DvzCanvas* canvas, DvzEvent event)
{
    ASSERT(canvas != NULL);
    DvzFifo* fifo = &canvas->event_queue;
    ASSERT(fifo != NULL);
    DvzEvent* ev = (DvzEvent*)calloc(1, sizeof(DvzEvent));
    *ev = event;
    dvz_fifo_enqueue(fifo, ev);
}



// Dequeue an event, immediately, or waiting until an event is available.
static DvzEvent _event_dequeue(DvzCanvas* canvas, bool wait)
{
    ASSERT(canvas != NULL);
    DvzFifo* fifo = &canvas->event_queue;
    ASSERT(fifo != NULL);
    DvzEvent* item = (DvzEvent*)dvz_fifo_dequeue(fifo, wait);
    DvzEvent out;
    out.type = DVZ_EVENT_NONE;
    if (item == NULL)
        return out;
    ASSERT(item != NULL);
    out = *item;
    FREE(item);
    return out;
}



// Whether there is at least one async callback.
static bool _has_async_callbacks(DvzCanvas* canvas, DvzEventType type)
{
    if (canvas == NULL)
        return false;
    ASSERT(canvas != NULL);
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
    {
        if (canvas->callbacks[i].type == type && canvas->callbacks[i].mode == DVZ_EVENT_MODE_ASYNC)
            return true;
    }
    return false;
}



// Whether there is at least one event callback.
static bool _has_event_callbacks(DvzCanvas* canvas, DvzEventType type)
{
    ASSERT(canvas != NULL);
    if (type == DVZ_EVENT_NONE || type == DVZ_EVENT_INIT)
        return true;
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
        if (canvas->callbacks[i].type == type)
            return true;
    return false;
}



// Consume an event, return the number of callbacks called.
static int _event_consume(DvzCanvas* canvas, DvzEvent ev, DvzEventMode mode)
{
    ASSERT(canvas != NULL);

    if (canvas->enable_lock)
        dvz_thread_lock(&canvas->event_thread);

    // HACK: we first call the callbacks with no param, then we call the callbacks with a non-zero
    // param. This is a way to use the param as a priority value. This is used by the scene FRAME
    // callback so that it occurs after the user callbacks.
    int n_callbacks = 0;
    DvzEventCallbackRegister* r = NULL;
    for (uint32_t pass = 0; pass < 2; pass++)
    {
        for (uint32_t i = 0; i < canvas->callbacks_count; i++)
        {
            r = &canvas->callbacks[i];
            // Will pass the user_data that was registered, to the callback function.
            ev.user_data = r->user_data;

            // Only call the callbacks registered for the specified type.
            if ((r->type == ev.type) &&              //
                (r->mode == mode) &&                 //
                (((pass == 0) && (r->param == 0)) || //
                 ((pass == 1) && (r->param > 0))))   //
            {
                r->callback(canvas, ev);
                n_callbacks++;
            }
        }
    }

    if (canvas->enable_lock)
        dvz_thread_unlock(&canvas->event_thread);

    return n_callbacks;
}



// Produce an event, call the sync callbacks, and enqueue the event if there is at least one async
// callback.
static int _event_produce(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);

    // Call the sync callbacks directly.
    int n_callbacks = _event_consume(canvas, ev, DVZ_EVENT_MODE_SYNC);

    // Enqueue the event only if there is at least one async callback for that event type.
    if (_has_async_callbacks(canvas, ev.type))
        _event_enqueue(canvas, ev);

    return n_callbacks;
}



// Event loop running in the background thread, waiting for events and dequeuing them.
static void* _event_thread(void* p_canvas)
{
    DvzCanvas* canvas = (DvzCanvas*)p_canvas;
    ASSERT(canvas != NULL);
    log_debug("starting event thread");

    DvzEvent ev;
    double avg_event_time = 0; // average event callback time across all event types
    double elapsed = 0;        // average time of the event callbacks in the current iteration
    int n_callbacks = 0;       // number of event callbacks in the current event loop iteration
    int counter = 0;           // number of iterations in the event loop
    int events_to_keep = 0;    // maximum number of pending events to keep in the queue

    while (true)
    {
        // log_trace("event thread awaits for events...");
        // Wait until an event is available
        ev = _event_dequeue(canvas, true);
        canvas->event_processing = ev.type; // type of the event being processed
        if (ev.type == DVZ_EVENT_NONE)
        {
            log_trace("received empty event, stopping the event thread");
            break;
        }

        // Logic to discard some events if the queue is getting overloaded because of long-running
        // callbacks.

        // TODO: there are ways to improve the mechanism dropping events from the queue when the
        // queue is getting overloaded. Doing it on a per-type basis, better estimating the avg
        // time taken by each callback, etc.

        // log_trace("event dequeued type %d, processing it...", ev.type);
        // process the dequeued task
        elapsed = _clock_get(&canvas->clock);
        n_callbacks = _event_consume(canvas, ev, DVZ_EVENT_MODE_ASYNC);
        elapsed = _clock_get(&canvas->clock) - elapsed;
        // NOTE: avoid division by zero.
        if (n_callbacks > 0)
            elapsed /= n_callbacks; // average duration of the events

        // Update the average event time.
        avg_event_time = ((avg_event_time * counter) + elapsed) / (counter + 1);
        if (avg_event_time > 0)
        {
            events_to_keep =
                CLIP(DVZ_MAX_EVENT_DURATION / avg_event_time, 1, DVZ_MAX_FIFO_CAPACITY);
            if (events_to_keep == DVZ_MAX_FIFO_CAPACITY)
                events_to_keep = 0;
        }

        // Handle event queue overloading: if events are enqueued faster than
        // they are consumed, we should discard the older events so that the
        // queue doesn't keep filling up.
        dvz_fifo_discard(&canvas->event_queue, events_to_keep);

        canvas->event_processing = DVZ_EVENT_NONE;
        counter++;
    }
    log_debug("end event thread");

    return NULL;
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzRenderpass default_renderpass(
    DvzGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, //
    bool overlay, bool pick)
{
    DvzRenderpass renderpass = dvz_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;

    VkClearValue clear_color_pick = {0};

    dvz_renderpass_clear(&renderpass, clear_color);
    dvz_renderpass_clear(&renderpass, clear_depth);
    if (pick)
        dvz_renderpass_clear(&renderpass, clear_color_pick);

    VkImageLayout layout =
        overlay ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Color attachment.
    dvz_renderpass_attachment(
        &renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(&renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    dvz_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    dvz_renderpass_attachment(
        &renderpass, 1, //
        DVZ_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Pick attachment.
    if (pick)
    {
        dvz_renderpass_attachment(
            &renderpass, 2, //
            DVZ_RENDERPASS_ATTACHMENT_PICK, DVZ_PICK_IMAGE_FORMAT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_renderpass_attachment_layout(
            &renderpass, 2, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_renderpass_attachment_ops(
            &renderpass, 2, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    }

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(&renderpass, 0, 1);
    if (pick)
        dvz_renderpass_subpass_attachment(&renderpass, 0, 2);
    dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    dvz_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_renderpass_subpass_dependency_access(
        &renderpass, 0, //
        0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static DvzRenderpass renderpass_overlay(DvzGpu* gpu, VkFormat format, VkImageLayout layout)
{
    DvzRenderpass renderpass = dvz_renderpass(gpu);

    // Color attachment.
    dvz_renderpass_attachment(
        &renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, layout);
    dvz_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    dvz_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_renderpass_subpass_dependency_access(
        &renderpass, 0, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static void
depth_image(DvzImages* depth_images, DvzRenderpass* renderpass, uint32_t width, uint32_t height)
{
    // Depth attachment
    dvz_images_format(depth_images, renderpass->attachments[1].format);
    dvz_images_size(depth_images, width, height, 1);
    dvz_images_tiling(depth_images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_memory(depth_images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(depth_images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth_images, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth_images, 0);
    dvz_images_create(depth_images);
}



static void pick_image(DvzImages* pick, DvzRenderpass* renderpass, uint32_t width, uint32_t height)
{
    dvz_images_format(pick, renderpass->attachments[2].format);
    dvz_images_size(pick, width, height, 1);
    dvz_images_tiling(pick, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(pick, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(pick, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(pick, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(pick, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_queue_access(pick, 0);
    dvz_images_create(pick);
}



#ifdef __cplusplus
}
#endif

#endif
