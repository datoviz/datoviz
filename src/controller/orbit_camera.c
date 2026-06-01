/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Orbit camera controller                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_controller.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/controller/orbit_camera.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ORBIT_CAMERA_DEFAULT_WIDTH  800.0f
#define DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT 600.0f
#define DVZ_ORBIT_CAMERA_ZOOM_COEF      0.0125f
#define DVZ_ORBIT_CAMERA_MIN_DISTANCE   0.01f
#define DVZ_ORBIT_CAMERA_MAX_DISTANCE   100000.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzOrbitCamera
{
    vec2 viewport_origin;
    vec2 viewport_size;
    bool has_viewport;
    bool interacting;
    int flags;

    vec3 pivot;
    mat4 rotation;
    versor drag_rotation;
    float distance;
    vec2 pan;
    vec2 pan_center;

    DvzCamera* camera;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static float _clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}



static void _screen_to_arcball(vec2 p, versor q)
{
    float dist = glm_vec2_dot(p, p);
    if (dist <= 1.0f)
        glm_vec4_copy((vec4){p[0], p[1], sqrtf(1.0f - dist), 0.0f}, q);
    else
    {
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], p[1], 0.0f, 0.0f}, q);
    }
}



static void _orbit_drag_rotation(vec2 cur_pos, vec2 last_pos, versor out)
{
    versor cur_ball = {0}, prev_ball = {0};
    _screen_to_arcball(cur_pos, cur_ball);
    _screen_to_arcball(last_pos, prev_ball);

    vec3 axis = {0};
    glm_vec3_cross(prev_ball, cur_ball, axis);
    float dot = _clampf(glm_vec3_dot(prev_ball, cur_ball), -1.0f, 1.0f);
    out[0] = axis[0];
    out[1] = axis[1];
    out[2] = axis[2];
    out[3] = 1.0f + dot;
    if (out[3] <= 1e-6f && glm_vec3_norm(axis) <= 1e-6f)
    {
        glm_vec3_cross(prev_ball, (vec3){1.0f, 0.0f, 0.0f}, axis);
        if (glm_vec3_norm(axis) <= 1e-6f)
            glm_vec3_cross(prev_ball, (vec3){0.0f, 1.0f, 0.0f}, axis);
        out[0] = axis[0];
        out[1] = axis[1];
        out[2] = axis[2];
        out[3] = 0.0f;
    }
    glm_quat_normalize(out);
}



static void _orbit_current_rotation(const DvzOrbitCamera* orbit, mat4 out)
{
    ANN(orbit);
    ANN(out);
    mat4 drag = GLM_MAT4_IDENTITY_INIT;
    glm_quat_mat4(orbit->drag_rotation, drag);
    glm_mat4_mul(drag, orbit->rotation, out);
}



static void _orbit_pan_drag(DvzOrbitCamera* orbit, vec2 shift_px)
{
    ANN(orbit);
    float width = orbit->viewport_size[0];
    float height = orbit->viewport_size[1];
    if (width <= 0.0f || height <= 0.0f)
        return;
    orbit->pan[0] = orbit->pan_center[0] + 2.0f * shift_px[0] / width;
    orbit->pan[1] = orbit->pan_center[1] - 2.0f * shift_px[1] / height;
}



