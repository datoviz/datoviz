#include "glfw.h"



static void _mouse_wheel_callback(GLFWwindow* window, double dx, double dy)
{
    // Update the MouseStateMachine when scrolling.
    VkyCanvas* canvas = (VkyCanvas*)glfwGetWindowUserPointer(window);
    VkyMouse* mouse = canvas->event_controller->mouse;

    if (!canvas->event_controller->do_process_input)
        return;

    mouse->prev_state = mouse->cur_state;
    mouse->cur_state = VKY_MOUSE_STATE_WHEEL;

    float delta = (float)dy * .1f;
    mouse->wheel_delta[0] = delta;
    mouse->wheel_delta[1] = delta;

    if (VKY_INVERSE_MOUSE_WHEEL)
    {
        mouse->wheel_delta[0] *= -1;
        mouse->wheel_delta[1] *= -1;
    }
}

static void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    VkyCanvas* canvas = (VkyCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas->event_controller != NULL);

    if (!canvas->event_controller->do_process_input)
        return;

    VkyKeyboard* keyboard = canvas->event_controller->keyboard;
    ASSERT(keyboard != NULL);

    if (action == GLFW_PRESS && keyboard->key == VKY_KEY_NONE)
    {
        keyboard->key = key;
    }
    else if (action == GLFW_RELEASE && keyboard->key != VKY_KEY_NONE)
    {
        keyboard->key = VKY_KEY_NONE;
    }
    else if (action == GLFW_REPEAT && keyboard->key == VKY_KEY_NONE)
    {
        keyboard->key = key;
    }
    else
    {
        keyboard->key = VKY_KEY_NONE;
    }

    // Set modifiers.
    ASSERT(mods >= 0);
    keyboard->modifiers = (uint32_t)mods;
}



VkyMouseButton vky_glfw_get_mouse_button(GLFWwindow* window)
{
    VkyMouseButton button = VKY_MOUSE_BUTTON_NONE;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        button = VKY_MOUSE_BUTTON_LEFT;
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    {
        button = VKY_MOUSE_BUTTON_MIDDLE;
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        button = VKY_MOUSE_BUTTON_RIGHT;
    }
    return button;
}

void vky_glfw_get_mouse_pos(GLFWwindow* window, vec2 pos)
{
    // Get the mouse position.
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    pos[0] = (float)x;
    pos[1] = (float)y;
}

void vky_glfw_get_keyboard(GLFWwindow* window, VkyKey* key, uint32_t* modifiers)
{
    VkyCanvas* canvas = (VkyCanvas*)glfwGetWindowUserPointer(window);
    ASSERT(canvas->event_controller != NULL);
    VkyKeyboard* keyboard = canvas->event_controller->keyboard;
    ASSERT(keyboard != NULL);
    if (keyboard->key != VKY_KEY_NONE)
    {
        *key = keyboard->key;
        *modifiers = keyboard->modifiers;
    }
}

VkyCanvas* vky_glfw_create_canvas(VkyApp* app, uint32_t width, uint32_t height)
{
    log_trace("create glfw canvas with size %dx%d", width, height);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow((int)width, (int)height, APPLICATION_NAME, NULL, NULL);
    ASSERT(window != NULL);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(app->gpu->instance, window, NULL, &surface) != VK_SUCCESS)
    {
        log_error("error creating the GLFW surface");
    }

    // Register the mouse wheel callback.
    glfwSetScrollCallback(window, _mouse_wheel_callback);

    // Register the key callback.
    glfwSetKeyCallback(window, _key_callback);

    // Prepare the GPU for the first canvas (to get the surface info to create the Device).
    if (app->gpu->image_count == 0)
    {
        vky_prepare_gpu(app->gpu, &surface);
    }

    // Create the Canvas.
    VkyCanvas* canvas = vky_create_canvas_from_surface(app, window, &surface);

    float xscale = 0, yscale = 0;

#if GLFW_GT_33
    // This function is only available in glfw3 >= 3.3
    glfwGetWindowContentScale(window, &xscale, &yscale);
#else
    log_warn("Visky has not been compiled with glfw3>=3.3 so won't be DPI-aware");
    xscale = yscale = 1;
#endif

    canvas->size.content_scale = .5 * (xscale + yscale);
    canvas->dpi_factor *= canvas->size.content_scale;
#if OS_MACOS
    // HACK: scaling is too important on macOS Retina screen
    canvas->dpi_factor *= .75;
