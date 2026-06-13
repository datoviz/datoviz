/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Arcball controller                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_controller.h"
#include "_log.h"
#include "datoviz/controller/arcball.h"
#include "datoviz/math/_cglm.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ARCBALL_ZOOM_MIN         0.01f
#define DVZ_ARCBALL_ZOOM_MAX         250.00f
#define DVZ_ARCBALL_DESC_KNOWN_FLAGS 0u

#if defined(__APPLE__)
#define DVZ_ARCBALL_ZOOM_WHEEL_COEF 0.0125f
#else
#define DVZ_ARCBALL_ZOOM_WHEEL_COEF 0.05f
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a float into a closed range.
 *
 * @param value input value
 * @param min_value minimum accepted value
 * @param max_value maximum accepted value
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



static void _screen_to_arcball(vec2 p, versor q)
{
    float dist = glm_vec2_dot(p, p);
    if (dist <= 1.0f)
    {
        glm_vec4_copy((vec4){p[0], p[1], sqrtf(1.0f - dist), 0.0f}, q);
    }
    else
    {
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], p[1], 0.0f, 0.0f}, q);
    }
}



static void _constrain(versor q, vec3 axis)
{
    glm_vec3_normalize(axis);
    float dot = glm_vec3_dot(q, axis);
    vec3 proj, t;
    glm_vec3_scale(axis, dot, t);
    glm_vec3_sub(q, t, proj);
    float norm = glm_vec3_norm(proj);
    if (norm > 0.0f)
    {
        float s = proj[2] >= 0.0f ? 1.0f / norm : -1.0f / norm;
        glm_vec3_scale(proj, s, q);
    }
    else if (axis[2] == 1.0f)
    {
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, q);
    }
    else
    {
        glm_vec3_normalize_to((vec3){-axis[1], axis[0], 0.0f}, q);
    }
}


/**
 * Return the current drag rotation in model/world coordinates.
 *
 * @param arcball arcball controller
 * @param out rotation matrix
 */
static void _arcball_drag_rotation_matrix(DvzArcball* arcball, mat4 out)
{
    ANN(arcball);
    ANN(out);

    glm_quat_mat4(arcball->rotation, out);
    if (!arcball->has_view)
        return;

    mat4 view_rot = GLM_MAT4_IDENTITY_INIT;
    for (uint32_t col = 0; col < 3; col++)
    {
        for (uint32_t row = 0; row < 3; row++)
            view_rot[col][row] = arcball->view[col][row];
    }

    mat4 inv_view_rot = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(view_rot, inv_view_rot);

    mat4 tmp = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_mul(inv_view_rot, out, tmp);
    glm_mat4_mul(tmp, view_rot, out);
}


/**
 * Apply a wheel zoom delta to an arcball.
 *
 * @param arcball arcball controller
 * @param dir wheel direction
 */
static void _arcball_zoom_wheel(DvzArcball* arcball, vec2 dir)
{
    ANN(arcball);
    if (dir[1] == 0.0f)
        return;
    dvz_arcball_zoom(arcball, arcball->zoom * expf(DVZ_ARCBALL_ZOOM_WHEEL_COEF * dir[1]));
}


/**
 * Apply a drag-relative panel-plane pan shift.
 *
 * @param arcball arcball controller
 * @param shift_px pointer shift from the drag press position in pixels
 */
static void _arcball_pan_drag(DvzArcball* arcball, vec2 shift_px)
{
    ANN(arcball);
    float width = arcball->viewport_size[0];
    float height = arcball->viewport_size[1];
    if (width <= 0.0f || height <= 0.0f)
        return;

    arcball->pan[0] = arcball->pan_center[0] + 2.0f * shift_px[0] / width;
    arcball->pan[1] = arcball->pan_center[1] - 2.0f * shift_px[1] / height;
}


/**
 * Apply camera pan/dolly to an MVP view matrix.
 *
 * @param arcball arcball controller
 * @param mvp target MVP
 */
