#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../src/vklite2_utils.h"
#include <stdlib.h>


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_DEFAULT_BACKGROUND                                                                    \
    (VkClearColorValue)                                                                           \
    {                                                                                             \
        {                                                                                         \
            0, .03, .07, 1.0f                                                                     \
        }                                                                                         \
    }
#define VKL_DEFAULT_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define VKL_DEFAULT_PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
// #define VKL_DEFAULT_PRESENT_MODE      VK_PRESENT_MODE_IMMEDIATE_KHR
#define VKL_MIN_SWAPCHAIN_IMAGE_COUNT 3
#define VKL_SEMAPHORE_IMG_AVAILABLE   0
#define VKL_SEMAPHORE_RENDER_FINISHED 1
#define VKL_FENCE_RENDER_FINISHED     0
#define VKL_FENCES_FLIGHT             1
#define VKL_DEFAULT_COMMANDS_TRANSFER 0
#define VKL_DEFAULT_COMMANDS_RENDER   1
#define VKL_MAX_FRAMES_IN_FLIGHT      2


/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklRenderpass default_renderpass(
    VklGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, VkImageLayout layout)
{
    VklRenderpass renderpass = vkl_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;

    vkl_renderpass_clear(&renderpass, clear_color);
    vkl_renderpass_clear(&renderpass, clear_depth);

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
    vkl_cmd_begin_renderpass(cmds, cmd_idx, &canvas->renderpasses[0], &canvas->framebuffers);
    vkl_cmd_end_renderpass(cmds, cmd_idx);
    vkl_cmd_end(cmds, cmd_idx);
}



/*************************************************************************************************/
/*  Private event sending                                                                        */
/*************************************************************************************************/

static int _canvas_callbacks(VklCanvas* canvas, VklPrivateEvent event)
{
    int n_callbacks = 0;
    for (uint32_t i = 0; i < canvas->canvas_callbacks_count; i++)
    {
        // Will pass the user_data that was registered, to the callback function.
        event.user_data = canvas->canvas_callbacks[i].user_data;

        // Only call the callbacks registered for the specified type.
        if (canvas->canvas_callbacks[i].type == event.type)
        {
            canvas->canvas_callbacks[i].callback(canvas, event);
            n_callbacks++;
        }
    }
    return n_callbacks;
}



static int _interact_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_INTERACT;
    return _canvas_callbacks(canvas, ev);
}



static int _frame_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_FRAME;
    ev.u.f.idx = canvas->frame_idx;
    ev.u.f.interval = canvas->clock.interval;
    ev.u.f.time = canvas->clock.elapsed;
    return _canvas_callbacks(canvas, ev);
}



