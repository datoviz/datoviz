#include "../include/visky/interact.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_NEVER                        -1000000
#define VKL_MOUSE_CLICK_MAX_DELAY        .5
#define VKL_MOUSE_CLICK_MAX_SHIFT        10
#define VKL_MOUSE_DOUBLE_CLICK_MAX_DELAY .2
#define VKL_KEY_PRESS_DELAY              .05
#define VKL_PANZOOM_MOUSE_WHEEL_FACTOR   .2
#define VKL_PANZOOM_MIN_ZOOM             1e-6
#define VKL_PANZOOM_MAX_ZOOM             1e+6


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

    // Reset wheel event.
    if (mouse->cur_state == VKL_MOUSE_STATE_WHEEL)
        mouse->cur_state = VKL_MOUSE_STATE_INACTIVE;

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
            }

            // Double click event.
            else if (time - mouse->click_time < VKL_MOUSE_DOUBLE_CLICK_MAX_DELAY)
            {
                // NOTE: when releasing, current button is NONE so we must use the previously set
                // button in mouse->button.
                log_trace("double click event on button %d", mouse->button);
                mouse->cur_state = VKL_MOUSE_STATE_DOUBLE_CLICK;
                mouse->click_time = time;
            }

            // Click event.
            else if (
                time - mouse->press_time < VKL_MOUSE_CLICK_MAX_DELAY &&
                mouse->shift_length < VKL_MOUSE_CLICK_MAX_SHIFT)
            {
                log_trace("click event on button %d", mouse->button);
                mouse->cur_state = VKL_MOUSE_STATE_CLICK;
                mouse->click_time = time;
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

        // Mouse move event only if the shift length is larger than the click area.
        if (mouse->shift_length > VKL_MOUSE_CLICK_MAX_SHIFT)
        {
            // Mouse move.
            if (mouse->cur_state == VKL_MOUSE_STATE_INACTIVE &&
                mouse->button != VKL_MOUSE_BUTTON_NONE)
            {
                log_trace("drag event on button %d", mouse->button);
                mouse->cur_state = VKL_MOUSE_STATE_DRAG;
            }
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



static void _normalize(vec2 pos_out, vec2 pos_in, uvec2 size)
{
    pos_out[0] = -1 + 2 * (pos_in[0] / size[0]);
    pos_out[1] = +1 - 2 * (pos_in[1] / size[1]);
}

// From pixel coordinates (top left origin) to local coordinates (center origin)
void vkl_mouse_local(
    VklMouse* mouse, VklMouseLocal* mouse_local, VklCanvas* canvas, VklViewport viewport)
{

    // Window size in screen coordinates.
    uvec2 size_screen = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size_screen);
    ASSERT(size_screen[0] > 0);
    ASSERT(size_screen[1] > 0);

    _normalize(mouse_local->cur_pos, mouse->cur_pos, size_screen);
    _normalize(mouse_local->last_pos, mouse->last_pos, size_screen);
    _normalize(mouse_local->press_pos, mouse->press_pos, size_screen);
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



static bool _is_key_modifier(VklKeyCode key)
{
    return (
        key == VKL_KEY_LEFT_SHIFT || key == VKL_KEY_RIGHT_SHIFT || key == VKL_KEY_LEFT_CONTROL ||
        key == VKL_KEY_RIGHT_CONTROL || key == VKL_KEY_LEFT_ALT || key == VKL_KEY_RIGHT_ALT ||
        key == VKL_KEY_LEFT_SUPER || key == VKL_KEY_RIGHT_SUPER);
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
/*  Panzoom                                                                                      */
/*************************************************************************************************/

static VklPanzoom _panzoom()
{
    VklPanzoom p = {0};
    p.camera_pos[2] = +2;
    p.zoom[0] = 1;
    p.zoom[1] = 1;
    return p;
}

static void _panzoom_copy_prev_state(VklPanzoom* panzoom)
{
    ASSERT(panzoom != NULL);
    glm_vec2_copy(panzoom->camera_pos, panzoom->last_camera_pos);
    glm_vec2_copy(panzoom->zoom, panzoom->last_zoom);
}

static void _panzoom_reset(VklPanzoom* panzoom)
{
    ASSERT(panzoom != NULL);
    panzoom->camera_pos[0] = 0;
    panzoom->camera_pos[1] = 0;
    panzoom->last_camera_pos[0] = 0;
    panzoom->last_camera_pos[1] = 0;
    panzoom->zoom[0] = 1;
    panzoom->zoom[1] = 1;
    panzoom->last_zoom[0] = 0;
    panzoom->last_zoom[1] = 0;
}

static void _panzoom_pan(VklPanzoom* panzoom, vec2 delta)
{
    ASSERT(panzoom != NULL);
    panzoom->camera_pos[0] = panzoom->last_camera_pos[0] - delta[0] / panzoom->zoom[0];
    panzoom->camera_pos[1] = panzoom->last_camera_pos[1] - delta[1] / panzoom->zoom[1];
}

static void _panzoom_zoom(VklPanzoom* panzoom, vec2 delta, vec2 center)
{
    ASSERT(panzoom != NULL);
    vec2 pan, zoom_prev, zoom_new;

    // Update the zoom.
    delta[0] = CLIP(delta[0], -10, +10);
    delta[1] = CLIP(delta[1], -10, +10);
    glm_vec2_mul(panzoom->last_zoom, (vec2){exp(delta[0]), exp(delta[1])}, zoom_new);

    // Clip zoom x.
    double zx = zoom_new[0];
    if (zx <= VKL_PANZOOM_MIN_ZOOM || zx >= VKL_PANZOOM_MAX_ZOOM)
    {
        zoom_new[0] = CLIP(zx, VKL_PANZOOM_MIN_ZOOM, VKL_PANZOOM_MAX_ZOOM);
        panzoom->lim_reached[0] = true;
    }
    // Clip zoom y.
    double zy = zoom_new[1];
    if (zy <= VKL_PANZOOM_MIN_ZOOM || zy >= VKL_PANZOOM_MAX_ZOOM)
    {
        zoom_new[1] = CLIP(zy, VKL_PANZOOM_MIN_ZOOM, VKL_PANZOOM_MAX_ZOOM);
        panzoom->lim_reached[1] = true;
    }

    // Update zoom.
    glm_vec2_copy(panzoom->zoom, zoom_prev);
    if (!panzoom->lim_reached[0])
        panzoom->zoom[0] = zoom_new[0];
    if (!panzoom->lim_reached[1])
        panzoom->zoom[1] = zoom_new[1];

    // Update pan.
    pan[0] = -center[0] * (1.0f / zoom_prev[0] - 1.0f / zoom_new[0]) * zoom_new[0];
    pan[1] = -center[1] * (1.0f / zoom_prev[1] - 1.0f / zoom_new[1]) * zoom_new[1];

    if (!panzoom->lim_reached[0])
        panzoom->camera_pos[0] -= pan[0] / panzoom->zoom[0];
    if (!panzoom->lim_reached[1])
        panzoom->camera_pos[1] -= pan[1] / panzoom->zoom[1];
}

static void _panzoom_update_mvp(VklPanzoom* panzoom, VklMVP* mvp)
{
    ASSERT(panzoom != NULL);
    // View matrix (depends on the pan).
    {
        vec3 center;
        glm_vec3_copy(panzoom->camera_pos, center);
        center[2] = 0.0f; // only the z coord changes between panel and center.
        vec3 lookup = {0, 1, 0};
        glm_lookat(panzoom->camera_pos, center, lookup, mvp->view);
    }
    // Proj matrix (depends on the zoom).
    {
        float zx = panzoom->zoom[0];
        float zy = panzoom->zoom[1];
        glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f, mvp->proj);
    }
}

static void _panzoom_callback(
    VklInteract* interact, VklViewport viewport, VklMouse* mouse, VklKeyboard* keyboard)
{
    ASSERT(interact != NULL);
    ASSERT(
        interact->type == VKL_INTERACT_PANZOOM ||
        interact->type == VKL_INTERACT_PANZOOM_FIXED_ASPECT);
    VklCanvas* canvas = interact->canvas;
    VklPanzoom* panzoom = &interact->u.p;
    bool update = false;

    // Update the last camera/zoom variables.
    if (mouse->prev_state == VKL_MOUSE_STATE_INACTIVE)
        _panzoom_copy_prev_state(panzoom);

    float wheel_factor = VKL_PANZOOM_MOUSE_WHEEL_FACTOR;
    vec2 delta = {0};

    // Window size.
    uvec2 size_w, size_b;
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size_w);
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size_b);

#if OS_MACOS
    // HACK: touchpad wheel too sensitive on macOS
    wheel_factor *= -.1;
#endif

    // Pan.
    if (mouse->cur_state == VKL_MOUSE_STATE_DRAG && mouse->button == VKL_MOUSE_BUTTON_LEFT)
    {
        // TODO
        // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (vkl_panel_from_mouse(scene, mouse->press_pos) != panel)
        //     return;
        // panel->status = VKL_PANEL_STATUS_ACTIVE;

        glm_vec2_sub(interact->mouse_local.cur_pos, interact->mouse_local.press_pos, delta);
        _panzoom_pan(panzoom, delta);
        update = true;
    } // end pan

    // Zoom.
    if ((mouse->cur_state == VKL_MOUSE_STATE_DRAG && mouse->button == VKL_MOUSE_BUTTON_RIGHT) ||
        mouse->cur_state == VKL_MOUSE_STATE_WHEEL)
    {
        vec2 center = {0};

        // Right drag.
        if (mouse->cur_state == VKL_MOUSE_STATE_DRAG && mouse->button == VKL_MOUSE_BUTTON_RIGHT)
        {

            // TODO
            // Restrict the panzoom updates to cases when the mouse press position was in the
            // panel.
            // if (vkl_panel_from_mouse(scene, mouse->press_pos) != panel)
            //     return;
            // panel->status = VKL_PANEL_STATUS_ACTIVE;

            // Get the center position: mouse press position.
            glm_vec2_copy(interact->mouse_local.press_pos, center);
            glm_vec2_sub(interact->mouse_local.cur_pos, interact->mouse_local.press_pos, delta);

            delta[0] *= .002 * size_b[0];
            delta[1] *= .002 * size_b[1];
        }
        // Mouse wheel.
        else
        {
            // TODO
            // Restrict the panzoom updates to cases when the mouse press position was in the
            // panel.
            // if (vkl_panel_from_mouse(scene, mouse->cur_pos) != panel)
            //     return;
            // panel->status = VKL_PANEL_STATUS_ACTIVE;

            glm_vec2_copy(interact->mouse_local.cur_pos, center);
            glm_vec2_copy(panzoom->zoom, panzoom->last_zoom);

            delta[0] = delta[1] = mouse->wheel_delta[1] * wheel_factor;
        }

        // Fixed aspect ratio.
        if (panzoom->fixed_aspect)
        {
            delta[0] = delta[1] = .5 * (delta[0] + delta[1]);
        }

        _panzoom_zoom(panzoom, delta, center);
        update = true;
    } // end zoom

    // Reset on double-click.
    if (mouse->cur_state == VKL_MOUSE_STATE_DOUBLE_CLICK)
    {
        // TODO
        // // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (vkl_panel_from_mouse(scene, mouse->cur_pos) != panel)
        //     return;
        // panel->status = VKL_PANEL_STATUS_RESET;
        _panzoom_reset(panzoom);
        update = true;
    }

    if (mouse->cur_state == VKL_MOUSE_STATE_INACTIVE)
    {
        // Reset the last camera/zoom variables.
        glm_vec2_zero(panzoom->last_camera_pos);
        glm_vec2_zero(panzoom->last_zoom);

        // TODO
        //     panel->status = VKL_PANEL_STATUS_NONE;
    }

    if (update)
        _panzoom_update_mvp(panzoom, &interact->mvp);
    interact->to_update = update;
}



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