static void _arcball_camera_mvp(DvzArcball* arcball, DvzMVP* mvp)
{
    ANN(arcball);
    ANN(mvp);

    mat4 translate = GLM_MAT4_IDENTITY_INIT;
    glm_translate_make(translate, (vec3){arcball->pan[0], arcball->pan[1], logf(arcball->zoom)});

    mat4 view = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_copy(mvp->view, view);
    glm_mat4_mul(translate, view, mvp->view);
}



/*************************************************************************************************/
/*  Input callback                                                                               */
/*                                                                                               */
/*  Subscribed to union events so gesture-derived events (DRAG, DRAG_STOP, DOUBLE_CLICK) are */
/*  delivered. The pointer-only stream skips those because the gesture handler emits them via    */
/*  dvz_input_emit_event.                                                                        */
/*************************************************************************************************/

static void
_arcball_input_callback(DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
{
    DvzArcball* arcball = (DvzArcball*)user_data;
    if (ev->type == DVZ_INPUT_EVENT_POINTER)
    {
        dvz_arcball_pointer(arcball, &ev->content.pointer);
    }
    else if (ev->type == DVZ_INPUT_EVENT_RESIZE)
    {
        const DvzInputResizeEvent* r = &ev->content.resize;
        if (r->window_width > 0 && r->window_height > 0)
            dvz_arcball_resize(arcball, (float)r->window_width, (float)r->window_height);
    }
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

static bool _arcball_desc_validate(const DvzArcballDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzArcballDesc, DVZ_ARCBALL_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzArcballDesc ABI prologue");
        return false;
    }
    return true;
}



/**
 * Return a default arcball descriptor.
 *
 * @return the arcball descriptor
 */
DvzArcballDesc dvz_arcball_desc(void)
{
    return (DvzArcballDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzArcballDesc),
        .width = 800.0f,
        .height = 600.0f,
        .controller_flags = 0,
    };
}



DvzArcball* _dvz_arcball(float width, float height, int flags)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzArcball* arcball = (DvzArcball*)dvz_calloc(1, sizeof(DvzArcball));
    arcball->flags = flags;
    arcball->viewport_size[0] = width;
    arcball->viewport_size[1] = height;
    glm_mat4_identity(arcball->view);
    dvz_arcball_reset(arcball);
    return arcball;
}


/**
 * Create a standalone arcball controller.
 *
 * @param desc arcball descriptor, or NULL for defaults
 * @return the arcball controller, or NULL on allocation failure
 */
DvzArcball* dvz_arcball_create(const DvzArcballDesc* desc)
{
    DvzArcballDesc default_desc = dvz_arcball_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_arcball_desc_validate(desc))
        return NULL;
    return _dvz_arcball(desc->width, desc->height, (int)desc->controller_flags);
}


/**
 * Set the camera view matrix used to interpret drag axes.
 *
 * @param arcball arcball controller
 * @param view camera view matrix
 */
void _dvz_arcball_view(DvzArcball* arcball, mat4 view)
{
    ANN(arcball);
    glm_mat4_copy(view, arcball->view);
    arcball->has_view = true;
}


/**
 * Clear the camera view matrix used to interpret drag axes.
 *
 * @param arcball arcball controller
 */
void _dvz_arcball_clear_view(DvzArcball* arcball)
{
    ANN(arcball);
    glm_mat4_identity(arcball->view);
    arcball->has_view = false;
}



void dvz_arcball_initial(DvzArcball* arcball, vec3 angles)
{
    ANN(arcball);
    glm_vec3_copy(angles, arcball->init);
    dvz_arcball_reset(arcball);
}



void dvz_arcball_reset(DvzArcball* arcball)
{
    ANN(arcball);
    dvz_arcball_set(arcball, arcball->init);
    glm_quat_identity(arcball->rotation);
    arcball->zoom = 1.0f;
    glm_vec2_zero(arcball->pan);
    glm_vec2_zero(arcball->pan_center);
    arcball->interacting = false;
}



void dvz_arcball_set(DvzArcball* arcball, vec3 angles)
{
    ANN(arcball);
    glm_euler(angles, arcball->mat);
    glm_quat_identity(arcball->rotation);
}



/**
 * Apply an incremental rotation around an axis to the accumulated orientation.
 *
 * @param arcball arcball controller
 * @param angle rotation angle in radians
 * @param axis rotation axis
 */
