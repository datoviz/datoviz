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
#include "_log.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/controller/orbit_camera.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ORBIT_CAMERA_DEFAULT_WIDTH  800.0f
#define DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT 600.0f
#define DVZ_ORBIT_CAMERA_ZOOM_COEF      0.05f
#define DVZ_ORBIT_CAMERA_MIN_DISTANCE   0.01f
#define DVZ_ORBIT_CAMERA_MAX_DISTANCE   100000.0f
#define DVZ_ORBIT_CAMERA_DESC_KNOWN_FLAGS 0u



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
    vec3 initial_pivot;
    mat4 initial_rotation;
    float initial_distance;
    vec2 initial_pan;

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



static void _orbit_rotation_axis(const mat4 rotation, uint32_t column, vec3 out)
{
    ANN(out);
    if (column >= 3)
    {
        glm_vec3_zero(out);
        return;
    }

    for (uint32_t row = 0; row < 3; row++)
        out[row] = rotation[column][row];
    if (glm_vec3_norm(out) > 0.0f)
        glm_vec3_normalize(out);
}



static void _orbit_drag_rotation(
    const DvzOrbitCamera* orbit, vec2 shift_px, float width, float height, versor out)
{
    ANN(orbit);
    ANN(out);

    if (width <= 0.0f || height <= 0.0f)
    {
        glm_quat_identity(out);
        return;
    }

    vec3 up_axis = {0};
    _orbit_rotation_axis(orbit->initial_rotation, 1, up_axis);
    if (glm_vec3_norm(up_axis) <= 0.0f)
        _orbit_rotation_axis(orbit->rotation, 1, up_axis);
    if (glm_vec3_norm(up_axis) <= 0.0f)
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up_axis);
    else
        glm_vec3_normalize(up_axis);

    vec3 right_axis = {0};
    _orbit_rotation_axis(orbit->rotation, 0, right_axis);
    if (glm_vec3_norm(right_axis) <= 0.0f)
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, right_axis);

    float yaw_angle = -(shift_px[0] / width) * GLM_PIf;
    float pitch_angle = -(shift_px[1] / height) * GLM_PIf;

    versor yaw = GLM_QUAT_IDENTITY_INIT;
    glm_quatv(yaw, yaw_angle, up_axis);
    glm_quat_rotatev(yaw, right_axis, right_axis);
    if (glm_vec3_norm(right_axis) <= 0.0f)
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, right_axis);
    else
        glm_vec3_normalize(right_axis);

    versor pitch = GLM_QUAT_IDENTITY_INIT;
    glm_quatv(pitch, pitch_angle, right_axis);

    glm_quat_mul(pitch, yaw, out);
    glm_quat_normalize(out);
}



static void _orbit_current_rotation(const DvzOrbitCamera* orbit, mat4 out)
{
    ANN(orbit);
    ANN(out);
    versor drag_q = {0};
    for (uint32_t i = 0; i < 4; i++)
        drag_q[i] = orbit->drag_rotation[i];
    mat4 drag = GLM_MAT4_IDENTITY_INIT;
    mat4 rotation = GLM_MAT4_IDENTITY_INIT;
    for (uint32_t col = 0; col < 4; col++)
        for (uint32_t row = 0; row < 4; row++)
            rotation[col][row] = orbit->rotation[col][row];
    glm_quat_mat4(drag_q, drag);
    glm_mat4_mul(drag, rotation, out);
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



static void _orbit_view(const DvzOrbitCamera* orbit, vec3 eye, vec3 target, vec3 up)
{
    ANN(orbit);
    ANN(eye);
    ANN(target);
    ANN(up);

    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    _orbit_current_rotation(orbit, rot);
    vec4 base_eye = {0.0f, 0.0f, orbit->distance, 1.0f};
    vec4 base_up = {0.0f, 1.0f, 0.0f, 0.0f};
    vec4 eye4 = {0};
    vec4 up4 = {0};
    glm_mat4_mulv(rot, base_eye, eye4);
    glm_mat4_mulv(rot, base_up, up4);

    for (uint32_t i = 0; i < 3; i++)
    {
        target[i] = orbit->pivot[i];
        eye[i] = orbit->pivot[i] + eye4[i];
        up[i] = up4[i];
    }
    if (glm_vec3_norm(up) <= 0.0f)
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
    else
        glm_vec3_normalize(up);
}



static void _orbit_store_initial(DvzOrbitCamera* orbit)
{
    ANN(orbit);
    glm_vec3_copy(orbit->pivot, orbit->initial_pivot);
    glm_mat4_copy(orbit->rotation, orbit->initial_rotation);
    orbit->initial_distance = orbit->distance;
    glm_vec2_copy(orbit->pan, orbit->initial_pan);
}



static void _orbit_reset_initial(DvzOrbitCamera* orbit)
{
    ANN(orbit);
    glm_vec3_copy(orbit->initial_pivot, orbit->pivot);
    glm_mat4_copy(orbit->initial_rotation, orbit->rotation);
    orbit->distance = orbit->initial_distance;
    glm_vec2_copy(orbit->initial_pan, orbit->pan);
    glm_vec2_copy(orbit->initial_pan, orbit->pan_center);
    glm_quat_identity(orbit->drag_rotation);
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

static bool _orbit_camera_desc_validate(const DvzOrbitCameraDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzOrbitCameraDesc, DVZ_ORBIT_CAMERA_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzOrbitCameraDesc ABI prologue");
        return false;
    }
    return true;
}



DvzOrbitCameraDesc dvz_orbit_camera_desc(void)
{
    return (DvzOrbitCameraDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzOrbitCameraDesc),
        .width = DVZ_ORBIT_CAMERA_DEFAULT_WIDTH,
        .height = DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT,
        .controller_flags = 0,
        .pivot = {0.0f, 0.0f, 0.0f},
    };
}