static void _camera_reset(VklCamera* camera)
{
    ASSERT(camera != NULL);
    glm_vec3_copy((vec3){0, 0, 3}, camera->eye);
    glm_vec3_copy(camera->eye, camera->target);
    glm_vec3_copy((vec3){0, 0, -1}, camera->forward);
    glm_vec3_copy((vec3){0, 1, 0}, camera->up);
}

static VklCamera _camera(VklInteractType type)
{
    VklCamera c = {0};
    _camera_reset(&c);
    return c;
}

static void _camera_update_mvp(VklCamera* camera, VklMVP* mvp)
{
    ASSERT(camera != NULL);
    vec3 center;
    glm_vec3_add(camera->eye, camera->forward, center);
    glm_lookat(camera->eye, center, camera->up, mvp->view);
    float ratio = 1; // TODO: viewport.w / viewport.h;
    glm_perspective(GLM_PI_4, ratio, -1, 1, mvp->proj);
}

static void _camera_callback(
    VklInteract* interact, VklViewport viewport, VklMouse* mouse, VklKeyboard* keyboard)
{
    ASSERT(interact != NULL);
    VklCamera* camera = &interact->u.c;
    VklMouseLocal* mouse_local = &interact->mouse_local;
    bool is_fly = interact->type == VKL_INTERACT_FLY;

    const float dl = .075;
    const float max_pitch = .99;

    // Variables for the look-around camera with the mouse.
    vec3 yaw_axis, pitch_axis;
    glm_vec3_zero(yaw_axis);
    glm_vec3_zero(pitch_axis);
    yaw_axis[1] = 1;
    mat4 rot;
    float ymin = -10;
    vec3 advance = {0};

    switch (interact->type)
    {

    case VKL_INTERACT_FPS:
    case VKL_INTERACT_FLY:
        if (mouse->cur_state == VKL_MOUSE_STATE_DRAG)
        {
            // Change the camera orientation with the mouse.
            camera->speed = 1.0f;
            float incrx = -(mouse_local->cur_pos[0] - mouse_local->last_pos[0]) * camera->speed;
            float incry = -(mouse_local->cur_pos[1] - mouse_local->last_pos[1]) * camera->speed;

            glm_rotate_make(rot, incrx, yaw_axis);
            glm_mat4_mulv3(rot, camera->forward, 1, camera->forward);

            glm_vec3_crossn(camera->up, camera->forward, pitch_axis);
            if ((camera->forward[1] > max_pitch && incry > 0) ||
                (camera->forward[1] < -max_pitch && incry < 0))
                incry = 0;
            glm_rotate_make(rot, incry, pitch_axis);
            glm_mat4_mulv3(rot, camera->forward, 1, camera->forward);
        }

        // Change the camera elevation with the mouse wheel.
        if (mouse->cur_state == VKL_MOUSE_STATE_WHEEL)
        {
            camera->target[1] -= .1 * mouse->wheel_delta[1];
        }

        // Arrow keys navigation.
        if (keyboard->cur_state == VKL_KEYBOARD_STATE_ACTIVE)
        {
            if (keyboard->key_code == VKL_KEY_UP)
            {
                advance[0] = +camera->forward[0];
                advance[1] = +camera->forward[1];
                advance[2] = +camera->forward[2];
            }
            else if (keyboard->key_code == VKL_KEY_DOWN)
            {
                advance[0] = -camera->forward[0];
                advance[1] = -camera->forward[1];
                advance[2] = -camera->forward[2];
            }
            else if (keyboard->key_code == VKL_KEY_RIGHT)
            {
                advance[0] = -camera->forward[2];
                advance[2] = +camera->forward[0];
            }
            else if (keyboard->key_code == VKL_KEY_LEFT)
            {
                advance[0] = +camera->forward[2];
                advance[2] = -camera->forward[0];
            }
        }

        break;

    default:
        break;
    }

    // Reset the camera on double click.
    if (mouse->cur_state == VKL_MOUSE_STATE_DOUBLE_CLICK)
    {
        _camera_reset(camera);
        return;
        // panel->status = VKL_PANEL_STATUS_RESET;
    }

    // Add the advance vector to the target position.
    if (!is_fly)
        advance[1] = 0;
    if (glm_vec3_norm(advance) > 1e-3)
    {
        glm_vec3_normalize(advance);
        glm_vec3_scale(advance, dl, advance);
        glm_vec3_add(camera->target, advance, camera->target);
    }

    // Smooth move.
    vec3 u = {0};
    glm_vec3_sub(camera->target, camera->eye, u);
    if (glm_vec3_norm(u) > 1e-3)
    {
        glm_vec3_scale(u, .25, u);
        glm_vec3_add(camera->eye, u, camera->eye);

        // Prevent going below y=0 plane.
        if (!is_fly)
            ymin = 0;
        camera->eye[1] = CLIP(camera->eye[1], ymin, 10);
        camera->target[1] = CLIP(camera->target[1], ymin, 10);
    }

    _camera_update_mvp(camera, &interact->mvp);
    interact->to_update = true;
}



