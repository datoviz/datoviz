#include "../include/datoviz/canvas.h"
#include "../external/video.h"
#include "../include/datoviz/context.h"
#include "../include/datoviz/controls.h"
#include "../include/datoviz/gui.h"
#include "../include/datoviz/vklite.h"
#include "../src/vklite_utils.h"

#include <stdlib.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_NEVER                        -1000000
#define DVZ_MOUSE_CLICK_MAX_DELAY        .25
#define DVZ_MOUSE_CLICK_MAX_SHIFT        5
#define DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY .2
#define DVZ_KEY_PRESS_DELAY              .05



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

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

    // Depth attachment.
    dvz_renderpass_attachment(
        &renderpass, 1, //
        DVZ_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(&renderpass, 0, 1);
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



static void blank_commands(DvzCanvas* canvas, DvzCommands* cmds, uint32_t cmd_idx)
{
    dvz_cmd_begin(cmds, cmd_idx);
    dvz_cmd_begin_renderpass(cmds, cmd_idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_end_renderpass(cmds, cmd_idx);
    dvz_cmd_end(cmds, cmd_idx);
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
/*  Backend-specific event callbacks                                                             */
/*************************************************************************************************/

static void _glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    DvzCanvas* canvas = (DvzCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    // Special handling of ESC key.
    if (canvas->window->close_on_esc && action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        canvas->window->obj.status = DVZ_OBJECT_STATUS_NEED_DESTROY;
        return;
    }

    DvzKeyType type = {0};
    DvzKeyCode key_code = {0};

    // Find the key event type.
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        type = DVZ_KEY_PRESS;
    else
        type = DVZ_KEY_RELEASE;

    // NOTE: we use the GLFW key codes here, should actually do a proper mapping between GLFW
    // key codes and Datoviz key codes.
    key_code = key;

    // Enqueue the key event.
    dvz_event_key(canvas, type, key_code, mods);
}

static void _glfw_wheel_callback(GLFWwindow* window, double dx, double dy)
{
    DvzCanvas* canvas = (DvzCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    dvz_event_mouse_wheel(canvas, (vec2){dx, dy});
}

static void _glfw_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    DvzCanvas* canvas = (DvzCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    // Find mouse button action type
    DvzMouseButtonType type = {0};
    if (action == GLFW_PRESS)
        type = DVZ_MOUSE_PRESS;
    else
        type = DVZ_MOUSE_RELEASE;

    // Map mouse button.
    DvzMouseButton b = {0};
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        b = DVZ_MOUSE_BUTTON_LEFT;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        b = DVZ_MOUSE_BUTTON_RIGHT;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        b = DVZ_MOUSE_BUTTON_MIDDLE;

    // NOTE: Datoviz modifiers code must match GLFW
    dvz_event_mouse_button(canvas, type, b, mods);
}

static void _glfw_move_callback(GLFWwindow* window, double xpos, double ypos)
{
    DvzCanvas* canvas = (DvzCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);

    dvz_event_mouse_move(canvas, (vec2){xpos, ypos});
}

static void _glfw_frame_callback(DvzCanvas* canvas, DvzEvent ev)
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
        dvz_event_mouse_move(canvas, pos);

    // TODO
    // // Reset click events as soon as the next loop iteration after they were raised.
    // if (mouse->cur_state == DVZ_MOUSE_STATE_CLICK ||
    //     mouse->cur_state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    // {
    //     mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
    //     mouse->button = DVZ_MOUSE_BUTTON_NONE;
    // }
}

static void _backend_next_frame(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);

    // Reset wheel event.
    if (canvas->mouse.cur_state == DVZ_MOUSE_STATE_WHEEL)
    {
        // log_debug("reset wheel state %d", canvas->frame_idx);
        canvas->mouse.cur_state = DVZ_MOUSE_STATE_INACTIVE;
    }
}

static void backend_event_callbacks(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->app != NULL);
    switch (canvas->app->backend)
    {
    case DVZ_BACKEND_GLFW:;
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
        dvz_event_callback(
            canvas, DVZ_EVENT_INTERACT, 0, DVZ_EVENT_MODE_SYNC, _glfw_frame_callback, NULL);

        break;
    default:
        break;
    }
}



/*************************************************************************************************/
/*  Event producers                                                                              */
/*************************************************************************************************/

static int _event_interact(DvzCanvas* canvas)
{
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_INTERACT;
    return _event_produce(canvas, ev);
}



static int _event_frame(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_FRAME;
    ev.u.f.idx = canvas->frame_idx;
    ev.u.f.interval = canvas->clock.interval;
    ev.u.f.time = canvas->clock.elapsed;
    return _event_produce(canvas, ev);
}



static void _event_timer(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    // Go through all TIMER callbacks
    double last_time = 0;
    double expected_time = 0;
    double cur_time = canvas->clock.elapsed;
    double interval = 0;
    DvzEvent ev = {0};
    DvzEventCallbackRegister* r = NULL;
    ev.type = DVZ_EVENT_TIMER;
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
    {
        r = &canvas->callbacks[i];
        if (r->type == DVZ_EVENT_TIMER)
        {
            interval = r->param;

            // At what time was the last TIMER event for this callback?
            last_time = r->idx * interval;
            if (cur_time < last_time)
                log_warn("%.3f %.3f", cur_time, last_time);

            // What is the next expected time?
            expected_time = (r->idx + 1) * interval;

            // If we reached the expected time, we raise the TIMER event immediately.
            if (cur_time >= expected_time)
            {
                ev.user_data = r->user_data;
                r->idx++;
                ev.u.t.idx = r->idx;
                ev.u.t.time = cur_time;
                // NOTE: this is the time since the last *expected* time of the previous TIMER
                // event, not the actual time.
                ev.u.t.interval = cur_time - last_time;

                // Call this TIMER callback.
                r->callback(canvas, ev);
            }
        }
    }
}



static void _event_refill(DvzCanvas* canvas, DvzEvent ev)
{
    // log_debug("refill callbacks for image #%d", img_idx);
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count > 0);
    uint32_t img_idx = ev.u.rf.img_idx;

    // Reset all command buffers before calling the REFILL callbacks.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
        dvz_cmd_reset(ev.u.rf.cmds[i], img_idx);

    int res = _event_produce(canvas, ev);
    if (res == 0)
    {
        log_trace("no REFILL callback registered, filling command buffers with blank screen");
        for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
        {
            blank_commands(canvas, ev.u.rf.cmds[i], img_idx);
        }
    }
}



static int _event_resize(DvzCanvas* canvas)
{
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_RESIZE;
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, ev.u.r.size_screen);
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, ev.u.r.size_framebuffer);
    canvas->viewport = dvz_viewport_full(canvas);
    return _event_produce(canvas, ev);
}



static int _event_presend(DvzCanvas* canvas)
{
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_PRE_SEND;
    ev.u.s.submit = &canvas->submit;
    return _event_produce(canvas, ev);
}



static int _event_postsend(DvzCanvas* canvas)
{
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_POST_SEND;
    ev.u.s.submit = &canvas->submit;
    return _event_produce(canvas, ev);
}



/*************************************************************************************************/
/*  Event utils                                                                                  */
/*************************************************************************************************/