void dvz_arcball_rotate_axis(DvzArcball* arcball, float angle, vec3 axis)
{
    ANN(arcball);
    if (angle == 0.0f)
        return;
    if (glm_vec3_norm(axis) == 0.0f)
    {
        log_warn("null arcball rotation axis, ignoring");
        return;
    }

    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_make(rot, angle, axis);
    glm_mat4_mul(rot, arcball->mat, arcball->mat);
}


/**
 * Set the uniform zoom factor.
 *
 * @param arcball arcball controller
 * @param zoom uniform zoom factor
 */
void dvz_arcball_zoom(DvzArcball* arcball, float zoom)
{
    ANN(arcball);
    arcball->zoom = _clampf(zoom, DVZ_ARCBALL_ZOOM_MIN, DVZ_ARCBALL_ZOOM_MAX);
}


/**
 * Set the panel-plane pan offset.
 *
 * @param arcball arcball controller
 * @param pan panel-plane pan offset
 */
void dvz_arcball_pan(DvzArcball* arcball, vec2 pan)
{
    ANN(arcball);
    glm_vec2_copy(pan, arcball->pan);
    glm_vec2_copy(pan, arcball->pan_center);
}


/**
 * Apply an incremental panel-plane pan shift in pixels.
 *
 * @param arcball arcball controller
 * @param shift_px shift in viewport pixels
 */
void dvz_arcball_pan_shift(DvzArcball* arcball, vec2 shift_px)
{
    ANN(arcball);
    glm_vec2_copy(arcball->pan, arcball->pan_center);
    _arcball_pan_drag(arcball, shift_px);
    glm_vec2_copy(arcball->pan, arcball->pan_center);
}



void dvz_arcball_resize(DvzArcball* arcball, float width, float height)
{
    ANN(arcball);
    arcball->viewport_size[0] = width;
    arcball->viewport_size[1] = height;
}



void dvz_arcball_constrain(DvzArcball* arcball, vec3 axis)
{
    ANN(arcball);
    if (glm_vec3_norm(axis) == 0.0f)
    {
        log_warn("null arcball constrain axis, ignoring");
        return;
    }
    glm_vec3_normalize_to(axis, arcball->constrain);
    arcball->flags |= DVZ_ARCBALL_FLAGS_CONSTRAIN;
}



void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles)
{
    ANN(arcball);
    glm_euler_angles(arcball->mat, out_angles);
}



void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos)
{
    ANN(arcball);

    versor cur_ball = {0}, prev_ball = {0};
    _screen_to_arcball(cur_pos, cur_ball);
    _screen_to_arcball(last_pos, prev_ball);

    if ((arcball->flags & DVZ_ARCBALL_FLAGS_CONSTRAIN) != 0)
    {
        _constrain(cur_ball, arcball->constrain);
        _constrain(prev_ball, arcball->constrain);
    }

    vec3 axis = {0};
    glm_vec3_cross(prev_ball, cur_ball, axis);
    float dot = _clampf(glm_vec3_dot(prev_ball, cur_ball), -1.0f, 1.0f);
    arcball->rotation[0] = axis[0];
    arcball->rotation[1] = axis[1];
    arcball->rotation[2] = axis[2];
    arcball->rotation[3] = 1.0f + dot;
    if (arcball->rotation[3] <= 1e-6f && glm_vec3_norm(axis) <= 1e-6f)
    {
        glm_vec3_cross(prev_ball, (vec3){1.0f, 0.0f, 0.0f}, axis);
        if (glm_vec3_norm(axis) <= 1e-6f)
            glm_vec3_cross(prev_ball, (vec3){0.0f, 1.0f, 0.0f}, axis);
        arcball->rotation[0] = axis[0];
        arcball->rotation[1] = axis[1];
        arcball->rotation[2] = axis[2];
        arcball->rotation[3] = 0.0f;
    }
    glm_quat_normalize(arcball->rotation);
}



void dvz_arcball_model(DvzArcball* arcball, mat4 model)
{
    ANN(arcball);
    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    _arcball_drag_rotation_matrix(arcball, rot);
    glm_mat4_mul(rot, arcball->mat, model);
}