/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

// adapted from https://github.com/Twinklebear/arcball-cpp/blob/master/arcball_panel.cpp

static void _arcball_reset(VklArcball* arcball)
{
    ASSERT(arcball != NULL);

    vec3 eye, center, up, dir, x_axis, y_axis, z_axis;
    glm_vec3_copy(arcball->camera.eye, eye);
    glm_vec3_copy((vec3){0, 0, 0}, center);
    glm_vec3_copy((vec3){0, +1, 0}, up);

    glm_vec3_sub(center, eye, dir);
    glm_vec3_copy(dir, z_axis);
    glm_vec3_normalize(z_axis);
    glm_vec3_normalize(up);

    glm_vec3_cross(z_axis, up, x_axis);
    glm_vec3_normalize(x_axis);

    glm_vec3_cross(x_axis, z_axis, y_axis);
    glm_vec3_normalize(y_axis);

    glm_vec3_cross(z_axis, y_axis, x_axis);
    glm_vec3_normalize(x_axis);

    glm_translate_make(arcball->center_translation, center);
    glm_mat4_inv(arcball->center_translation, arcball->center_translation);

    glm_translate_make(arcball->translation, (vec3){0, 0, -glm_vec3_norm(dir)});

    mat3 m;
    glm_vec3_copy((vec3){-x_axis[0], -x_axis[1], -x_axis[2]}, m[0]);
    glm_vec3_copy((vec3){+y_axis[0], +y_axis[1], +y_axis[2]}, m[1]);
    glm_vec3_copy((vec3){+z_axis[0], +z_axis[1], +z_axis[2]}, m[2]);

    glm_mat3_transpose(m);
    glm_mat3_quat(m, arcball->rotation);
    glm_quat_normalize(arcball->rotation);

    // glm_mat4_identity(arcball->mat_user);
}