static void _refill_canvas(DvzCanvas* canvas, uint32_t img_idx)
{
    ASSERT(canvas != NULL);
    log_debug("refill canvas %d", img_idx);
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_REFILL;
    ev.u.rf.img_idx = img_idx;

    // First commands passed is the default cmds_render DvzCommands instance used for rendering.
    uint32_t k = 0;
    if (canvas->cmds_render.obj.status >= DVZ_OBJECT_STATUS_INIT)
        ev.u.rf.cmds[k++] = &canvas->cmds_render;

    // // Fill the active command buffers for the RENDER queue.
    uint32_t img_count = canvas->cmds_render.count;
    // DvzCommands* cmds = dvz_container_iter(&canvas->commands);
    // while (cmds != NULL)
    // {
    //     ASSERT(cmds != NULL);
    //     if (cmds->obj.status == DVZ_OBJECT_STATUS_NONE)
    //         break;
    //     if (cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER &&
    //         cmds->obj.status >= DVZ_OBJECT_STATUS_INIT)
    //     {
    //         ev.u.rf.cmds[k++] = cmds;
    //         img_count = cmds->count;
    //     }
    //     cmds = dvz_container_iter(&canvas->commands);
    // }

    ASSERT(k > 0);
    ASSERT(img_count > 0);
    ev.u.rf.cmd_count = k;

    // Refill either all commands in each DvzCommand (init and resize), or just one (custom
    // refill)
    if (img_idx == UINT32_MAX)
    {
        log_debug("complete refill of the canvas");
        for (img_idx = 0; img_idx < img_count; img_idx++)
            _event_refill(canvas, ev);
    }
    else
    {
        log_trace("refill of the canvas for image idx #%d", img_idx);
        _event_refill(canvas, ev);
    }
}



static void _refill_frame(DvzCanvas* canvas)
{
    uint32_t img_idx = canvas->swapchain.img_idx;
    // Only proceed if the current swapchain image has not been processed yet.
    if (atomic_load(&canvas->refills.status) == DVZ_REFILL_REQUESTED ||
        atomic_load(&canvas->refills.status) == DVZ_REFILL_PROCESSING)
    {
        // If refill has just been requested, reset the ongoing refill by setting completed to
        // false for all swapchain images.
        if (atomic_load(&canvas->refills.status) == DVZ_REFILL_REQUESTED)
            memset(canvas->refills.completed, 0, DVZ_MAX_SWAPCHAIN_IMAGES);

        // Skip this step if the current swapchain image has already been processed.
        if (canvas->refills.completed[img_idx])
            return;

        DvzRefillStatus status = DVZ_REFILL_PROCESSING;
        atomic_store(&canvas->refills.status, status);

        // Wait for command buffer to be ready for update.
        dvz_fences_wait(&canvas->fences_flight, img_idx);

        // HACK: avoid edge effects when the resize takes some time and the dt becomes too large
        canvas->clock.interval = 0;

        // Refill the command buffer for the current swapchain image.
        _refill_canvas(canvas, img_idx);

        // Mark that command buffer as updated.
        canvas->refills.completed[img_idx] = true;

        // We move away from NEED_UPDATE status only if all swapchain images have been updated.
        if (_all_true(canvas->swapchain.img_count, canvas->refills.completed))
        {
            log_trace("all command buffers updated, no longer need to update");
            status = DVZ_REFILL_NONE;
            atomic_store(&canvas->refills.status, status);
            // Reset the img_updated bool array.
            memset(canvas->refills.completed, 0, DVZ_MAX_SWAPCHAIN_IMAGES);
        }
    }
}



static int _destroy_callbacks(DvzCanvas* canvas)
{
    DvzEvent ev = {0};
    ev.type = DVZ_EVENT_DESTROY;
    return _event_produce(canvas, ev);
}



static void _fps(DvzCanvas* canvas, DvzEvent ev)
{
    canvas->fps = (canvas->frame_idx - canvas->clock.checkpoint_value) / ev.u.t.interval;
    canvas->clock.checkpoint_value = canvas->frame_idx;
    // log_info("FPS: %.1f", canvas->fps);
}



/*************************************************************************************************/
/*  Canvas creation                                                                              */
/*************************************************************************************************/

static DvzCanvas*
_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, bool offscreen, bool overlay, int flags)
{
    ASSERT(gpu != NULL);
    DvzApp* app = gpu->app;

    ASSERT(app != NULL);
    // HACK: create the canvas container here because vklite.c does not know the size of DvzCanvas.
    if (app->canvases.capacity == 0)
    {
        log_trace("create canvases container");
        app->canvases =
            dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCanvas), DVZ_OBJECT_TYPE_CANVAS);
    }

    DvzCanvas* canvas = dvz_container_alloc(&app->canvases);
    canvas->app = app;
    canvas->gpu = gpu;
    canvas->offscreen = offscreen;

    canvas->dpi_scaling = DVZ_DEFAULT_DPI_SCALING;
    int flag_dpi = flags >> 12;
    if (flag_dpi > 0)
        canvas->dpi_scaling *= (.5 * flag_dpi);

    canvas->overlay = overlay;
    canvas->flags = flags;
    bool show_fps = ((canvas->flags >> 1) & DVZ_CANVAS_FLAGS_FPS) != 0;

    // Initialize the canvas local clock.
    _clock_init(&canvas->clock);

    // Initialize the atomic variables used to communicate state changes from a background thread
    // to the main thread (REFILL or CLOSE events).
    atomic_init(&canvas->to_close, false);
    atomic_init(&canvas->refills.status, DVZ_REFILL_NONE);

    // Allocate memory for canvas objects.
    canvas->commands =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCommands), DVZ_OBJECT_TYPE_COMMANDS);
    canvas->graphics =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGraphics), DVZ_OBJECT_TYPE_GRAPHICS);

    // Create the window.
    DvzWindow* window = NULL;
    if (!offscreen)
    {
        window = dvz_window(app, width, height);
        ASSERT(window->app == app);
        ASSERT(window->app != NULL);
        canvas->window = window;
        uint32_t framebuffer_width, framebuffer_height;
        dvz_window_get_size(window, &framebuffer_width, &framebuffer_height);
        ASSERT(framebuffer_width > 0);
        ASSERT(framebuffer_height > 0);
    }

    if (gpu->context == NULL || !dvz_obj_is_created(&gpu->context->obj))
    {
        log_trace("canvas automatically create the GPU context");
        gpu->context = dvz_context(gpu, window);
    }

    // Create default renderpass.
    canvas->renderpass =
        default_renderpass(gpu, DVZ_DEFAULT_BACKGROUND, DVZ_DEFAULT_IMAGE_FORMAT, overlay);
    if (overlay)
        canvas->renderpass_overlay =
            renderpass_overlay(gpu, DVZ_DEFAULT_IMAGE_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Create swapchain
    {
        uint32_t min_img_count = offscreen ? 1 : DVZ_MIN_SWAPCHAIN_IMAGE_COUNT;
        canvas->swapchain = dvz_swapchain(gpu, window, min_img_count);
        dvz_swapchain_format(&canvas->swapchain, DVZ_DEFAULT_IMAGE_FORMAT);

        if (!offscreen)
        {
            dvz_swapchain_present_mode(
                &canvas->swapchain,
                show_fps ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR);
            dvz_swapchain_create(&canvas->swapchain);
        }
        else
        {
            canvas->swapchain.images = calloc(1, sizeof(DvzImages));
            ASSERT(canvas->swapchain.img_count == 1);
            *canvas->swapchain.images = dvz_images(canvas->swapchain.gpu, VK_IMAGE_TYPE_2D, 1);
            DvzImages* images = canvas->swapchain.images;

            // Color attachment
            dvz_images_format(images, canvas->renderpass.attachments[0].format);
            dvz_images_size(images, width, height, 1);
            dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
            dvz_images_usage(
                images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
            dvz_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            dvz_images_queue_access(images, DVZ_DEFAULT_QUEUE_RENDER);
            dvz_images_create(images);

            dvz_obj_created(&canvas->swapchain.obj);
        }

        // Depth attachment.
        canvas->depth_image = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
        depth_image(
            &canvas->depth_image, &canvas->renderpass, //
            canvas->swapchain.images->width, canvas->swapchain.images->height);
    }

    // Create renderpass.
    dvz_renderpass_create(&canvas->renderpass);
    if (overlay)
        dvz_renderpass_create(&canvas->renderpass_overlay);

    // Create framebuffers.
    {
        canvas->framebuffers = dvz_framebuffers(gpu);
        dvz_framebuffers_attachment(&canvas->framebuffers, 0, canvas->swapchain.images);
        dvz_framebuffers_attachment(&canvas->framebuffers, 1, &canvas->depth_image);
        dvz_framebuffers_create(&canvas->framebuffers, &canvas->renderpass);

        if (overlay)
        {
            canvas->framebuffers_overlay = dvz_framebuffers(gpu);
            dvz_framebuffers_attachment(
                &canvas->framebuffers_overlay, 0, canvas->swapchain.images);
            dvz_framebuffers_attachment(&canvas->framebuffers_overlay, 1, &canvas->depth_image);
            dvz_framebuffers_create(&canvas->framebuffers_overlay, &canvas->renderpass_overlay);
        }
    }

    // Create synchronization objects.
    {
        uint32_t frames_in_flight = offscreen ? 1 : DVZ_MAX_FRAMES_IN_FLIGHT;

        canvas->sem_img_available = dvz_semaphores(gpu, frames_in_flight);
        canvas->sem_render_finished = dvz_semaphores(gpu, frames_in_flight);
        canvas->present_semaphores = &canvas->sem_render_finished;

        canvas->fences_render_finished = dvz_fences(gpu, frames_in_flight, true);
        canvas->fences_flight.gpu = gpu;
        canvas->fences_flight.count = canvas->swapchain.img_count;
    }

    // Default transfer commands.
    {
        canvas->cmds_transfer = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_TRANSFER, 1);
    }

    // Default render commands.
    {
        canvas->cmds_render =
            dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);
    }

    // Default submit instance.
    canvas->submit = dvz_submit(gpu);

    canvas->transfers = dvz_fifo(DVZ_MAX_FIFO_CAPACITY);

    // Event system.
    {
        canvas->event_queue = dvz_fifo(DVZ_MAX_FIFO_CAPACITY);
        canvas->event_thread = dvz_thread(_event_thread, canvas);

        canvas->mouse = dvz_mouse();
        canvas->keyboard = dvz_keyboard();

        backend_event_callbacks(canvas);
    }

    dvz_obj_created(&canvas->obj);

    // Update the viewport field.
    canvas->viewport = dvz_viewport_full(canvas);

    // GUI.
    canvas->guis = dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGui), DVZ_OBJECT_TYPE_GUI);
    if (overlay)
    {
        dvz_imgui_init(canvas);
        dvz_event_callback(
            canvas, DVZ_EVENT_IMGUI, 0, DVZ_EVENT_MODE_SYNC, dvz_gui_callback, NULL);
    }

    // FPS callback.
    {
        canvas->fps = 60;
        // Compute FPS every 250 ms, even if FPS is not shown (so that the value remains accessible
        // in callbacks if needed).
        dvz_event_callback(canvas, DVZ_EVENT_TIMER, .25, DVZ_EVENT_MODE_SYNC, _fps, NULL);

        if (show_fps)
            dvz_event_callback(
                canvas, DVZ_EVENT_IMGUI, 0, DVZ_EVENT_MODE_SYNC, dvz_gui_callback_fps, NULL);
    }

    ASSERT(canvas->swapchain.images != NULL);
    log_debug(
        "created canvas of size %dx%d", //
        canvas->swapchain.images->width, canvas->swapchain.images->height);
    return canvas;
}



