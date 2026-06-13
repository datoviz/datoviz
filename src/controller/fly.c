/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Fly camera controller                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_controller.h"
#include "_log.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/controller/fly.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FLY_DEFAULT_WIDTH  800.0f
#define DVZ_FLY_DEFAULT_HEIGHT 600.0f
#define DVZ_FLY_DEFAULT_SPEED  0.70f
#define DVZ_FLY_DEFAULT_FAST   5.0f
#define DVZ_FLY_DEFAULT_SLOW   0.2f
#define DVZ_FLY_DEFAULT_LOOK   0.7f
#define DVZ_FLY_DEFAULT_WHEEL  0.04f
#define DVZ_FLY_VERTICAL_SPEED 0.75f
#define DVZ_FLY_PITCH_EPS     0.001f
#define DVZ_FLY_PIVOT_MARKER_S 1.0
#define DVZ_FLY_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a float to a closed range.
 *
 * @param value input value
 * @param min_value minimum value
 * @param max_value maximum value
 * @return clamped value
 */
static float _clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}



/**
 * Return whether a vector has usable length.
 *
 * @param v vector
 * @return whether the vector length is non-zero
 */
static bool _vec3_valid(vec3 v)
{
    return glm_vec3_norm(v) > 0.0f;
}



static bool _fly_desc_validate(const DvzFlyDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzFlyDesc, DVZ_FLY_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzFlyDesc ABI prologue");
        return false;
    }
    return true;
}



/**
 * Clamp a pitch angle away from singular poles.
 *
 * @param pitch pitch angle
 * @return clamped pitch
 */
static float _fly_clamp_pitch(float pitch)
{
    return _clampf(pitch, -GLM_PI_2f + DVZ_FLY_PITCH_EPS, +GLM_PI_2f - DVZ_FLY_PITCH_EPS);
}



/**
 * Normalize an angle to [-pi, pi].
 *
 * @param angle input angle
 * @return normalized angle
 */
static float _fly_wrap_angle(float angle)
{
    while (angle > GLM_PIf)
        angle -= 2.0f * GLM_PIf;
    while (angle < -GLM_PIf)
        angle += 2.0f * GLM_PIf;
    return angle;
}



/**
 * Compute the horizontal yaw basis for the configured world-up axis.
 *
 * @param world_up normalized world-up vector
 * @param out_base_x yaw-zero direction
 * @param out_base_y positive-yaw direction in the horizontal plane
 */
static void _fly_yaw_basis(vec3 world_up, vec3 out_base_x, vec3 out_base_y)
{
    ANN(out_base_x);
    ANN(out_base_y);

    vec3 up = {0};
    if (_vec3_valid(world_up))
        glm_vec3_normalize_to(world_up, up);
    else
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);

    vec3 ref = {0.0f, 0.0f, 1.0f};
    if (fabsf(glm_vec3_dot(up, ref)) > 0.92f)
        glm_vec3_copy((vec3){0.0f, -1.0f, 0.0f}, ref);

    glm_vec3_cross(up, ref, out_base_x);
    if (!_vec3_valid(out_base_x))
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, out_base_x);
    else
        glm_vec3_normalize(out_base_x);

    glm_vec3_cross(out_base_x, up, out_base_y);
    if (!_vec3_valid(out_base_y))
        glm_vec3_copy(ref, out_base_y);
    else
        glm_vec3_normalize(out_base_y);
}



/**
 * Compute yaw and pitch from a direction vector.
 *
 * @param dir input direction
 * @param world_up world-up vector
 * @param out_yaw output yaw angle
 * @param out_pitch output pitch angle
 * @return whether the direction was usable
 */
