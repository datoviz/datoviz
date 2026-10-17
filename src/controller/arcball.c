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
#include "controller_internal.h"
#include "datoviz/math/_cglm.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ARCBALL_ZOOM_MIN         0.001f
#define DVZ_ARCBALL_ZOOM_MAX         1000.00f
#define DVZ_ARCBALL_DESC_KNOWN_FLAGS 0u
#define DVZ_ARCBALL_DEFAULT_ZOOM_SPEED 2.0f
#define DVZ_ARCBALL_ADAPTIVE_FRAMING 0.05f
#define DVZ_ARCBALL_ADAPTIVE_DEPTH_FLOOR 0.1f

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
    if (!arcball->has_view && !arcball->orbit_active)
        return;
    mat4* basis = arcball->orbit_active ? &arcball->orbit_view : &arcball->view;

    mat4 view_rot = GLM_MAT4_IDENTITY_INIT;
    for (uint32_t col = 0; col < 3; col++)
    {
        for (uint32_t row = 0; row < 3; row++)
            view_rot[col][row] = (*basis)[col][row];
    }

    mat4 inv_view_rot = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(view_rot, inv_view_rot);

    mat4 tmp = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_mul(inv_view_rot, out, tmp);
    glm_mat4_mul(tmp, view_rot, out);
}


/**
 * Evaluate the world translation for the current pointer orbit.
 *
 * @param arcball arcball controller
 * @param out current world translation
 */
void _dvz_arcball_orbit_translation(DvzArcball* arcball, vec3 out)
{
    glm_vec3_copy(arcball->orbit_translation, out);
    if (!arcball->orbit_active)
        return;
    mat4 delta = GLM_MAT4_IDENTITY_INIT;
    _arcball_drag_rotation_matrix(arcball, delta);
    vec3 offset = {0};
    glm_vec3_sub(out, arcball->orbit_pivot, offset);
    glm_mat4_mulv3(delta, offset, 0.0f, out);
    glm_vec3_add(out, arcball->orbit_pivot, out);
}



/**
 * Set the live world translation while preserving an active pointer rotation baseline.
 *
 * @param arcball arcball controller
 * @param translation requested live world translation
 */
void _dvz_arcball_orbit_translation_set(DvzArcball* arcball, vec3 translation)
{
    if (!arcball->orbit_active)
    {
        glm_vec3_copy(translation, arcball->orbit_translation);
        return;
    }
    mat4 delta = GLM_MAT4_IDENTITY_INIT;
    mat4 inverse = GLM_MAT4_IDENTITY_INIT;
    _arcball_drag_rotation_matrix(arcball, delta);
    glm_mat4_inv(delta, inverse);
    vec3 offset = {0};
    glm_vec3_sub(translation, arcball->orbit_pivot, offset);
    glm_mat4_mulv3(inverse, offset, 0.0f, arcball->orbit_translation);
    glm_vec3_add(arcball->orbit_translation, arcball->orbit_pivot, arcball->orbit_translation);
}



/**
 * Apply normalized image framing to a camera projection.
 *
 * @param arcball screen-pan controller
 * @param source camera projection
 * @param out shifted projection; may alias source
 */
static void _arcball_screen_projection(DvzArcball* arcball, mat4 source, mat4 out)
{
    glm_mat4_copy(source, out);
    for (uint32_t col = 0; col < 4; col++)
    {
        out[col][0] += arcball->pan[0] * out[col][3];
        out[col][1] += arcball->pan[1] * out[col][3];
        if (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM)
        {
            out[col][0] *= arcball->zoom;
            out[col][1] *= arcball->zoom;
        }
    }
}



/**
 * Select a zoom-dependent pivot without changing the committed rendered pose.
 *
 * @param arcball adaptive screen-pan controller
 */
