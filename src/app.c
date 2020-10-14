#include "../include/visky/visky.h"

#include "glfw.h"
// #include "vnc.h"



/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/

static void _esc_close_canvas(VkyCanvas* canvas)
{
    VkyKeyboard* keyboard = canvas->event_controller->keyboard;
    if (keyboard->key == VKY_KEY_ESCAPE)
    {
        vky_close_canvas(canvas);
    }
    // return 0;
}

static void fcb(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);
    vky_end_render_pass(cmd_buf, canvas);
}



void vky_wait_canvas_ready(VkyCanvas* canvas)
{
    VkyApp* app = canvas->app;
    ASSERT(app != NULL);
    if (app->cb_wait != NULL)
    {
        app->cb_wait(canvas);
    }
}

VkyApp* vky_create_app(VkyBackendType backend, void* backend_params)
{
    log_trace("create app with backend %d", backend);
    VkyApp* app = (VkyApp*)calloc(1, sizeof(VkyApp));

    app->canvases = calloc(VKY_MAX_CANVASES, sizeof(VkyCanvas*));

    app->backend = backend;
    app->backend_params = backend_params;

    // Extensions required by glfw.
    uint32_t required_extension_count = 0;
    const char** required_extensions = NULL;

    switch (backend)
    {

    case VKY_BACKEND_GLFW:
        app->cb_wait = vky_glfw_wait;
        log_trace("init glfw backend");
        glfwInit();
        ASSERT(glfwVulkanSupported() != 0);
        required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
        ASSERT(required_extensions != NULL);
        ASSERT(required_extension_count > 0);
        break;

    default:
        break;
    }

    app->gpu = calloc(1, sizeof(VkyGpu));
    *app->gpu = vky_create_device(required_extension_count, required_extensions);

    return app;
}

VkyCanvas* vky_create_canvas(VkyApp* app, uint32_t width, uint32_t height)
{
    log_trace("create canvas with size %dx%d", width, height);
    VkyCanvas* canvas = NULL;
    VkyBackendType backend = app->backend;

    switch (backend)
    {

    case VKY_BACKEND_GLFW:
        canvas = vky_glfw_create_canvas(app, width, height);
        break;

    case VKY_BACKEND_VIDEO:
    // case VKY_BACKEND_VNC:
    case VKY_BACKEND_OFFSCREEN:
    case VKY_BACKEND_SCREENSHOT:
        canvas = vky_create_offscreen_canvas(app->gpu, width, height);
        break;

    default:
        break;
    }

    ASSERT(canvas != NULL);
    canvas->app = app;

    vky_create_event_controller(canvas);
    ASSERT(canvas->event_controller != NULL);

    app->canvases[app->canvas_count] = canvas;
    app->canvas_count++;

    // Black canvas by default.
    canvas->cb_fill_command_buffer = fcb;

    vky_add_frame_callback(canvas, _esc_close_canvas);

    return canvas;
}

void vky_run_app(VkyApp* app)
{
    log_trace("run app");
    VkyBackendType backend = app->backend;
    switch (backend)
    {

    case VKY_BACKEND_GLFW:
        vky_glfw_run_app(app);
        break;

    case VKY_BACKEND_VIDEO:;
        vky_run_video_app(app);
        break;

        // case VKY_BACKEND_VNC:
        //     vky_run_vnc_app(app);
        //     break;

    case VKY_BACKEND_OFFSCREEN:
        vky_run_offscreen_app(app);
        break;

    case VKY_BACKEND_SCREENSHOT:
        vky_run_screenshot_app(app);
        break;

    default:
        break;
    }
}

void vky_close_canvas(VkyCanvas* canvas)
{
    VkyBackendType backend = canvas->app->backend;
    switch (backend)
    {

    case VKY_BACKEND_GLFW:
        glfwSetWindowShouldClose(canvas->window, 1);
        break;

    default:
        break;
    }
}

