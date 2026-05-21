/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Turntable controller                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene/turntable.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TURNTABLE_DEFAULT_WIDTH  800.0f
#define DVZ_TURNTABLE_DEFAULT_HEIGHT 600.0f
#define DVZ_TURNTABLE_DEFAULT_DIST   3.0f
#define DVZ_TURNTABLE_DEFAULT_SPEED  1.0f
#define DVZ_TURNTABLE_DEFAULT_ZOOM   0.2f
#define DVZ_TURNTABLE_DEFAULT_PAN    0.003f
#define DVZ_TURNTABLE_PITCH_EPS      0.001f
#define DVZ_TURNTABLE_MARKER_S       1.0



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



/**
 * Normalize an angle to [-pi, pi].
 *
 * @param angle input angle
 * @return normalized angle
 */
static float _turntable_wrap_angle(float angle)
{
    while (angle > GLM_PIf)
        angle -= 2.0f * GLM_PIf;
    while (angle < -GLM_PIf)
        angle += 2.0f * GLM_PIf;
    return angle;
}



/**
 * Clamp a turntable pitch angle.
 *
 * @param turntable the turntable controller
 * @param pitch pitch angle
 * @return clamped pitch
 */
static float _turntable_clamp_pitch(const DvzTurntable* turntable, float pitch)
{
    ANN(turntable);
    float min_pitch = turntable->min_pitch;
    float max_pitch = turntable->max_pitch;
    if (max_pitch <= min_pitch)
    {
        min_pitch = -GLM_PI_2f + DVZ_TURNTABLE_PITCH_EPS;
        max_pitch = +GLM_PI_2f - DVZ_TURNTABLE_PITCH_EPS;
    }
    return _clampf(pitch, min_pitch, max_pitch);
}



/**
 * Clamp a turntable distance.
 *
 * @param turntable the turntable controller
 * @param distance distance value
 * @return clamped distance
 */
static float _turntable_clamp_distance(const DvzTurntable* turntable, float distance)
{
    ANN(turntable);
    if ((turntable->flags & DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE) == 0)
        return distance > 0.0f ? distance : turntable->min_distance;
    return _clampf(distance, turntable->min_distance, turntable->max_distance);
}



/**
 * Compute a front direction from yaw and pitch.
 *
 * @param yaw yaw angle
 * @param pitch pitch angle
 * @param out_front output front vector
 */
static void _turntable_front(float yaw, float pitch, vec3 out_front)
{
    ANN(out_front);
    glm_vec3_copy(
        (vec3){cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch)}, out_front);
    glm_vec3_normalize(out_front);
}



/**
 * Compute the basis vectors for the current turntable pose.
 *
 * @param turntable the turntable controller
 * @param out_front output front vector
 * @param out_right output right vector
 * @param out_up output up vector
 */
static void _turntable_basis(
    const DvzTurntable* turntable, vec3 out_front, vec3 out_right, vec3 out_up)
{
    ANN(turntable);
    _turntable_front(turntable->yaw, turntable->pitch, out_front);

    vec3 stable_up = {turntable->up[0], turntable->up[1], turntable->up[2]};
    if (!_vec3_valid(stable_up))
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, stable_up);
    glm_vec3_normalize(stable_up);

    glm_vec3_cross(out_front, stable_up, out_right);
    if (!_vec3_valid(out_right))
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, out_right);
    else
        glm_vec3_normalize(out_right);

    glm_vec3_cross(out_right, out_front, out_up);
    if (!_vec3_valid(out_up))
        glm_vec3_copy(stable_up, out_up);
    else
        glm_vec3_normalize(out_up);
}



/**
 * Recompute the eye from pivot, distance, yaw, and pitch.
 *
 * @param turntable the turntable controller
 */
static void _turntable_update_eye(DvzTurntable* turntable)
{
    ANN(turntable);
    vec3 front = {0};
    _turntable_front(turntable->yaw, turntable->pitch, front);
    vec3 offset = {0};
    glm_vec3_scale(front, -turntable->distance, offset);
    glm_vec3_add(turntable->pivot, offset, turntable->eye);
}



/**
 * Recompute yaw, pitch, and distance from the current eye and pivot.
 *
 * @param turntable the turntable controller
 */