static void _arcball_adaptive_pivot(DvzArcball* arcball)
{
    // The anatomical origin moves after a local orbit; never reuse the previous local pivot.
    glm_vec3_copy(arcball->orbit_translation, arcball->orbit_pivot);
    if (arcball->pan[0] == 0 && arcball->pan[1] == 0 &&
        arcball->zoom_offset[0] == 0 && arcball->zoom_offset[1] == 0)
        return;
    float reference_depth = -arcball->view[3][2];
    if (!isfinite(reference_depth) || reference_depth <= 1e-6f ||
        fabsf(arcball->projection[2][3]) <= 1e-6f)
        return;
    float start = 1.0f - 1.0f / arcball->orbit_zoom_start;
    float end = 1.0f - 1.0f / arcball->orbit_zoom_end;
    if (end <= start)
        return;
    bool projection_zoom = (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM) != 0;
    float travel = projection_zoom ? 1.0f - 1.0f / arcball->zoom
                                   : logf(arcball->zoom) / reference_depth;
    float u = _clampf((travel - start) / (end - start), 0, 1);
    float weight = u * u * (3 - 2 * u);
    // Taper tiny framing changes to the exact unpanned policy, including at deep zoom.
    float offset_x = arcball->projection[0][0] * arcball->zoom_offset[0] +
                     arcball->projection[1][0] * arcball->zoom_offset[1];
    float offset_y = arcball->projection[0][1] * arcball->zoom_offset[0] +
                     arcball->projection[1][1] * arcball->zoom_offset[1];
    float framing = projection_zoom ? arcball->zoom * hypotf(arcball->pan[0], arcball->pan[1])
                                    : hypotf(hypotf(arcball->pan[0], arcball->pan[1]),
                                             hypotf(offset_x, offset_y) / reference_depth);
    if (!isfinite(framing))
        return;
    float activation = _clampf(framing / DVZ_ARCBALL_ADAPTIVE_FRAMING, 0, 1);
    weight *= activation * activation * (3 - 2 * activation);
    if (weight == 0)
        return;

    mat4 view = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_copy(arcball->view, view);
    if (!projection_zoom)
    {
        view[3][0] += arcball->zoom_offset[0];
        view[3][1] += arcball->zoom_offset[1];
        view[3][2] += logf(arcball->zoom);
    }
    mat4 projection = GLM_MAT4_IDENTITY_INIT;
    _arcball_screen_projection(arcball, arcball->projection, projection);
    float view_det = glm_mat4_det(view);
    float proj_det = glm_mat4_det(projection);
    if (!isfinite(view_det) || !isfinite(proj_det) ||
        fabsf(view_det) < 1e-12f || fabsf(proj_det) < 1e-12f)
        return;
    mat4 inverse = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(projection, inverse);
    vec4 near = {0}, far = {0};
    glm_mat4_mulv(inverse, (vec4){0, 0, -1, 1}, near);
    glm_mat4_mulv(inverse, (vec4){0, 0, 0.5f, 1}, far);
    if (!isfinite(near[3]) || !isfinite(far[3]) ||
        fabsf(near[3]) < 1e-12f || fabsf(far[3]) < 1e-12f)
        return;
    glm_vec4_scale(near, 1.0f / near[3], near);
    glm_vec4_scale(far, 1.0f / far[3], far);
    float dz = far[2] - near[2];
    if (!isfinite(dz) || fabsf(dz) < 1e-12f)
        return;
    vec4 eye = {0};
    glm_mat4_mulv3(view, arcball->orbit_translation, 1.0f, eye);
    // C1 positive floor: preserve the anatomical depth exactly above twice the floor.
    // This is a stable inspection plane, independent of translucent surface picking.
    float floor_depth = fmaxf(DVZ_ARCBALL_ADAPTIVE_DEPTH_FLOOR * reference_depth, -2.0f * near[2]);
    float depth = -eye[2];
    if (!isfinite(depth) || !isfinite(floor_depth) || floor_depth <= 0)
        return;
    if (depth < 2 * floor_depth)
        depth = depth <= 0 ? floor_depth : floor_depth + depth * depth / (4 * floor_depth);
    float t = (-depth - near[2]) / dz;
    for (uint32_t i = 0; i < 3; i++)
        eye[i] = near[i] + t * (far[i] - near[i]);
    eye[3] = 1;
    glm_mat4_inv(view, inverse);
    vec4 center = {0};
    glm_mat4_mulv(inverse, eye, center);
    if (!isfinite(center[3]) || fabsf(center[3]) < 1e-12f)
        return;
    vec3 pivot = {0};
    for (uint32_t i = 0; i < 3; i++)
    {
        pivot[i] = arcball->orbit_translation[i] +
                   weight * (center[i] / center[3] - arcball->orbit_translation[i]);
        if (!isfinite(pivot[i]))
            return;
    }
    glm_vec3_copy(pivot, arcball->orbit_pivot);
}