DvzOrbitCamera* dvz_orbit_camera_create(const DvzOrbitCameraDesc* desc)
{
    DvzOrbitCameraDesc default_desc = dvz_orbit_camera_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_orbit_camera_desc_validate(desc))
        return NULL;

    DvzOrbitCamera* orbit = (DvzOrbitCamera*)dvz_calloc(1, sizeof(DvzOrbitCamera));
    if (orbit == NULL)
        return NULL;
    orbit->flags = (int)desc->controller_flags;
    orbit->viewport_size[0] = desc->width > 0.0f ? desc->width : DVZ_ORBIT_CAMERA_DEFAULT_WIDTH;
    orbit->viewport_size[1] = desc->height > 0.0f ? desc->height : DVZ_ORBIT_CAMERA_DEFAULT_HEIGHT;
    glm_vec3_copy((vec3){desc->pivot[0], desc->pivot[1], desc->pivot[2]}, orbit->pivot);
    glm_mat4_identity(orbit->rotation);
    glm_quat_identity(orbit->drag_rotation);
    orbit->distance = 3.0f;
    _orbit_store_initial(orbit);
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
    glm_vec3_copy(pivot, orbit->initial_pivot);
    dvz_orbit_camera_apply_camera(orbit);
}



int dvz_orbit_camera_get_pivot(const DvzOrbitCamera* orbit, vec3 out)
{
    ANN(orbit);
    ANN(out);
    for (uint32_t i = 0; i < 3; i++)
        out[i] = orbit->pivot[i];
    return 0;
}



float dvz_orbit_camera_get_distance(const DvzOrbitCamera* orbit)
{
    ANN(orbit);
    return orbit->distance;
}



int dvz_orbit_camera_get_view(const DvzOrbitCamera* orbit, vec3 eye, vec3 target, vec3 up)
{
    ANN(orbit);
    _orbit_view(orbit, eye, target, up);
    return 0;
}



void dvz_orbit_camera_set_camera(DvzOrbitCamera* orbit, DvzCamera* camera)
{
    ANN(orbit);
    _dvz_orbit_camera_sync_camera(orbit, camera);
    if (camera != NULL)
        _orbit_store_initial(orbit);
}



void _dvz_orbit_camera_sync_camera(DvzOrbitCamera* orbit, DvzCamera* camera)
{
    ANN(orbit);
    _dvz_orbit_camera_attach_camera(orbit, camera);
    if (camera != NULL)
    {
        vec3 eye = {0}, target = {0}, up = {0};
        dvz_camera_get_view(camera, eye, target, up);
        vec3 delta = {0};
        glm_vec3_sub(eye, orbit->pivot, delta);
        float distance = glm_vec3_norm(delta);
        if (distance > 0.0f)
        {
            orbit->distance = distance;
            vec3 forward = {0};
            glm_vec3_scale(delta, 1.0f / distance, forward);
            if (glm_vec3_norm(up) <= 0.0f)
                glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
            else
                glm_vec3_normalize(up);
            vec3 right = {0};
            glm_vec3_cross(up, forward, right);
            if (glm_vec3_norm(right) <= 0.0f)
                glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, right);
            else
                glm_vec3_normalize(right);
            glm_vec3_cross(forward, right, up);
            glm_vec3_normalize(up);
            glm_mat4_identity(orbit->rotation);
            for (uint32_t row = 0; row < 3; row++)
            {
                orbit->rotation[0][row] = right[row];
                orbit->rotation[1][row] = up[row];
                orbit->rotation[2][row] = forward[row];
            }
        }
        glm_quat_identity(orbit->drag_rotation);
    }
    dvz_orbit_camera_apply_camera(orbit);
}



void _dvz_orbit_camera_attach_camera(DvzOrbitCamera* orbit, DvzCamera* camera)
{
    ANN(orbit);
    orbit->camera = camera;
}



void dvz_orbit_camera_apply_camera(DvzOrbitCamera* orbit)
{
    ANN(orbit);
    if (orbit->camera == NULL)
        return;

    vec3 eye = {0}, target = {0}, up = {0};
    _orbit_view(orbit, eye, target, up);
    dvz_camera_set_view(orbit->camera, eye, target, up);
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
            vec2 shift = {
                ev->pos[0] - ev->content.d.press_pos[0],
                ev->pos[1] - ev->content.d.press_pos[1],
            };
            _orbit_drag_rotation(orbit, shift, width, height, orbit->drag_rotation);
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
        _orbit_reset_initial(orbit);
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
