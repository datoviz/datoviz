#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../src/imgui.h"
#include "../src/transfers.h"
#include "../src/vklite_utils.h"
#include <stdlib.h>


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_NEVER                        -1000000
#define VKL_MOUSE_CLICK_MAX_DELAY        .25
#define VKL_MOUSE_CLICK_MAX_SHIFT        5
#define VKL_MOUSE_DOUBLE_CLICK_MAX_DELAY .2
#define VKL_KEY_PRESS_DELAY              .05



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklRenderpass
default_renderpass(VklGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, bool overlay)
{
    VklRenderpass renderpass = vkl_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;

    vkl_renderpass_clear(&renderpass, clear_color);
    vkl_renderpass_clear(&renderpass, clear_depth);

    VkImageLayout layout =
        overlay ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Color attachment.
    vkl_renderpass_attachment(
        &renderpass, 0, //
        VKL_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_layout(&renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    vkl_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    vkl_renderpass_attachment(
        &renderpass, 1, //
        VKL_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    vkl_renderpass_subpass_attachment(&renderpass, 0, 0);
    vkl_renderpass_subpass_attachment(&renderpass, 0, 1);
    vkl_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    vkl_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    vkl_renderpass_subpass_dependency_access(
        &renderpass, 0, 0,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static VklRenderpass renderpass_overlay(VklGpu* gpu, VkFormat format, VkImageLayout layout)
{
    VklRenderpass renderpass = vkl_renderpass(gpu);

    // Color attachment.
    vkl_renderpass_attachment(
        &renderpass, 0, //
        VKL_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_layout(
        &renderpass, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, layout);
    vkl_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    vkl_renderpass_attachment(
        &renderpass, 1, //
        VKL_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    vkl_renderpass_subpass_attachment(&renderpass, 0, 0);
    vkl_renderpass_subpass_attachment(&renderpass, 0, 1);
    vkl_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    vkl_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    vkl_renderpass_subpass_dependency_access(
        &renderpass, 0, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static void
depth_image(VklImages* depth_images, VklRenderpass* renderpass, uint32_t width, uint32_t height)
{
    // Depth attachment
    vkl_images_format(depth_images, renderpass->attachments[1].format);
    vkl_images_size(depth_images, width, height, 1);
    vkl_images_tiling(depth_images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    vkl_images_memory(depth_images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_layout(depth_images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_images_aspect(depth_images, VK_IMAGE_ASPECT_DEPTH_BIT);
    vkl_images_queue_access(depth_images, 0);
    vkl_images_create(depth_images);
}



static void blank_commands(VklCanvas* canvas, VklCommands* cmds, uint32_t cmd_idx)
{
    vkl_cmd_begin(cmds, cmd_idx);
    vkl_cmd_begin_renderpass(cmds, cmd_idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_end_renderpass(cmds, cmd_idx);
    vkl_cmd_end(cmds, cmd_idx);
}



static inline bool _all_true(uint32_t n, bool* arr)
{
    bool all_updated = true;
    for (uint32_t i = 0; i < n; i++)
    {
        if (!arr[i])
        {
            all_updated = false;
            break;
        }
    }
    return all_updated;
}



/*************************************************************************************************/
/*  Public event sending                                                                         */
/*************************************************************************************************/

static int _event_callbacks(VklCanvas* canvas, VklEvent* event)
{
    // NOTE: no need for thread synchronization as long as only the event thread manipulates
    // the event callbacks.
    int n_callbacks = 0;
    vkl_thread_lock(&canvas->event_thread);
    for (uint32_t i = 0; i < canvas->event_callbacks_count; i++)
    {
        // Will pass the user_data that was registered, to the callback function.
        event->user_data = canvas->event_callbacks[i].user_data;

        // Only call the callbacks registered for the specified type.
        if (canvas->event_callbacks[i].type == event->type)
        {
            canvas->event_callbacks[i].callback(canvas, *event);
            n_callbacks++;
        }
    }
    vkl_thread_unlock(&canvas->event_thread);
    return n_callbacks;
}



/*************************************************************************************************/
/*  Event thread                                                                                 */
/*************************************************************************************************/

static void* _event_thread(void* p_canvas)
{
    VklCanvas* canvas = (VklCanvas*)p_canvas;
    ASSERT(canvas != NULL);
    log_debug("starting event thread");

    VklEvent* ev = NULL;
    double avg_event_time = 0; // average event callback time across all event types
    double elapsed = 0;        // average time of the event callbacks in the current iteration
    int n_callbacks = 0;       // number of event callbacks in the current event loop iteration
    int counter = 0;           // number of iterations in the event loop
    int events_to_keep = 0;    // maximum number of pending events to keep in the queue

    while (true)
    {
        // log_trace("event thread awaits for events...");
        // Wait until an event is available
        ev = vkl_event_dequeue(canvas, true);
        canvas->event_processing = ev->type; // type of the event being processed
        if (ev->type == VKL_EVENT_NONE)
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
        n_callbacks = _event_callbacks(canvas, ev);
        elapsed = _clock_get(&canvas->clock) - elapsed;
        // NOTE: avoid division by zero.
        if (n_callbacks > 0)
            elapsed /= n_callbacks; // average duration of the events

        // Update the average event time.
        avg_event_time = ((avg_event_time * counter) + elapsed) / (counter + 1);
        if (avg_event_time > 0)
        {
            events_to_keep =
                CLIP(VKL_MAX_EVENT_DURATION / avg_event_time, 1, VKL_MAX_FIFO_CAPACITY);
            if (events_to_keep == VKL_MAX_FIFO_CAPACITY)
                events_to_keep = 0;
        }

        // Handle event queue overloading: if events are enqueued faster than
        // they are consumed, we should discard the older events so that the
        // queue doesn't keep filling up.
        vkl_fifo_discard(&canvas->event_queue, events_to_keep);

        canvas->event_processing = VKL_EVENT_NONE;
        counter++;
    }
    log_debug("end event thread");

    return NULL;
}



/*************************************************************************************************/
/*  Canvas-specific transfers                                                                    */
/*************************************************************************************************/

static void process_buffer_upload_immediate(VklCanvas* canvas, VklTransfer tr, uint32_t img_idx)
{
    // Must be called by the main event loop, just after the fence for the current frame
    // so we're sure the buffer region corresponding to the current swapchain image is not being
    // used and we can update the buffer safely.
    ASSERT(tr.type == VKL_TRANSFER_BUFFER_UPLOAD_IMMEDIATE);

    // Size of the buffer to transfer.
    VkDeviceSize region_size = tr.u.buf.size;
    ASSERT(region_size > 0);

    VklBufferRegions* br = &tr.u.buf.regions;
    VkDeviceSize alsize = br->aligned_size;
    if (alsize == 0)
        alsize = region_size;
    ASSERT(alsize > 0);
    ASSERT(alsize >= region_size);

    ASSERT(tr.u.buf.regions.count > 0);

    // We only upload to the buffer region that corresponds to the current swapchain image, to
    // avoid having to deal with synchronization.

    // Copy the data as many times as there are buffer regions, and make sure the array is
    // aligned if using a UNIFORM buffer.
    VklPointer pointer = aligned_repeat(region_size, tr.u.buf.data, 1, br->alignment);
    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_upload(br->buffer, br->offsets[img_idx] + tr.u.buf.offset, alsize, pointer.pointer);
    ALIGNED_FREE(pointer)
}



static void _immediate_transfers(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    // Special FIFO queue for IMMEDIATE transfers.
    VklFifo* fifo = &canvas->immediate_queue;
    ASSERT(fifo != NULL);

    // See if there is a current _immediate transfer going on.
    VklTransfer* tr = canvas->immediate_transfer_cur;
    // If not, dequeue.
    if (tr == NULL)
    {
        tr = vkl_fifo_dequeue(fifo, false);
    }
    // Here, we have nothing to do.
    if (tr == NULL)
        return;
    // Here we have a IMMEDIATE transfer to process, as many times as there are swap chain images.
    ASSERT(tr != NULL);

    // Current swapchain image.
    uint32_t n = tr->u.buf.update_count; // number of buffer regions to update

    // 2 cases: only 1 region to update (any swapchain image), or 1 per swapchain image.
    ASSERT(n == 1 || n == canvas->swapchain.img_count);
    uint32_t img_idx = CLIP(canvas->swapchain.img_idx, 0, tr->u.buf.regions.count - 1);
    // ASSERT(img_idx < tr->u.buf.regions.count);
    ASSERT(img_idx < VKL_MAX_SWAPCHAIN_IMAGES);

    // Skip the update if this swapchain image has already been processed.
    if (canvas->immediate_transfer_updated[img_idx])
        return;

    // Mark the dequeued transfer as being currently processed.
    canvas->immediate_transfer_cur = tr;

    // Process it.
    process_buffer_upload_immediate(canvas, *tr, img_idx);

    // Mark the buffer region corresponding to the current swapchain image as done.
    canvas->immediate_transfer_updated[img_idx] = true;

    // If all regions corresponding to all swapchain images have been updated, reset.
    if (n == 1 || _all_true(n, canvas->immediate_transfer_updated))
    {
        memset(canvas->immediate_transfer_updated, 0, n * sizeof(bool));
        canvas->immediate_transfer_cur = NULL;
    }
}



/*************************************************************************************************/
/*  Backend-specific event callbacks                                                             */
/*************************************************************************************************/

static void _glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    VklCanvas* canvas = (VklCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    // Special handling of ESC key.
    if (canvas->window->close_on_esc && action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        canvas->window->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
        return;
    }

    VklKeyType type = {0};
    VklKeyCode key_code = {0};

    // Find the key event type.
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        type = VKL_KEY_PRESS;
    else
        type = VKL_KEY_RELEASE;

    // NOTE: we use the GLFW key codes here, should actually do a proper mapping between GLFW
    // key codes and Visky key codes.
    key_code = key;

    // Enqueue the key event.
    vkl_event_key(canvas, type, key_code, mods);
}

static void _glfw_wheel_callback(GLFWwindow* window, double dx, double dy)
{
    VklCanvas* canvas = (VklCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    vkl_event_mouse_wheel(canvas, (vec2){dx, dy});
}

static void _glfw_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    VklCanvas* canvas = (VklCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    // Find mouse button action type
    VklMouseButtonType type = {0};
    if (action == GLFW_PRESS)
        type = VKL_MOUSE_PRESS;
    else
        type = VKL_MOUSE_RELEASE;

    // Map mouse button.
    VklMouseButton b = {0};
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        b = VKL_MOUSE_BUTTON_LEFT;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        b = VKL_MOUSE_BUTTON_RIGHT;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        b = VKL_MOUSE_BUTTON_MIDDLE;

    // NOTE: Visky modifiers code must match GLFW
    vkl_event_mouse_button(canvas, type, b, mods);
}

static void _glfw_move_callback(GLFWwindow* window, double xpos, double ypos)
{
    VklCanvas* canvas = (VklCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    vkl_event_mouse_move(canvas, (vec2){xpos, ypos});
}

static void _glfw_frame_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    GLFWwindow* w = canvas->window->backend_window;
    ASSERT(w != NULL);

    glm_vec2_copy(canvas->mouse.cur_pos, canvas->mouse.last_pos);

    // log_debug("mouse event %d", canvas->frame_idx);
    canvas->mouse.prev_state = canvas->mouse.cur_state;

    // Mouse move event.
    double xpos, ypos;
    glfwGetCursorPos(w, &xpos, &ypos);
    vec2 pos = {xpos, ypos};
    if (canvas->mouse.cur_pos[0] != pos[0] || canvas->mouse.cur_pos[1] != pos[1])
        vkl_event_mouse_move(canvas, pos);

    // TODO
    // // Reset click events as soon as the next loop iteration after they were raised.
    // if (mouse->cur_state == VKL_MOUSE_STATE_CLICK ||
    //     mouse->cur_state == VKL_MOUSE_STATE_DOUBLE_CLICK)
    // {
    //     mouse->cur_state = VKL_MOUSE_STATE_INACTIVE;
    //     mouse->button = VKL_MOUSE_BUTTON_NONE;
    // }
}

static void _backend_next_frame(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);

    // Reset wheel event.
    if (canvas->mouse.cur_state == VKL_MOUSE_STATE_WHEEL)
    {
        // log_debug("reset wheel state %d", canvas->frame_idx);
        canvas->mouse.cur_state = VKL_MOUSE_STATE_INACTIVE;
    }
}

static void backend_event_callbacks(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->app != NULL);
    switch (canvas->app->backend)
    {
    case VKL_BACKEND_GLFW:;
        ASSERT(canvas->window != NULL);
        GLFWwindow* w = canvas->window->backend_window;

        // The canvas pointer will be available to callback functions.
        glfwSetWindowUserPointer(w, canvas);

        // Register the key callback.
        glfwSetKeyCallback(w, _glfw_key_callback);

        // Register the mouse wheel callback.
        glfwSetScrollCallback(w, _glfw_wheel_callback);

        // Register the mouse button callback.
        glfwSetMouseButtonCallback(w, _glfw_button_callback);

        // Register the mouse move callback.
        // glfwSetCursorPosCallback(w, _glfw_move_callback);

        // Register a function called at every frame, after event polling and state update
        vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_INTERACT, 0, _glfw_frame_callback, NULL);

        break;
    default:
        break;
    }
}



/*************************************************************************************************/
/*  Private event sending                                                                        */
/*************************************************************************************************/

static int _interact_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_INTERACT;
    return _canvas_callbacks(canvas, ev);
}



static int _frame_callbacks(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_FRAME;
    ev.u.f.idx = canvas->frame_idx;
    ev.u.f.interval = canvas->clock.interval;
    ev.u.f.time = canvas->clock.elapsed;
    return _canvas_callbacks(canvas, ev);
}



static void _refill_callbacks(VklCanvas* canvas, VklPrivateEvent ev, uint32_t img_idx)
{
    log_debug("refill callbacks for image #%d", img_idx);
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count > 0);
    ev.u.rf.img_idx = img_idx;

    // Reset all command buffers before calling the REFILL callbacks.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
        vkl_cmd_reset(ev.u.rf.cmds[i], img_idx);

    int res = _canvas_callbacks(canvas, ev);
    if (res == 0)
    {
        log_trace("no REFILL callback registered, filling command buffers with blank screen");
        for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
        {
            blank_commands(canvas, ev.u.rf.cmds[i], img_idx);
        }
    }
}



static void _refill_canvas(VklCanvas* canvas, uint32_t img_idx)
{
    ASSERT(canvas != NULL);
    log_debug("refill canvas %d", img_idx);
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_REFILL;

    // First commands passed is the default cmds_render VklCommands instance used for rendering.
    uint32_t k = 0;
    if (canvas->cmds_render.obj.status >= VKL_OBJECT_STATUS_INIT)
    {
        ev.u.rf.cmds[k++] = &canvas->cmds_render;
    }

    // // Fill the active command buffers for the RENDER queue.
    uint32_t img_count = canvas->cmds_render.count;
    // VklCommands* cmds = vkl_container_iter(&canvas->commands);
    // while (cmds != NULL)
    // {
    //     ASSERT(cmds != NULL);
    //     if (cmds->obj.status == VKL_OBJECT_STATUS_NONE)
    //         break;
    //     if (cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER &&
    //         cmds->obj.status >= VKL_OBJECT_STATUS_INIT)
    //     {
    //         ev.u.rf.cmds[k++] = cmds;
    //         img_count = cmds->count;
    //     }
    //     cmds = vkl_container_iter(&canvas->commands);
    // }

    ASSERT(k > 0);
    ASSERT(img_count > 0);
    ev.u.rf.cmd_count = k;

    // Refill either all commands in each VklCommand (init and resize), or just one (custom
    // refill)
    if (img_idx == UINT32_MAX)
    {
        log_debug("complete refill of the canvas");
        for (img_idx = 0; img_idx < img_count; img_idx++)
        {
            _refill_callbacks(canvas, ev, img_idx);
        }
    }
    else
    {
        log_trace("refill of the canvas for image idx #%d", img_idx);
        _refill_callbacks(canvas, ev, img_idx);
    }
}



static int _resize_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_RESIZE;
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, ev.u.r.size_screen);
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, ev.u.r.size_framebuffer);
    canvas->viewport = vkl_viewport_full(canvas);
    return _canvas_callbacks(canvas, ev);
}



static int _pre_send_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_PRE_SEND;
    ev.u.s.submit = &canvas->submit;
    return _canvas_callbacks(canvas, ev);
}



static int _post_send_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_POST_SEND;
    ev.u.s.submit = &canvas->submit;
    return _canvas_callbacks(canvas, ev);
}



static bool _has_event_callbacks(VklCanvas* canvas, VklEventType type)
{
    ASSERT(canvas != NULL);
    if (type == VKL_EVENT_NONE || type == VKL_EVENT_INIT)
        return true;
    for (uint32_t i = 0; i < canvas->event_callbacks_count; i++)
        if (canvas->event_callbacks[i].type == type)
            return true;
    return false;
}



static int _destroy_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_DESTROY;
    return _canvas_callbacks(canvas, ev);
}



static void _fps(VklCanvas* canvas, VklPrivateEvent ev)
{
    canvas->fps = (canvas->frame_idx - canvas->clock.checkpoint_value) / ev.u.t.interval;
    canvas->clock.checkpoint_value = canvas->frame_idx;
}



/*************************************************************************************************/
/*  Canvas creation                                                                              */
/*************************************************************************************************/

static VklCanvas*
_canvas(VklGpu* gpu, uint32_t width, uint32_t height, bool offscreen, bool overlay, int flags)
{
    ASSERT(gpu != NULL);
    VklApp* app = gpu->app;

    ASSERT(app != NULL);
    // HACK: create the canvas container here because vklite.c does not know the size of VklCanvas.
    if (app->canvases.capacity == 0)
    {
        log_trace("create canvases container");
        app->canvases =
            vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklCanvas), VKL_OBJECT_TYPE_CANVAS);
    }

    VklCanvas* canvas = vkl_container_alloc(&app->canvases);
    canvas->app = app;
    canvas->gpu = gpu;
    canvas->offscreen = offscreen;
    canvas->overlay = overlay;
    canvas->flags = flags;

    // Initialize the canvas local clock.
    _clock_init(&canvas->clock);

    // Initialize the atomic variables used to communicate state changes from a background thread
    // to the main thread (REFILL or CLOSE events).
    atomic_init(&canvas->cur_status, VKL_OBJECT_STATUS_NONE);
    atomic_init(&canvas->next_status, VKL_OBJECT_STATUS_NONE);

    // Allocate memory for canvas objects.
    canvas->commands =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklCommands), VKL_OBJECT_TYPE_COMMANDS);
    canvas->graphics =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklGraphics), VKL_OBJECT_TYPE_GRAPHICS);

    // Create the window.
    VklWindow* window = NULL;
    if (!offscreen)
    {
        window = vkl_window(app, width, height);
        ASSERT(window->app == app);
        ASSERT(window->app != NULL);
        canvas->window = window;
        uint32_t framebuffer_width, framebuffer_height;
        vkl_window_get_size(window, &framebuffer_width, &framebuffer_height);
        ASSERT(framebuffer_width > 0);
        ASSERT(framebuffer_height > 0);
    }

    if (gpu->context == NULL || !is_obj_created(&gpu->context->obj))
    {
        log_trace("canvas automatically create the GPU context");
        gpu->context = vkl_context(gpu, window);
        // Important: the transfer mode must be async as we will be dealing with a swapchain
        // and multiple threads.
        vkl_transfer_mode(gpu->context, VKL_TRANSFER_MODE_ASYNC);
    }

    // Create default renderpass.
    canvas->renderpass =
        default_renderpass(gpu, VKL_DEFAULT_BACKGROUND, VKL_DEFAULT_IMAGE_FORMAT, overlay);
    if (overlay)
        canvas->renderpass_overlay =
            renderpass_overlay(gpu, VKL_DEFAULT_IMAGE_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Create swapchain
    {
        uint32_t min_img_count = offscreen ? 1 : VKL_MIN_SWAPCHAIN_IMAGE_COUNT;
        canvas->swapchain = vkl_swapchain(gpu, window, min_img_count);
        vkl_swapchain_format(&canvas->swapchain, VKL_DEFAULT_IMAGE_FORMAT);

        if (!offscreen)
        {
            vkl_swapchain_present_mode(&canvas->swapchain, VKL_DEFAULT_PRESENT_MODE);
            vkl_swapchain_create(&canvas->swapchain);
        }
        else
        {
            canvas->swapchain.images = calloc(1, sizeof(VklImages));
            ASSERT(canvas->swapchain.img_count == 1);
            *canvas->swapchain.images = vkl_images(canvas->swapchain.gpu, VK_IMAGE_TYPE_2D, 1);
            VklImages* images = canvas->swapchain.images;

            // Color attachment
            vkl_images_format(images, canvas->renderpass.attachments[0].format);
            vkl_images_size(images, width, height, 1);
            vkl_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
            vkl_images_usage(
                images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            vkl_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkl_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
            vkl_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            vkl_images_queue_access(images, VKL_DEFAULT_QUEUE_RENDER);
            vkl_images_create(images);

            obj_created(&canvas->swapchain.obj);
        }

        // Depth attachment.
        canvas->depth_image = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
        depth_image(
            &canvas->depth_image, &canvas->renderpass, //
            canvas->swapchain.images->width, canvas->swapchain.images->height);
    }

    // Create renderpass.
    vkl_renderpass_create(&canvas->renderpass);
    if (overlay)
        vkl_renderpass_create(&canvas->renderpass_overlay);

    // Create framebuffers.
    {
        canvas->framebuffers = vkl_framebuffers(gpu);
        vkl_framebuffers_attachment(&canvas->framebuffers, 0, canvas->swapchain.images);
        vkl_framebuffers_attachment(&canvas->framebuffers, 1, &canvas->depth_image);
        vkl_framebuffers_create(&canvas->framebuffers, &canvas->renderpass);

        if (overlay)
        {
            canvas->framebuffers_overlay = vkl_framebuffers(gpu);
            vkl_framebuffers_attachment(
                &canvas->framebuffers_overlay, 0, canvas->swapchain.images);
            vkl_framebuffers_attachment(&canvas->framebuffers_overlay, 1, &canvas->depth_image);
            vkl_framebuffers_create(&canvas->framebuffers_overlay, &canvas->renderpass_overlay);
        }
    }

    // Create synchronization objects.
    {
        uint32_t frames_in_flight = offscreen ? 1 : VKL_MAX_FRAMES_IN_FLIGHT;

        canvas->sem_img_available = vkl_semaphores(gpu, frames_in_flight);
        canvas->sem_render_finished = vkl_semaphores(gpu, frames_in_flight);
        canvas->present_semaphores = &canvas->sem_render_finished;

        canvas->fences_render_finished = vkl_fences(gpu, frames_in_flight);
        canvas->fences_flight.gpu = gpu;
        canvas->fences_flight.count = canvas->swapchain.img_count;
    }

    // Default transfer commands.
    {
        canvas->cmds_transfer = vkl_commands(gpu, VKL_DEFAULT_QUEUE_TRANSFER, 1);
    }

    // Default render commands.
    {
        canvas->cmds_render =
            vkl_commands(gpu, VKL_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);
    }

    // Default submit instance.
    canvas->submit = vkl_submit(gpu);

    canvas->immediate_queue = vkl_fifo(VKL_MAX_FIFO_CAPACITY);
    canvas->transfers = vkl_fifo(VKL_MAX_FIFO_CAPACITY);

    // Event system.
    {
        canvas->event_queue = vkl_fifo(VKL_MAX_FIFO_CAPACITY);
        canvas->event_thread = vkl_thread(_event_thread, canvas);

        canvas->mouse = vkl_mouse();
        canvas->keyboard = vkl_keyboard();

        backend_event_callbacks(canvas);
    }

    obj_created(&canvas->obj);

    // Update the viewport field.
    canvas->viewport = vkl_viewport_full(canvas);

    if (overlay)
    {
        vkl_imgui_init(canvas);
    }

    // FPS callback.
    {
        canvas->fps = 60;
        vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, .25, _fps, NULL);

        if (((canvas->flags >> 1) & VKL_CANVAS_FLAGS_FPS) != 0)
            vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_IMGUI, 0, vkl_imgui_callback_fps, NULL);
    }

    return canvas;
}



VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    ASSERT(gpu != NULL);
    bool offscreen = gpu->app->backend == VKL_BACKEND_GLFW ? false : true;
    bool overlay = (flags & VKL_CANVAS_FLAGS_IMGUI) > 0;
    return _canvas(gpu, width, height, offscreen, overlay, flags);
}



void vkl_canvas_recreate(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklBackend backend = canvas->app->backend;
    VklWindow* window = canvas->window;
    VklGpu* gpu = canvas->gpu;
    VklSwapchain* swapchain = &canvas->swapchain;
    VklFramebuffers* framebuffers = &canvas->framebuffers;
    VklRenderpass* renderpass = &canvas->renderpass;
    VklFramebuffers* framebuffers_overlay = &canvas->framebuffers_overlay;
    VklRenderpass* renderpass_overlay = &canvas->renderpass_overlay;

    ASSERT(window != NULL);
    ASSERT(gpu != NULL);
    ASSERT(swapchain != NULL);
    ASSERT(framebuffers != NULL);
    ASSERT(framebuffers_overlay != NULL);
    ASSERT(renderpass != NULL);
    ASSERT(renderpass_overlay != NULL);

    log_trace("recreate canvas after resize");

    // Wait until the device is ready and the window fully resized.
    // Framebuffer new size.
    uint32_t width, height;
    backend_window_get_size(
        backend, window->backend_window, //
        &window->width, &window->height, //
        &width, &height);
    vkl_gpu_wait(gpu);

    // Destroy swapchain resources.
    vkl_framebuffers_destroy(&canvas->framebuffers);
    if (canvas->overlay)
        vkl_framebuffers_destroy(&canvas->framebuffers_overlay);
    vkl_images_destroy(&canvas->depth_image);
    vkl_images_destroy(canvas->swapchain.images);

    // Recreate the swapchain. This will automatically set the swapchain->images new size.
    vkl_swapchain_recreate(swapchain);

    // Find the new framebuffer size as determined by the swapchain recreation.
    width = swapchain->images->width;
    height = swapchain->images->height;

    // Check that we use the same VklImages struct here.
    ASSERT(swapchain->images == framebuffers->attachments[0]);

    // Need to recreate the depth image with the new size.
    vkl_images_size(&canvas->depth_image, width, height, 1);
    vkl_images_create(&canvas->depth_image);

    // Recreate the framebuffers with the new size.
    ASSERT(framebuffers->attachments[0]->width == width);
    ASSERT(framebuffers->attachments[0]->height == height);
    ASSERT(framebuffers->attachments[1]->width == width);
    ASSERT(framebuffers->attachments[1]->height == height);
    vkl_framebuffers_create(framebuffers, renderpass);
    if (canvas->overlay)
        vkl_framebuffers_create(framebuffers_overlay, renderpass_overlay);

    _refill_canvas(canvas, UINT32_MAX);
}