/**
 * Freeze pointer controls, optionally rebasing the physical target for viewport orbit.
 *
 * @param arcball arcball controller
 * @return true when the camera matrices admit a finite center target
 */
static bool _arcball_orbit_begin(DvzArcball* arcball)
{
    glm_vec2_zero(arcball->orbit_center);
    if ((arcball->flags & DVZ_ARCBALL_FLAGS_VIEWPORT_ORBIT) == 0)
    {
        if (arcball->flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN)
            glm_vec3_copy(arcball->orbit_translation, arcball->orbit_pivot);
        if (arcball->flags & DVZ_ARCBALL_FLAGS_ADAPTIVE_ORBIT)
            _arcball_adaptive_pivot(arcball);
        // Keep the original pointer mapping, excluding only view-plane pan.
        bool projection_zoom = (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM) != 0;
        vec4 eye = {arcball->view[3][0], arcball->view[3][1],
                    arcball->view[3][2] + (projection_zoom ? 0 : logf(arcball->zoom)),
                    arcball->view[3][3]};
        vec4 clip = {0};
        glm_mat4_mulv(arcball->projection, eye, clip);
        if (clip[3] > 1e-6f)
        {
            float scale = projection_zoom ? arcball->zoom : 1;
            arcball->orbit_center[0] = scale * clip[0] / clip[3];
            arcball->orbit_center[1] = scale * clip[1] / clip[3];
        }
        glm_mat4_copy(arcball->view, arcball->orbit_view);
        arcball->orbit_active = true;
        return true;
    }
    mat4 view = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_copy(arcball->view, view);
    view[3][0] += arcball->pan[0];
    view[3][1] += arcball->pan[1];
    view[3][2] += logf(arcball->zoom);
    if (fabsf(glm_mat4_det(view)) < 1e-12f || fabsf(glm_mat4_det(arcball->projection)) < 1e-12f)
        return false;

    vec4 eye = {0}, clip = {0};
    glm_mat4_mulv3(view, arcball->orbit_pivot, 1.0f, eye);
    eye[3] = 1.0f;
    glm_mat4_mulv(arcball->projection, eye, clip);
    if (!isfinite(clip[3]) || fabsf(clip[3]) < 1e-12f)
        return false;
    // Intersect the center ray with the retained eye-depth plane directly.
    // Reusing projected depth loses precision for perspective cameras with a small near plane.
    mat4 inverse = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(arcball->projection, inverse);
    vec4 near = {0}, far = {0};
    glm_mat4_mulv(inverse, (vec4){0, 0, 0, 1}, near);
    glm_mat4_mulv(inverse, (vec4){0, 0, 0.5f, 1}, far);
    if (fabsf(near[3]) < 1e-12f || fabsf(far[3]) < 1e-12f)
        return false;
    glm_vec4_scale(near, 1.0f / near[3], near);
    glm_vec4_scale(far, 1.0f / far[3], far);
    float dz = far[2] - near[2];
    if (!isfinite(dz) || fabsf(dz) < 1e-12f)
        return false;
    float t = (eye[2] - near[2]) / dz;
    for (uint32_t i = 0; i < 3; i++)
        eye[i] = near[i] + t * (far[i] - near[i]);
    eye[3] = 1.0f;
    vec4 center = {0};
    glm_mat4_inv(view, inverse);
    glm_mat4_mulv(inverse, eye, center);
    if (!isfinite(center[3]) || fabsf(center[3]) < 1e-12f)
        return false;
    vec3 pivot = {0};
    for (uint32_t i = 0; i < 3; i++)
    {
        pivot[i] = center[i] / center[3];
        if (!isfinite(pivot[i]))
            return false;
    }
    glm_vec3_copy(pivot, arcball->orbit_pivot);
    glm_mat4_copy(arcball->view, arcball->orbit_view);
    arcball->orbit_active = true;
    return true;
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
    float old_zoom = arcball->zoom;
    float gain = DVZ_ARCBALL_ZOOM_WHEEL_COEF * arcball->zoom_speed;
    if (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM)
        gain /= 3.0f;
    (void)dvz_arcball_zoom(arcball, arcball->zoom * expf(gain * dir[1]));
    if (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM)
        return;
    if (!(arcball->flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN) || arcball->zoom == old_zoom)
        return;

    // Dolly along the current viewport-center ray, including the image framing offset.
    // Moving only along eye Z would instead zoom toward the pre-pan optical center.
    mat4 projection = GLM_MAT4_IDENTITY_INIT;
    _arcball_screen_projection(arcball, arcball->projection, projection);
    float determinant = glm_mat4_det(projection);
    if (!isfinite(determinant) || fabsf(determinant) < 1e-12f)
        return;
    mat4 inverse = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(projection, inverse);
    vec4 near = {0}, far = {0};
    glm_mat4_mulv(inverse, (vec4){0, 0, -0.5f, 1}, near);
    glm_mat4_mulv(inverse, (vec4){0, 0, 0.5f, 1}, far);
    if (!isfinite(near[3]) || !isfinite(far[3]) || fabsf(near[3]) < 1e-12f ||
        fabsf(far[3]) < 1e-12f)
        return;
    glm_vec4_scale(near, 1.0f / near[3], near);
    glm_vec4_scale(far, 1.0f / far[3], far);
    float dz = far[2] - near[2];
    if (!isfinite(dz) || fabsf(dz) < 1e-12f)
        return;
    float travel = logf(arcball->zoom) - logf(old_zoom);
    vec2 offset = {0};
    for (uint32_t i = 0; i < 2; i++)
    {
        offset[i] = arcball->zoom_offset[i] + travel * (far[i] - near[i]) / dz;
        if (!isfinite(offset[i]))
            return;
    }
    glm_vec2_copy(offset, arcball->zoom_offset);
}