void vky_destroy_app(VkyApp* app)
{
    log_trace("destroy app");
    VkyCanvas* canvas;
    VkyBackendType backend = app->backend;

    // Destroy the canvases.
    for (uint32_t i = 0; i < app->canvas_count; i++)
    {
        log_trace("destroy canvas %d", i);
        canvas = app->canvases[i];
        if (canvas == NULL)
            continue;
        vky_destroy_event_controller(canvas->event_controller);
        GLFWwindow* window = (GLFWwindow*)canvas->window;
        vky_destroy_canvas(canvas);
        // Backend-specific window destruction.
        switch (backend)
        {

        case VKY_BACKEND_GLFW:
            if (window != NULL)
                glfwDestroyWindow(window);
            break;

            // case VKY_BACKEND_VNC:
            //     vky_destroy_screenshot(
            //         ((VkyBackendVNCParams*)canvas->app->backend_params)->screenshot);
            //     break;

        default:
            break;
        }
        free(canvas);
    }

    free(app->canvases);
    vky_destroy_device(app->gpu);

    switch (backend)
    {

    case VKY_BACKEND_GLFW:
        // Backend-specific app destruction.
        glfwTerminate();
        break;

    default:
        break;
    }

    if (app->links != NULL)
    {
        free(app->links);
        app->link_count = 0;
        app->links = NULL;
    }

    free(app->gpu);
    free(app);
}

bool vky_all_windows_closed(VkyApp* app) { return app->all_windows_closed; }



/*************************************************************************************************/
/*  Event controller                                                                             */
/*************************************************************************************************/

void vky_create_event_controller(VkyCanvas* canvas)
{
    log_trace("create event controller");

    canvas->event_controller = calloc(1, sizeof(VkyEventController));
    canvas->event_controller->do_process_input = true;
    canvas->event_controller->canvas = canvas;

    // Create the VkyMouse struct.
    canvas->event_controller->mouse = calloc(1, sizeof(VkyMouse));
    canvas->event_controller->mouse->press_time = VKY_NEVER;
    canvas->event_controller->mouse->click_time = VKY_NEVER;

    // Create the VkyKeyboard struct.
    canvas->event_controller->keyboard = calloc(1, sizeof(VkyKeyboard));
    canvas->event_controller->keyboard->key = VKY_KEY_NONE;
    canvas->event_controller->keyboard->modifiers = 0;

    canvas->event_controller->frame_callbacks =
        calloc(VKY_MAX_EVENT_CALLBACKS, sizeof(VkyFrameCallback));
    canvas->event_controller->mock_input_callbacks =
        calloc(VKY_MAX_EVENT_CALLBACKS, sizeof(VkyFrameCallback));
}

void vky_reset_event_controller(VkyEventController* event_controller)
{
    event_controller->mouse->press_time = VKY_NEVER;
    event_controller->mouse->click_time = VKY_NEVER;
    event_controller->keyboard->key = VKY_KEY_NONE;
    event_controller->keyboard->modifiers = 0;
    event_controller->frame_callback_count = 1;
}

void vky_destroy_event_controller(VkyEventController* event_controller)
{
    log_trace("destroy event controller");
    if (event_controller == NULL)
    {
        log_debug("skipping destroy event controller");
    }
    if (event_controller->mouse != NULL)
        free(event_controller->mouse);

    if (event_controller->keyboard != NULL)
        free(event_controller->keyboard);

    if (event_controller->frame_callbacks != NULL)
        free(event_controller->frame_callbacks);

    if (event_controller->mock_input_callbacks != NULL)
        free(event_controller->mock_input_callbacks);

    free(event_controller);
}



/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

VkyMouse* vky_event_mouse(VkyCanvas* canvas) { return canvas->event_controller->mouse; }