#endif
    log_trace("content scale %.3f %.3f, dpi %.3f", xscale, yscale, canvas->dpi_factor);

    // The canvas pointer will be available to callback functions.
    glfwSetWindowUserPointer(window, canvas);

    return canvas;
}

void vky_glfw_begin_frame(VkyCanvas* canvas)
{
    // log_trace("begin frame");

    if (canvas->need_recreation)
    {
        return;
    }

    // Determine whether the window has been resized.
    // int lw = (int)canvas->window_size.lw;
    // int lh = (int)canvas->window_size.lh;
    int lw = 0, lh = 0, w = 0, h = 0;

    // Check window resize.
    lw = (int)canvas->size.window_width;
    lh = (int)canvas->size.window_height;
    glfwGetWindowSize(canvas->window, &w, &h);
    if (w != lw || h != lh)
    {
        log_trace("resized window %dx%d", w, h);
        ASSERT((w >= 0) && (h >= 0));
        canvas->size.resized = true;
        canvas->size.window_width = (uint32_t)w;
        canvas->size.window_height = (uint32_t)h;
    }

    // Check framebuffer resize.
    lw = (int)canvas->size.framebuffer_width;
    lh = (int)canvas->size.framebuffer_height;
    glfwGetFramebufferSize(canvas->window, &w, &h);
    if (w != lw || h != lh)
    {
        log_trace("resized framebuffer %dx%d", w, h);
        ASSERT((w >= 0) && (h >= 0));
        canvas->size.resized = true;
        canvas->size.framebuffer_width = (uint32_t)w;
        canvas->size.framebuffer_height = (uint32_t)h;
    }

    // Resize callback.
    if (canvas->size.resized && canvas->cb_resize != NULL)
    {
        canvas->cb_resize(canvas);
    }

    // canvas->window_size.lw = canvas->window_size.w;
    // canvas->window_size.lh = canvas->window_size.h;
    glfwPollEvents();

    // Acquire the next swap chain image.
    if (canvas->cb_fill_command_buffer != NULL)
    {

        VkyGpu* gpu = canvas->gpu;
        VkyDrawSync* draw_sync = &canvas->draw_sync;
        vkWaitForFences(
            gpu->device, 1, &(draw_sync->in_flight_fences[canvas->current_frame]), VK_TRUE,
            UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(
            gpu->device, canvas->swapchain, UINT64_MAX,
            draw_sync->image_available_semaphores[canvas->current_frame], VK_NULL_HANDLE,
            &(canvas->image_index));

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            log_trace("out of date begin frame");
            // Handle minimization: wait until the device is ready and the window fully resized.
            vky_glfw_wait(canvas);
            // in the main loop
            canvas->need_recreation = true;
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            log_error("failed acquiring the swap chain image");
        }

        if (draw_sync->images_in_flight[canvas->image_index] != VK_NULL_HANDLE)
        {
            vkWaitForFences(
                gpu->device, 1, &(draw_sync->images_in_flight[canvas->image_index]), VK_TRUE,
                UINT64_MAX);
        }
        draw_sync->images_in_flight[canvas->image_index] =
            draw_sync->in_flight_fences[canvas->current_frame];
    }
}

void vky_glfw_end_frame(VkyCanvas* canvas)
{
    // log_trace("end frame");
    ASSERT(!canvas->need_recreation);

    VkyGpu* gpu = canvas->gpu;

    if (canvas->cb_fill_command_buffer != NULL)
    {

        VkyDrawSync* draw_sync = &canvas->draw_sync;

        // Present the buffer to the surface.
        VkPresentInfoKHR presentInfo = {0};
        VkSemaphore signalSemaphores[] = {
            draw_sync->render_finished_semaphores[canvas->current_frame]};

        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = {canvas->swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &canvas->image_index;

        VkResult result = vkQueuePresentKHR(gpu->present_queue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            canvas->size.resized)
        {
            log_trace("out of date end frame");
            canvas->size.resized = false;
            canvas->need_recreation = true;
            vky_glfw_wait(canvas);
            canvas->current_frame = (canvas->current_frame + 1) % VKY_MAX_FRAMES_IN_FLIGHT;
            canvas->frame_count++;
            return;
        }
        else if (result != VK_SUCCESS)
        {
            log_error("failed presenting the swap chain image");
        }
    }

    canvas->current_frame = (canvas->current_frame + 1) % VKY_MAX_FRAMES_IN_FLIGHT;
    canvas->frame_count++;
}

void vky_glfw_wait(VkyCanvas* canvas)
{
    GLFWwindow* window = canvas->window;
    int w, h;
    // Wait until the device is ready and the window fully resized.
    glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0)
    {
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
    }
    ASSERT((w > 0) && (h > 0));
    // canvas->window_size.w = (uint32_t)w;
    // canvas->window_size.h = (uint32_t)h;
    canvas->size.framebuffer_width = (uint32_t)w;
    canvas->size.framebuffer_height = (uint32_t)h;
    vkDeviceWaitIdle(canvas->gpu->device);
}