static VklArcball _arcball()
{
    VklArcball arcball = {0};
    _arcball_reset(&arcball);
    return arcball;
}

static void _screen_to_arcball(vec2 p, versor q)
{
    float dist = glm_vec2_dot(p, p);
    // If we're on/in the sphere return the point on it
    if (dist <= 1.f)
    {
        glm_vec4_copy((vec4){p[0], p[1], sqrt(1 - dist), 0}, q);
    }
    else
    {
        // otherwise we project the point onto the sphere
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], p[1], 0, 0}, q);
    }
}

static void _arcball_rotate(VklArcball* arcball, vec2 cur_pos, vec2 last_pos)
{
    ASSERT(arcball != NULL);

    // // NOTE: need to invert the mouse normalized coordinates if the standard 3D view matrix
    // is
    // // also applied.
    // if (arcball->which_matrix == VKL_MVP_MODEL)
    // {
    //     cur_pos[0] *= -1;
    //     cur_pos[1] *= -1;
    //     last_pos[0] *= -1;
    //     last_pos[1] *= -1;
    // }

    versor mouse_cur_ball = {0}, mouse_prev_ball = {0};
    _screen_to_arcball(cur_pos, mouse_cur_ball);
    _screen_to_arcball(last_pos, mouse_prev_ball);

    glm_quat_mul(mouse_prev_ball, arcball->rotation, arcball->rotation);
    glm_quat_mul(mouse_cur_ball, arcball->rotation, arcball->rotation);
}