/**
 * Project the rotation pivot after the current camera pan and dolly.
 *
 * @param arcball arcball controller
 * @param clip output homogeneous clip coordinates
 */
static void _arcball_pivot_clip(DvzArcball* arcball, vec4 clip)
{
    vec4 pivot = {0};
    glm_mat4_mulv3(arcball->view, arcball->orbit_pivot, 1.0f, pivot);
    pivot[0] += arcball->pan[0];
    pivot[1] += arcball->pan[1];
    pivot[2] += logf(arcball->zoom);
    pivot[3] = 1.0f;
    glm_mat4_mulv(arcball->projection, pivot, clip);
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

    if (arcball->flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN)
    {
        float scale = (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM) ? arcball->zoom : 1;
        arcball->pan[0] = arcball->pan_center[0] + 2.0f * shift_px[0] / (width * scale);
        arcball->pan[1] = arcball->pan_center[1] - 2.0f * shift_px[1] / (height * scale);
        return;
    }

    glm_vec2_copy(shift_px, arcball->pan_pending_shift);
    if (arcball->pan_depth_pending)
        return;
    if (arcball->pan_plane_active)
    {
        arcball->pan[0] = arcball->pan_center[0] +
                          arcball->pan_depth * arcball->pan_pixel_scale[0] * shift_px[0];
        arcball->pan[1] = arcball->pan_center[1] -
                          arcball->pan_depth * arcball->pan_pixel_scale[1] * shift_px[1];
        return;
    }

    // Unproject pixel displacements at the arcball's rotation centre. Dolly
    // changes its eye-space depth, so a fixed view-space shift accelerates
    // panning on screen as the camera approaches the object.
    vec4 clip = {0};
    _arcball_pivot_clip(arcball, clip);
    float depth = fmaxf(clip[3], 1e-6f);
    float sx = fabsf(arcball->projection[0][0]);
    float sy = fabsf(arcball->projection[1][1]);
    if (sx <= 1e-6f || sy <= 1e-6f)
        return;
    arcball->pan[0] = arcball->pan_center[0] + 2.0f * depth * shift_px[0] / (width * sx);
    arcball->pan[1] = arcball->pan_center[1] - 2.0f * depth * shift_px[1] / (height * sy);
}