static bool _fly_angles_from_dir(vec3 dir, vec3 world_up, float* out_yaw, float* out_pitch)
{
    ANN(out_yaw);
    ANN(out_pitch);
    if (!_vec3_valid(dir))
        return false;
    vec3 n = {0};
    glm_vec3_normalize_to(dir, n);

    vec3 up = {0};
    if (_vec3_valid(world_up))
        glm_vec3_normalize_to(world_up, up);
    else
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);

    const float vertical = _clampf(glm_vec3_dot(n, up), -1.0f, +1.0f);
    *out_pitch = _fly_clamp_pitch(asinf(vertical));

    vec3 horizontal = {0};
    vec3 vertical_part = {0};
    glm_vec3_scale(up, vertical, vertical_part);
    glm_vec3_sub(n, vertical_part, horizontal);
    if (!_vec3_valid(horizontal))
    {
        *out_yaw = 0.0f;
        return true;
    }
    glm_vec3_normalize(horizontal);

    vec3 base_x = {0}, base_y = {0};
    _fly_yaw_basis(up, base_x, base_y);
    *out_yaw = atan2f(glm_vec3_dot(horizontal, base_y), glm_vec3_dot(horizontal, base_x));
    return true;
}



/**
 * Compute front, right, and up vectors from fly orientation.
 *
 * @param fly the fly controller
 * @param out_front output front vector
 * @param out_right output right vector
 * @param out_up output up vector
 */
static void _fly_vectors(const DvzFly* fly, vec3 out_front, vec3 out_right, vec3 out_up)
{
    ANN(fly);
    ANN(out_front);
    ANN(out_right);
    ANN(out_up);

    vec3 world_up = {fly->world_up[0], fly->world_up[1], fly->world_up[2]};
    if (!_vec3_valid(world_up))
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, world_up);
    glm_vec3_normalize(world_up);

    vec3 base_x = {0}, base_y = {0};
    _fly_yaw_basis(world_up, base_x, base_y);
    vec3 front = {0}, horizontal = {0}, horizontal_y = {0}, vertical = {0};
    glm_vec3_scale(base_x, cosf(fly->yaw), horizontal);
    glm_vec3_scale(base_y, sinf(fly->yaw), horizontal_y);
    glm_vec3_add(horizontal, horizontal_y, horizontal);
    glm_vec3_scale(horizontal, cosf(fly->pitch), front);
    glm_vec3_scale(world_up, sinf(fly->pitch), vertical);
    glm_vec3_add(front, vertical, front);
    glm_vec3_normalize(front);
    glm_vec3_copy(front, out_front);

    glm_vec3_cross(front, world_up, out_right);
    if (!_vec3_valid(out_right))
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, out_right);
    else
        glm_vec3_normalize(out_right);

    glm_vec3_cross(out_right, front, out_up);
    if (!_vec3_valid(out_up))
        glm_vec3_copy(world_up, out_up);
    else
        glm_vec3_normalize(out_up);

    if ((fly->flags & DVZ_FLY_FLAGS_DISABLE_ROLL) != 0 || fly->roll == 0.0f)
        return;

    mat4 roll = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_make(roll, fly->roll, front);
    glm_mat4_mulv3(roll, out_right, 1.0f, out_right);
    glm_mat4_mulv3(roll, out_up, 1.0f, out_up);
    glm_vec3_normalize(out_right);
    glm_vec3_normalize(out_up);
}



/**
 * Compute the movement basis for the current mode.
 *
 * @param fly the fly controller
 * @param out_forward output forward movement vector
 * @param out_right output right movement vector
 * @param out_up output up movement vector
 */
static void _fly_movement_basis(
    const DvzFly* fly, vec3 out_forward, vec3 out_right, vec3 out_up)
{
    ANN(fly);
    ANN(out_forward);
    ANN(out_right);
    ANN(out_up);

    vec3 front = {0}, right = {0}, up = {0};
    _fly_vectors(fly, front, right, up);
    glm_vec3_copy(front, out_forward);
    glm_vec3_copy(right, out_right);
    out_up[0] = fly->world_up[0];
    out_up[1] = fly->world_up[1];
    out_up[2] = fly->world_up[2];
    if (!_vec3_valid(out_up))
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, out_up);
    glm_vec3_normalize(out_up);

    if (fly->mode != DVZ_FLY_MODE_PLANE)
        return;

    float dot = glm_vec3_dot(out_forward, out_up);
    vec3 vertical = {0};
    glm_vec3_scale(out_up, dot, vertical);
    glm_vec3_sub(out_forward, vertical, out_forward);
    if (!_vec3_valid(out_forward))
        glm_vec3_cross(out_up, right, out_forward);
    glm_vec3_normalize(out_forward);

    glm_vec3_cross(out_forward, out_up, out_right);
    if (!_vec3_valid(out_right))
        glm_vec3_copy(right, out_right);
    else
        glm_vec3_normalize(out_right);
}