/* Update the mouse state from a mouse position and mouse button. */
void vky_update_mouse_state(VkyMouse* mouse, vec2 pos, VkyMouseButton button)
{
    double time = vky_get_timer();

    // Update the last pos.
    glm_vec2_copy(mouse->cur_pos, mouse->last_pos);
    // Update the mouse current position, in pixels.
    mouse->cur_pos[0] = pos[0];
    mouse->cur_pos[1] = pos[1];

    // Reset click events as soon as the next loop iteration after they were raised.
    if (mouse->cur_state == VKY_MOUSE_STATE_CLICK ||
        mouse->cur_state == VKY_MOUSE_STATE_DOUBLE_CLICK)
    {
        mouse->cur_state = VKY_MOUSE_STATE_STATIC;
        mouse->button = VKY_MOUSE_BUTTON_NONE;
    }

    bool pressed = button != VKY_MOUSE_BUTTON_NONE;

    // Net distance in pixels since the last press event.
    float shift_length = 0;
    vec2 shift;

    if (pressed)
    {

        // Press event.
        if (mouse->press_time == VKY_NEVER)
        {
            glm_vec2_copy(mouse->cur_pos, mouse->press_pos);
            mouse->press_time = time;
            mouse->button = button;
        }

        // Update the distance since the last press position.
        else
        {
            glm_vec2_sub(mouse->cur_pos, mouse->press_pos, shift);
            shift_length = glm_vec2_norm(shift);
        }
    }

    // Release event.
    if (!pressed && mouse->press_time > VKY_NEVER)
    {

        // End drag.
        if (mouse->cur_state == VKY_MOUSE_STATE_DRAG)
        {
            log_trace("end drag event");
            mouse->cur_state = VKY_MOUSE_STATE_STATIC;
            mouse->button = VKY_MOUSE_BUTTON_NONE;
        }

        // Double click event.
        else if (time - mouse->click_time < VKY_MOUSE_DOUBLE_CLICK_MAX_DELAY)
        {
            // NOTE: when releasing, current button is NONE so we must use the previously set
            // button in mouse->button.
            log_trace("double click event on button %d", mouse->button);
            mouse->cur_state = VKY_MOUSE_STATE_DOUBLE_CLICK;
            mouse->click_time = time;
        }

        // Click event.
        else if (
            time - mouse->press_time < VKY_MOUSE_CLICK_MAX_DELAY &&
            shift_length < VKY_MOUSE_CLICK_MAX_SHIFT)
        {
            log_trace("click event on button %d", mouse->button);
            mouse->cur_state = VKY_MOUSE_STATE_CLICK;
            mouse->click_time = time;
        }

        else
        {
            // Reset the mouse button state.
            mouse->button = VKY_MOUSE_BUTTON_NONE;
        }
        mouse->press_time = VKY_NEVER;
    }

    // Mouse move event only if the shift length is larger than the click area.
    if (shift_length > VKY_MOUSE_CLICK_MAX_SHIFT)
    {
        // Mouse move.
        if (mouse->cur_state == VKY_MOUSE_STATE_STATIC && button != VKY_MOUSE_BUTTON_NONE)
        {
            log_trace("drag event on button %d", button);
            mouse->cur_state = VKY_MOUSE_STATE_DRAG;
            mouse->button = button;
        }
    }
}

void vky_mouse_normalize(vec2 size, vec2 center, vec2 pos)
{
    pos[0] -= center[0];
    pos[1] -= center[1];

    pos[0] *= +2 / size[0];
    pos[1] *= -2 / size[1];
}

void vky_mouse_normalize_window(VkyCanvas* canvas, vec2 pos)
{
    float width = (float)canvas->size.window_width;
    float height = (float)canvas->size.window_height;
    vec2 center = {width / 2, height / 2};
    vec2 size = {width, height};
    vky_mouse_normalize(size, center, pos);
}

void vky_mouse_normalize_viewport(VkyViewport viewport, vec2 pos)
{
    if (viewport.canvas->size.window_width == 0)
    {
        log_trace("skipping normalization as the canvas size has not been set yet");
    }
    else
    {
        pos[0] *= (viewport.canvas->size.framebuffer_width / viewport.canvas->size.window_width);
        pos[1] *= (viewport.canvas->size.framebuffer_height / viewport.canvas->size.window_height);
    }

    // viewport center in pixels
    vec2 center = {viewport.x + viewport.w / 2, viewport.y + viewport.h / 2};
    vec2 size = {viewport.w, viewport.h};
    vky_mouse_normalize(size, center, pos);
}

void vky_mouse_move_delta(VkyMouse* mouse, VkyViewport viewport, vec2 delta)
{
    vec2 delta_ = {
        mouse->cur_pos[0] - mouse->last_pos[0],
        mouse->cur_pos[1] - mouse->last_pos[1],
    };
    delta_[0] = +2 * delta_[0] / viewport.w;
    delta_[1] = -2 * delta_[1] / viewport.h;
    delta_[0] *= (viewport.canvas->size.framebuffer_width / viewport.canvas->size.window_width);
    delta_[1] *= (viewport.canvas->size.framebuffer_height / viewport.canvas->size.window_height);
    glm_vec2_copy(delta_, delta);
}

void vky_mouse_press_delta(VkyMouse* mouse, VkyViewport viewport, vec2 delta)
{
    vec2 delta_ = {
        mouse->cur_pos[0] - mouse->press_pos[0],
        mouse->cur_pos[1] - mouse->press_pos[1],
    };
    delta_[0] /= viewport.w;
    delta_[1] /= viewport.h;
    delta_[0] *= (viewport.canvas->size.framebuffer_width / viewport.canvas->size.window_width);
    delta_[1] *= (viewport.canvas->size.framebuffer_height / viewport.canvas->size.window_height);
    glm_vec2_copy(delta_, delta);
}