static void _turntable_update_angles_from_eye(DvzTurntable* turntable)
{
    ANN(turntable);
    vec3 dir = {0};
    glm_vec3_sub(turntable->pivot, turntable->eye, dir);
    float distance = glm_vec3_norm(dir);
    if (distance <= 0.0f)
        return;
    glm_vec3_scale(dir, 1.0f / distance, dir);
    turntable->distance = _turntable_clamp_distance(turntable, distance);
    turntable->pitch = _turntable_clamp_pitch(turntable, asinf(_clampf(dir[1], -1.0f, +1.0f)));
    turntable->yaw = atan2f(dir[2], dir[0]);
    if ((turntable->flags & DVZ_TURNTABLE_FLAGS_WRAP_YAW) != 0)
        turntable->yaw = _turntable_wrap_angle(turntable->yaw);
}



/**
 * Return whether a pointer event is inside the viewport.
 *
 * @param turntable the turntable controller
 * @param ev pointer event
 * @return whether the event should be handled
 */
static bool _turntable_event_in_viewport(const DvzTurntable* turntable, const DvzPointerEvent* ev)
{
    ANN(turntable);
    ANN(ev);
    if (!turntable->has_viewport)
        return true;
    if (ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
        return turntable->interacting;

    vec2 pos = {ev->pos[0], ev->pos[1]};
    if (ev->type == DVZ_POINTER_EVENT_DRAG && ev->content.d.is_press_valid)
    {
        pos[0] = ev->content.d.press_pos[0];
        pos[1] = ev->content.d.press_pos[1];
    }

    float x = turntable->viewport_origin[0];
    float y = turntable->viewport_origin[1];
    float w = turntable->viewport_size[0];
    float h = turntable->viewport_size[1];
    return pos[0] >= x && pos[0] < x + w && pos[1] >= y && pos[1] < y + h;
}



/**
 * Convert a pointer event to viewport-local coordinates.
 *
 * @param turntable the turntable controller
 * @param ev input event
 * @param out output event
 */
static void _turntable_local_event(
    const DvzTurntable* turntable, const DvzPointerEvent* ev, DvzPointerEvent* out)
{
    ANN(turntable);
    ANN(ev);
    ANN(out);
    *out = *ev;
    if (!turntable->has_viewport)
        return;
    out->pos[0] = ev->pos[0] - turntable->viewport_origin[0];
    out->pos[1] = ev->pos[1] - turntable->viewport_origin[1];
    if (ev->type == DVZ_POINTER_EVENT_DRAG || ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
    {
        out->content.d.last_pos[0] =
            ev->content.d.last_pos[0] - turntable->viewport_origin[0];
        out->content.d.last_pos[1] =
            ev->content.d.last_pos[1] - turntable->viewport_origin[1];
        out->content.d.press_pos[0] =
            ev->content.d.press_pos[0] - turntable->viewport_origin[0];
        out->content.d.press_pos[1] =
            ev->content.d.press_pos[1] - turntable->viewport_origin[1];
    }
}



/**
 * Input callback subscribed to union input events.
 *
 * @param router input router
 * @param ev input event
 * @param user_data turntable controller
 */
static void
_turntable_input_callback(DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
{
    (void)router;
    DvzTurntable* turntable = (DvzTurntable*)user_data;
    if (ev->type == DVZ_INPUT_EVENT_POINTER)
    {
        dvz_turntable_pointer(turntable, &ev->content.pointer);
    }
    else if (ev->type == DVZ_INPUT_EVENT_RESIZE)
    {
        const DvzInputResizeEvent* r = &ev->content.resize;
        if (!turntable->has_viewport && r->window_width > 0 && r->window_height > 0)
            dvz_turntable_resize(turntable, (float)r->window_width, (float)r->window_height);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default turntable descriptor.
 *
 * @return the turntable descriptor
 */
DvzTurntableDesc dvz_turntable_desc(void)
{
    return (DvzTurntableDesc){
        .pivot = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .distance = DVZ_TURNTABLE_DEFAULT_DIST,
        .yaw = -GLM_PI_2f,
        .pitch = 0.0f,
        .yaw_speed = DVZ_TURNTABLE_DEFAULT_SPEED,
        .pitch_speed = DVZ_TURNTABLE_DEFAULT_SPEED,
        .zoom_speed = DVZ_TURNTABLE_DEFAULT_ZOOM,
        .pan_speed = DVZ_TURNTABLE_DEFAULT_PAN,
        .min_pitch = -GLM_PI_2f + DVZ_TURNTABLE_PITCH_EPS,
        .max_pitch = +GLM_PI_2f - DVZ_TURNTABLE_PITCH_EPS,
        .min_distance = 0.01f,
        .max_distance = 100000.0f,
        .flags = DVZ_TURNTABLE_FLAGS_ALLOW_PAN | DVZ_TURNTABLE_FLAGS_WRAP_YAW |
                 DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE,
    };
}



/**
 * Create a turntable controller.
 *
 * @param desc descriptor, or NULL for defaults
 * @return the turntable controller
 */
DvzTurntable* _dvz_turntable(const DvzTurntableDesc* desc)
{
    DvzTurntableDesc default_desc = dvz_turntable_desc();
    if (desc == NULL)
        desc = &default_desc;

    DvzTurntable* turntable = (DvzTurntable*)dvz_calloc(1, sizeof(DvzTurntable));
    if (turntable == NULL)
        return NULL;

    turntable->flags = desc->flags;
    turntable->viewport_size[0] = DVZ_TURNTABLE_DEFAULT_WIDTH;
    turntable->viewport_size[1] = DVZ_TURNTABLE_DEFAULT_HEIGHT;
    glm_vec3_copy(
        (vec3){desc->pivot[0], desc->pivot[1], desc->pivot[2]}, turntable->pivot_init);
    glm_vec3_copy((vec3){desc->pivot[0], desc->pivot[1], desc->pivot[2]}, turntable->pivot);
    glm_vec3_copy((vec3){desc->up[0], desc->up[1], desc->up[2]}, turntable->up);
    if (!_vec3_valid(turntable->up))
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, turntable->up);
    glm_vec3_normalize(turntable->up);

    turntable->distance_init = desc->distance > 0.0f ? desc->distance :
                                                        DVZ_TURNTABLE_DEFAULT_DIST;
    turntable->yaw_init = desc->yaw;
    turntable->pitch_init = desc->pitch;
    turntable->yaw_speed = desc->yaw_speed > 0.0f ? desc->yaw_speed :
                                                        DVZ_TURNTABLE_DEFAULT_SPEED;
    turntable->pitch_speed = desc->pitch_speed > 0.0f ? desc->pitch_speed :
                                                            DVZ_TURNTABLE_DEFAULT_SPEED;
    turntable->zoom_speed = desc->zoom_speed != 0.0f ? desc->zoom_speed :
                                                        DVZ_TURNTABLE_DEFAULT_ZOOM;
    turntable->pan_speed = desc->pan_speed > 0.0f ? desc->pan_speed :
                                                        DVZ_TURNTABLE_DEFAULT_PAN;
    turntable->min_pitch = desc->min_pitch;
    turntable->max_pitch = desc->max_pitch;
    turntable->min_distance = desc->min_distance > 0.0f ? desc->min_distance : 0.01f;
    turntable->max_distance =
        desc->max_distance > turntable->min_distance ? desc->max_distance : 100000.0f;
    dvz_turntable_reset(turntable);
    return turntable;
}



/**
 * Reset a turntable to its initial pose.
 *
 * @param turntable the turntable controller
 */
void dvz_turntable_reset(DvzTurntable* turntable)
{
    ANN(turntable);
    glm_vec3_copy(turntable->pivot_init, turntable->pivot);
    turntable->distance = _turntable_clamp_distance(turntable, turntable->distance_init);
    turntable->yaw = turntable->yaw_init;
    if ((turntable->flags & DVZ_TURNTABLE_FLAGS_WRAP_YAW) != 0)
        turntable->yaw = _turntable_wrap_angle(turntable->yaw);
    turntable->pitch = _turntable_clamp_pitch(turntable, turntable->pitch_init);
    turntable->interacting = false;
    _turntable_update_eye(turntable);
    dvz_turntable_apply_camera(turntable);
}



/**
 * Update the viewport rectangle in window coordinates.
 *
 * @param turntable the turntable controller
 * @param x viewport x origin in window pixels
 * @param y viewport y origin in window pixels
 * @param width viewport width in window pixels
 * @param height viewport height in window pixels
 */
void dvz_turntable_viewport(
    DvzTurntable* turntable, float x, float y, float width, float height)
{
    ANN(turntable);
    turntable->viewport_origin[0] = x;
    turntable->viewport_origin[1] = y;
    turntable->viewport_size[0] = width > 0.0f ? width : DVZ_TURNTABLE_DEFAULT_WIDTH;
    turntable->viewport_size[1] = height > 0.0f ? height : DVZ_TURNTABLE_DEFAULT_HEIGHT;
    turntable->has_viewport = true;
}



/**
 * Update the viewport size.
 *
 * @param turntable the turntable controller
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
void dvz_turntable_resize(DvzTurntable* turntable, float width, float height)
{
    ANN(turntable);
    turntable->viewport_size[0] = width > 0.0f ? width : DVZ_TURNTABLE_DEFAULT_WIDTH;
    turntable->viewport_size[1] = height > 0.0f ? height : DVZ_TURNTABLE_DEFAULT_HEIGHT;
}



/**
 * Set the pivot while preserving the current camera eye.
 *
 * @param turntable the turntable controller
 * @param pivot new world-space pivot
 */
void dvz_turntable_pivot(DvzTurntable* turntable, vec3 pivot)
{
    ANN(turntable);
    glm_vec3_copy(pivot, turntable->pivot);
    _turntable_update_angles_from_eye(turntable);
    turntable->pivot_marker_visible = true;
    turntable->pivot_marker_time_left = DVZ_TURNTABLE_MARKER_S;
    dvz_turntable_apply_camera(turntable);
}



/**
 * Orbit around the pivot.
 *
 * @param turntable the turntable controller
 * @param yaw_delta yaw delta in radians
 * @param pitch_delta pitch delta in radians
 */
void dvz_turntable_orbit(DvzTurntable* turntable, float yaw_delta, float pitch_delta)
{
    ANN(turntable);
    turntable->yaw += yaw_delta;
    if ((turntable->flags & DVZ_TURNTABLE_FLAGS_WRAP_YAW) != 0)
        turntable->yaw = _turntable_wrap_angle(turntable->yaw);
    if ((turntable->flags & DVZ_TURNTABLE_FLAGS_INVERT_Y) != 0)
        pitch_delta = -pitch_delta;
    turntable->pitch = _turntable_clamp_pitch(turntable, turntable->pitch + pitch_delta);
    turntable->pivot_marker_visible = true;
    turntable->pivot_marker_time_left = DVZ_TURNTABLE_MARKER_S;
    _turntable_update_eye(turntable);
    dvz_turntable_apply_camera(turntable);
}



/**
 * Dolly toward or away from the pivot.
 *
 * @param turntable the turntable controller
 * @param amount distance delta
 */
void dvz_turntable_dolly(DvzTurntable* turntable, float amount)
{
    ANN(turntable);
    turntable->distance = _turntable_clamp_distance(turntable, turntable->distance + amount);
    _turntable_update_eye(turntable);
    dvz_turntable_apply_camera(turntable);
}



/**
 * Pan the pivot in the current view plane.
 *
 * @param turntable the turntable controller
 * @param right_amount right-axis pan amount
 * @param up_amount up-axis pan amount
 */
void dvz_turntable_pan(DvzTurntable* turntable, float right_amount, float up_amount)
{
    ANN(turntable);
    if ((turntable->flags & DVZ_TURNTABLE_FLAGS_ALLOW_PAN) == 0)
        return;
    vec3 front = {0}, right = {0}, up = {0}, delta = {0}, tmp = {0};
    _turntable_basis(turntable, front, right, up);
    glm_vec3_scale(right, right_amount, delta);
    glm_vec3_scale(up, up_amount, tmp);
    glm_vec3_add(delta, tmp, delta);
    glm_vec3_add(turntable->pivot, delta, turntable->pivot);
    glm_vec3_add(turntable->eye, delta, turntable->eye);
    turntable->pivot_marker_visible = true;
    turntable->pivot_marker_time_left = DVZ_TURNTABLE_MARKER_S;
    dvz_turntable_apply_camera(turntable);
}



/**
 * Attach a camera updated by this turntable.
 *
 * @param turntable the turntable controller
 * @param camera the camera to update, or NULL
 */
void dvz_turntable_set_camera(DvzTurntable* turntable, DvzCamera* camera)
{
    ANN(turntable);
    turntable->camera = camera;
    dvz_turntable_apply_camera(turntable);
}



/**
 * Apply the turntable pose to the attached camera.
 *
 * @param turntable the turntable controller
 */
void dvz_turntable_apply_camera(DvzTurntable* turntable)
{
    ANN(turntable);
    if (turntable->camera == NULL)
        return;
    vec3 front = {0}, right = {0}, up = {0};
    _turntable_basis(turntable, front, right, up);
    dvz_camera_set_view(turntable->camera, turntable->eye, turntable->pivot, up);
}



/**
 * Process a pointer event.
 *
 * @param turntable the turntable controller
 * @param ev pointer event
 * @return true if the event was consumed
 */
bool dvz_turntable_pointer(DvzTurntable* turntable, const DvzPointerEvent* ev)
{
    ANN(turntable);
    ANN(ev);
    if (!_turntable_event_in_viewport(turntable, ev))
        return false;

    DvzPointerEvent local = {0};
    _turntable_local_event(turntable, ev, &local);
    ev = &local;

    float width = turntable->viewport_size[0] > 0.0f ? turntable->viewport_size[0] :
                                                       DVZ_TURNTABLE_DEFAULT_WIDTH;
    float height = turntable->viewport_size[1] > 0.0f ? turntable->viewport_size[1] :
                                                        DVZ_TURNTABLE_DEFAULT_HEIGHT;

    switch (ev->type)
    {
    case DVZ_POINTER_EVENT_PRESS:
    case DVZ_POINTER_EVENT_DRAG_START:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
            turntable->interacting = true;
        break;

    case DVZ_POINTER_EVENT_RELEASE:
    case DVZ_POINTER_EVENT_DRAG_STOP:
        turntable->interacting = false;
        break;

    case DVZ_POINTER_EVENT_DRAG:
    {
        float dx = ev->pos[0] - ev->content.d.last_pos[0];
        float dy = ev->pos[1] - ev->content.d.last_pos[1];
        if (ev->button == DVZ_POINTER_BUTTON_LEFT)
        {
            dvz_turntable_orbit(
                turntable, turntable->yaw_speed * dx / width * GLM_PIf,
                -turntable->pitch_speed * dy / height * GLM_PIf);
            turntable->interacting = true;
            return true;
        }
        if (ev->button == DVZ_POINTER_BUTTON_MIDDLE || ev->button == DVZ_POINTER_BUTTON_RIGHT)
        {
            float scale = turntable->pan_speed * turntable->distance;
            dvz_turntable_pan(turntable, -dx * scale, +dy * scale);
            turntable->interacting = true;
            return true;
        }
        break;
    }

    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        dvz_turntable_reset(turntable);
        return true;

    case DVZ_POINTER_EVENT_WHEEL:
        dvz_turntable_dolly(turntable, -turntable->zoom_speed * ev->content.w.dir[1]);
        return true;

    default:
        return false;
    }
    return false;
}



/**
 * Subscribe the turntable to an input router.
 *
 * @param turntable the turntable controller
 * @param router input router
 */
void dvz_turntable_connect(DvzTurntable* turntable, DvzInputRouter* router)
{
    ANN(turntable);
    ANN(router);
    DvzInputResizeEvent r;
    if (!turntable->has_viewport && dvz_input_router_last_resize(router, &r) &&
        r.window_width > 0 && r.window_height > 0)
    {
        dvz_turntable_resize(turntable, (float)r.window_width, (float)r.window_height);
    }
    dvz_input_subscribe_event(router, _turntable_input_callback, turntable);
}



/**
 * Unsubscribe the turntable from an input router.
 *
 * @param turntable the turntable controller
 * @param router input router
 */
void dvz_turntable_disconnect(DvzTurntable* turntable, DvzInputRouter* router)
{
    ANN(turntable);
    ANN(router);
    dvz_input_unsubscribe_event(router, _turntable_input_callback, turntable);
}



/**
 * Destroy a turntable controller.
 *
 * @param turntable the turntable controller
 */
void dvz_turntable_destroy(DvzTurntable* turntable)
{
    if (turntable == NULL)
        return;
    dvz_free(turntable);
}