static void _refill_callbacks(VklCanvas* canvas, VklPrivateEvent ev, uint32_t img_idx)
{
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
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_REFILL;

    // Fill the active command buffers for the RENDER queue.
    uint32_t k = 0;
    VklCommands* cmds = NULL;
    uint32_t img_count = 0;
    for (uint32_t i = 0; i < canvas->max_commands; i++)
    {
        cmds = &canvas->commands[i];
        if (cmds->obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        if (cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER &&
            cmds->obj.status >= VKL_OBJECT_STATUS_INIT)
        {
            ev.u.rf.cmds[k++] = &canvas->commands[i];
            img_count = canvas->commands[i].count;
        }
    }
    ASSERT(k > 0);
    ASSERT(img_count > 0);
    ev.u.rf.cmd_count = k;

    // Refill aither all commands in each VklCommand (init and resize), or just one (custom
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
    return _canvas_callbacks(canvas, ev);
}



static int _post_send_callbacks(VklCanvas* canvas)
{
    VklPrivateEvent ev = {0};
    ev.type = VKL_PRIVATE_EVENT_POST_SEND;
    return _canvas_callbacks(canvas, ev);
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
    vkl_event_key(canvas, type, key_code);
}

static void _glfw_wheel_callback(GLFWwindow* window, double dx, double dy)
{
    VklCanvas* canvas = (VklCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    vkl_event_mouse_wheel(canvas, (dvec2){dx, dy});
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

    vkl_event_mouse_move(canvas, (dvec2){xpos, ypos});
}

static void backend_event_callbacks(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->window != NULL);
    switch (canvas->app->backend)
    {
    case VKL_BACKEND_GLFW:;
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
        glfwSetCursorPosCallback(w, _glfw_move_callback);

        break;
    default:
        break;
    }
}



/*************************************************************************************************/
/*  Canvas creation                                                                              */
/*************************************************************************************************/

VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    VklApp* app = gpu->app;

    ASSERT(app != NULL);
    if (app->canvases == NULL)
    {
        INSTANCES_INIT(
            VklCanvas, app, canvases, max_canvases, VKL_MAX_WINDOWS, VKL_OBJECT_TYPE_CANVAS)
    }

    INSTANCE_NEW(VklCanvas, canvas, app->canvases, app->max_canvases)
    canvas->app = app;
    canvas->gpu = gpu;

    // Initialize the canvas local clock.
    _clock_init(&canvas->clock);

    // Initialize the atomic variables used to communicate state changes from a background thread
    // to the main thread (REFILL or CLOSE events).
    atomic_init(&canvas->cur_status, VKL_OBJECT_STATUS_NONE);
    atomic_init(&canvas->next_status, VKL_OBJECT_STATUS_NONE);

    // Allocate memory for canvas objects.
    INSTANCES_INIT(
        VklCommands, canvas, commands, max_commands, VKL_MAX_COMMANDS, VKL_OBJECT_TYPE_COMMANDS)
    INSTANCES_INIT(
        VklRenderpass, canvas, renderpasses, max_renderpasses, VKL_MAX_RENDERPASSES,
        VKL_OBJECT_TYPE_RENDERPASS)
    INSTANCES_INIT(
        VklSemaphores, canvas, semaphores, max_semaphores, VKL_MAX_SEMAPHORES,
        VKL_OBJECT_TYPE_SEMAPHORES)
    INSTANCES_INIT(VklFences, canvas, fences, max_fences, VKL_MAX_FENCES, VKL_OBJECT_TYPE_FENCES)
    INSTANCES_INIT(
        VklGraphics, canvas, graphics, max_graphics, VKL_MAX_GRAPHICS, VKL_OBJECT_TYPE_GRAPHICS)

    // Create the window.
    VklWindow* window = vkl_window(app, width, height);
    canvas->window = window;
    uint32_t framebuffer_width, framebuffer_height;
    vkl_window_get_size(window, &framebuffer_width, &framebuffer_height);
    ASSERT(framebuffer_width > 0);
    ASSERT(framebuffer_height > 0);

    if (gpu->context == NULL || !is_obj_created(&gpu->context->obj))
    {
        log_trace("canvas automatically create the GPU context");
        gpu->context = vkl_context(gpu, window);
        // Important: the transfer mode must be async as we will be dealing with a swapchain
        // and multiple threads.
        vkl_transfer_mode(gpu->context, VKL_TRANSFER_MODE_ASYNC);
    }

    // Create default renderpass.
    INSTANCE_NEW(VklRenderpass, renderpass, canvas->renderpasses, canvas->max_renderpasses)
    *renderpass = default_renderpass(
        gpu, VKL_DEFAULT_BACKGROUND, VKL_DEFAULT_IMAGE_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Create swapchain
    {
        canvas->swapchain = vkl_swapchain(gpu, window, VKL_MIN_SWAPCHAIN_IMAGE_COUNT);
        vkl_swapchain_format(&canvas->swapchain, VKL_DEFAULT_IMAGE_FORMAT);
        vkl_swapchain_present_mode(&canvas->swapchain, VKL_DEFAULT_PRESENT_MODE);
        vkl_swapchain_create(&canvas->swapchain);

        // Depth attachment.
        canvas->depth_image = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
        depth_image(
            &canvas->depth_image, renderpass, //
            canvas->swapchain.images->width, canvas->swapchain.images->height);
    }

    // Create renderpass.
    vkl_renderpass_create(renderpass);

    // Create framebuffers.
    {
        canvas->framebuffers = vkl_framebuffers(gpu);
        vkl_framebuffers_attachment(&canvas->framebuffers, 0, canvas->swapchain.images);
        vkl_framebuffers_attachment(&canvas->framebuffers, 1, &canvas->depth_image);
        vkl_framebuffers_create(&canvas->framebuffers, renderpass);
    }

    // Create synchronization objects.
    {
        canvas->semaphores[VKL_SEMAPHORE_IMG_AVAILABLE] =
            vkl_semaphores(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
        canvas->semaphores[VKL_SEMAPHORE_RENDER_FINISHED] =
            vkl_semaphores(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
        canvas->fences[VKL_FENCE_RENDER_FINISHED] = vkl_fences(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
        vkl_fences_create(&canvas->fences[VKL_FENCE_RENDER_FINISHED]);
        canvas->fences[VKL_FENCES_FLIGHT] = vkl_fences(gpu, canvas->swapchain.img_count);
    }

    // Default transfer commands.
    {
        INSTANCE_NEW(VklCommands, cmds, canvas->commands, canvas->max_commands)
        *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_TRANSFER, 1);
    }

    // Default render commands.
    {
        INSTANCE_NEW(VklCommands, cmds, canvas->commands, canvas->max_commands)
        *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);
    }

    // Default submit instance.
    canvas->submit = vkl_submit(gpu);

    // Event queue.
    canvas->event_queue = vkl_fifo(VKL_MAX_FIFO_CAPACITY);
    canvas->event_thread = vkl_thread(_event_thread, canvas);
    backend_event_callbacks(canvas);

    obj_created(&canvas->obj);

    return canvas;
}



void vkl_canvas_recreate(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklBackend backend = canvas->app->backend;
    VklWindow* window = canvas->window;
    VklGpu* gpu = canvas->gpu;
    VklSwapchain* swapchain = &canvas->swapchain;
    VklFramebuffers* framebuffers = &canvas->framebuffers;
    VklRenderpass* renderpass = &canvas->renderpasses[0];

    ASSERT(window != NULL);
    ASSERT(gpu != NULL);
    ASSERT(swapchain != NULL);
    ASSERT(framebuffers != NULL);

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
    vkl_framebuffers_create(framebuffers, renderpass);

    _refill_canvas(canvas, UINT32_MAX);
}



/*************************************************************************************************/
/*  Offscreen                                                                                    */
/*************************************************************************************************/

VklCanvas* vkl_canvas_offscreen(VklGpu* gpu, uint32_t width, uint32_t height)
{
    // TODO
    return NULL;
}



/*************************************************************************************************/
/*  Canvas misc                                                                                  */
/*************************************************************************************************/

void vkl_canvas_clear_color(VklCanvas* canvas, VkClearColorValue color)
{
    ASSERT(canvas != NULL);
    canvas->renderpasses[0].clear_values->color = color;
    vkl_canvas_to_refill(canvas, true);
}



void vkl_canvas_size(VklCanvas* canvas, VklCanvasSizeType type, uvec2 size)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);
    switch (type)
    {
    case VKL_CANVAS_SIZE_SCREEN:
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



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

void vkl_canvas_callback(
    VklCanvas* canvas, VklPrivateEventType type, double param, //
    VklCanvasCallback callback, void* user_data)
{
    ASSERT(canvas != NULL);

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
    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_move(VklCanvas* canvas, dvec2 pos)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_MOVE;
    event.u.m.pos[0] = pos[0];
    event.u.m.pos[1] = pos[1];
    vkl_event_enqueue(canvas, event);
}



void vkl_event_mouse_wheel(VklCanvas* canvas, dvec2 dir)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_MOUSE_WHEEL;
    event.u.w.dir[0] = dir[0];
    event.u.w.dir[1] = dir[1];
    vkl_event_enqueue(canvas, event);
}



void vkl_event_key(VklCanvas* canvas, VklKeyType type, VklKeyCode key_code)
{
    ASSERT(canvas != NULL);

    VklEvent event = {0};
    event.type = VKL_EVENT_KEY;
    event.u.k.type = type;
    event.u.k.key_code = key_code;
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
            ASSERT(cur_time >= last_time);

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
    ASSERT(canvas->window != NULL);
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

    // Call FRAME callbacks (rarely used).
    _frame_callbacks(canvas);

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
    vkl_fences_wait(&canvas->fences[VKL_FENCE_RENDER_FINISHED], canvas->cur_frame);

    // We acquire the next swapchain image.
    vkl_swapchain_acquire(
        &canvas->swapchain, &canvas->semaphores[VKL_SEMAPHORE_IMG_AVAILABLE], //
        canvas->cur_frame, NULL, 0);

    // Refill if needed.
    if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
    {
        log_trace("need to update canvas, will refill the command buffers");

        // Wait for command buffer to be ready for update.
        vkl_fences_wait(&canvas->fences[VKL_FENCES_FLIGHT], canvas->swapchain.img_idx);
        // vkl_queue_wait(canvas->gpu, VKL_DEFAULT_QUEUE_RENDER); // DEBUG

        // Refill the command buffer for the current swapchain image.
        _refill_canvas(canvas, canvas->swapchain.img_idx);

        // Mark that command buffer as updated.
        canvas->img_updated[canvas->swapchain.img_idx] = true;

        // We move away from NEED_UPDATE status only if all swapchain images have been updated.
        bool all_updated = true;
        for (uint32_t i = 0; i < canvas->swapchain.img_count; i++)
        {
            if (!canvas->img_updated[i])
            {
                all_updated = false;
                break;
            }
        }
        if (all_updated)
        {
            log_trace("all command buffers updated, no longer need to update");
            canvas->obj.status = VKL_OBJECT_STATUS_CREATED;
            // Reset the img_updated bool array.
            memset(canvas->img_updated, 0, VKL_MAX_SWAPCHAIN_IMAGES);
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
        &canvas->fences[VKL_FENCE_RENDER_FINISHED], f, //
        &canvas->fences[VKL_FENCES_FLIGHT], img_idx);

    // Reset the Submit instance before adding the command buffers.
    vkl_submit_reset(s);

    // Add the command buffers to the submit instance.
    for (uint32_t i = 0; i < canvas->max_commands; i++)
    {
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_INACTIVE)
            continue;
        if (canvas->commands[i].queue_idx == VKL_DEFAULT_QUEUE_RENDER)
        {
            vkl_submit_commands(s, &canvas->commands[i]);
        }
    }

    vkl_submit_wait_semaphores(
        s, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
        &canvas->semaphores[VKL_SEMAPHORE_IMG_AVAILABLE], f);
    // Once the render is finished, we signal another semaphore.
    vkl_submit_signal_semaphores(s, &canvas->semaphores[VKL_SEMAPHORE_RENDER_FINISHED], f);
    // Send the Submit instance.
    vkl_submit_send(s, img_idx, &canvas->fences[VKL_FENCE_RENDER_FINISHED], f);

    // Call POST_SEND callbacks
    _post_send_callbacks(canvas);

    // Once the image is rendered, we present the swapchain image.
    vkl_swapchain_present(
        &canvas->swapchain, 1, &canvas->semaphores[VKL_SEMAPHORE_RENDER_FINISHED], f);

    canvas->cur_frame = (f + 1) % VKL_MAX_FRAMES_IN_FLIGHT;
}



void vkl_app_run(VklApp* app, uint64_t frame_count)
{
    log_trace("run app");
    ASSERT(app != NULL);
    if (frame_count == 0)
        frame_count = UINT64_MAX;
    ASSERT(frame_count > 0);

    VklCanvas* canvas = NULL;

    // Main loop.
    uint32_t n_canvas_active = 0;
    for (uint64_t iter = 0; iter < frame_count; iter++)
    {
        if (frame_count > 0)
            log_trace("frame iteration %d/%d", iter, frame_count);
        n_canvas_active = 0;

        // Loop over the canvases.
        for (uint32_t canvas_idx = 0; canvas_idx < app->max_canvases; canvas_idx++)
        {
            // Get the current canvas.
            canvas = &app->canvases[canvas_idx];
            ASSERT(canvas != NULL);
            if (canvas->obj.status == VKL_OBJECT_STATUS_NONE)
                break;
            if (canvas->obj.status < VKL_OBJECT_STATUS_CREATED)
                continue;
            ASSERT(canvas->obj.status >= VKL_OBJECT_STATUS_CREATED);
            log_trace("processing frame #%d for canvas #%d", canvas->frame_idx, canvas_idx);

            // INIT event at the first frame
            if (canvas->frame_idx == 0)
            {
                VklEvent ev = {0};
                ev.type = VKL_EVENT_INIT;
                vkl_event_enqueue(canvas, ev);
            }

            // Poll events.
            ASSERT(canvas->window != NULL);
            vkl_window_poll_events(canvas->window);

            // Frame logic.
            log_trace("frame logic for canvas #%d", canvas_idx);
            // Swapchain image acquisition happens here:
            vkl_canvas_frame(canvas);

            // If there is a problem with swapchain image acquisition, wait and try again later.
            if (canvas->swapchain.obj.status == VKL_OBJECT_STATUS_INVALID)
            {
                log_trace("swapchain image acquisition failed, waiting and skipping this frame");
                vkl_gpu_wait(canvas->gpu);
                continue;
            }

            // If the swapchain needs to be recreated (for example, after a resize), do it.
            if (canvas->swapchain.obj.status == VKL_OBJECT_STATUS_NEED_RECREATE)
            {
                log_trace("swapchain image acquisition failed, recreating the canvas");

                // Recreate the canvas.
                vkl_canvas_recreate(canvas);

                // Call RESIZE callbacks.
                _resize_callbacks(canvas);

                n_canvas_active++;
                continue;
            }

            // Destroy the canvas if needed.
            if (backend_window_should_close(app->backend, canvas->window->backend_window))
                canvas->window->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
            if (canvas->window->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
                canvas->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
            if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
            {
                log_trace("destroying canvas #%d", canvas_idx);

                // Stop the transfer queue.
                vkl_transfer_stop(canvas->gpu->context);
                vkl_event_stop(canvas);

                // Wait for all GPUs to be idle.
                vkl_app_wait(app);

                // Destroy the canvas.
                vkl_canvas_destroy(canvas);
                continue;
            }

            // Submit the command buffers and swapchain logic.
            log_trace("submitting frame for canvas #%d", canvas_idx);
            vkl_canvas_frame_submit(canvas);
            canvas->frame_idx++;
            n_canvas_active++;
        }

        // Process the pending transfer tasks.
        // NOTE: this has never been tested with multiple GPUs yet.
        VklGpu* gpu = NULL;
        VklContext* ctx = NULL;
        for (uint32_t gpu_idx = 0; gpu_idx < app->gpu_count; gpu_idx++)
        {
            gpu = &app->gpus[gpu_idx];
            if (!is_obj_created(&gpu->obj))
                break;
            ctx = gpu->context;

            if (is_obj_created(&ctx->obj))
            {
                log_trace("processing transfers for GPU #%d", gpu_idx);
                vkl_transfer_loop(ctx, false);
            }

            // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
            // when waiting for fences (not sure why). The problem only arises when using different
            // queues for command buffer submission and swapchain present.
            if (gpu->queues.queues[VKL_DEFAULT_QUEUE_PRESENT] !=
                gpu->queues.queues[VKL_DEFAULT_QUEUE_RENDER])
            {
                vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_PRESENT);
            }
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

    // Stop the vent thread.
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    vkl_gpu_wait(canvas->gpu);
    vkl_event_stop(canvas);
    vkl_thread_join(&canvas->event_thread);
    vkl_fifo_destroy(&canvas->event_queue);

    // Destroy the graphics.
    log_trace("canvas destroy graphics pipelines");
    for (uint32_t i = 0; i < canvas->max_graphics; i++)
    {
        if (canvas->graphics[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_graphics_destroy(&canvas->graphics[i]);
    }
    INSTANCES_DESTROY(canvas->graphics)

    // Destroy the depth image.
    vkl_images_destroy(&canvas->depth_image);

    // Destroy the renderpasses.
    log_trace("canvas destroy renderpass(es)");
    for (uint32_t i = 0; i < canvas->max_renderpasses; i++)
    {
        if (canvas->renderpasses[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_renderpass_destroy(&canvas->renderpasses[i]);
    }
    INSTANCES_DESTROY(canvas->renderpasses)

    // Destroy the swapchain.
    vkl_swapchain_destroy(&canvas->swapchain);

    // Destroy the framebuffers.
    vkl_framebuffers_destroy(&canvas->framebuffers);

    // Destroy the window.
    vkl_window_destroy(canvas->window);

    log_trace("canvas destroy commands");
    for (uint32_t i = 0; i < canvas->max_commands; i++)
    {
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_commands_destroy(&canvas->commands[i]);
    }
    INSTANCES_DESTROY(canvas->commands)

    // Destroy the semaphores.
    log_trace("canvas destroy semaphores");
    for (uint32_t i = 0; i < canvas->max_semaphores; i++)
    {
        if (canvas->semaphores[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_semaphores_destroy(&canvas->semaphores[i]);
    }
    INSTANCES_DESTROY(canvas->semaphores)

    // Destroy the fences.
    log_trace("canvas destroy fences");
    for (uint32_t i = 0; i < canvas->max_fences; i++)
    {
        if (canvas->fences[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_fences_destroy(&canvas->fences[i]);
    }
    INSTANCES_DESTROY(canvas->fences)

    obj_destroyed(&canvas->obj);
}



void vkl_canvases_destroy(uint32_t canvas_count, VklCanvas* canvases)
{
    for (uint32_t i = 0; i < canvas_count; i++)
    {
        if (canvases[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_canvas_destroy(&canvases[i]);
    }
}