VklCommands* vkl_canvas_commands(VklCanvas* canvas, uint32_t queue_idx, uint32_t count)
{
    ASSERT(canvas != NULL);
    VklCommands* commands = vkl_container_alloc(&canvas->commands);
    *commands = vkl_commands(canvas->gpu, queue_idx, count);
    return commands;
}



/*************************************************************************************************/
/*  Offscreen                                                                                    */
/*************************************************************************************************/

VklCanvas* vkl_canvas_offscreen(VklGpu* gpu, uint32_t width, uint32_t height)
{
    // NOTE: no overlay for now in offscreen canvas
    return _canvas(gpu, width, height, true, false, 0);
}



/*************************************************************************************************/
/*  Canvas misc                                                                                  */
/*************************************************************************************************/

void vkl_canvas_clear_color(VklCanvas* canvas, VkClearColorValue color)
{
    ASSERT(canvas != NULL);
    canvas->renderpass.clear_values->color = color;
    vkl_canvas_to_refill(canvas, true);
}



void vkl_canvas_size(VklCanvas* canvas, VklCanvasSizeType type, uvec2 size)
{
    ASSERT(canvas != NULL);

    if (canvas->window == NULL && type == VKL_CANVAS_SIZE_SCREEN)
    {
        ASSERT(canvas->offscreen);
        log_trace("cannot determine window size in screen coordinates with offscreen canvas");
        type = VKL_CANVAS_SIZE_FRAMEBUFFER;
    }

    switch (type)
    {
    case VKL_CANVAS_SIZE_SCREEN:
        ASSERT(canvas->window != NULL);
        size[0] = canvas->window->width;
        size[1] = canvas->window->height;
        break;
    case VKL_CANVAS_SIZE_FRAMEBUFFER:
        size[0] = canvas->framebuffers.attachments[0]->width;
        size[1] = canvas->framebuffers.attachments[0]->height;
        break;
    default:
        log_warn("unknown size type %d", type);
        break;
    }
}