DvzCanvas* dvz_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    ASSERT(gpu != NULL);
    bool offscreen = gpu->app->backend == DVZ_BACKEND_GLFW ? false : true;
    bool overlay = (flags & DVZ_CANVAS_FLAGS_IMGUI) > 0;
    return _canvas(gpu, width, height, offscreen, overlay, flags);
}



void dvz_canvas_recreate(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzBackend backend = canvas->app->backend;
    DvzWindow* window = canvas->window;
    DvzGpu* gpu = canvas->gpu;
    DvzSwapchain* swapchain = &canvas->swapchain;
    DvzFramebuffers* framebuffers = &canvas->framebuffers;
    DvzRenderpass* renderpass = &canvas->renderpass;
    DvzFramebuffers* framebuffers_overlay = &canvas->framebuffers_overlay;
    DvzRenderpass* renderpass_overlay = &canvas->renderpass_overlay;

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
    dvz_gpu_wait(gpu);

    // Destroy swapchain resources.
    dvz_framebuffers_destroy(&canvas->framebuffers);
    if (canvas->overlay)
        dvz_framebuffers_destroy(&canvas->framebuffers_overlay);
    dvz_images_destroy(&canvas->depth_image);
    dvz_images_destroy(canvas->swapchain.images);

    // Recreate the swapchain. This will automatically set the swapchain->images new size.
    dvz_swapchain_recreate(swapchain);

    // Find the new framebuffer size as determined by the swapchain recreation.
    width = swapchain->images->width;
    height = swapchain->images->height;

    // Check that we use the same DvzImages struct here.
    ASSERT(swapchain->images == framebuffers->attachments[0]);

    // Need to recreate the depth image with the new size.
    dvz_images_size(&canvas->depth_image, width, height, 1);
    dvz_images_create(&canvas->depth_image);

    // Recreate the framebuffers with the new size.
    ASSERT(framebuffers->attachments[0]->width == width);
    ASSERT(framebuffers->attachments[0]->height == height);
    ASSERT(framebuffers->attachments[1]->width == width);
    ASSERT(framebuffers->attachments[1]->height == height);
    dvz_framebuffers_create(framebuffers, renderpass);
    if (canvas->overlay)
        dvz_framebuffers_create(framebuffers_overlay, renderpass_overlay);
}



DvzCommands* dvz_canvas_commands(DvzCanvas* canvas, uint32_t queue_idx, uint32_t count)
{
    ASSERT(canvas != NULL);
    DvzCommands* commands = dvz_container_alloc(&canvas->commands);
    *commands = dvz_commands(canvas->gpu, queue_idx, count);
    return commands;
}



/*************************************************************************************************/
/*  Offscreen                                                                                    */
/*************************************************************************************************/

DvzCanvas* dvz_canvas_offscreen(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    // NOTE: no overlay for now in offscreen canvas
    return _canvas(gpu, width, height, true, false, 0);
}



/*************************************************************************************************/
/*  Canvas misc                                                                                  */
/*************************************************************************************************/

void dvz_canvas_clear_color(DvzCanvas* canvas, float red, float green, float blue)
{
    ASSERT(canvas != NULL);
    canvas->renderpass.clear_values->color = (VkClearColorValue){{red, green, blue, 1}};
    dvz_canvas_to_refill(canvas);
}



void dvz_canvas_size(DvzCanvas* canvas, DvzCanvasSizeType type, uvec2 size)
{
    ASSERT(canvas != NULL);

    if (canvas->window == NULL && type == DVZ_CANVAS_SIZE_SCREEN)
    {
        ASSERT(canvas->offscreen);
        log_trace("cannot determine window size in screen coordinates with offscreen canvas");
        type = DVZ_CANVAS_SIZE_FRAMEBUFFER;
    }

    switch (type)
    {
    case DVZ_CANVAS_SIZE_SCREEN:
        ASSERT(canvas->window != NULL);
        size[0] = canvas->window->width;
        size[1] = canvas->window->height;
        break;
    case DVZ_CANVAS_SIZE_FRAMEBUFFER:
        size[0] = canvas->framebuffers.attachments[0]->width;
        size[1] = canvas->framebuffers.attachments[0]->height;
        break;
    default:
        log_warn("unknown size type %d", type);
        break;
    }
}