/**
 * Invalidate a pending navigation depth without changing the rendered pose.
 *
 * @param arcball arcball controller
 */
void _dvz_arcball_pan_depth_cancel(DvzArcball* arcball)
{
    arcball->pan_depth_pending = false;
    arcball->pan_plane_active = false;
    arcball->pan_pending_stopped = false;
    arcball->pan_depth = 0.0f;
    if (++arcball->pan_epoch == 0)
        ++arcball->pan_epoch;
}



/**
 * Freeze the pan frame while awaiting GPU surface depth.
 *
 * @param arcball arcball controller
 */
void _dvz_arcball_pan_depth_wait(DvzArcball* arcball)
{
    _dvz_arcball_pan_depth_cancel(arcball);
    vec4 clip = {0};
    _arcball_pivot_clip(arcball, clip);
    arcball->pan_depth_fallback = isfinite(clip[3]) && clip[3] > 0.0f ? clip[3] : 0.0f;
    for (uint32_t i = 0; i < 2; i++)
    {
        float extent = arcball->viewport_size[i] * fabsf(arcball->projection[i][i]);
        arcball->pan_pixel_scale[i] = extent > 1e-6f ? 2.0f / extent : 0.0f;
    }
    glm_vec2_copy(arcball->pan, arcball->pan_center);
    glm_vec2_zero(arcball->pan_pending_shift);
    arcball->pan_depth_pending = true;
    arcball->pan_plane_active = true;
}



/**
 * Resolve a press-time pan plane and apply the latest accumulated shift.
 *
 * @param arcball arcball controller
 * @param clip_w positive surface clip W, or zero for the captured fallback plane
 */
void _dvz_arcball_pan_depth_resolve(DvzArcball* arcball, float clip_w)
{
    if (!arcball->pan_depth_pending)
        return;
    arcball->pan_depth = isfinite(clip_w) && clip_w > 0.0f ?
                             clip_w : arcball->pan_depth_fallback;
    arcball->pan_depth_pending = false;
    vec2 shift = {arcball->pan_pending_shift[0], arcball->pan_pending_shift[1]};
    _arcball_pan_drag(arcball, shift);
    if (arcball->pan_pending_stopped)
    {
        glm_vec2_copy(arcball->pan, arcball->pan_center);
        arcball->pan_plane_active = false;
        arcball->pan_pending_stopped = false;
    }
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
    bool screen_pan = (arcball->flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN) != 0;
    bool projection_zoom = (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM) != 0;
    vec3 camera_offset = {0};
    if (!projection_zoom)
    {
        camera_offset[0] = screen_pan ? arcball->zoom_offset[0] : arcball->pan[0];
        camera_offset[1] = screen_pan ? arcball->zoom_offset[1] : arcball->pan[1];
        camera_offset[2] = logf(arcball->zoom);
    }
    glm_translate_make(translate, camera_offset);

    mat4 view = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_copy(mvp->view, view);
    glm_mat4_mul(translate, view, mvp->view);
    vec3 offset = {0};
    _dvz_arcball_orbit_translation(arcball, offset);
    glm_translate(mvp->view, offset);
    if (screen_pan)
    {
        _arcball_screen_projection(arcball, mvp->proj, mvp->proj);
    }
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
            (void)dvz_arcball_resize(arcball, (float)r->window_width, (float)r->window_height);
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
    if (!isfinite(desc->zoom_speed) || desc->zoom_speed < 0.0f)
    {
        log_error("invalid arcball zoom speed");
        return false;
    }
    if ((desc->controller_flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN) &&
        (desc->controller_flags & DVZ_ARCBALL_FLAGS_VIEWPORT_ORBIT))
    {
        log_error("screen pan and viewport orbit cannot be combined");
        return false;
    }
    if ((desc->controller_flags & DVZ_ARCBALL_FLAGS_ADAPTIVE_ORBIT) &&
        !(desc->controller_flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN))
    {
        log_error("adaptive orbit requires screen pan");
        return false;
    }
    if ((desc->controller_flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM) &&
        !(desc->controller_flags & DVZ_ARCBALL_FLAGS_SCREEN_PAN))
    {
        log_error("projection zoom requires screen pan");
        return false;
    }
    float start = desc->orbit_zoom_start == 0 ? 1.0f : desc->orbit_zoom_start;
    float end = desc->orbit_zoom_end == 0 ? 3.0f : desc->orbit_zoom_end;
    if (!isfinite(start) || !isfinite(end) || start < 1 || end <= start ||
        1.0f - 1.0f / start >= 1.0f - 1.0f / end)
    {
        log_error("invalid adaptive orbit magnification thresholds");
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
        .zoom_speed = DVZ_ARCBALL_DEFAULT_ZOOM_SPEED,
        .orbit_zoom_start = 1.0f,
        .orbit_zoom_end = 3.0f,
    };
}