/**
 * Return whether a position is inside the configured viewport.
 *
 * @param fly the fly controller
 * @param pos position in window pixels
 * @return whether the position is inside the viewport
 */
static bool _fly_contains_pos(const DvzFly* fly, const vec2 pos)
{
    ANN(fly);
    float x = fly->viewport_origin[0];
    float y = fly->viewport_origin[1];
    float w = fly->viewport_size[0];
    float h = fly->viewport_size[1];
    return pos[0] >= x && pos[0] < x + w && pos[1] >= y && pos[1] < y + h;
}



/**
 * Return whether an event should be handled by this controller.
 *
 * @param fly the fly controller
 * @param ev pointer event
 * @return whether the event is in the controller viewport
 */
static bool _fly_event_in_viewport(const DvzFly* fly, const DvzPointerEvent* ev)
{
    ANN(fly);
    ANN(ev);
    if (!fly->has_viewport)
        return true;
    if (ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
        return fly->interacting;
    if (ev->type == DVZ_POINTER_EVENT_DRAG && ev->content.d.is_press_valid)
        return _fly_contains_pos(fly, ev->content.d.press_pos);
    return _fly_contains_pos(fly, ev->pos);
}



/**
 * Convert a pointer event to viewport-local coordinates.
 *
 * @param fly the fly controller
 * @param ev input event
 * @param out output local event
 */
static void _fly_local_event(const DvzFly* fly, const DvzPointerEvent* ev, DvzPointerEvent* out)
{
    ANN(fly);
    ANN(ev);
    ANN(out);
    *out = *ev;
    if (!fly->has_viewport)
        return;
    out->pos[0] = ev->pos[0] - fly->viewport_origin[0];
    out->pos[1] = ev->pos[1] - fly->viewport_origin[1];
    if (ev->type == DVZ_POINTER_EVENT_DRAG || ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
    {
        out->content.d.press_pos[0] = ev->content.d.press_pos[0] - fly->viewport_origin[0];
        out->content.d.press_pos[1] = ev->content.d.press_pos[1] - fly->viewport_origin[1];
        out->content.d.last_pos[0] = ev->content.d.last_pos[0] - fly->viewport_origin[0];
        out->content.d.last_pos[1] = ev->content.d.last_pos[1] - fly->viewport_origin[1];
    }
}



/**
 * Set a key-state flag from a keyboard event.
 *
 * @param fly the fly controller
 * @param ev keyboard event
 * @return whether the key was recognized
 */
static bool _fly_keyboard_state(DvzFly* fly, const DvzKeyboardEvent* ev)
{
    ANN(fly);
    ANN(ev);
    bool down = ev->type == DVZ_KEYBOARD_EVENT_PRESS || ev->type == DVZ_KEYBOARD_EVENT_REPEAT;
    bool up = ev->type == DVZ_KEYBOARD_EVENT_RELEASE;
    if (!down && !up)
        return false;
    bool state = down;

    switch (ev->key)
    {
    case DVZ_KEY_W:
    case DVZ_KEY_UP:
        fly->key_forward = state;
        return true;
    case DVZ_KEY_S:
    case DVZ_KEY_DOWN:
        fly->key_backward = state;
        return true;
    case DVZ_KEY_A:
    case DVZ_KEY_LEFT:
        fly->key_left = state;
        return true;
    case DVZ_KEY_D:
    case DVZ_KEY_RIGHT:
        fly->key_right = state;
        return true;
    case DVZ_KEY_SPACE:
        fly->key_up = state;
        return true;
    case DVZ_KEY_C:
    case DVZ_KEY_LEFT_CONTROL:
    case DVZ_KEY_RIGHT_CONTROL:
        fly->key_down = state;
        return true;
    case DVZ_KEY_LEFT_SHIFT:
    case DVZ_KEY_RIGHT_SHIFT:
        fly->key_fast = state;
        return true;
    default:
        return false;
    }
}



/**
 * Input callback subscribed to union input events.
 *
 * @param router input router
 * @param ev input event
 * @param user_data fly controller
 */
static void _fly_input_callback(DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
{
    (void)router;
    DvzFly* fly = (DvzFly*)user_data;
    if (ev->type == DVZ_INPUT_EVENT_POINTER)
    {
        dvz_fly_pointer(fly, &ev->content.pointer);
    }
    else if (ev->type == DVZ_INPUT_EVENT_KEYBOARD)
    {
        dvz_fly_keyboard(fly, &ev->content.keyboard);
    }
    else if (ev->type == DVZ_INPUT_EVENT_RESIZE)
    {
        const DvzInputResizeEvent* r = &ev->content.resize;
        if (!fly->has_viewport && r->window_width > 0 && r->window_height > 0)
            dvz_fly_resize(fly, (float)r->window_width, (float)r->window_height);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default fly-controller descriptor.
 *
 * @return the fly descriptor
 */
DvzFlyDesc dvz_fly_desc(void)
{
    return (DvzFlyDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzFlyDesc),
        .mode = DVZ_FLY_MODE_FREE,
        .controller_flags = DVZ_FLY_FLAGS_FIXED_UP | DVZ_FLY_FLAGS_DISABLE_ROLL,
        .position = {0.0f, 0.0f, 3.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .yaw = -GLM_PI_2f,
        .pitch = 0.0f,
        .roll = 0.0f,
        .use_angles = false,
        .speed = DVZ_FLY_DEFAULT_SPEED,
        .fast_multiplier = DVZ_FLY_DEFAULT_FAST,
        .slow_multiplier = DVZ_FLY_DEFAULT_SLOW,
        .look_speed = DVZ_FLY_DEFAULT_LOOK,
        .wheel_speed = DVZ_FLY_DEFAULT_WHEEL,
    };
}



/**
 * Create a fly camera controller.
 *
 * @param desc fly descriptor, or NULL for defaults
 * @return the fly controller
 */
DvzFly* _dvz_fly(const DvzFlyDesc* desc)
{
    DvzFlyDesc default_desc = dvz_fly_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_fly_desc_validate(desc))
        return NULL;

    DvzFly* fly = (DvzFly*)dvz_calloc(1, sizeof(DvzFly));
    if (fly == NULL)
        return NULL;

    fly->mode = desc->mode;
    fly->flags = (int)desc->controller_flags;
    fly->viewport_size[0] = DVZ_FLY_DEFAULT_WIDTH;
    fly->viewport_size[1] = DVZ_FLY_DEFAULT_HEIGHT;
    fly->world_up[0] = desc->up[0];
    fly->world_up[1] = desc->up[1];
    fly->world_up[2] = desc->up[2];
    if (!_vec3_valid(fly->world_up))
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, fly->world_up);
    glm_vec3_normalize(fly->world_up);

    fly->speed = desc->speed > 0.0f ? desc->speed : DVZ_FLY_DEFAULT_SPEED;
    fly->fast_multiplier =
        desc->fast_multiplier > 0.0f ? desc->fast_multiplier : DVZ_FLY_DEFAULT_FAST;
    fly->slow_multiplier =
        desc->slow_multiplier > 0.0f ? desc->slow_multiplier : DVZ_FLY_DEFAULT_SLOW;
    fly->look_speed = desc->look_speed > 0.0f ? desc->look_speed : DVZ_FLY_DEFAULT_LOOK;
    fly->wheel_speed = desc->wheel_speed;

    if (desc->use_angles)
        dvz_fly_initial(
            fly, (vec3){desc->position[0], desc->position[1], desc->position[2]}, desc->yaw,
            desc->pitch, desc->roll);
    else
        dvz_fly_initial_lookat(
            fly, (vec3){desc->position[0], desc->position[1], desc->position[2]},
            (vec3){desc->target[0], desc->target[1], desc->target[2]});
    return fly;
}


/**
 * Create a standalone fly camera controller.
 *
 * @param desc fly descriptor, or NULL for defaults
 * @return the fly controller, or NULL on allocation failure
 */
DvzFly* dvz_fly_create(const DvzFlyDesc* desc)
{
    return _dvz_fly(desc);
}



/**
 * Reset a fly controller to its initial pose.
 *
 * @param fly the fly controller
 */
void dvz_fly_reset(DvzFly* fly)
{
    ANN(fly);
    glm_vec3_copy(fly->position_init, fly->position);
    fly->yaw = fly->yaw_init;
    fly->pitch = fly->pitch_init;
    fly->roll = fly->roll_init;
    fly->interacting = false;
    fly->key_forward = false;
    fly->key_backward = false;
    fly->key_left = false;
    fly->key_right = false;
    fly->key_up = false;
    fly->key_down = false;
    fly->key_fast = false;
    fly->key_slow = false;
    dvz_fly_apply_camera(fly);
}



/**
 * Update the viewport rectangle in window coordinates.
 *
 * @param fly the fly controller
 * @param x viewport x origin in window pixels
 * @param y viewport y origin in window pixels
 * @param width viewport width in window pixels
 * @param height viewport height in window pixels
 */
void dvz_fly_viewport(DvzFly* fly, float x, float y, float width, float height)
{
    ANN(fly);
    fly->viewport_origin[0] = x;
    fly->viewport_origin[1] = y;
    fly->viewport_size[0] = width > 0.0f ? width : DVZ_FLY_DEFAULT_WIDTH;
    fly->viewport_size[1] = height > 0.0f ? height : DVZ_FLY_DEFAULT_HEIGHT;
    fly->has_viewport = true;
}



/**
 * Update the viewport size.
 *
 * @param fly the fly controller
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
void dvz_fly_resize(DvzFly* fly, float width, float height)
{
    ANN(fly);
    fly->viewport_size[0] = width > 0.0f ? width : DVZ_FLY_DEFAULT_WIDTH;
    fly->viewport_size[1] = height > 0.0f ? height : DVZ_FLY_DEFAULT_HEIGHT;
}



/**
 * Set the initial pose from angles and reset.
 *
 * @param fly the fly controller
 * @param position initial camera position
 * @param yaw initial yaw angle in radians
 * @param pitch initial pitch angle in radians
 * @param roll initial roll angle in radians
 */
void dvz_fly_initial(DvzFly* fly, vec3 position, float yaw, float pitch, float roll)
{
    ANN(fly);
    glm_vec3_copy(position, fly->position_init);
    fly->yaw_init = _fly_wrap_angle(yaw);
    fly->pitch_init = _fly_clamp_pitch(pitch);
    fly->roll_init = (fly->flags & DVZ_FLY_FLAGS_DISABLE_ROLL) != 0 ? 0.0f : _fly_wrap_angle(roll);
    dvz_fly_reset(fly);
}



/**
 * Set the initial pose from a look-at point and reset.
 *
 * @param fly the fly controller
 * @param position initial camera position
 * @param target initial look-at target
 */
void dvz_fly_initial_lookat(DvzFly* fly, vec3 position, vec3 target)
{
    ANN(fly);
    vec3 dir = {0};
    glm_vec3_sub(target, position, dir);
    float yaw = -GLM_PI_2f;
    float pitch = 0.0f;
    if (!_fly_angles_from_dir(dir, fly->world_up, &yaw, &pitch))
        log_warn("invalid fly initial look-at direction, using default orientation");
    dvz_fly_initial(fly, position, yaw, pitch, 0.0f);
}



/**
 * Set the movement mode.
 *
 * @param fly the fly controller
 * @param mode movement mode
 */
void dvz_fly_set_mode(DvzFly* fly, DvzFlyMode mode)
{
    ANN(fly);
    fly->mode = mode;
}



/**
 * Move forward along the active movement direction.
 *
 * @param fly the fly controller
 * @param amount movement amount in world units
 */
void dvz_fly_move_forward(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 forward = {0}, right = {0}, up = {0}, move = {0};
    _fly_movement_basis(fly, forward, right, up);
    glm_vec3_scale(forward, amount, move);
    glm_vec3_add(fly->position, move, fly->position);
    dvz_fly_apply_camera(fly);
}



/**
 * Move right relative to the active movement direction.
 *
 * @param fly the fly controller
 * @param amount movement amount in world units
 */
void dvz_fly_move_right(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 forward = {0}, right = {0}, up = {0}, move = {0};
    _fly_movement_basis(fly, forward, right, up);
    glm_vec3_scale(right, amount, move);
    glm_vec3_add(fly->position, move, fly->position);
    dvz_fly_apply_camera(fly);
}



/**
 * Move up along the fly controller's world-up direction.
 *
 * @param fly the fly controller
 * @param amount movement amount in world units
 */
void dvz_fly_move_up(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 up = {0}, move = {0};
    glm_vec3_copy(fly->world_up, up);
    if (!_vec3_valid(up))
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
    glm_vec3_normalize(up);
    glm_vec3_scale(up, amount, move);
    glm_vec3_add(fly->position, move, fly->position);
    dvz_fly_apply_camera(fly);
}



/**
 * Rotate the fly view direction.
 *
 * @param fly the fly controller
 * @param dx yaw delta in radians
 * @param dy pitch delta in radians
 */
void dvz_fly_rotate(DvzFly* fly, float dx, float dy)
{
    ANN(fly);
    if ((fly->flags & DVZ_FLY_FLAGS_INVERT_Y) != 0)
        dy = -dy;
    fly->yaw = _fly_wrap_angle(fly->yaw + dx);
    fly->pitch = _fly_clamp_pitch(fly->pitch + dy);
    dvz_fly_apply_camera(fly);
}



/**
 * Roll the fly camera around its view direction.
 *
 * @param fly the fly controller
 * @param dx roll delta in radians
 */
void dvz_fly_roll(DvzFly* fly, float dx)
{
    ANN(fly);
    if ((fly->flags & DVZ_FLY_FLAGS_DISABLE_ROLL) != 0)
        return;
    fly->roll = _fly_wrap_angle(fly->roll + dx);
    dvz_fly_apply_camera(fly);
}



/**
 * Return the current position.
 *
 * @param fly the fly controller
 * @param out_pos output position
 */
void dvz_fly_get_position(const DvzFly* fly, vec3 out_pos)
{
    ANN(fly);
    ANN(out_pos);
    out_pos[0] = fly->position[0];
    out_pos[1] = fly->position[1];
    out_pos[2] = fly->position[2];
}



/**
 * Return the current look-at target.
 *
 * @param fly the fly controller
 * @param out_target output target
 */
void dvz_fly_get_target(const DvzFly* fly, vec3 out_target)
{
    ANN(fly);
    ANN(out_target);
    vec3 front = {0}, right = {0}, up = {0};
    _fly_vectors(fly, front, right, up);
    out_target[0] = fly->position[0] + front[0];
    out_target[1] = fly->position[1] + front[1];
    out_target[2] = fly->position[2] + front[2];
}



/**
 * Return the current up vector.
 *
 * @param fly the fly controller
 * @param out_up output up vector
 */
void dvz_fly_get_up(const DvzFly* fly, vec3 out_up)
{
    ANN(fly);
    ANN(out_up);
    vec3 front = {0}, right = {0};
    _fly_vectors(fly, front, right, out_up);
}



/**
 * Set or move the optional orbit pivot while preserving the camera eye.
 *
 * @param fly the fly controller
 * @param pivot new world-space pivot point
 */
void dvz_fly_pivot(DvzFly* fly, vec3 pivot)
{
    ANN(fly);
    glm_vec3_copy(pivot, fly->pivot);
    fly->has_pivot = true;
    fly->pivot_marker_visible = true;
    fly->pivot_marker_time_left = DVZ_FLY_PIVOT_MARKER_S;
    dvz_fly_look_at_pivot(fly);
}



/**
 * Clear the optional orbit pivot.
 *
 * @param fly the fly controller
 */
void dvz_fly_clear_pivot(DvzFly* fly)
{
    ANN(fly);
    fly->has_pivot = false;
    fly->pivot_distance = 0.0f;
    fly->pivot_marker_visible = false;
    fly->pivot_marker_time_left = 0.0;
}



/**
 * Return whether an orbit pivot is set.
 *
 * @param fly the fly controller
 * @return whether the fly controller has a pivot
 */
bool dvz_fly_has_pivot(const DvzFly* fly)
{
    if (fly == NULL)
        return false;
    return fly->has_pivot;
}



/**
 * Reorient the camera toward the active pivot without moving the eye.
 *
 * @param fly the fly controller
 */
void dvz_fly_look_at_pivot(DvzFly* fly)
{
    ANN(fly);
    if (!fly->has_pivot)
        return;
    vec3 dir = {0};
    glm_vec3_sub(fly->pivot, fly->position, dir);
    fly->pivot_distance = glm_vec3_norm(dir);
    float yaw = 0.0f, pitch = 0.0f;
    if (_fly_angles_from_dir(dir, fly->world_up, &yaw, &pitch))
    {
        fly->yaw = yaw;
        fly->pitch = pitch;
        dvz_fly_apply_camera(fly);
    }
}



/**
 * Orbit the camera around the active pivot.
 *
 * @param fly the fly controller
 * @param yaw_delta yaw delta in radians
 * @param pitch_delta pitch delta in radians
 * @return whether the orbit was applied
 */
bool dvz_fly_orbit(DvzFly* fly, float yaw_delta, float pitch_delta)
{
    ANN(fly);
    if (!fly->has_pivot || fly->pivot_distance <= 0.0f)
        return false;

    fly->yaw = _fly_wrap_angle(fly->yaw + yaw_delta);
    fly->pitch = _fly_clamp_pitch(fly->pitch + pitch_delta);

    vec3 front = {0}, right = {0}, up = {0}, offset = {0};
    _fly_vectors(fly, front, right, up);
    glm_vec3_scale(front, -fly->pivot_distance, offset);
    glm_vec3_add(fly->pivot, offset, fly->position);
    fly->pivot_marker_visible = true;
    fly->pivot_marker_time_left = DVZ_FLY_PIVOT_MARKER_S;
    dvz_fly_apply_camera(fly);
    return true;
}



/**
 * Attach a camera updated by this fly controller.
 *
 * @param fly the fly controller
 * @param camera the camera to update, or NULL
 */
void dvz_fly_set_camera(DvzFly* fly, DvzCamera* camera)
{
    ANN(fly);
    fly->camera = camera;
    dvz_fly_apply_camera(fly);
}



/**
 * Apply the current fly pose to the attached camera.
 *
 * @param fly the fly controller
 */
void dvz_fly_apply_camera(DvzFly* fly)
{
    ANN(fly);
    if (fly->camera == NULL)
        return;
    vec3 target = {0}, up = {0};
    dvz_fly_get_target(fly, target);
    dvz_fly_get_up(fly, up);
    dvz_camera_set_view(fly->camera, fly->position, target, up);
}



/**
 * Advance held-key movement.
 *
 * @param fly the fly controller
 * @param dt elapsed time in seconds
 */
void dvz_fly_update(DvzFly* fly, double dt)
{
    ANN(fly);
    if (dt <= 0.0)
        return;

    float speed = fly->speed;
    if (fly->key_fast)
        speed *= fly->fast_multiplier;
    if (fly->key_slow)
        speed *= fly->slow_multiplier;
    float amount = speed * (float)dt;

    float forward = 0.0f;
    float right = 0.0f;
    float up = 0.0f;
    if (fly->key_forward)
        forward += amount;
    if (fly->key_backward)
        forward -= amount;
    if (fly->key_right)
        right += amount;
    if (fly->key_left)
        right -= amount;
    if (fly->key_up)
        up += DVZ_FLY_VERTICAL_SPEED * amount;
    if (fly->key_down)
        up -= DVZ_FLY_VERTICAL_SPEED * amount;

    if (forward != 0.0f)
        dvz_fly_move_forward(fly, forward);
    if (right != 0.0f)
        dvz_fly_move_right(fly, right);
    if (up != 0.0f)
        dvz_fly_move_up(fly, up);

    if (fly->pivot_marker_time_left > 0.0)
    {
        fly->pivot_marker_time_left -= dt;
        if (fly->pivot_marker_time_left <= 0.0 && !fly->interacting)
            fly->pivot_marker_visible = false;
    }
}



/**
 * Process a pointer event.
 *
 * @param fly the fly controller
 * @param ev pointer event
 * @return true if the event was consumed
 */
bool dvz_fly_pointer(DvzFly* fly, const DvzPointerEvent* ev)
{
    ANN(fly);
    ANN(ev);
    if (!_fly_event_in_viewport(fly, ev))
        return false;

    DvzPointerEvent local = {0};
    _fly_local_event(fly, ev, &local);
    ev = &local;

    float width = fly->viewport_size[0] > 0.0f ? fly->viewport_size[0] : DVZ_FLY_DEFAULT_WIDTH;
    float height = fly->viewport_size[1] > 0.0f ? fly->viewport_size[1] : DVZ_FLY_DEFAULT_HEIGHT;

    switch (ev->type)
    {
    case DVZ_POINTER_EVENT_PRESS:
    case DVZ_POINTER_EVENT_DRAG_START:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_RIGHT)
            fly->interacting = true;
        break;

    case DVZ_POINTER_EVENT_RELEASE:
    case DVZ_POINTER_EVENT_DRAG_STOP:
        fly->interacting = false;
        if (fly->pivot_marker_time_left <= 0.0)
            fly->pivot_marker_visible = false;
        break;

    case DVZ_POINTER_EVENT_DRAG:
    {
        vec2 delta = {
            ev->pos[0] - ev->content.d.last_pos[0],
            ev->pos[1] - ev->content.d.last_pos[1],
        };
        if (ev->button == DVZ_POINTER_BUTTON_LEFT)
        {
            float dx = fly->look_speed * delta[0] / width * GLM_PIf;
            float dy = -fly->look_speed * delta[1] / height * GLM_PIf;
            dvz_fly_rotate(fly, dx, dy);
            fly->interacting = true;
            return true;
        }
        if (ev->button == DVZ_POINTER_BUTTON_RIGHT && fly->has_pivot)
        {
            float dx = -fly->look_speed * delta[0] / width * GLM_PIf;
            float dy = +fly->look_speed * delta[1] / height * GLM_PIf;
            fly->interacting = dvz_fly_orbit(fly, dx, dy);
            return fly->interacting;
        }
        if (ev->button == DVZ_POINTER_BUTTON_RIGHT)
        {
            float scale = 2.0f * fly->speed;
            dvz_fly_move_right(fly, scale * delta[0] / width);
            dvz_fly_move_up(fly, -DVZ_FLY_VERTICAL_SPEED * scale * delta[1] / height);
            fly->interacting = true;
            return true;
        }
        break;
    }

    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        dvz_fly_reset(fly);
        return true;

    case DVZ_POINTER_EVENT_WHEEL:
        dvz_fly_move_forward(fly, fly->wheel_speed * ev->content.w.dir[1]);
        return true;

    default:
        return false;
    }
    return false;
}



/**
 * Process a keyboard event.
 *
 * @param fly the fly controller
 * @param ev keyboard event
 * @return true if the event was consumed
 */
bool dvz_fly_keyboard(DvzFly* fly, const DvzKeyboardEvent* ev)
{
    ANN(fly);
    ANN(ev);
    if (ev->type == DVZ_KEYBOARD_EVENT_PRESS && ev->key == DVZ_KEY_R)
    {
        dvz_fly_reset(fly);
        return true;
    }
    return _fly_keyboard_state(fly, ev);
}



/**
 * Subscribe the fly controller to an input router.
 *
 * @param fly the fly controller
 * @param router input router
 */
void dvz_fly_connect(DvzFly* fly, DvzInputRouter* router)
{
    ANN(fly);
    ANN(router);
    DvzInputResizeEvent r;
    if (!fly->has_viewport && dvz_input_router_last_resize(router, &r) && r.window_width > 0 &&
        r.window_height > 0)
    {
        dvz_fly_resize(fly, (float)r.window_width, (float)r.window_height);
    }
    dvz_input_subscribe_event(router, _fly_input_callback, fly);
}



/**
 * Unsubscribe the fly controller from an input router.
 *
 * @param fly the fly controller
 * @param router input router
 */
void dvz_fly_disconnect(DvzFly* fly, DvzInputRouter* router)
{
    ANN(fly);
    ANN(router);
    dvz_input_unsubscribe_event(router, _fly_input_callback, fly);
}



/**
 * Destroy a fly controller.
 *
 * @param fly the fly controller
 */
void dvz_fly_destroy(DvzFly* fly)
{
    if (fly == NULL)
        return;
    dvz_free(fly);
}