void vkl_canvas_close_on_esc(VklCanvas* canvas, bool value)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);
    canvas->window->close_on_esc = value;
}



VklViewport vkl_viewport_full(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklViewport viewport = {0};

    viewport.viewport.x = 0;
    viewport.viewport.y = 0;
    viewport.viewport.minDepth = +0;
    viewport.viewport.maxDepth = +1;

    ASSERT(canvas->swapchain.images != NULL);
    viewport.size_framebuffer[0] = viewport.viewport.width =
        (float)canvas->swapchain.images->width;
    viewport.size_framebuffer[1] = viewport.viewport.height =
        (float)canvas->swapchain.images->height;

    if (canvas->window != NULL)
    {
        viewport.size_screen[0] = canvas->window->width;
        viewport.size_screen[1] = canvas->window->height;
    }
    else
    {
        // If offscreen canvas, there is no window and we use the same units for screen coords
        // and framebuffer coords.
        viewport.size_screen[0] = viewport.size_framebuffer[0];
        viewport.size_screen[1] = viewport.size_framebuffer[1];
    }

    viewport.dpi_scaling = VKL_DEFAULT_DPI_SCALING;
    viewport.clip = VKL_VIEWPORT_FULL;

    return viewport;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

void vkl_canvas_callback(
    VklCanvas* canvas, VklPrivateEventType type, double param, //
    VklCanvasCallback callback, void* user_data)
{
    ASSERT(canvas != NULL);

    if (type == VKL_PRIVATE_EVENT_IMGUI && !canvas->overlay)
    {
        log_error("the canvas must be created with the VKL_CANVAS_FLAGS_IMGUI flag before a GUI "
                  "can be shown");
        return;
    }

    VklCanvasCallbackRegister r = {0};
    r.callback = callback;
    r.type = type;
    r.user_data = user_data;
    r.param = param;

    canvas->canvas_callbacks[canvas->canvas_callbacks_count++] = r;
}



void vkl_event_callback(
    VklCanvas* canvas, VklEventType type, double param, //
    VklEventCallback callback, void* user_data)
{
    ASSERT(canvas != NULL);

    VklEventCallbackRegister r = {0};
    r.callback = callback;
    r.type = type;
    r.user_data = user_data;
    r.param = param;

    vkl_thread_lock(&canvas->event_thread);
    canvas->event_callbacks[canvas->event_callbacks_count++] = r;
    vkl_thread_unlock(&canvas->event_thread);
}



/*************************************************************************************************/
/*  Thread-safe state changes                                                                    */
/*************************************************************************************************/

void vkl_canvas_set_status(VklCanvas* canvas, VklObjectStatus status)
{
    ASSERT(canvas != NULL);
    atomic_store(&canvas->next_status, status);
}



void vkl_canvas_to_refill(VklCanvas* canvas, bool value)
{
    vkl_canvas_set_status(canvas, value ? VKL_OBJECT_STATUS_NEED_UPDATE : canvas->cur_status);
}



void vkl_canvas_to_close(VklCanvas* canvas, bool value)
{
    vkl_canvas_set_status(canvas, value ? VKL_OBJECT_STATUS_NEED_DESTROY : canvas->cur_status);
}



/*************************************************************************************************/
/*  Fast transfers                                                                               */
/*************************************************************************************************/

void vkl_upload_buffers_immediate(
    VklCanvas* canvas, VklBufferRegions regions, bool update_all_regions, //
    VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(canvas != NULL);
    VklFifo* fifo = &canvas->immediate_queue;
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    ASSERT(regions.count == canvas->swapchain.img_count);

    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_BUFFER_UPLOAD_IMMEDIATE;
    tr.u.buf.regions = regions;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;
    tr.u.buf.update_count = update_all_regions ? canvas->swapchain.img_count : 1;

    canvas->immediate_transfers[fifo->head] = tr;
    vkl_fifo_enqueue(fifo, &canvas->immediate_transfers[fifo->head]);
}



static void _process_transfers(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    //
}



void vkl_canvas_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data,
    bool need_refill)
{
    ASSERT(canvas != NULL);
    ASSERT(size > 0);
    ASSERT(br.buffer != NULL);
    ASSERT(is_obj_created(&br.buffer->obj));
    ASSERT(data != NULL);

    ASSERT(canvas->gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_BUFFER_UPLOAD;
    tr.u.buf.regions = br;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;

    fifo_enqueue(context, &context->fifo, tr);
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _normalize(vec2 pos_out, vec2 pos_in, VklViewport viewport)
{
    // from screen coords to [-1, 1], normalized into the viewport
    float x = viewport.offset_screen[0];
    float y = viewport.offset_screen[1];
    float w = viewport.size_screen[0];
    float h = viewport.size_screen[1];

    // Take viewport margins into account for normalization.
    x += viewport.margins[3];
    y += viewport.margins[0];
    w -= (viewport.margins[1] + viewport.margins[3]);
    h -= (viewport.margins[0] + viewport.margins[2]);

    pos_out[0] = -1 + 2 * ((pos_in[0] - x) / w);
    pos_out[1] = +1 - 2 * ((pos_in[1] - y) / h);
}



static bool _is_key_modifier(VklKeyCode key)
{
    return (
        key == VKL_KEY_LEFT_SHIFT || key == VKL_KEY_RIGHT_SHIFT || key == VKL_KEY_LEFT_CONTROL ||
        key == VKL_KEY_RIGHT_CONTROL || key == VKL_KEY_LEFT_ALT || key == VKL_KEY_RIGHT_ALT ||
        key == VKL_KEY_LEFT_SUPER || key == VKL_KEY_RIGHT_SUPER);
}



/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

VklMouse vkl_mouse()
{
    VklMouse mouse = {0};
    vkl_mouse_reset(&mouse);
    return mouse;
}



void vkl_mouse_reset(VklMouse* mouse)
{
    ASSERT(mouse != NULL);
    memset(mouse, 0, sizeof(VklMouse));
    mouse->button = VKL_MOUSE_BUTTON_NONE;
    glm_vec2_zero(mouse->cur_pos);
    glm_vec2_zero(mouse->press_pos);
    glm_vec2_zero(mouse->last_pos);
    mouse->cur_state = VKL_MOUSE_STATE_INACTIVE;
    mouse->press_time = VKL_NEVER;
    mouse->click_time = VKL_NEVER;
}



void vkl_mouse_event(VklMouse* mouse, VklCanvas* canvas, VklEvent ev)
{
    ASSERT(mouse != NULL);
    ASSERT(canvas != NULL);

    // log_debug("mouse event %d", canvas->frame_idx);
    mouse->prev_state = mouse->cur_state;

    double time = canvas->clock.elapsed;

    // Update the last pos.
    glm_vec2_copy(mouse->cur_pos, mouse->last_pos);

    // Reset click events as soon as the next loop iteration after they were raised.
    if (mouse->cur_state == VKL_MOUSE_STATE_CLICK ||
        mouse->cur_state == VKL_MOUSE_STATE_DOUBLE_CLICK)
    {
        mouse->cur_state = VKL_MOUSE_STATE_INACTIVE;
        mouse->button = VKL_MOUSE_BUTTON_NONE;
    }

    // Net distance in pixels since the last press event.
    vec2 shift = {0};

    switch (ev.type)
    {

    case VKL_EVENT_MOUSE_BUTTON:

        // Press event.
        if (ev.u.b.type == VKL_MOUSE_PRESS && mouse->press_time == VKL_NEVER)
        {
            glm_vec2_copy(mouse->cur_pos, mouse->press_pos);
            mouse->press_time = time;
            mouse->button = ev.u.b.button;
        }

        // Release event.
        else if (ev.u.b.type == VKL_MOUSE_RELEASE)
        {
            // End drag.
            if (mouse->cur_state == VKL_MOUSE_STATE_DRAG)
            {
                log_trace("end drag event");
                mouse->cur_state = VKL_MOUSE_STATE_INACTIVE;
                mouse->button = VKL_MOUSE_BUTTON_NONE;
                vkl_event_mouse_drag_end(canvas, mouse->cur_pos, mouse->button);
            }

            // Double click event.
            else if (time - mouse->click_time < VKL_MOUSE_DOUBLE_CLICK_MAX_DELAY)
            {
                // NOTE: when releasing, current button is NONE so we must use the previously set
                // button in mouse->button.
                log_trace("double click event on button %d", mouse->button);
                mouse->cur_state = VKL_MOUSE_STATE_DOUBLE_CLICK;
                mouse->click_time = time;
                vkl_event_mouse_double_click(canvas, mouse->cur_pos, mouse->button);
            }

            // Click event.
            else if (
                time - mouse->press_time < VKL_MOUSE_CLICK_MAX_DELAY &&
                mouse->shift_length < VKL_MOUSE_CLICK_MAX_SHIFT)
            {
                log_trace("click event on button %d", mouse->button);
                mouse->cur_state = VKL_MOUSE_STATE_CLICK;
                mouse->click_time = time;
                vkl_event_mouse_click(canvas, mouse->cur_pos, mouse->button);
            }

            else
            {
                // Reset the mouse button state.
                mouse->button = VKL_MOUSE_BUTTON_NONE;
            }
            mouse->press_time = VKL_NEVER;
        }
        mouse->shift_length = 0;
        // mouse->button = ev.u.b.button;

        // log_trace("mouse button %d", mouse->button);
        break;


    case VKL_EVENT_MOUSE_MOVE:
        glm_vec2_copy(ev.u.m.pos, mouse->cur_pos);

        // Update the distance since the last press position.
        if (mouse->button != VKL_MOUSE_BUTTON_NONE)
        {
            glm_vec2_sub(mouse->cur_pos, mouse->press_pos, shift);
            mouse->shift_length = glm_vec2_norm(shift);
        }

        // Mouse move.
        // NOTE: do not DRAG if we are clicking, with short press time and shift length
        if (mouse->cur_state == VKL_MOUSE_STATE_INACTIVE &&
            mouse->button != VKL_MOUSE_BUTTON_NONE &&
            !(time - mouse->press_time < VKL_MOUSE_CLICK_MAX_DELAY &&
              mouse->shift_length < VKL_MOUSE_CLICK_MAX_SHIFT))
        {
            log_trace("drag event on button %d", mouse->button);
            mouse->cur_state = VKL_MOUSE_STATE_DRAG;
            vkl_event_mouse_drag(canvas, mouse->cur_pos, mouse->button);
        }
        // log_trace("mouse mouse %.1fx%.1f", mouse->cur_pos[0], mouse->cur_pos[1]);
        break;


    case VKL_EVENT_MOUSE_WHEEL:
        glm_vec2_copy(ev.u.w.dir, mouse->wheel_delta);
        mouse->cur_state = VKL_MOUSE_STATE_WHEEL;
        break;

    default:
        break;
    }
}



// From pixel coordinates (top left origin) to local coordinates (center origin)
void vkl_mouse_local(
    VklMouse* mouse, VklMouseLocal* mouse_local, VklCanvas* canvas, VklViewport viewport)
{
    _normalize(mouse_local->cur_pos, mouse->cur_pos, viewport);
    _normalize(mouse_local->last_pos, mouse->last_pos, viewport);
    _normalize(mouse_local->press_pos, mouse->press_pos, viewport);
}



/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

VklKeyboard vkl_keyboard()
{
    VklKeyboard keyboard = {0};
    vkl_keyboard_reset(&keyboard);
    return keyboard;
}



void vkl_keyboard_reset(VklKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);
    memset(keyboard, 0, sizeof(VklKeyboard));
    // keyboard->key_code = VKL_KEY_NONE;
    // keyboard->modifiers = 0;
    keyboard->press_time = VKL_NEVER;
}



void vkl_keyboard_event(VklKeyboard* keyboard, VklCanvas* canvas, VklEvent ev)
{
    ASSERT(keyboard != NULL);
    ASSERT(canvas != NULL);

    keyboard->prev_state = keyboard->cur_state;

    double time = canvas->clock.elapsed;
    VklKeyCode key = ev.u.k.key_code;

    if (ev.u.k.type == VKL_KEY_PRESS && time - keyboard->press_time > .025)
    {
        log_trace("key pressed %d mods %d", key, ev.u.k.modifiers);
        keyboard->key_code = key;
        keyboard->modifiers = ev.u.k.modifiers;
        keyboard->press_time = time;
        if (keyboard->cur_state == VKL_KEYBOARD_STATE_INACTIVE)
            keyboard->cur_state = VKL_KEYBOARD_STATE_ACTIVE;
    }
    else
    {
        if (keyboard->cur_state == VKL_KEYBOARD_STATE_ACTIVE)
            keyboard->cur_state = VKL_KEYBOARD_STATE_INACTIVE;
    }
}



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

void vkl_event_mouse_button(
    VklCanvas* canvas, VklMouseButtonType type, VklMouseButton button, int modifiers)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_BUTTON;
    event.u.b.button = button;
    event.u.b.type = type;
    event.u.b.modifiers = modifiers;

    // Update the mouse state.
    vkl_mouse_event(&canvas->mouse, canvas, event);

    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_move(VklCanvas* canvas, vec2 pos)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_MOVE;
    event.u.m.pos[0] = pos[0];
    event.u.m.pos[1] = pos[1];

    // Update the mouse state.
    vkl_mouse_event(&canvas->mouse, canvas, event);

    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_wheel(VklCanvas* canvas, vec2 dir)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_WHEEL;
    event.u.w.dir[0] = dir[0];
    event.u.w.dir[1] = dir[1];

    // Update the mouse state.
    vkl_mouse_event(&canvas->mouse, canvas, event);

    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_click(VklCanvas* canvas, vec2 pos, VklMouseButton button)
{
    ASSERT(canvas != NULL);
    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_CLICK;
    event.u.c.pos[0] = pos[0];
    event.u.c.pos[1] = pos[1];
    event.u.c.button = button;
    event.u.c.double_click = false;
    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_double_click(VklCanvas* canvas, vec2 pos, VklMouseButton button)
{
    ASSERT(canvas != NULL);
    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_DOUBLE_CLICK;
    event.u.c.pos[0] = pos[0];
    event.u.c.pos[1] = pos[1];
    event.u.c.button = button;
    event.u.c.double_click = true;
    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_drag(VklCanvas* canvas, vec2 pos, VklMouseButton button)
{
    ASSERT(canvas != NULL);
    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_DRAG_BEGIN;
    event.u.d.pos[0] = pos[0];
    event.u.d.pos[1] = pos[1];
    event.u.d.button = button;
    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_drag_end(VklCanvas* canvas, vec2 pos, VklMouseButton button)
{
    ASSERT(canvas != NULL);
    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_DRAG_END;
    event.u.d.pos[0] = pos[0];
    event.u.d.pos[1] = pos[1];
    event.u.d.button = button;
    vkl_event_enqueue(canvas, event);
}



void vkl_event_key(VklCanvas* canvas, VklKeyType type, VklKeyCode key_code, int modifiers)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_KEY;
    event.u.k.type = type;
    event.u.k.key_code = key_code;
    event.u.k.modifiers = modifiers;

    // Update the keyboard state.
    vkl_keyboard_event(&canvas->keyboard, canvas, event);

    vkl_event_enqueue(canvas, event);
}



void vkl_event_frame(VklCanvas* canvas, uint64_t idx, double time, double interval)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_FRAME;
    event.u.f.idx = idx;
    event.u.f.time = time;
    event.u.f.interval = interval;

    vkl_event_enqueue(canvas, event);
}



void vkl_event_enqueue(VklCanvas* canvas, VklEvent event)
{
    ASSERT(canvas != NULL);
    if (!_has_event_callbacks(canvas, event.type))
        return;
    VklFifo* fifo = &canvas->event_queue;
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    canvas->events[fifo->head] = event;
    vkl_fifo_enqueue(fifo, &canvas->events[fifo->head]);
}



VklEvent* vkl_event_dequeue(VklCanvas* canvas, bool wait)
{
    ASSERT(canvas != NULL);
    VklFifo* fifo = &canvas->event_queue;
    return vkl_fifo_dequeue(fifo, wait);
}



int vkl_event_pending(VklCanvas* canvas, VklEventType type)
{
    ASSERT(canvas != NULL);
    VklFifo* fifo = &canvas->event_queue;
    pthread_mutex_lock(&fifo->lock);
    int i, j;
    if (fifo->tail <= fifo->head)
    {
        i = fifo->tail;
        j = fifo->head;
    }
    else
    {
        i = fifo->head;
        j = fifo->tail;
    }
    ASSERT(i <= j);
    // Count the pending events with the given type.
    int count = 0;
    for (int k = i; k <= j; k++)
    {
        if (((VklEvent*)fifo->items[k])->type == type)
            count++;
    }

    // Add 1 if the event being processed in the event thread has the requested type.
    if (canvas->event_processing == type)
        count++;

    pthread_mutex_unlock(&fifo->lock);
    ASSERT(count >= 0);
    return count;
}



void vkl_event_stop(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklFifo* fifo = &canvas->event_queue;
    vkl_fifo_reset(fifo);
    // Send a null event to the queue which causes the dequeue awaiting thread to end.
    vkl_event_enqueue(canvas, (VklEvent){0});
}



/*************************************************************************************************/
/*  Screencast                                                                                   */
/*************************************************************************************************/

static void _screencast_cmds(VklScreencast* screencast)
{
    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    VklImages* images = screencast->canvas->swapchain.images;
    uint32_t img_count = images->count;

    VklBarrier barrier = vkl_barrier(screencast->canvas->gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&barrier, images);

    for (uint32_t i = 0; i < img_count; i++)
    {
        vkl_cmd_reset(&screencast->cmds, i);
        vkl_cmd_begin(&screencast->cmds, i);

        // Transition to SRC layout
        vkl_barrier_images_layout(
            &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkl_barrier_images_access(
            &barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
        vkl_cmd_barrier(&screencast->cmds, i, &barrier);

        // Copy swapchain image to screencast image
        vkl_cmd_copy_image(&screencast->cmds, i, images, &screencast->staging);

        // Transition back to previous layout
        vkl_barrier_images_layout(
            &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkl_barrier_images_access(
            &barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        vkl_cmd_barrier(&screencast->cmds, i, &barrier);

        vkl_cmd_end(&screencast->cmds, i);
    }
}



static void _screencast_timer_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklScreencast* screencast = (VklScreencast*)ev.user_data;

    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    log_trace("screencast timer frame #%d", screencast->frame_idx);

    VklSubmit* submit = &screencast->submit;
    vkl_submit_reset(submit);
    vkl_submit_commands(submit, &screencast->cmds);

    // Wait for "image_ready" semaphore
    vkl_submit_wait_semaphores(
        submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
        &canvas->sem_render_finished, canvas->cur_frame);

    // Signal screencast_finished semaphore
    vkl_submit_signal_semaphores(submit, &screencast->semaphore, 0);

    // Send screencast cmd buf to transfer queue and signal screencast fence when submitting.
    screencast->status = VKL_SCREENCAST_AWAIT_COPY;
}



static void _screencast_post_send(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklScreencast* screencast = (VklScreencast*)ev.user_data;

    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    uint32_t img_idx = canvas->swapchain.img_idx;
    // Always make sure the present semaphore is reset to its original value.
    canvas->present_semaphores = &canvas->sem_render_finished;

    // Do nothing if the screencast image is not ready.
    if (screencast->status <= VKL_SCREENCAST_IDLE)
        return;

    log_trace("screencast event #%d", screencast->frame_idx);

    // Send the copy job
    if (screencast->status == VKL_SCREENCAST_AWAIT_COPY)
    {
        log_trace("screencast await copy");
        // The copy job waits for the current image to be ready.
        // It signals the screencast semaphore when the copy is done.
        // The present swapchain command must wait for the screencast semaphore rather than
        // the render_finished semaphore.
        vkl_submit_send(&screencast->submit, img_idx, &screencast->fence, 0);

        canvas->present_semaphores = &screencast->semaphore;
        screencast->status = VKL_SCREENCAST_AWAIT_TRANSFER;
    }

    else if (screencast->status == VKL_SCREENCAST_AWAIT_TRANSFER)
    {
        log_trace("screencast await transfer but fence not ready");
        if (!vkl_fences_ready(&screencast->fence, 0))
            return;
        log_trace("screencast await transfer and fence ready");

        // To be freed by the SCREENCAST event callback.
        uint8_t* rgba =
            calloc(screencast->staging.width * screencast->staging.height, 4 * sizeof(uint8_t));

        // Copy the image from the staging image to the CPU.
        log_trace("screencast CPU download");
        vkl_images_download(&screencast->staging, 0, true, rgba);

        // Enqueue a special SCREENCAST public event with a pointer to the CPU buffer user
        VklEvent sev = {0};
        sev.type = VKL_EVENT_SCREENCAST;
        sev.u.s.idx = screencast->frame_idx;
        sev.u.s.interval = screencast->clock.interval;
        sev.u.s.rgba = rgba;
        sev.u.s.width = screencast->staging.width;
        sev.u.s.height = screencast->staging.height;
        log_trace("send SCREENCAST event");
        vkl_event_enqueue(canvas, sev);

        // Reset screencast status.
        _clock_set(&screencast->clock);
        screencast->status = VKL_SCREENCAST_IDLE;
        screencast->frame_idx++;
    }
}



static void _screencast_resize(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklScreencast* screencast = (VklScreencast*)ev.user_data;

    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    screencast->status = VKL_SCREENCAST_NONE;
    vkl_images_resize(
        &screencast->staging, canvas->swapchain.images->width, canvas->swapchain.images->height,
        canvas->swapchain.images->depth);
    vkl_images_transition(&screencast->staging);

    _screencast_cmds(screencast);
}



static void _screencast_destroy(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    vkl_screencast_destroy(canvas);
}



void vkl_screencast(VklCanvas* canvas, double interval)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);

    VklGpu* gpu = canvas->gpu;
    VklImages* images = canvas->swapchain.images;

    canvas->screencast = calloc(1, sizeof(VklScreencast));
    VklScreencast* sc = canvas->screencast;
    sc->canvas = canvas;

    sc->staging = vkl_images(canvas->gpu, VK_IMAGE_TYPE_2D, 1);
    vkl_images_format(&sc->staging, images->format);
    vkl_images_size(&sc->staging, images->width, images->height, images->depth);
    vkl_images_tiling(&sc->staging, VK_IMAGE_TILING_LINEAR);
    vkl_images_usage(&sc->staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    vkl_images_layout(&sc->staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_images_memory(
        &sc->staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_images_create(&sc->staging);

    // Transition the staging image to its layout.
    vkl_images_transition(&sc->staging);

    sc->fence = vkl_fences(gpu, 1);
    sc->semaphore = vkl_semaphores(gpu, 1);

    sc->cmds =
        vkl_commands(canvas->gpu, VKL_DEFAULT_QUEUE_TRANSFER, canvas->swapchain.images->count);
    _screencast_cmds(sc);
    sc->submit = vkl_submit(canvas->gpu);

    _clock_init(&sc->clock);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, interval, _screencast_timer_callback, sc);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_POST_SEND, 0, _screencast_post_send, sc);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_RESIZE, 0, _screencast_resize, sc);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_DESTROY, 0, _screencast_destroy, sc);

    sc->obj.type = VKL_OBJECT_TYPE_SCREENCAST;
    obj_created(&sc->obj);
}



void vkl_screencast_destroy(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklScreencast* screencast = canvas->screencast;
    if (screencast == NULL)
        return;
    ASSERT(screencast != NULL);
    if (!is_obj_created(&screencast->obj))
        return;

    vkl_fences_destroy(&screencast->fence);
    vkl_semaphores_destroy(&screencast->semaphore);
    vkl_images_destroy(&screencast->staging);

    obj_destroyed(&screencast->obj);
    FREE(screencast);
    canvas->screencast = NULL;
}



uint8_t* vkl_screenshot(VklCanvas* canvas) { return NULL; }



void vkl_screenshot_file(VklCanvas* canvas, const char* filename) {}



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

static void _timer_callbacks(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    // Go through all private TIMER callbacks
    double last_time = 0;
    double expected_time = 0;
    double cur_time = canvas->clock.elapsed;
    double interval = 0;
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_TIMER;
    for (uint32_t i = 0; i < canvas->canvas_callbacks_count; i++)
    {
        if (canvas->canvas_callbacks[i].type == VKL_PRIVATE_EVENT_TIMER)
        {
            interval = canvas->canvas_callbacks[i].param;

            // At what time was the last TIMER event for this callback?
            last_time = canvas->canvas_callbacks[i].idx * interval;
            if (cur_time < last_time)
                log_warn("%.3f %.3f", cur_time, last_time);

            // What is the next expected time?
            expected_time = (canvas->canvas_callbacks[i].idx + 1) * interval;

            // If we reached the expected time, we raise the TIMER event immediately.
            if (cur_time >= expected_time)
            {
                ev.user_data = canvas->canvas_callbacks[i].user_data;
                canvas->canvas_callbacks[i].idx++;
                ev.u.t.idx = canvas->canvas_callbacks[i].idx;
                ev.u.t.time = cur_time;
                // NOTE: this is the time since the last *expected* time of the previous TIMER
                // event, not the actual time.
                ev.u.t.interval = cur_time - last_time;

                // Call this TIMER callback.
                canvas->canvas_callbacks[i].callback(canvas, ev);
            }
        }
    }
}



void vkl_canvas_frame(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->gpu != NULL);

    // Update the global and local clocks.
    // These calls update canvas->clock.elapsed and canvas->clock.interval, the latter is
    // the delay since the last frame.
    _clock_set(&canvas->app->clock); // global clock
    _clock_set(&canvas->clock);      // canvas-local clock

    // Update cur_status
    atomic_store(&canvas->cur_status, canvas->obj.status);

    // Call INTERACT callbacks (for backends only), which may enqueue some events.
    _interact_callbacks(canvas);

    // Call FRAME callbacks.
    _frame_callbacks(canvas);

    // Give a chance to update event structures in the main loop, for example reset wheel.
    _backend_next_frame(canvas);

    // Call TIMER private callbacks, in the main thread.
    _timer_callbacks(canvas);

    // Refill all command buffers at the first iteration.
    if (canvas->frame_idx == 0)
    {
        log_debug("fill the command buffers at the first frame");
        vkl_gpu_wait(canvas->gpu);
        _refill_canvas(canvas, UINT32_MAX);
        canvas->obj.status = VKL_OBJECT_STATUS_CREATED;
    }

    // Get the next status.
    VklObjectStatus next_status = atomic_load(&canvas->next_status);
    // If the next status is set, update the actual canvas status.
    if (next_status != VKL_OBJECT_STATUS_NONE)
    {
        canvas->obj.status = next_status;
        // Reset the next_status atomic variable.
        atomic_store(&canvas->next_status, VKL_OBJECT_STATUS_NONE);
        // Now, the canvas actual status has been updated with the value set by a background
        // thread. This may cause a REFILL or CLOSE or other.
    }

    // Wait for fence.
    vkl_fences_wait(&canvas->fences_render_finished, canvas->cur_frame);

    // Process IMMEDIATE buffer transfers (used by uniform buffers that exist in multiple copies
    // to avoid GPU synchronization by making it such that each swapchain image has its own
    // buffer region)
    _immediate_transfers(canvas);

    // We acquire the next swapchain image.
    if (!canvas->offscreen)
        vkl_swapchain_acquire(
            &canvas->swapchain, &canvas->sem_img_available, //
            canvas->cur_frame, NULL, 0);

    // Refill if needed, only 1 swapchain command buffer per frame to avoid waiting on the device.
    if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
    {
        log_debug("need to update canvas, will refill the command buffers");

        // DEBUG
        if (0)
        {
            vkl_queue_wait(canvas->gpu, VKL_DEFAULT_QUEUE_RENDER);
            _refill_canvas(canvas, UINT32_MAX);
            canvas->obj.status = VKL_OBJECT_STATUS_CREATED;
        }
        else
        {
            // Wait for command buffer to be ready for update.
            vkl_fences_wait(&canvas->fences_flight, canvas->swapchain.img_idx);

            // HACK: avoid edge effects when the resize takes some time and the dt becomes too
            // large
            canvas->clock.interval = 0;

            // Refill the command buffer for the current swapchain image.
            _refill_canvas(canvas, canvas->swapchain.img_idx);

            // Mark that command buffer as updated.
            canvas->img_updated[canvas->swapchain.img_idx] = true;

            // We move away from NEED_UPDATE status only if all swapchain images have been updated.
            if (_all_true(canvas->swapchain.img_count, canvas->img_updated))
            {
                log_trace("all command buffers updated, no longer need to update");
                canvas->obj.status = VKL_OBJECT_STATUS_CREATED;
                // Reset the img_updated bool array.
                memset(canvas->img_updated, 0, VKL_MAX_SWAPCHAIN_IMAGES);
            }
        }
    }
}



void vkl_canvas_frame_submit(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    VklSubmit* s = &canvas->submit;
    uint32_t f = canvas->cur_frame;
    uint32_t img_idx = canvas->swapchain.img_idx;

    // Keep track of the fence associated to the current swapchain image.
    vkl_fences_copy(
        &canvas->fences_render_finished, f, //
        &canvas->fences_flight, img_idx);

    // Reset the Submit instance before adding the command buffers.
    vkl_submit_reset(s);

    // Add the command buffers to the submit instance.
    // Default render commands.
    if (canvas->cmds_render.obj.status == VKL_OBJECT_STATUS_CREATED)
        vkl_submit_commands(s, &canvas->cmds_render);

    // // Extra render commands.
    // VklCommands* cmds = vkl_container_iter(&canvas->commands);
    // while (cmds != NULL)
    // {
    //     if (cmds->obj.status == VKL_OBJECT_STATUS_NONE)
    //         break;
    //     if (cmds->obj.status == VKL_OBJECT_STATUS_INACTIVE)
    //         continue;
    //     if (cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER)
    //         vkl_submit_commands(s, cmds);
    //     cmds = vkl_container_iter(&canvas->commands);
    // }
    if (s->commands_count == 0)
    {
        log_error("no recorded command buffers");
        return;
    }

    if (!canvas->offscreen)
    {
        vkl_submit_wait_semaphores(
            s, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
            &canvas->sem_img_available, f);

        // Once the render is finished, we signal another semaphore.
        vkl_submit_signal_semaphores(s, &canvas->sem_render_finished, f);
    }

    // SEND callbacks and send the Submit instance.
    {
        // Call PRE_SEND callbacks
        _pre_send_callbacks(canvas);

        // Send the Submit instance.
        vkl_submit_send(s, img_idx, &canvas->fences_render_finished, f);

        // Call POST_SEND callbacks
        _post_send_callbacks(canvas);
    }

    // Once the image is rendered, we present the swapchain image.
    // The semaphore used for waiting during presentation may be changed by the canvas
    // callbacks.
    if (!canvas->offscreen)
        vkl_swapchain_present(
            &canvas->swapchain, 1, //
            canvas->present_semaphores, CLIP(f, 0, canvas->present_semaphores->count - 1));

    canvas->cur_frame = (f + 1) % canvas->fences_render_finished.count;
}



void vkl_app_run(VklApp* app, uint64_t frame_count)
{
    log_trace("start main loop");
    ASSERT(app != NULL);
    app->is_running = true;
    if (frame_count == 0)
        frame_count = UINT64_MAX;
    ASSERT(frame_count > 0);

    VklCanvas* canvas = NULL;

    // Main loop.
    uint32_t n_canvas_active = 0;
    for (uint64_t iter = 0; iter < frame_count; iter++)
    {
        // if (frame_count > 0)
        //     log_trace("frame iteration %d/%d", iter, frame_count);
        n_canvas_active = 0;

        // Loop over the canvases.
        canvas = vkl_container_iter_init(&app->canvases);
        while (canvas != NULL)
        {
            ASSERT(canvas != NULL);
            // if (canvas->obj.status == VKL_OBJECT_STATUS_NONE)
            //     break;
            if (canvas->obj.status < VKL_OBJECT_STATUS_CREATED)
            {
                canvas = vkl_container_iter(&app->canvases);
                continue;
            }
            ASSERT(canvas->obj.status >= VKL_OBJECT_STATUS_CREATED);
            // log_trace("processing frame #%d for canvas #%d", canvas->frame_idx, canvas_idx);

            // INIT event at the first frame
            if (canvas->frame_idx == 0)
            {
                // Call RESIZE callbacks at initialization.
                _resize_callbacks(canvas);

                VklEvent ev = {0};
                ev.type = VKL_EVENT_INIT;
                vkl_event_enqueue(canvas, ev);
            }

            // Poll events.
            if (canvas->window != NULL)
                vkl_window_poll_events(canvas->window);

            // Frame logic.
            // log_trace("frame logic for canvas #%d", canvas_idx);
            // Swapchain image acquisition happens here:
            vkl_canvas_frame(canvas);

            // If there is a problem with swapchain image acquisition, wait and try again later.
            if (canvas->swapchain.obj.status == VKL_OBJECT_STATUS_INVALID)
            {
                // log_trace("swapchain image acquisition failed, waiting and skipping this
                // frame");
                vkl_gpu_wait(canvas->gpu);
                canvas = vkl_container_iter(&app->canvases);
                continue;
            }

            // If the swapchain needs to be recreated (for example, after a resize), do it.
            if (canvas->swapchain.obj.status == VKL_OBJECT_STATUS_NEED_RECREATE)
            {
                // log_trace("swapchain image acquisition failed, recreating the canvas");

                // Recreate the canvas.
                vkl_canvas_recreate(canvas);

                // Call RESIZE callbacks.
                _resize_callbacks(canvas);

                n_canvas_active++;
                canvas = vkl_container_iter(&app->canvases);
                continue;
            }

            // Destroy the canvas if needed.
            if (canvas->window != NULL)
            {
                if (backend_window_should_close(app->backend, canvas->window->backend_window))
                    canvas->window->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
                if (canvas->window->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
                    canvas->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
            }
            if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
            {
                log_trace("destroying canvas");

                // Stop the transfer queue.
                vkl_transfer_stop(canvas->gpu->context);
                vkl_event_stop(canvas);

                // Wait for all GPUs to be idle.
                vkl_app_wait(app);

                // Destroy the canvas.
                vkl_canvas_destroy(canvas);
                canvas = vkl_container_iter(&app->canvases);
                continue;
            }

            // Submit the command buffers and swapchain logic.
            // log_trace("submitting frame for canvas #%d", canvas_idx);
            vkl_canvas_frame_submit(canvas);
            canvas->frame_idx++;
            n_canvas_active++;

            canvas = vkl_container_iter(&app->canvases);
        }

        // Process the pending transfer tasks.
        // NOTE: this has never been tested with multiple GPUs yet.
        VklContext* context = NULL;
        VklGpu* gpu = vkl_container_iter_init(&app->gpus);
        while (gpu != NULL)
        {
            if (!is_obj_created(&gpu->obj))
                break;
            context = gpu->context;

            if (is_obj_created(&context->obj))
            {
                // log_trace("processing transfers for GPU #%d", gpu_idx);
                vkl_transfer_loop(context, false);
            }

            // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
            // when waiting for fences (not sure why). The problem only arises when using different
            // queues for command buffer submission and swapchain present.
            if (gpu->queues.queues[VKL_DEFAULT_QUEUE_PRESENT] != VK_NULL_HANDLE &&
                gpu->queues.queues[VKL_DEFAULT_QUEUE_PRESENT] !=
                    gpu->queues.queues[VKL_DEFAULT_QUEUE_RENDER])
            {
                vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_PRESENT);
            }

            gpu = vkl_container_iter(&app->gpus);
        }

        // Full update: complete refill of all swapchain command buffers after data transfers
        // that changed the number of items in VERTEX or INDEX sources, which means the number
        // of vertices/indices to draw (fixed in the command buffer) has to change.
        canvas = vkl_container_iter_init(&app->canvases);
        while (canvas != NULL)
        {
            ASSERT(canvas != NULL);
            if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_FULL_UPDATE)
            {
                log_info("full update requested on canvas, triggering full refill");
                // We need to stop the rendering because we'll update all command buffers at once.
                vkl_queue_wait(canvas->gpu, VKL_DEFAULT_QUEUE_RENDER);
                // Complete refill of the canvas that require a full update.
                _refill_canvas(canvas, UINT32_MAX);
                canvas->obj.status = VKL_OBJECT_STATUS_CREATED;
            }
            canvas = vkl_container_iter(&app->canvases);
        }

        // Close the application if all canvases have been closed.
        if (n_canvas_active == 0)
        {
            log_trace("no more active canvas, closing the app");
            break;
        }
    }
    log_trace("end main loop");

    vkl_app_wait(app);
    app->is_running = false;
}