// in local normalized coordinates in the viewport
void vky_mouse_cur_pos(VkyMouse* mouse, VkyViewport viewport, vec2 pos)
{
    // Mouse position in window coordinates.
    glm_vec2_copy(mouse->cur_pos, pos);
    // Mouse position in viewport normalized coordinates.
    vky_mouse_normalize_viewport(viewport, pos);
}

// in local normalized coordinates in the viewport
void vky_mouse_last_pos(VkyMouse* mouse, VkyViewport viewport, vec2 pos)
{
    // Mouse position in window coordinates.
    glm_vec2_copy(mouse->last_pos, pos);
    // Mouse position in viewport normalized coordinates.
    vky_mouse_normalize_viewport(viewport, pos);
}

// in local normalized coordinates in the viewport
void vky_mouse_press_pos(VkyMouse* mouse, VkyViewport viewport, vec2 pos)
{
    // Mouse position in window coordinates.
    glm_vec2_copy(mouse->press_pos, pos);
    // Mouse position in viewport normalized coordinates.
    vky_mouse_normalize_viewport(viewport, pos);
}



/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

VkyKeyboard* vky_event_keyboard(VkyCanvas* canvas) { return canvas->event_controller->keyboard; }

bool vky_is_key_modifier(VkyKey key)
{
    return (
        key == VKY_KEY_LEFT_SHIFT || key == VKY_KEY_RIGHT_SHIFT || key == VKY_KEY_LEFT_CONTROL ||
        key == VKY_KEY_RIGHT_CONTROL || key == VKY_KEY_LEFT_ALT || key == VKY_KEY_RIGHT_ALT ||
        key == VKY_KEY_LEFT_SUPER || key == VKY_KEY_RIGHT_SUPER);
}