DvzArcball* _dvz_arcball(float width, float height, int flags)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzArcball* arcball = (DvzArcball*)dvz_calloc(1, sizeof(DvzArcball));
    arcball->flags = flags;
    arcball->zoom_speed = DVZ_ARCBALL_DEFAULT_ZOOM_SPEED;
    arcball->orbit_zoom_start = 1.0f;
    arcball->orbit_zoom_end = 3.0f;
    arcball->viewport_size[0] = width;
    arcball->viewport_size[1] = height;
    glm_mat4_identity(arcball->view);
    glm_mat4_identity(arcball->projection);
    (void)dvz_arcball_reset(arcball);
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
    DvzArcball* arcball = _dvz_arcball(desc->width, desc->height, (int)desc->controller_flags);
    arcball->zoom_speed = desc->zoom_speed > 0.0f ? desc->zoom_speed : DVZ_ARCBALL_DEFAULT_ZOOM_SPEED;
    arcball->orbit_zoom_start = desc->orbit_zoom_start > 0 ? desc->orbit_zoom_start : 1.0f;
    arcball->orbit_zoom_end = desc->orbit_zoom_end > 0 ? desc->orbit_zoom_end : 3.0f;
    return arcball;
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
 * Set the camera basis and projection used for rotation and pixel panning.
 *
 * @param arcball arcball controller
 * @param view camera view matrix
 * @param projection camera projection matrix
 */
void _dvz_arcball_camera(DvzArcball* arcball, mat4 view, mat4 projection)
{
    _dvz_arcball_view(arcball, view);
    glm_mat4_copy(projection, arcball->projection);
}


/**
 * Clear the camera view and projection used to interpret pointer motion.
 *
 * @param arcball arcball controller
 */
void _dvz_arcball_clear_view(DvzArcball* arcball)
{
    ANN(arcball);
    glm_mat4_identity(arcball->view);
    glm_mat4_identity(arcball->projection);
    arcball->has_view = false;
}



DvzResult dvz_arcball_initial(DvzArcball* arcball, vec3 angles)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    glm_vec3_copy(angles, arcball->init);
    return dvz_arcball_reset(arcball);
}



DvzResult dvz_arcball_reset(DvzArcball* arcball)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    _dvz_arcball_pan_depth_cancel(arcball);
    if (dvz_arcball_set(arcball, arcball->init) != DVZ_OK)
        return DVZ_ERROR;
    glm_quat_identity(arcball->rotation);
    arcball->zoom = 1.0f;
    glm_vec2_zero(arcball->zoom_offset);
    glm_vec2_zero(arcball->pan);
    glm_vec2_zero(arcball->pan_center);
    arcball->interacting = false;
    arcball->orbit_active = false;
    glm_vec3_zero(arcball->orbit_translation);
    glm_vec3_zero(arcball->orbit_pivot);
    return DVZ_OK;
}



DvzResult dvz_arcball_set(DvzArcball* arcball, vec3 angles)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    _dvz_arcball_orbit_translation(arcball, arcball->orbit_translation);
    arcball->orbit_active = false;
    glm_euler(angles, arcball->mat);
    glm_quat_identity(arcball->rotation);
    return DVZ_OK;
}



/**
 * Apply an incremental rotation around an axis to the accumulated orientation.
 *
 * @param arcball arcball controller
 * @param angle rotation angle in radians
 * @param axis rotation axis
 */