void vky_glfw_stop_app(VkyApp* app)
{
    for (uint32_t i = 0; i < app->canvas_count; i++)
    {
        app->canvases[i]->to_close = true;
    }
}



void vky_glfw_run_app_begin(VkyApp* app)
{
    log_trace("main glfw event loop");
    vky_start_timer();

    // Create the swap chain and all objects that will need to be recreated upon resize.
    for (uint32_t i = 0; i < app->canvas_count; i++)
    {
        vky_upload_pending_data(app->canvases[i]);
        vky_fill_command_buffers(app->canvases[i]);
    }
}

void vky_glfw_run_app_process(VkyApp* app)
{
    // Check whether all windows are closed
    app->all_windows_closed = true;
    VkyCanvas* canvas;
    bool should_close = false;
    uint32_t cmd_buf_count = 1;
    VkCommandBuffer submit_cmd_bufs[2];
    uint64_t fps = 0;

    for (uint32_t i = 0; i < app->canvas_count; i++)
    {
        canvas = app->canvases[i];
        if (canvas == NULL)
            continue;
        if (canvas->window == NULL)
            continue;

        // Close one canvas.
        should_close = glfwWindowShouldClose((GLFWwindow*)canvas->window) || canvas->to_close;
        if (should_close)
        {
            glfwWaitEvents();
            glfwPollEvents();
            glfwDestroyWindow(canvas->window);
            app->canvases[i]->window = NULL;
            continue;
        }

        // Update the mouse/keyboard states, and call the user callbacks for the mouse,
        // keyboard, and frame.
        canvas->dt = vky_get_timer() - canvas->local_time; // time since last frame
        canvas->local_time = vky_get_timer();
        vky_next_frame(canvas);

        // Begin of the frame, skipped if the canvas need to be recreated after a resize.
        // This method also updates the window size.
        vky_glfw_begin_frame(canvas);

        // Empty canvases: skip the vulkan swapchain logic.
        if (canvas->cb_fill_command_buffer != NULL)
        {
            // Handle resizing.
            if (canvas->need_recreation)
            {
                // Recreate the swap chain resources.
                vky_destroy_swapchain_resources(canvas);
                vky_glfw_wait(canvas);
                vky_create_swapchain_resources(canvas);
                // Reset and refill the command buffers.
                vky_fill_command_buffers(canvas);
                canvas->need_recreation = false;
                // We go directly to the next frame.
            }
            else
            {
                submit_cmd_bufs[0] = canvas->command_buffers[canvas->image_index];
                if (canvas->cb_fill_live_command_buffer == NULL)
                {
                    cmd_buf_count = 1;
                }
                else
                {
                    cmd_buf_count = 2;
                    // Fill the live command buffer.
                    vky_fill_live_command_buffers(canvas);
                    submit_cmd_bufs[1] = canvas->live_command_buffers[canvas->image_index];
                }
                vky_submit_command_buffers(canvas, cmd_buf_count, submit_cmd_bufs);
                // Swap chain logic and presentation of the current frame buffer.
                vky_glfw_end_frame(canvas);
                vky_compute_submit(canvas->gpu);
            }
        }
        else
        {
            // For empty canvases.
            vky_glfw_wait(canvas);
            vky_glfw_end_frame(canvas);
        }

        app->all_windows_closed &= should_close;
        fps = vky_get_fps(canvas->frame_count);
        if (fps > 0)
            canvas->fps = fps;
    }
}

void vky_glfw_run_app_end(VkyApp* app)
{
    log_trace("all windows have closed");
    if (app != NULL && app->gpu != NULL && app->gpu->device != 0)
        vkDeviceWaitIdle(app->gpu->device);
}

void vky_glfw_run_app(VkyApp* app)
{
    vky_glfw_run_app_begin(app);
    while (!vky_all_windows_closed(app))
    {
        vky_glfw_run_app_process(app);
    }
    vky_glfw_run_app_end(app);
}