/*************************************************************************************************/
/*  Canvas destruction                                                                           */
/*************************************************************************************************/

void vkl_canvas_destroy(VklCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status == VKL_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed canvas");
        return;
    }
    log_trace("destroying canvas");

    // DEBUG: only in non offscreen mode
    if (!canvas->offscreen)
    {
        ASSERT(canvas->window != NULL);
        ASSERT(canvas->window->app != NULL);
    }

    // Stop the vent thread.
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    vkl_gpu_wait(canvas->gpu);
    vkl_event_stop(canvas);
    vkl_thread_join(&canvas->event_thread);
    vkl_fifo_destroy(&canvas->event_queue);

    // Fast transfers.
    vkl_fifo_destroy(&canvas->immediate_queue);

    // Destroy callbacks.
    _destroy_callbacks(canvas);

    // Destroy the graphics.
    log_trace("canvas destroy graphics pipelines");
    CONTAINER_DESTROY_ITEMS(VklGraphics, canvas->graphics, vkl_graphics_destroy)
    vkl_container_destroy(&canvas->graphics);

    // Destroy the depth image.
    vkl_images_destroy(&canvas->depth_image);

    // Destroy the renderpasses.
    log_trace("canvas destroy renderpass");
    vkl_renderpass_destroy(&canvas->renderpass);
    if (canvas->overlay)
        vkl_renderpass_destroy(&canvas->renderpass_overlay);

    // Destroy the swapchain.
    log_trace("canvas destroy swapchain");
    vkl_swapchain_destroy(&canvas->swapchain);

    // Destroy the framebuffers.
    log_trace("canvas destroy framebuffers");
    vkl_framebuffers_destroy(&canvas->framebuffers);
    if (canvas->overlay)
        vkl_framebuffers_destroy(&canvas->framebuffers_overlay);

    // Destroy the window.
    log_trace("canvas destroy window");
    if (canvas->window != NULL)
    {
        ASSERT(canvas->window->app != NULL);
        vkl_window_destroy(canvas->window);
    }

    log_trace("canvas destroy commands");
    CONTAINER_DESTROY_ITEMS(VklCommands, canvas->commands, vkl_commands_destroy)
    vkl_container_destroy(&canvas->commands);

    // Destroy the semaphores.
    log_trace("canvas destroy semaphores");
    vkl_semaphores_destroy(&canvas->sem_img_available);
    vkl_semaphores_destroy(&canvas->sem_render_finished);

    // Destroy the fences.
    log_trace("canvas destroy fences");
    vkl_fences_destroy(&canvas->fences_render_finished);

    if (canvas->overlay)
        vkl_imgui_destroy();

    obj_destroyed(&canvas->obj);
}



void vkl_canvases_destroy(VklContainer* canvases)
{
    if (canvases == NULL || canvases->capacity == 0)
        return;
    log_trace("destroy all canvases");
    CONTAINER_DESTROY_ITEMS(VklCanvas, (*canvases), vkl_canvas_destroy)
    vkl_container_destroy(canvases);
}