static void _arcball_zoom(VklArcball* arcball, vec3 motion)
{
    ASSERT(arcball != NULL);

    mat4 tr;
    glm_translate_make(tr, motion);
    glm_mat4_mul(tr, arcball->translation, arcball->translation);
    // Zoom bound.
    // float zoom_max = 1;
    // if (arcball->translation[3][2] > -zoom_max)
    //     arcball->translation[3][2] = -zoom_max;
}

static void _arcball_pan(VklArcball* arcball, vec2 delta)
{
    vec4 motion = {delta[0], delta[1], 0, 0};
    // Find the panning amount in the world space
    mat4 inv_panel;
    glm_mat4_inv(arcball->mat, inv_panel);
    glm_mat4_mulv(inv_panel, motion, motion);
    mat4 tr;
    glm_translate_make(tr, motion);
    glm_mat4_mul(tr, arcball->center_translation, arcball->center_translation);

    // Update the view matrix.
    glm_quat_mat4(arcball->rotation, arcball->mat);
    glm_mat4_mul(arcball->mat, arcball->center_translation, arcball->mat);
    glm_mat4_mul(arcball->translation, arcball->mat, arcball->mat);
}

static void _arcball_update_mvp(VklArcball* arcball, VklMVP* mvp)
{
    ASSERT(arcball != NULL);
    glm_mat4_copy(arcball->mat, mvp->model);

    // // NOTE: need to invert the mouse normalized coordinates if the standard 3D view matrix
    // is
    // // also applied.
    // if (arcball->which_matrix == VKL_MVP_MODEL)
    // {
    //     delta[0] *= -1;
    //     delta[1] *= -1;
    // }

    vec3 center = {0};
    vec3 eye = {0, 0, 2.5};
    vec3 up = {0, 1, 0};
    glm_lookat(eye, center, up, mvp->view);
    float ratio = 1; // TODO: viewport.w / viewport.h;
    glm_perspective(GLM_PI_4, ratio, -1, 1, mvp->proj);

    // mvp->view
}