void dvz_arcball_end(DvzArcball* arcball)
{
    ANN(arcball);
    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    _arcball_drag_rotation_matrix(arcball, rot);
    glm_mat4_mul(rot, arcball->mat, arcball->mat);
    glm_quat_identity(arcball->rotation);
}



void dvz_arcball_mvp(DvzArcball* arcball, DvzMVP* mvp)
{
    ANN(arcball);
    ANN(mvp);
    dvz_arcball_model(arcball, mvp->model);
    _arcball_camera_mvp(arcball, mvp);
}



/**
 * Return whether the pointer is currently interacting with the arcball.
 *
 * @param arcball arcball controller
 * @return true while the user is pressing or dragging the arcball
 */
bool dvz_arcball_is_interacting(DvzArcball* arcball)
{
    if (arcball == NULL)
        return false;
    return arcball->interacting;
}



bool dvz_arcball_pointer(DvzArcball* arcball, const DvzPointerEvent* ev)
{
    ANN(arcball);
    ANN(ev);

    float width = arcball->viewport_size[0];
    float height = arcball->viewport_size[1];

    switch (ev->type)
    {
    case DVZ_POINTER_EVENT_PRESS:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
            arcball->interacting = true;
        break;

    case DVZ_POINTER_EVENT_RELEASE:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
            arcball->interacting = false;
        break;

    case DVZ_POINTER_EVENT_DRAG_START:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT || ev->button == DVZ_POINTER_BUTTON_MIDDLE ||
            ev->button == DVZ_POINTER_BUTTON_RIGHT)
        {
            if (ev->button == DVZ_POINTER_BUTTON_MIDDLE || ev->button == DVZ_POINTER_BUTTON_RIGHT)
                glm_vec2_copy(arcball->pan, arcball->pan_center);
            arcball->interacting = true;
        }
        break;

    case DVZ_POINTER_EVENT_DRAG:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT)
        {
            arcball->interacting = true;
            vec2 cur_pos = {
                -1.0f + 2.0f * ev->pos[0] / width,
                +1.0f - 2.0f * ev->pos[1] / height,
            };
            vec2 last_pos = {
                -1.0f + 2.0f * ev->content.d.press_pos[0] / width,
                +1.0f - 2.0f * ev->content.d.press_pos[1] / height,
            };
            dvz_arcball_rotate(arcball, cur_pos, last_pos);
        }
        else if (
            (ev->button == DVZ_POINTER_BUTTON_MIDDLE || ev->button == DVZ_POINTER_BUTTON_RIGHT) &&
            ev->content.d.is_press_valid)
        {
            vec2 shift = {ev->content.d.shift[0], ev->content.d.shift[1]};
            _arcball_pan_drag(arcball, shift);
            arcball->interacting = true;
        }
        break;

    case DVZ_POINTER_EVENT_DRAG_STOP:
        dvz_arcball_end(arcball);
        glm_vec2_copy(arcball->pan, arcball->pan_center);
        arcball->interacting = false;
        break;

    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        dvz_arcball_reset(arcball);
        break;

    case DVZ_POINTER_EVENT_WHEEL:
    {
        vec2 dir = {ev->content.w.dir[0], ev->content.w.dir[1]};
        _arcball_zoom_wheel(arcball, dir);
        break;
    }

    default:
        return false;
    }

    return true;
}



void dvz_arcball_connect(DvzArcball* arcball, DvzInputRouter* router)
{
    ANN(arcball);
    ANN(router);
    DvzInputResizeEvent r;
    if (dvz_input_router_last_resize(router, &r) && r.window_width > 0 && r.window_height > 0)
        dvz_arcball_resize(arcball, (float)r.window_width, (float)r.window_height);
    dvz_input_subscribe_event(router, _arcball_input_callback, arcball);
}



void dvz_arcball_disconnect(DvzArcball* arcball, DvzInputRouter* router)
{
    ANN(arcball);
    ANN(router);
    dvz_input_unsubscribe_event(router, _arcball_input_callback, arcball);
}



void dvz_arcball_destroy(DvzArcball* arcball)
{
    ANN(arcball);
    dvz_free(arcball);
}