DvzResult dvz_arcball_rotate_axis(DvzArcball* arcball, float angle, vec3 axis)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    if (angle == 0.0f)
        return DVZ_OK;
    if (glm_vec3_norm(axis) == 0.0f)
    {
        log_warn("null arcball rotation axis, ignoring");
        return DVZ_ERROR;
    }

    (void)dvz_arcball_end(arcball);
    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_make(rot, angle, axis);
    glm_mat4_mul(rot, arcball->mat, arcball->mat);
    return DVZ_OK;
}


/**
 * Set the uniform zoom factor.
 *
 * @param arcball arcball controller
 * @param zoom uniform zoom factor
 */
DvzResult dvz_arcball_zoom(DvzArcball* arcball, float zoom)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    _dvz_arcball_pan_depth_cancel(arcball);
    float old_zoom = arcball->zoom;
    arcball->zoom = _clampf(zoom, DVZ_ARCBALL_ZOOM_MIN, DVZ_ARCBALL_ZOOM_MAX);
    if (arcball->flags & DVZ_ARCBALL_FLAGS_PROJECTION_ZOOM)
    {
        // Keep a live cumulative pan stable when magnification changes mid-gesture.
        float ratio = old_zoom / arcball->zoom;
        for (uint32_t i = 0; i < 2; i++)
            arcball->pan_center[i] =
                arcball->pan[i] - (arcball->pan[i] - arcball->pan_center[i]) * ratio;
    }
    return DVZ_OK;
}


/**
 * Set the panel-plane pan offset.
 *
 * @param arcball arcball controller
 * @param pan panel-plane pan offset
 */
DvzResult dvz_arcball_pan(DvzArcball* arcball, vec2 pan)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    _dvz_arcball_pan_depth_cancel(arcball);
    glm_vec2_copy(pan, arcball->pan);
    glm_vec2_copy(pan, arcball->pan_center);
    return DVZ_OK;
}


bool dvz_arcball_state(const DvzArcball* arcball, DvzArcballState* out)
{
    if (arcball == NULL || out == NULL)
        return false;

    out->zoom = arcball->zoom;
    out->pan[0] = arcball->pan[0];
    out->pan[1] = arcball->pan[1];
    out->interacting = arcball->interacting;
    return true;
}


/**
 * Apply an incremental panel-plane pan shift in pixels.
 *
 * @param arcball arcball controller
 * @param shift_px shift in viewport pixels
 */
DvzResult dvz_arcball_pan_shift(DvzArcball* arcball, vec2 shift_px)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    _dvz_arcball_pan_depth_cancel(arcball);
    glm_vec2_copy(arcball->pan, arcball->pan_center);
    _arcball_pan_drag(arcball, shift_px);
    glm_vec2_copy(arcball->pan, arcball->pan_center);
    return DVZ_OK;
}



DvzResult dvz_arcball_resize(DvzArcball* arcball, float width, float height)
{
    if (arcball == NULL || !isfinite(width) || !isfinite(height) || width <= 0.0f ||
        height <= 0.0f)
        return DVZ_ERROR;
    arcball->viewport_size[0] = width;
    arcball->viewport_size[1] = height;
    return DVZ_OK;
}



DvzResult dvz_arcball_constrain(DvzArcball* arcball, vec3 axis)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    if (glm_vec3_norm(axis) == 0.0f)
    {
        log_warn("null arcball constrain axis, ignoring");
        return DVZ_ERROR;
    }
    glm_vec3_normalize_to(axis, arcball->constrain);
    arcball->flags |= DVZ_ARCBALL_FLAGS_CONSTRAIN;
    return DVZ_OK;
}



void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles)
{
    ANN(arcball);
    glm_euler_angles(arcball->mat, out_angles);
}



DvzResult dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos)
{
    if (arcball == NULL)
        return DVZ_ERROR;

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
    return DVZ_OK;
}



DvzResult dvz_arcball_model(DvzArcball* arcball, mat4 model)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    _arcball_drag_rotation_matrix(arcball, rot);
    glm_mat4_mul(rot, arcball->mat, model);
    return DVZ_OK;
}