static bool _orbit_event_in_viewport(const DvzOrbitCamera* orbit, const DvzPointerEvent* ev)
{
    ANN(orbit);
    ANN(ev);
    if (!orbit->has_viewport)
        return true;
    if (ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
        return orbit->interacting;

    vec2 pos = {ev->pos[0], ev->pos[1]};
    if (ev->type == DVZ_POINTER_EVENT_DRAG && ev->content.d.is_press_valid)
    {
        pos[0] = ev->content.d.press_pos[0];
        pos[1] = ev->content.d.press_pos[1];
    }
    float x = orbit->viewport_origin[0];
    float y = orbit->viewport_origin[1];
    float w = orbit->viewport_size[0];
    float h = orbit->viewport_size[1];
    return pos[0] >= x && pos[0] < x + w && pos[1] >= y && pos[1] < y + h;
}



static void _orbit_local_event(
    const DvzOrbitCamera* orbit, const DvzPointerEvent* ev, DvzPointerEvent* out)
{
    ANN(orbit);
    ANN(ev);
    ANN(out);
    *out = *ev;
    if (!orbit->has_viewport)
        return;
    out->pos[0] -= orbit->viewport_origin[0];
    out->pos[1] -= orbit->viewport_origin[1];
    if (ev->type == DVZ_POINTER_EVENT_DRAG || ev->type == DVZ_POINTER_EVENT_DRAG_STOP)
    {
        out->content.d.last_pos[0] -= orbit->viewport_origin[0];
        out->content.d.last_pos[1] -= orbit->viewport_origin[1];
        out->content.d.press_pos[0] -= orbit->viewport_origin[0];
        out->content.d.press_pos[1] -= orbit->viewport_origin[1];
    }
}



static void _orbit_input_callback(DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
{
    (void)router;
    DvzOrbitCamera* orbit = (DvzOrbitCamera*)user_data;
    if (ev->type == DVZ_INPUT_EVENT_POINTER)
        dvz_orbit_camera_pointer(orbit, &ev->content.pointer);
    else if (ev->type == DVZ_INPUT_EVENT_RESIZE)
    {
        const DvzInputResizeEvent* r = &ev->content.resize;
        if (!orbit->has_viewport && r->window_width > 0 && r->window_height > 0)
            dvz_orbit_camera_resize(orbit, (float)r->window_width, (float)r->window_height);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzOrbitCameraDesc dvz_orbit_camera_desc(void)
{
    return (DvzOrbitCameraDesc){
        .width = DVZ_ORBIT_CAMERA_DEFAULT_WIDTH,
        .height = DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT,
        .flags = 0,
        .pivot = {0.0f, 0.0f, 0.0f},
    };
}



DvzOrbitCamera* dvz_orbit_camera_create(const DvzOrbitCameraDesc* desc)
{
    DvzOrbitCameraDesc default_desc = dvz_orbit_camera_desc();
    if (desc == NULL)
        desc = &default_desc;

    DvzOrbitCamera* orbit = (DvzOrbitCamera*)dvz_calloc(1, sizeof(DvzOrbitCamera));
    if (orbit == NULL)
        return NULL;
    orbit->flags = desc->flags;
    orbit->viewport_size[0] = desc->width > 0.0f ? desc->width : DVZ_ORBIT_CAMERA_DEFAULT_WIDTH;
    orbit->viewport_size[1] = desc->height > 0.0f ? desc->height : DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT;
    glm_vec3_copy((vec3){desc->pivot[0], desc->pivot[1], desc->pivot[2]}, orbit->pivot);
    glm_mat4_identity(orbit->rotation);
    glm_quat_identity(orbit->drag_rotation);
    orbit->distance = 3.0f;
    return orbit;
}



void dvz_orbit_camera_viewport(
    DvzOrbitCamera* orbit, float x, float y, float width, float height)
{
    ANN(orbit);
    orbit->viewport_origin[0] = x;
    orbit->viewport_origin[1] = y;
    orbit->viewport_size[0] = width > 0.0f ? width : DVZ_ORBIT_CAMERA_DEFAULT_WIDTH;
    orbit->viewport_size[1] = height > 0.0f ? height : DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT;
    orbit->has_viewport = true;
}



void dvz_orbit_camera_resize(DvzOrbitCamera* orbit, float width, float height)
{
    ANN(orbit);
    orbit->viewport_size[0] = width > 0.0f ? width : DVZ_ORBIT_CAMERA_DEFAULT_WIDTH;
    orbit->viewport_size[1] = height > 0.0f ? height : DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT;
}



void dvz_orbit_camera_pivot(DvzOrbitCamera* orbit, vec3 pivot)
{
    ANN(orbit);
    glm_vec3_copy(pivot, orbit->pivot);
    dvz_orbit_camera_apply_camera(orbit);
}



void dvz_orbit_camera_set_camera(DvzOrbitCamera* orbit, DvzCamera* camera)
{
    ANN(orbit);
    orbit->camera = camera;
    if (camera != NULL)
    {
        vec3 eye = {0}, target = {0}, up = {0};
        dvz_camera_get_view(camera, eye, target, up);
        vec3 delta = {0};
        glm_vec3_sub(eye, orbit->pivot, delta);
        float distance = glm_vec3_norm(delta);
        if (distance > 0.0f)
            orbit->distance = distance;
    }
    dvz_orbit_camera_apply_camera(orbit);
}



void dvz_orbit_camera_apply_camera(DvzOrbitCamera* orbit)
{
    ANN(orbit);
    if (orbit->camera == NULL)
        return;

    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    _orbit_current_rotation(orbit, rot);
    vec4 base_eye = {0.0f, 0.0f, orbit->distance, 1.0f};
    vec4 base_up = {0.0f, 1.0f, 0.0f, 0.0f};
    vec4 eye4 = {0};
    vec4 up4 = {0};
    glm_mat4_mulv(rot, base_eye, eye4);
    glm_mat4_mulv(rot, base_up, up4);

    vec3 eye = {orbit->pivot[0] + eye4[0], orbit->pivot[1] + eye4[1], orbit->pivot[2] + eye4[2]};
    vec3 up = {up4[0], up4[1], up4[2]};
    if (glm_vec3_norm(up) <= 0.0f)
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
    else
        glm_vec3_normalize(up);
    dvz_camera_set_view(orbit->camera, eye, orbit->pivot, up);
}



bool dvz_orbit_camera_pointer(DvzOrbitCamera* orbit, const DvzPointerEvent* ev)
{
    ANN(orbit);
    ANN(ev);
    if (!_orbit_event_in_viewport(orbit, ev))
        return false;

    DvzPointerEvent local = {0};
    _orbit_local_event(orbit, ev, &local);
    ev = &local;

    float width = orbit->viewport_size[0] > 0.0f ? orbit->viewport_size[0] :
                                                   DVZ_ORBIT_CAMERA_DEFAULT_WIDTH;
    float height = orbit->viewport_size[1] > 0.0f ? orbit->viewport_size[1] :
                                                    DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT;

    switch (ev->type)
    {
    case DVZ_POINTER_EVENT_PRESS:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
            orbit->interacting = true;
        return true;

    case DVZ_POINTER_EVENT_RELEASE:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
            orbit->interacting = false;
        return true;

    case DVZ_POINTER_EVENT_DRAG_START:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
        {
            if (ev->button == DVZ_POINTER_BUTTON_MIDDLE || ev->button == DVZ_POINTER_BUTTON_RIGHT)
                glm_vec2_copy(orbit->pan, orbit->pan_center);
            orbit->interacting = true;
        }
        return true;

    case DVZ_POINTER_EVENT_DRAG:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT)
        {
            vec2 cur_pos = {
                -1.0f + 2.0f * ev->pos[0] / width,
                +1.0f - 2.0f * ev->pos[1] / height,
            };
            vec2 last_pos = {
                -1.0f + 2.0f * ev->content.d.press_pos[0] / width,
                +1.0f - 2.0f * ev->content.d.press_pos[1] / height,
            };
            _orbit_drag_rotation(cur_pos, last_pos, orbit->drag_rotation);
            orbit->interacting = true;
            dvz_orbit_camera_apply_camera(orbit);
            return true;
        }
        if ((ev->button == DVZ_POINTER_BUTTON_MIDDLE || ev->button == DVZ_POINTER_BUTTON_RIGHT) &&
            ev->content.d.is_press_valid)
        {
            vec2 shift = {ev->content.d.shift[0], ev->content.d.shift[1]};
            _orbit_pan_drag(orbit, shift);
            orbit->interacting = true;
            return true;
        }
        break;

    case DVZ_POINTER_EVENT_DRAG_STOP:
    {
        mat4 rot = GLM_MAT4_IDENTITY_INIT;
        glm_quat_mat4(orbit->drag_rotation, rot);
        glm_mat4_mul(rot, orbit->rotation, orbit->rotation);
        glm_quat_identity(orbit->drag_rotation);
        glm_vec2_copy(orbit->pan, orbit->pan_center);
        orbit->interacting = false;
        dvz_orbit_camera_apply_camera(orbit);
        return true;
    }

    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        glm_mat4_identity(orbit->rotation);
        glm_quat_identity(orbit->drag_rotation);
        glm_vec2_zero(orbit->pan);
        glm_vec2_zero(orbit->pan_center);
        orbit->interacting = false;
        dvz_orbit_camera_apply_camera(orbit);
        return true;

    case DVZ_POINTER_EVENT_WHEEL:
        orbit->distance = _clampf(
            orbit->distance * expf(-DVZ_ORBIT_CAMERA_ZOOM_COEF * ev->content.w.dir[1]),
            DVZ_ORBIT_CAMERA_MIN_DISTANCE, DVZ_ORBIT_CAMERA_MAX_DISTANCE);
        dvz_orbit_camera_apply_camera(orbit);
        return true;

    default:
        return false;
    }
    return false;
}



bool dvz_orbit_camera_is_interacting(const DvzOrbitCamera* orbit)
{
    return orbit != NULL && orbit->interacting;
}



void dvz_orbit_camera_connect(DvzOrbitCamera* orbit, DvzInputRouter* router)
{
    ANN(orbit);
    ANN(router);
    DvzInputResizeEvent r;
    if (!orbit->has_viewport && dvz_input_router_last_resize(router, &r) &&
        r.window_width > 0 && r.window_height > 0)
        dvz_orbit_camera_resize(orbit, (float)r.window_width, (float)r.window_height);
    dvz_input_subscribe_event(router, _orbit_input_callback, orbit);
}



void dvz_orbit_camera_disconnect(DvzOrbitCamera* orbit, DvzInputRouter* router)
{
    ANN(orbit);
    ANN(router);
    dvz_input_unsubscribe_event(router, _orbit_input_callback, orbit);
}



void dvz_orbit_camera_destroy(DvzOrbitCamera* orbit)
{
    if (orbit == NULL)
        return;
    dvz_free(orbit);
}