void dvz_canvas_close_on_esc(DvzCanvas* canvas, bool value)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);
    canvas->window->close_on_esc = value;
}



void dvz_canvas_dpi_scaling(DvzCanvas* canvas, float scaling)
{
    ASSERT(canvas != NULL);
    scaling = CLIP(scaling, .01, 100);
    ASSERT(scaling > 0);
    canvas->dpi_scaling = scaling;
}



DvzViewport dvz_viewport_default(uint32_t width, uint32_t height)
{
    DvzViewport viewport = {0};

    viewport.viewport.x = 0;
    viewport.viewport.y = 0;
    viewport.viewport.minDepth = +0;
    viewport.viewport.maxDepth = +1;

    viewport.size_framebuffer[0] = viewport.viewport.width = (float)width;
    viewport.size_framebuffer[1] = viewport.viewport.height = (float)height;
    viewport.size_screen[0] = viewport.size_framebuffer[0];
    viewport.size_screen[1] = viewport.size_framebuffer[1];

    return viewport;
}



DvzViewport dvz_viewport_full(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzViewport viewport = {0};

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

    viewport.clip = DVZ_VIEWPORT_FULL;

    return viewport;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

void dvz_event_callback(
    DvzCanvas* canvas, DvzEventType type, double param, DvzEventMode mode, //
    DvzEventCallback callback, void* user_data)
{
    ASSERT(canvas != NULL);

    if (type == DVZ_EVENT_IMGUI && !canvas->overlay)
    {
        log_error("the canvas must be created with the DVZ_CANVAS_FLAGS_IMGUI flag before a GUI "
                  "can be shown");
        return;
    }

    DvzEventCallbackRegister r = {0};
    r.callback = callback;
    r.type = type;
    r.mode = mode;
    r.user_data = user_data;
    r.param = param;

    // Automatically enable the global lock if there is at least one async callback. The lock
    // is used when calling any callback.
    // TODO: improve this if this causes performance issues (locking/unlocking at every frame)
    if (mode == DVZ_EVENT_MODE_ASYNC)
    {
        // enable the global callback lock that surrounds all callbacks. This ensures
        // that scene objects can be modified by both the main thread (internal scene
        // system implemented in a sync callback) and in async callbacks (running in the
        // background thread)
        log_debug("enable the global callback lock");
        canvas->enable_lock = true;
    }

    if (canvas->enable_lock)
        dvz_thread_lock(&canvas->event_thread);

    canvas->callbacks[canvas->callbacks_count++] = r;

    if (canvas->enable_lock)
        dvz_thread_unlock(&canvas->event_thread);
}



/*************************************************************************************************/
/*  Thread-safe state changes                                                                    */
/*************************************************************************************************/

void dvz_canvas_to_refill(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzRefillStatus status = DVZ_REFILL_REQUESTED;
    atomic_store(&canvas->refills.status, status);
}



void dvz_canvas_to_close(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    bool value = true;
    atomic_store(&canvas->to_close, value);
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _normalize(vec2 pos_out, vec2 pos_in, DvzViewport viewport)
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



static bool _is_key_modifier(DvzKeyCode key)
{
    return (
        key == DVZ_KEY_LEFT_SHIFT || key == DVZ_KEY_RIGHT_SHIFT || key == DVZ_KEY_LEFT_CONTROL ||
        key == DVZ_KEY_RIGHT_CONTROL || key == DVZ_KEY_LEFT_ALT || key == DVZ_KEY_RIGHT_ALT ||
        key == DVZ_KEY_LEFT_SUPER || key == DVZ_KEY_RIGHT_SUPER);
}



/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

DvzMouse dvz_mouse()
{
    DvzMouse mouse = {0};
    dvz_mouse_reset(&mouse);
    return mouse;
}



void dvz_mouse_reset(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);
    memset(mouse, 0, sizeof(DvzMouse));
    mouse->button = DVZ_MOUSE_BUTTON_NONE;
    glm_vec2_zero(mouse->cur_pos);
    glm_vec2_zero(mouse->press_pos);
    glm_vec2_zero(mouse->last_pos);
    mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
    mouse->press_time = DVZ_NEVER;
    mouse->click_time = DVZ_NEVER;
}



void dvz_mouse_event(DvzMouse* mouse, DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(mouse != NULL);
    ASSERT(canvas != NULL);
    if (canvas->captured)
        return;

    // log_debug("mouse event %d", canvas->frame_idx);
    mouse->prev_state = mouse->cur_state;

    double time = canvas->clock.elapsed;

    // Update the last pos.
    glm_vec2_copy(mouse->cur_pos, mouse->last_pos);

    // Reset click events as soon as the next loop iteration after they were raised.
    if (mouse->cur_state == DVZ_MOUSE_STATE_CLICK ||
        mouse->cur_state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    {
        mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
        mouse->button = DVZ_MOUSE_BUTTON_NONE;
    }

    // Net distance in pixels since the last press event.
    vec2 shift = {0};

    switch (ev.type)
    {

    case DVZ_EVENT_MOUSE_BUTTON:

        // Press event.
        if (ev.u.b.type == DVZ_MOUSE_PRESS && mouse->press_time == DVZ_NEVER)
        {
            glm_vec2_copy(mouse->cur_pos, mouse->press_pos);
            mouse->press_time = time;
            mouse->button = ev.u.b.button;
        }

        // Release event.
        else if (ev.u.b.type == DVZ_MOUSE_RELEASE)
        {
            // End drag.
            if (mouse->cur_state == DVZ_MOUSE_STATE_DRAG)
            {
                log_trace("end drag event");
                mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
                mouse->button = DVZ_MOUSE_BUTTON_NONE;
                dvz_event_mouse_drag_end(canvas, mouse->cur_pos, mouse->button);
            }

            // Double click event.
            else if (time - mouse->click_time < DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY)
            {
                // NOTE: when releasing, current button is NONE so we must use the previously set
                // button in mouse->button.
                log_trace("double click event on button %d", mouse->button);
                mouse->cur_state = DVZ_MOUSE_STATE_DOUBLE_CLICK;
                mouse->click_time = time;
                dvz_event_mouse_double_click(canvas, mouse->cur_pos, mouse->button);
            }

            // Click event.
            else if (
                time - mouse->press_time < DVZ_MOUSE_CLICK_MAX_DELAY &&
                mouse->shift_length < DVZ_MOUSE_CLICK_MAX_SHIFT)
            {
                log_trace("click event on button %d", mouse->button);
                mouse->cur_state = DVZ_MOUSE_STATE_CLICK;
                mouse->click_time = time;
                dvz_event_mouse_click(canvas, mouse->cur_pos, mouse->button);
            }

            else
            {
                // Reset the mouse button state.
                mouse->button = DVZ_MOUSE_BUTTON_NONE;
            }
            mouse->press_time = DVZ_NEVER;
        }
        mouse->shift_length = 0;
        // mouse->button = ev.u.b.button;

        // log_trace("mouse button %d", mouse->button);
        break;


    case DVZ_EVENT_MOUSE_MOVE:
        glm_vec2_copy(ev.u.m.pos, mouse->cur_pos);

        // Update the distance since the last press position.
        if (mouse->button != DVZ_MOUSE_BUTTON_NONE)
        {
            glm_vec2_sub(mouse->cur_pos, mouse->press_pos, shift);
            mouse->shift_length = glm_vec2_norm(shift);
        }

        // Mouse move.
        // NOTE: do not DRAG if we are clicking, with short press time and shift length
        if (mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE &&
            mouse->button != DVZ_MOUSE_BUTTON_NONE &&
            !(time - mouse->press_time < DVZ_MOUSE_CLICK_MAX_DELAY &&
              mouse->shift_length < DVZ_MOUSE_CLICK_MAX_SHIFT))
        {
            log_trace("drag event on button %d", mouse->button);
            mouse->cur_state = DVZ_MOUSE_STATE_DRAG;
            dvz_event_mouse_drag(canvas, mouse->cur_pos, mouse->button);
        }
        // log_trace("mouse mouse %.1fx%.1f", mouse->cur_pos[0], mouse->cur_pos[1]);
        break;


    case DVZ_EVENT_MOUSE_WHEEL:
        glm_vec2_copy(ev.u.w.dir, mouse->wheel_delta);
        mouse->cur_state = DVZ_MOUSE_STATE_WHEEL;
        break;

    default:
        break;
    }
}



// From pixel coordinates (top left origin) to local coordinates (center origin)
void dvz_mouse_local(
    DvzMouse* mouse, DvzMouseLocal* mouse_local, DvzCanvas* canvas, DvzViewport viewport)
{
    _normalize(mouse_local->cur_pos, mouse->cur_pos, viewport);
    _normalize(mouse_local->last_pos, mouse->last_pos, viewport);
    _normalize(mouse_local->press_pos, mouse->press_pos, viewport);
}



/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

DvzKeyboard dvz_keyboard()
{
    DvzKeyboard keyboard = {0};
    dvz_keyboard_reset(&keyboard);
    return keyboard;
}



void dvz_keyboard_reset(DvzKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);
    memset(keyboard, 0, sizeof(DvzKeyboard));
    // keyboard->key_code = DVZ_KEY_NONE;
    // keyboard->modifiers = 0;
    keyboard->press_time = DVZ_NEVER;
}



void dvz_keyboard_event(DvzKeyboard* keyboard, DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(keyboard != NULL);
    ASSERT(canvas != NULL);

    if (canvas->captured)
        return;

    keyboard->prev_state = keyboard->cur_state;

    double time = canvas->clock.elapsed;
    DvzKeyCode key = ev.u.k.key_code;

    if (ev.u.k.type == DVZ_KEY_PRESS && time - keyboard->press_time > .025)
    {
        log_trace("key pressed %d mods %d", key, ev.u.k.modifiers);
        keyboard->key_code = key;
        keyboard->modifiers = ev.u.k.modifiers;
        keyboard->press_time = time;
        if (keyboard->cur_state == DVZ_KEYBOARD_STATE_INACTIVE)
            keyboard->cur_state = DVZ_KEYBOARD_STATE_ACTIVE;
    }
    else
    {
        if (keyboard->cur_state == DVZ_KEYBOARD_STATE_ACTIVE)
            keyboard->cur_state = DVZ_KEYBOARD_STATE_INACTIVE;
    }
}



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

void dvz_event_mouse_button(
    DvzCanvas* canvas, DvzMouseButtonType type, DvzMouseButton button, int modifiers)
{
    ASSERT(canvas != NULL);

    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_BUTTON;
    event.u.b.button = button;
    event.u.b.type = type;
    event.u.b.modifiers = modifiers;

    // Update the mouse state.
    dvz_mouse_event(&canvas->mouse, canvas, event);

    _event_produce(canvas, event);
}



void dvz_event_mouse_move(DvzCanvas* canvas, vec2 pos)
{
    ASSERT(canvas != NULL);

    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_MOVE;
    event.u.m.pos[0] = pos[0];
    event.u.m.pos[1] = pos[1];

    // Update the mouse state.
    dvz_mouse_event(&canvas->mouse, canvas, event);

    _event_produce(canvas, event);
}



void dvz_event_mouse_wheel(DvzCanvas* canvas, vec2 dir)
{
    ASSERT(canvas != NULL);

    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_WHEEL;
    event.u.w.dir[0] = dir[0];
    event.u.w.dir[1] = dir[1];

    // Update the mouse state.
    dvz_mouse_event(&canvas->mouse, canvas, event);

    _event_produce(canvas, event);
}



void dvz_event_mouse_click(DvzCanvas* canvas, vec2 pos, DvzMouseButton button)
{
    ASSERT(canvas != NULL);
    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_CLICK;
    event.u.c.pos[0] = pos[0];
    event.u.c.pos[1] = pos[1];
    event.u.c.button = button;
    event.u.c.double_click = false;
    _event_produce(canvas, event);
}



void dvz_event_mouse_double_click(DvzCanvas* canvas, vec2 pos, DvzMouseButton button)
{
    ASSERT(canvas != NULL);
    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_DOUBLE_CLICK;
    event.u.c.pos[0] = pos[0];
    event.u.c.pos[1] = pos[1];
    event.u.c.button = button;
    event.u.c.double_click = true;
    _event_produce(canvas, event);
}



void dvz_event_mouse_drag(DvzCanvas* canvas, vec2 pos, DvzMouseButton button)
{
    ASSERT(canvas != NULL);
    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_DRAG_BEGIN;
    event.u.d.pos[0] = pos[0];
    event.u.d.pos[1] = pos[1];
    event.u.d.button = button;
    _event_produce(canvas, event);
}



void dvz_event_mouse_drag_end(DvzCanvas* canvas, vec2 pos, DvzMouseButton button)
{
    ASSERT(canvas != NULL);
    DvzEvent event = {0};
    event.type = DVZ_EVENT_MOUSE_DRAG_END;
    event.u.d.pos[0] = pos[0];
    event.u.d.pos[1] = pos[1];
    event.u.d.button = button;
    _event_produce(canvas, event);
}



void dvz_event_key(DvzCanvas* canvas, DvzKeyType type, DvzKeyCode key_code, int modifiers)
{
    ASSERT(canvas != NULL);

    DvzEvent event = {0};
    event.type = DVZ_EVENT_KEY;
    event.u.k.type = type;
    event.u.k.key_code = key_code;
    event.u.k.modifiers = modifiers;

    // Update the keyboard state.
    dvz_keyboard_event(&canvas->keyboard, canvas, event);

    _event_produce(canvas, event);
}



void dvz_event_frame(DvzCanvas* canvas, uint64_t idx, double time, double interval)
{
    ASSERT(canvas != NULL);

    DvzEvent event = {0};
    event.type = DVZ_EVENT_FRAME;
    event.u.f.idx = idx;
    event.u.f.time = time;
    event.u.f.interval = interval;

    _event_produce(canvas, event);
}



int dvz_event_pending(DvzCanvas* canvas, DvzEventType type)
{
    ASSERT(canvas != NULL);
    DvzFifo* fifo = &canvas->event_queue;
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
        if (((DvzEvent*)fifo->items[k])->type == type)
            count++;
    }

    // Add 1 if the event being processed in the event thread has the requested type.
    if (canvas->event_processing == type)
        count++;

    pthread_mutex_unlock(&fifo->lock);
    ASSERT(count >= 0);
    return count;
}