void vky_update_keyboard_state(VkyKeyboard* keyboard, VkyKey key, VkyKeyModifiers modifiers)
{
    double time = vky_get_timer();
    if (time - keyboard->press_time < VKY_KEY_PRESS_DELAY)
        return;
    if (key != VKY_KEY_NONE)
    {
        log_trace("key pressed %d", key);
        keyboard->key = key;
        keyboard->modifiers = modifiers;
        keyboard->press_time = time;
    }
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

void vky_add_mock_input_callback(VkyCanvas* canvas, VkyFrameCallback cb)
{
    /* Mock input callbacks may call vky_update_mouse_state() and vky_update_keyboard_state()
    to manually update the mouse and keyboard states. Useful for testing purposes. */

    uint32_t n = canvas->event_controller->mock_input_callback_count;
    ASSERT(canvas->event_controller->mock_input_callbacks != NULL);
    canvas->event_controller->mock_input_callbacks[n] = cb;
    canvas->event_controller->mock_input_callback_count++;
}

int vky_call_mock_input_callbacks(VkyEventController* event_controller)
{
    ASSERT(event_controller != NULL);
    for (uint32_t i = 0; i < event_controller->mock_input_callback_count; i++)
    {
        ASSERT(event_controller->mock_input_callbacks[i] != NULL);
        event_controller->mock_input_callbacks[i](event_controller->canvas);
    }
    return (int)event_controller->mock_input_callback_count;
}

void vky_add_frame_callback(VkyCanvas* canvas, VkyFrameCallback cb)
{
    uint32_t n = canvas->event_controller->frame_callback_count;
    ASSERT(canvas->event_controller->frame_callbacks != NULL);
    canvas->event_controller->frame_callbacks[n] = cb;
    canvas->event_controller->frame_callback_count++;
}

void vky_call_frame_callbacks(VkyEventController* event_controller)
{
    ASSERT(event_controller != NULL);
    for (uint32_t i = 0; i < event_controller->frame_callback_count; i++)
    {
        ASSERT(event_controller->frame_callbacks[i] != NULL);
        event_controller->frame_callbacks[i](event_controller->canvas);
    }
}



/*************************************************************************************************/
/*  Data upload                                                                                  */
/*************************************************************************************************/

static void _upload_visual_data(VkyVisual* visual, VkyPanel* panel)
{
    if (visual->need_data_upload)
    {
        vky_visual_data_upload(visual, panel);
        visual->need_data_upload = false;
    }
}

// Called at every frame.
void vky_upload_pending_data(VkyCanvas* canvas)
{
    if (canvas->scene != NULL && canvas->scene->grid != NULL && canvas->need_data_upload)
    {
        log_debug("upload pending data");
        VkyVisualPanel* vp = NULL;
        for (uint32_t i = 0; i < canvas->scene->grid->visual_panel_count; i++)
        {
            vp = &canvas->scene->grid->visual_panels[i];

            // Upload the visual's data if any.
            _upload_visual_data(vp->visual, vp->panel);

            // Upload the visual's children data if any.
            for (uint32_t j = 0; j < vp->visual->children_count; j++)
                _upload_visual_data(vp->visual->children[j], vp->panel);
        }
        canvas->need_data_upload = false;
    }
}



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

// This function updates the VkyMouse and VkyKeyboard structures:
//
// 1. If the canvas is handled by a pull-based backend, then Visky is in charge of the
//    (explicit) event loop. The vky_update_event_states() function is called at every
//    iteration. It updates the mouse and keyboard states. Then, the vky_next_frame() will
//    process these state updates.
//
// 2. If the canvas is handled by a push-based backend, then the backend is in charge
//    of the (implicit) event loop. The backend proposes to register callbacks that are
//    automatically called whenever a mouse/keyboard event occurs. In these callbacks,
//    Visky/the application calls vky_update_mouse_state() and vky_update_keyboard_state()
//    manually to update the VkyMouse and VkyKeyboard structures. Then, the vky_next_frame() will
//    process these state updates. The vky_update_event_states() does nothing in this case.
//
//  This function may be blocking or non-blocking. If blocking, it waits until an event occurs.

void vky_update_event_states(VkyEventController* event_controller)
{
    if (event_controller == NULL)
    {
        log_debug("skipping event controller update");
        return;
    }
    if (!event_controller->do_process_input)
    {
        return;
    }
    ASSERT(event_controller->mouse != NULL);
    ASSERT(event_controller->keyboard != NULL);

    VkyMouse* mouse = event_controller->mouse;
    VkyKeyboard* keyboard = event_controller->keyboard;
    if (event_controller->canvas->app == NULL)
    {
        // log_warn("canvas app is not set");
        return;
    }
    VkyBackendType backend = event_controller->canvas->app->backend;

    // Mock input.
    if (vky_call_mock_input_callbacks(event_controller) > 0)
        // Do not process normal interactive events when using mock input in order to avoid
        // conflicts.
        return;

    // Fill the pos and button variables, depending on the backend.
    vec2 pos = {0, 0};
    VkyMouseButton button = {0};
    VkyKey key = {0};
    uint32_t modifiers = 0;
    switch (backend)
    {
    case VKY_BACKEND_GLFW:

        // Get the mouse button.
        button = vky_glfw_get_mouse_button((GLFWwindow*)event_controller->canvas->window);

        // Get the mouse position.
        vky_glfw_get_mouse_pos((GLFWwindow*)event_controller->canvas->window, pos);

        // Get the keyboard keys.
        vky_glfw_get_keyboard((GLFWwindow*)event_controller->canvas->window, &key, &modifiers);

        break;

        // case VKY_BACKEND_VNC:;
        //     VkyBackendVNCParams* params =
        //         (VkyBackendVNCParams*)event_controller->canvas->app->backend_params;
        //     glm_vec2_copy(params->mouse_pos, pos);
        //     button = params->mouse_button;
        //     break;

    default:
        break;
    }

    // Update the event states.
    vky_update_mouse_state(mouse, pos, button);
    vky_update_keyboard_state(keyboard, key, modifiers);
}

void vky_finish_event_states(VkyEventController* event_controller)
{
    VkyMouse* mouse = event_controller->mouse;
    VkyKeyboard* keyboard = event_controller->keyboard;
    // Reset the wheel state.
    if (mouse->cur_state == VKY_MOUSE_STATE_WHEEL)
    {
        mouse->cur_state = VKY_MOUSE_STATE_STATIC;
        mouse->button = VKY_MOUSE_BUTTON_NONE;
    }
    // Make sure the keyboard event is only emitted during a single frame.
    if (keyboard->key != VKY_KEY_NONE)
    {
        keyboard->key = VKY_KEY_NONE;
        keyboard->modifiers = VKY_KEY_MODIFIER_NONE;
    }
}

void vky_next_frame(VkyCanvas* canvas)
{
    // Upload the pending data for the visuals.
    vky_upload_pending_data(canvas);

    VkyEventController* event_controller = canvas->event_controller;

    // Update the mouse state following mouse events raised by glfw.
    vky_update_event_states(canvas->event_controller);

    // Call the update callbacks.
    vky_call_frame_callbacks(event_controller);

    // Require for some vents like mouse wheel which should be raised in a single frame.
    vky_finish_event_states(canvas->event_controller);
}