static void _arcball_callback(
    VklInteract* interact, VklViewport viewport, VklMouse* mouse, VklKeyboard* keyboard)
{
    ASSERT(interact != NULL);
    ASSERT(interact->type == VKL_INTERACT_ARCBALL);
    VklArcball* arcball = &interact->u.a;
    bool update = false;

    // Rotate.
    if (mouse->cur_state == VKL_MOUSE_STATE_DRAG && mouse->button == VKL_MOUSE_BUTTON_LEFT)
    {
        // // TODO
        // // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (vkl_panel_from_mouse(scene, mouse->press_pos) != panel)
        //     return;
        // panel->status = VKL_PANEL_STATUS_ACTIVE;
        _arcball_rotate(arcball, interact->mouse_local.cur_pos, interact->mouse_local.last_pos);
        update = true;
    }

    // Zoom.
    if (mouse->cur_state == VKL_MOUSE_STATE_WHEEL)
    {
        // // TODO
        // // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (vkl_panel_from_mouse(scene, mouse->cur_pos) != panel)
        //     return;
        // panel->status = VKL_PANEL_STATUS_ACTIVE;
        vec3 motion = {0, 0, +.2 * mouse->wheel_delta[1]};
        _arcball_zoom(arcball, motion);
        update = true;
    }

    // Reset with double-click.
    if (mouse->cur_state == VKL_MOUSE_STATE_DOUBLE_CLICK)
    {
        _arcball_reset(arcball);
        // panel->status = VKL_PANEL_STATUS_RESET;
        update = true;
    }

    // Compute the View matrix.
    glm_quat_mat4(arcball->rotation, arcball->mat);
    glm_mat4_mul(arcball->mat, arcball->center_translation, arcball->mat);
    glm_mat4_mul(arcball->translation, arcball->mat, arcball->mat);

    // Pan.
    if (mouse->cur_state == VKL_MOUSE_STATE_DRAG && mouse->button == VKL_MOUSE_BUTTON_RIGHT)
    {
        // // TODO
        // if (vkl_panel_from_mouse(scene, mouse->press_pos) != panel)
        //     return;
        // panel->status = VKL_PANEL_STATUS_ACTIVE;

        vec2 delta;
        glm_vec2_sub(interact->mouse_local.cur_pos, interact->mouse_local.last_pos, delta);
        delta[0] *= .5;
        delta[1] *= .5;
        _arcball_pan(arcball, delta);
        update = true;
    }

    // Make a copy of the transformation matrix, if other controllers or the user want to modify
    // the model/view matrix.
    // glm_mat4_copy(arcball_mat, arcball->mat_arcball);

    // Take the user matrix into account.
    // glm_mat4_mul(arcball->mat, arcball->mat_user, arcball->mat);

    if (update)
        _arcball_update_mvp(arcball, &interact->mvp);
    interact->to_update = update;

    // if (mouse->cur_state == VKL_MOUSE_STATE_INACTIVE)
    // {
    //     panel->status = VKL_PANEL_STATUS_NONE;
    // }
}