void dvz_event_stop(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzFifo* fifo = &canvas->event_queue;
    dvz_fifo_reset(fifo);
    // Send a null event to the queue which causes the dequeue awaiting thread to end.
    _event_enqueue(canvas, (DvzEvent){0});
}



/*************************************************************************************************/
/*  Screencast                                                                                   */
/*************************************************************************************************/

static void _screencast_cmds(DvzScreencast* screencast)
{
    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    DvzImages* images = screencast->canvas->swapchain.images;
    uint32_t img_count = images->count;

    DvzBarrier barrier = dvz_barrier(screencast->canvas->gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, images);

    for (uint32_t i = 0; i < img_count; i++)
    {
        dvz_cmd_reset(&screencast->cmds, i);
        dvz_cmd_begin(&screencast->cmds, i);

        // Transition to SRC layout
        dvz_barrier_images_layout(
            &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        dvz_barrier_images_access(
            &barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
        dvz_cmd_barrier(&screencast->cmds, i, &barrier);

        // Copy swapchain image to screencast image
        dvz_cmd_copy_image(&screencast->cmds, i, images, &screencast->staging);

        // Transition back to previous layout
        dvz_barrier_images_layout(
            &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        dvz_barrier_images_access(
            &barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        dvz_cmd_barrier(&screencast->cmds, i, &barrier);

        dvz_cmd_end(&screencast->cmds, i);
    }
}



static void _screencast_timer_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzScreencast* screencast = (DvzScreencast*)ev.user_data;

    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    log_trace("screencast timer frame #%d", screencast->frame_idx);

    dvz_fences_wait(&screencast->fence, 0);

    DvzSubmit* submit = &screencast->submit;
    dvz_submit_reset(submit);
    dvz_submit_commands(submit, &screencast->cmds);

    // Wait for "image_ready" semaphore
    dvz_submit_wait_semaphores(
        submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
        &canvas->sem_render_finished, canvas->cur_frame);

    // Signal screencast_finished semaphore
    dvz_submit_signal_semaphores(submit, &screencast->semaphore, 0);

    // Send screencast cmd buf to transfer queue and signal screencast fence when submitting.
    screencast->status = DVZ_SCREENCAST_AWAIT_COPY;
}



static void _screencast_post_send(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzScreencast* screencast = (DvzScreencast*)ev.user_data;

    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    uint32_t img_idx = canvas->swapchain.img_idx;
    // Always make sure the present semaphore is reset to its original value.
    canvas->present_semaphores = &canvas->sem_render_finished;

    // Do nothing if the screencast image is not ready.
    if (screencast->status <= DVZ_SCREENCAST_IDLE)
        return;

    log_trace("screencast event #%d", screencast->frame_idx);

    // Send the copy job
    if (screencast->status == DVZ_SCREENCAST_AWAIT_COPY)
    {
        log_trace("screencast send");
        // The copy job waits for the current image to be ready.
        // It signals the screencast semaphore when the copy is done.
        // The present swapchain command must wait for the screencast semaphore rather than
        // the render_finished semaphore.
        // HACK: do not wait when submitting at the first frame.
        dvz_submit_send(&screencast->submit, img_idx, &screencast->fence, 0);
        // canvas->frame_idx == 0 ? NULL : &screencast->fence, 0);

        canvas->present_semaphores = &screencast->semaphore;
        screencast->status = DVZ_SCREENCAST_AWAIT_TRANSFER;
    }

    else if (screencast->status == DVZ_SCREENCAST_AWAIT_TRANSFER)
    {
        // if (!dvz_fences_ready(&screencast->fence, 0))
        // {
        //     log_trace("screencast await transfer but fence not ready");
        //     return;
        // }
        // else
        // {
        //     log_trace("screencast await transfer and fence ready");
        // }
        dvz_fences_wait(&screencast->fence, 0);

        // To be freed by the SCREENCAST event callback.
        uint8_t* rgb_a =
            calloc(screencast->staging.width * screencast->staging.height, 4 * sizeof(uint8_t));

        // Copy the image from the staging image to the CPU.
        log_trace("screencast CPU download");
        dvz_images_download(&screencast->staging, 0, true, screencast->has_alpha, rgb_a);

        // Enqueue a special SCREENCAST public event with a pointer to the CPU buffer user
        DvzEvent sev = {0};
        sev.type = DVZ_EVENT_SCREENCAST;
        sev.u.sc.idx = screencast->frame_idx;
        sev.u.sc.interval = screencast->clock.interval;
        sev.u.sc.rgba = rgb_a;
        sev.u.sc.width = screencast->staging.width;
        sev.u.sc.height = screencast->staging.height;
        log_trace("send SCREENCAST event");
        _event_produce(canvas, sev);

        // Reset screencast status.
        _clock_set(&screencast->clock);
        screencast->status = DVZ_SCREENCAST_IDLE;
        screencast->frame_idx++;
    }
}



static void _screencast_resize(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzScreencast* screencast = (DvzScreencast*)ev.user_data;

    ASSERT(screencast != NULL);
    ASSERT(screencast->canvas != NULL);
    ASSERT(screencast->canvas->gpu != NULL);

    screencast->status = DVZ_SCREENCAST_NONE;
    dvz_images_resize(
        &screencast->staging, canvas->swapchain.images->width, canvas->swapchain.images->height,
        canvas->swapchain.images->depth);
    dvz_images_transition(&screencast->staging);

    _screencast_cmds(screencast);
}



static void _screencast_destroy(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    dvz_screencast_destroy(canvas);
}



void dvz_screencast(DvzCanvas* canvas, double interval, bool has_alpha)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);

    DvzGpu* gpu = canvas->gpu;
    DvzImages* images = canvas->swapchain.images;

    canvas->screencast = calloc(1, sizeof(DvzScreencast));
    DvzScreencast* sc = canvas->screencast;
    sc->canvas = canvas;
    sc->has_alpha = has_alpha;

    sc->staging = dvz_images(canvas->gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(&sc->staging, images->format);
    dvz_images_size(&sc->staging, images->width, images->height, images->depth);
    dvz_images_tiling(&sc->staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(&sc->staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(&sc->staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_images_memory(
        &sc->staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_images_create(&sc->staging);

    // Transition the staging image to its layout.
    dvz_images_transition(&sc->staging);

    sc->fence = dvz_fences(gpu, 1, true);
    ASSERT(dvz_fences_ready(&sc->fence, 0));
    sc->semaphore = dvz_semaphores(gpu, 1);

    // NOTE: we predefine the transfer command buffers, one per swapchain image.
    sc->cmds =
        dvz_commands(canvas->gpu, DVZ_DEFAULT_QUEUE_TRANSFER, canvas->swapchain.images->count);
    _screencast_cmds(sc);
    sc->submit = dvz_submit(canvas->gpu);

    _clock_init(&sc->clock);

    dvz_event_callback(
        canvas, DVZ_EVENT_TIMER, interval, DVZ_EVENT_MODE_SYNC, _screencast_timer_callback, sc);
    dvz_event_callback(
        canvas, DVZ_EVENT_POST_SEND, 0, DVZ_EVENT_MODE_SYNC, _screencast_post_send, sc);
    dvz_event_callback(canvas, DVZ_EVENT_RESIZE, 0, DVZ_EVENT_MODE_SYNC, _screencast_resize, sc);
    dvz_event_callback(canvas, DVZ_EVENT_DESTROY, 0, DVZ_EVENT_MODE_SYNC, _screencast_destroy, sc);

    sc->obj.type = DVZ_OBJECT_TYPE_SCREENCAST;
    dvz_obj_created(&sc->obj);
}



void dvz_screencast_destroy(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzScreencast* screencast = canvas->screencast;
    if (screencast == NULL)
        return;
    ASSERT(screencast != NULL);
    if (!dvz_obj_is_created(&screencast->obj))
        return;

    dvz_fences_destroy(&screencast->fence);
    dvz_semaphores_destroy(&screencast->semaphore);
    dvz_images_destroy(&screencast->staging);

    dvz_obj_destroyed(&screencast->obj);
    FREE(screencast);
    canvas->screencast = NULL;
}



void dvz_screenshot_file(DvzCanvas* canvas, const char* png_path)
{
    log_info("saving screenshot of canvas to %s with full synchronization (slow)", png_path);
    uint8_t* rgb = dvz_screenshot(canvas, false);
    DvzImages* images = canvas->swapchain.images;
    dvz_write_png(png_path, images->width, images->height, rgb);
    FREE(rgb);
}



uint8_t* dvz_screenshot(DvzCanvas* canvas, bool has_alpha)
{
    // TODO: more efficient screenshot saving with screencast
    DvzGpu* gpu = canvas->gpu;

    dvz_gpu_wait(gpu);
    DvzImages* images = canvas->swapchain.images;
    DvzImages staging = dvz_images(canvas->gpu, VK_IMAGE_TYPE_2D, 1);

    // Staging images.
    {
        dvz_images_format(&staging, images->format);
        dvz_images_size(&staging, images->width, images->height, images->depth);
        dvz_images_tiling(&staging, VK_IMAGE_TILING_LINEAR);
        dvz_images_usage(&staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        dvz_images_layout(&staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        dvz_images_memory(
            &staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        dvz_images_create(&staging);
        dvz_images_transition(&staging);
    }

    // Copy from the swapchain image to the staging image.
    {
        DvzBarrier barrier = dvz_barrier(canvas->gpu);
        dvz_barrier_stages(
            &barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        dvz_barrier_images(&barrier, images);
        DvzCommands* cmds = &canvas->cmds_transfer;
        dvz_cmd_reset(cmds, 0);
        dvz_cmd_begin(cmds, 0);
        dvz_barrier_images_layout(
            &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        dvz_barrier_images_access(
            &barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
        dvz_cmd_barrier(cmds, 0, &barrier);
        dvz_cmd_copy_image(cmds, 0, images, &staging);
        dvz_barrier_images_layout(
            &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        dvz_barrier_images_access(
            &barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        dvz_cmd_barrier(cmds, 0, &barrier);
        dvz_cmd_end(cmds, 0);
        dvz_cmd_submit_sync(cmds, 0);
    }

    // Make the screenshot.
    uint8_t* rgba = calloc(staging.width * staging.height, (has_alpha ? 4 : 3) * sizeof(uint8_t));
    dvz_images_download(&staging, 0, true, has_alpha, rgba);
    dvz_gpu_wait(gpu);
    dvz_images_destroy(&staging);
    // NOTE: the caller MUST free the returned pointer.
    return rgba;
}



static void _video_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    log_debug("video frame #%d", ev.u.sc.idx);
    add_frame((Video*)ev.user_data, ev.u.sc.rgba);
    FREE(ev.u.sc.rgba);
}

static void _video_destroy(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    if (ev.user_data != NULL)
        end_video((Video*)ev.user_data);
}

void dvz_canvas_video(DvzCanvas* canvas, int framerate, int bitrate, const char* path)
{
    uvec2 size;
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);
    Video* video = create_video(path, (int)size[0], (int)size[1], framerate, bitrate);
    if (video == NULL)
        return;

    dvz_event_callback(
        canvas, DVZ_EVENT_SCREENCAST, 0, DVZ_EVENT_MODE_SYNC, _video_callback, video);
    dvz_event_callback(canvas, DVZ_EVENT_DESTROY, 0, DVZ_EVENT_MODE_SYNC, _video_destroy, video);

    dvz_screencast(canvas, 1. / framerate, true);
}



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

void dvz_canvas_frame(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->gpu != NULL);

    // Update the global and local clocks.
    // These calls update canvas->clock.elapsed and canvas->clock.interval, the latter is
    // the delay since the last frame.
    _clock_set(&canvas->app->clock); // global clock
    _clock_set(&canvas->clock);      // canvas-local clock

    // Call INTERACT callbacks (for backends only), which may enqueue some events.
    _event_interact(canvas);

    // Call FRAME callbacks.
    _event_frame(canvas);

    // Give a chance to update event structures in the main loop, for example reset wheel.
    _backend_next_frame(canvas);

    // Call TIMER callbacks, in the main thread.
    _event_timer(canvas);

    // Refill all command buffers at the first iteration.
    if (canvas->frame_idx == 0)
        dvz_canvas_to_refill(canvas);

    // Pending transfers.
    dvz_process_transfers(canvas);

    // Refill if needed, only 1 swapchain command buffer per frame to avoid waiting on the device.
    _refill_frame(canvas);
}



void dvz_canvas_frame_submit(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    DvzSubmit* s = &canvas->submit;
    uint32_t f = canvas->cur_frame;
    uint32_t img_idx = canvas->swapchain.img_idx;

    // Keep track of the fence associated to the current swapchain image.
    dvz_fences_copy(
        &canvas->fences_render_finished, f, //
        &canvas->fences_flight, img_idx);

    // Reset the Submit instance before adding the command buffers.
    dvz_submit_reset(s);

    // Add the command buffers to the submit instance.
    // Default render commands.
    if (canvas->cmds_render.obj.status == DVZ_OBJECT_STATUS_CREATED)
        dvz_submit_commands(s, &canvas->cmds_render);

    // // Extra render commands.
    // DvzCommands* cmds = dvz_container_iter(&canvas->commands);
    // while (cmds != NULL)
    // {
    //     if (cmds->obj.status == DVZ_OBJECT_STATUS_NONE)
    //         break;
    //     if (cmds->obj.status == DVZ_OBJECT_STATUS_INACTIVE)
    //         continue;
    //     if (cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER)
    //         dvz_submit_commands(s, cmds);
    //     cmds = dvz_container_iter(&canvas->commands);
    // }
    if (s->commands_count == 0)
    {
        log_error("no recorded command buffers");
        return;
    }

    if (!canvas->offscreen)
    {
        dvz_submit_wait_semaphores(
            s, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
            &canvas->sem_img_available, f);

        // Once the render is finished, we signal another semaphore.
        dvz_submit_signal_semaphores(s, &canvas->sem_render_finished, f);
    }

    // SEND callbacks and send the Submit instance.
    {
        // Call PRE_SEND callbacks
        _event_presend(canvas);

        // Send the Submit instance.
        dvz_submit_send(s, img_idx, &canvas->fences_render_finished, f);

        // Call POST_SEND callbacks
        _event_postsend(canvas);
    }

    // Once the image is rendered, we present the swapchain image.
    // The semaphore used for waiting during presentation may be changed by the canvas
    // callbacks.
    if (!canvas->offscreen)
        dvz_swapchain_present(
            &canvas->swapchain, 1, //
            canvas->present_semaphores, CLIP(f, 0, canvas->present_semaphores->count - 1));

    canvas->cur_frame = (f + 1) % canvas->fences_render_finished.count;
}



void dvz_app_run(DvzApp* app, uint64_t frame_count)
{
    if (frame_count > 1)
        log_debug("start main loop with %d frames", frame_count);
    ASSERT(app != NULL);
    app->is_running = true;
    if (frame_count == 0)
        frame_count = UINT64_MAX;
    ASSERT(frame_count > 0);

    DvzContainerIterator iterator;
    DvzCanvas* canvas = NULL;

    // Main loop.
    uint32_t n_canvas_active = 0;
    for (uint64_t iter = 0; iter < frame_count; iter++)
    {
        n_canvas_active = 0;

        // Loop over the canvases.
        iterator = dvz_container_iterator(&app->canvases);
        canvas = NULL;
        while (iterator.item != NULL)
        {
            canvas = (DvzCanvas*)iterator.item;
            ASSERT(canvas != NULL);
            if (canvas->obj.status < DVZ_OBJECT_STATUS_CREATED)
            {
                dvz_container_iter(&iterator);
                continue;
            }
            ASSERT(canvas->obj.status >= DVZ_OBJECT_STATUS_CREATED);

            // INIT event at the first frame
            if (canvas->frame_idx == 0)
            {
                // Call RESIZE callbacks at initialization.
                _event_resize(canvas);

                DvzEvent ev = {0};
                ev.type = DVZ_EVENT_INIT;
                _event_produce(canvas, ev);
            }

            // Poll events.
            if (canvas->window != NULL)
                dvz_window_poll_events(canvas->window);

            // NOTE: swapchain image acquisition happens here

            // Wait for fence.
            dvz_fences_wait(&canvas->fences_render_finished, canvas->cur_frame);

            // We acquire the next swapchain image.
            // NOTE: this call modifies swapchain->img_idx
            if (!canvas->offscreen)
                dvz_swapchain_acquire(
                    &canvas->swapchain, &canvas->sem_img_available, //
                    canvas->cur_frame, NULL, 0);

            // If there is a problem with swapchain image acquisition, wait and try again later.
            if (canvas->swapchain.obj.status == DVZ_OBJECT_STATUS_INVALID)
            {
                log_trace("swapchain image acquisition failed, waiting and skipping this frame");
                dvz_gpu_wait(canvas->gpu);
                dvz_container_iter(&iterator);
                continue;
            }

            // If the swapchain needs to be recreated (for example, after a resize), do it.
            if (canvas->swapchain.obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
            {
                log_trace("swapchain image acquisition failed, recreating the canvas");

                // Recreate the canvas.
                dvz_canvas_recreate(canvas);

                // Update the DvzViewport struct and call RESIZE callbacks.
                _event_resize(canvas);
                canvas->resized = true;
                if (canvas->screencast != NULL)
                    log_error("resizing is not supported during a screencast");

                // Refill the canvas after the DvzViewport has been updated.
                // _refill_canvas(canvas, UINT32_MAX);
                dvz_canvas_to_refill(canvas);

                n_canvas_active++;
                dvz_container_iter(&iterator);
                continue;
            }

            // Destroy the canvas if needed.
            if (canvas->window != NULL)
            {
                if (backend_window_should_close(app->backend, canvas->window->backend_window))
                    canvas->window->obj.status = DVZ_OBJECT_STATUS_NEED_DESTROY;
                if (canvas->window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
                    canvas->obj.status = DVZ_OBJECT_STATUS_NEED_DESTROY;
            }
            if (canvas->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
            {
                log_trace("destroying canvas");

                // Stop the transfer queue.
                dvz_event_stop(canvas);

                // Wait for all GPUs to be idle.
                dvz_app_wait(app);

                // Destroy the canvas.
                dvz_canvas_destroy(canvas);
                dvz_container_iter(&iterator);
                continue;
            }

            // Frame logic.
            dvz_canvas_frame(canvas);
            canvas->resized = false;

            // Submit the command buffers and swapchain logic.
            // log_trace("submitting frame for canvas #%d", canvas_idx);
            dvz_canvas_frame_submit(canvas);
            canvas->frame_idx++;
            n_canvas_active++;


            dvz_container_iter(&iterator);
        }

        // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
        // when waiting for fences (not sure why). The problem only arises when using different
        // queues for command buffer submission and swapchain present. There has be a better way
        // to fix this.

        // NOTE: this has never been tested with multiple GPUs yet.
        iterator = dvz_container_iterator(&app->gpus);
        DvzGpu* gpu = NULL;
        while (iterator.item != NULL)
        {
            gpu = iterator.item;
            if (!dvz_obj_is_created(&gpu->obj))
                break;
            if (gpu->queues.queues[DVZ_DEFAULT_QUEUE_PRESENT] != VK_NULL_HANDLE &&
                gpu->queues.queues[DVZ_DEFAULT_QUEUE_PRESENT] !=
                    gpu->queues.queues[DVZ_DEFAULT_QUEUE_RENDER])
            // && iter % DVZ_MAX_SWAPCHAIN_IMAGES == 0)
            {
                dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_PRESENT);
            }

            dvz_container_iter(&iterator);
        }

        // Close the application if all canvases have been closed.
        if (n_canvas_active == 0)
        {
            log_trace("no more active canvas, closing the app");
            break;
        }
    }
    log_trace("end main loop");

    dvz_app_wait(app);
    app->is_running = false;
}



/*************************************************************************************************/
/*  Canvas destruction                                                                           */
/*************************************************************************************************/

void dvz_canvas_destroy(DvzCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
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

    // Stop the event thread.
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    dvz_gpu_wait(canvas->gpu);
    dvz_event_stop(canvas);
    dvz_thread_join(&canvas->event_thread);
    dvz_fifo_destroy(&canvas->event_queue);

    // Destroy the transfers queue.
    dvz_fifo_destroy(&canvas->transfers);

    // Destroy callbacks.
    _destroy_callbacks(canvas);

    // Destroy the graphics.
    log_trace("canvas destroy graphics pipelines");
    CONTAINER_DESTROY_ITEMS(DvzGraphics, canvas->graphics, dvz_graphics_destroy)
    dvz_container_destroy(&canvas->graphics);

    // Destroy the depth image.
    dvz_images_destroy(&canvas->depth_image);

    // Destroy the renderpasses.
    log_trace("canvas destroy renderpass");
    dvz_renderpass_destroy(&canvas->renderpass);
    if (canvas->overlay)
        dvz_renderpass_destroy(&canvas->renderpass_overlay);

    // Destroy the swapchain.
    log_trace("canvas destroy swapchain");
    dvz_swapchain_destroy(&canvas->swapchain);

    // Destroy the framebuffers.
    log_trace("canvas destroy framebuffers");
    dvz_framebuffers_destroy(&canvas->framebuffers);
    if (canvas->overlay)
        dvz_framebuffers_destroy(&canvas->framebuffers_overlay);

    // Destroy the window.
    log_trace("canvas destroy window");
    if (canvas->window != NULL)
    {
        ASSERT(canvas->window->app != NULL);
        dvz_window_destroy(canvas->window);
    }

    log_trace("canvas destroy commands");
    CONTAINER_DESTROY_ITEMS(DvzCommands, canvas->commands, dvz_commands_destroy)
    dvz_container_destroy(&canvas->commands);

    // Destroy the semaphores.
    log_trace("canvas destroy semaphores");
    dvz_semaphores_destroy(&canvas->sem_img_available);
    dvz_semaphores_destroy(&canvas->sem_render_finished);

    // Destroy the fences.
    log_trace("canvas destroy fences");
    dvz_fences_destroy(&canvas->fences_render_finished);

    if (canvas->overlay)
        dvz_imgui_destroy(canvas);
    CONTAINER_DESTROY_ITEMS(DvzGui, canvas->guis, dvz_gui_destroy)
    dvz_container_destroy(&canvas->guis);


    dvz_obj_destroyed(&canvas->obj);
}



void dvz_canvases_destroy(DvzContainer* canvases)
{
    if (canvases == NULL || canvases->capacity == 0)
        return;
    log_trace("destroy all canvases");
    CONTAINER_DESTROY_ITEMS(DvzCanvas, (*canvases), dvz_canvas_destroy)
    dvz_container_destroy(canvases);
}