DvzResult dvz_arcball_end(DvzArcball* arcball)
{
    if (arcball == NULL)
        return DVZ_ERROR;
    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    _arcball_drag_rotation_matrix(arcball, rot);
    _dvz_arcball_orbit_translation(arcball, arcball->orbit_translation);
    arcball->orbit_active = false;
    glm_mat4_mul(rot, arcball->mat, arcball->mat);
    glm_quat_identity(arcball->rotation);
    return DVZ_OK;
}



void dvz_arcball_mvp(DvzArcball* arcball, DvzMVP* mvp)
{
    ANN(arcball);
    ANN(mvp);
    (void)dvz_arcball_model(arcball, mvp->model);
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
        _dvz_arcball_pan_depth_cancel(arcball);
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
            (void)dvz_arcball_end(arcball);
            if (ev->button == DVZ_POINTER_BUTTON_LEFT)
            {
                _dvz_arcball_pan_depth_cancel(arcball);
                (void)_arcball_orbit_begin(arcball);
            }
            if (ev->button == DVZ_POINTER_BUTTON_MIDDLE || ev->button == DVZ_POINTER_BUTTON_RIGHT)
                glm_vec2_copy(arcball->pan, arcball->pan_center);
            arcball->interacting = true;
        }
        break;

    case DVZ_POINTER_EVENT_DRAG:
        if (ev->button == DVZ_POINTER_BUTTON_LEFT)
        {
            arcball->interacting = true;
            if (!arcball->orbit_active && !_arcball_orbit_begin(arcball))
                break;
            vec2 cur_pos = {
                -1.0f + 2.0f * ev->pos[0] / width,
                +1.0f - 2.0f * ev->pos[1] / height,
            };
            vec2 last_pos = {
                -1.0f + 2.0f * ev->content.d.press_pos[0] / width,
                +1.0f - 2.0f * ev->content.d.press_pos[1] / height,
            };
            glm_vec2_sub(cur_pos, arcball->orbit_center, cur_pos);
            glm_vec2_sub(last_pos, arcball->orbit_center, last_pos);
            (void)dvz_arcball_rotate(arcball, cur_pos, last_pos);
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
        if (arcball->pan_depth_pending)
            arcball->pan_pending_stopped = true;
        else
            arcball->pan_plane_active = false;
        (void)dvz_arcball_end(arcball);
        glm_vec2_copy(arcball->pan, arcball->pan_center);
        arcball->interacting = false;
        break;

    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        (void)dvz_arcball_reset(arcball);
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



DvzResult dvz_arcball_connect(DvzArcball* arcball, DvzInputRouter* router)
{
    if (arcball == NULL || router == NULL)
        return DVZ_ERROR;
    DvzInputResizeEvent r;
    if (dvz_input_router_last_resize(router, &r) && r.window_width > 0 && r.window_height > 0)
        (void)dvz_arcball_resize(arcball, (float)r.window_width, (float)r.window_height);
    if (arcball->input_router != NULL && arcball->input_subscription_id != DVZ_CALLBACK_ID_NONE)
        dvz_input_unsubscribe(arcball->input_router, arcball->input_subscription_id);
    arcball->input_router = router;
    arcball->input_subscription_id =
        dvz_input_subscribe_event(router, _arcball_input_callback, arcball);
    if (arcball->input_subscription_id == DVZ_CALLBACK_ID_NONE)
    {
        arcball->input_router = NULL;
        return DVZ_ERROR;
    }
    return DVZ_OK;
}



DvzResult dvz_arcball_disconnect(DvzArcball* arcball, DvzInputRouter* router)
{
    if (arcball == NULL || router == NULL)
        return DVZ_ERROR;
    if (arcball->input_subscription_id != DVZ_CALLBACK_ID_NONE)
    {
        DvzInputRouter* subscribed_router =
            arcball->input_router != NULL ? arcball->input_router : router;
        dvz_input_unsubscribe(subscribed_router, arcball->input_subscription_id);
        arcball->input_router = NULL;
        arcball->input_subscription_id = DVZ_CALLBACK_ID_NONE;
    }
    return DVZ_OK;
}



void dvz_arcball_destroy(DvzArcball* arcball)
{
    ANN(arcball);
    dvz_free(arcball);
}