/*************************************************************************************************/
/*  Interact                                                                                     */
/*************************************************************************************************/

VklInteract vkl_interact(VklCanvas* canvas, void* user_data)
{
    ASSERT(canvas != NULL);
    VklInteract interact = {0};
    interact.canvas = canvas;
    glm_mat4_identity(interact.mvp.model);
    glm_mat4_identity(interact.mvp.view);
    glm_mat4_identity(interact.mvp.proj);
    interact.user_data = user_data;
    return interact;
}



void vkl_interact_callback(VklInteract* interact, VklInteractCallback callback)
{
    ASSERT(interact != NULL);
    interact->callback = callback;
}



VklInteract vkl_interact_builtin(VklCanvas* canvas, VklInteractType type)
{
    VklInteract interact = vkl_interact(canvas, NULL);
    interact.type = type;
    switch (type)
    {
    case VKL_INTERACT_PANZOOM:
    case VKL_INTERACT_PANZOOM_FIXED_ASPECT:
        interact.u.p = _panzoom();
        if (type == VKL_INTERACT_PANZOOM_FIXED_ASPECT)
            interact.u.p.fixed_aspect = true;
        interact.callback = _panzoom_callback;
        break;

    case VKL_INTERACT_ARCBALL:
        interact.u.a = _arcball();
        interact.callback = _arcball_callback;
        break;

    case VKL_INTERACT_FLY:
    case VKL_INTERACT_FPS:
    case VKL_INTERACT_TURNTABLE:
        interact.u.c = _camera(type);
        interact.callback = _camera_callback;
        break;

    default:
        break;
    }
    return interact;
}



void vkl_interact_update(
    VklInteract* interact, VklViewport viewport, VklMouse* mouse, VklKeyboard* keyboard)
{
    ASSERT(interact != NULL);

    // Update the local coordinates of the mouse before calling the interact callback.
    vkl_mouse_local(mouse, &interact->mouse_local, interact->canvas, viewport);

    if (interact->callback != NULL)
        interact->callback(interact, viewport, mouse, keyboard);
}
