/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_alloc.h"
#include "_scene.h"
#include "controller_internal.h"
#include "datoviz/math/_cglm.h"
#include "pan_anchor_internal.h"
#include "query/internal.h"
#include "scene_notify_internal.h"



/*************************************************************************************************/
/*  Helpers and functions                                                                        */
/*************************************************************************************************/

/**
 * Resolve a weak controller identity and the still-current pan gesture.
 *
 * @param pending private pending query
 * @return matching live controller, or NULL
 */
static DvzController* _pan_anchor_controller(const DvzPendingQueryRequest* pending)
{
    DvzScene* scene = pending->navigation_scene;
    if (scene == NULL)
        return NULL;
    for (uint32_t i = 0; i < scene->controller_count; i++)
    {
        DvzController* controller = &scene->controllers[i];
        if (!controller->active || controller->id != pending->navigation_controller_id ||
            controller->type != DVZ_CONTROLLER_TYPE_ARCBALL || controller->arcball == NULL)
            continue;
        DvzArcball* arcball = controller->arcball;
        if (arcball->pan_depth_pending && arcball->pan_epoch == pending->navigation_pan_epoch)
            return controller;
    }
    return NULL;
}



/**
 * Compare finite matrices without hiding nonfinite values behind a tolerance.
 *
 * @param a first matrix
 * @param b second matrix
 * @return whether every element agrees
 */
static bool _pan_anchor_matrix_equal(const float* a, const float* b)
{
    for (uint32_t i = 0; i < 16; i++)
        if (!isfinite(a[i]) || !isfinite(b[i]) || fabsf(a[i] - b[i]) > 1e-6f)
            return false;
    return true;
}



/**
 * Queue a private rendered-depth request when a right or middle drag starts.
 *
 * @param panel source panel
 * @param controller bound arcball controller
 * @param pointer panel-local drag-start event
 */
void _scene_arcball_pan_anchor_begin(
    DvzPanel* panel, DvzController* controller, const DvzPointerEvent* pointer)
{
    if (pointer->type != DVZ_POINTER_EVENT_DRAG_START ||
        (pointer->button != DVZ_POINTER_BUTTON_RIGHT &&
         pointer->button != DVZ_POINTER_BUTTON_MIDDLE))
        return;
    DvzQueryRequest request = dvz_query_request();
    request.target = DVZ_SCENE_TARGET_FACE;
    request.profile = DVZ_QUERY_PROFILE_U64_RG32;
    bool supported = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        if (attach->visual != NULL && attach->visual->visible &&
            attach->controller_mode != DVZ_CONTROLLER_FIXED &&
            _dvz_scene_query_family_ops_for_navigation_depth(
                panel, attach->visual, &request) != NULL)
        {
            supported = true;
            break;
        }
    }
    if (!supported)
        return;

    DvzScene* scene = controller->scene;
    // Replace only this controller's private request; public query scopes remain independent.
    uint32_t write = 0;
    for (uint32_t read = 0; read < scene->pending_query_count; read++)
    {
        const DvzPendingQueryRequest* pending = &scene->pending_queries[read];
        if (pending->navigation_depth && pending->navigation_controller_id == controller->id)
            continue;
        if (write != read)
            scene->pending_queries[write] = *pending;
        write++;
    }
    scene->pending_query_count = write;
    if (write >= DVZ_SCENE_MAX_PENDING_REQUESTS)
        return;

    DvzArcball* arcball = controller->arcball;
    _dvz_arcball_pan_depth_wait(arcball);
    DvzPendingQueryRequest* pending = &scene->pending_queries[scene->pending_query_count++];
    *pending = (DvzPendingQueryRequest){0};
    pending->panel = panel;
    const float* press_pos = pointer->content.d.is_press_valid
                                 ? pointer->content.d.press_pos : pointer->pos;
    pending->x = press_pos[0];
    pending->y = press_pos[1];
    pending->request = request;
    pending->freshness_serial = _scene_next_request_serial(scene);
    pending->navigation_depth = true;
    pending->navigation_scene = scene;
    pending->navigation_controller_id = controller->id;
    pending->navigation_pan_epoch = arcball->pan_epoch;
    pending->navigation_query_input_revision = panel->figure->query_input_revision;
    _scene_panel_apply_mvp(panel, &pending->navigation_mvp);
    _scene_panel_pixel_rect(
        panel, &pending->navigation_panel_rect[0], &pending->navigation_panel_rect[1],
        &pending->navigation_panel_rect[2], &pending->navigation_panel_rect[3]);
    _scene_notify_request_frame(panel->figure);
}



/**
 * Reject changes to the gesture, binding, rendered camera, geometry or panel layout.
 *
 * @param pending private pending query
 * @return whether its captured rendered frame is still current
 */
bool _scene_arcball_pan_anchor_valid(const DvzPendingQueryRequest* pending)
{
    DvzController* controller = _pan_anchor_controller(pending);
    DvzPanel* panel = pending->panel;
    if (controller == NULL || panel == NULL || panel->figure == NULL ||
        panel->figure->scene != pending->navigation_scene ||
        panel->figure->query_input_revision != pending->navigation_query_input_revision)
        return false;
    bool bound = false;
    for (uint32_t dim = 0; dim < 3; dim++)
        bound = bound || panel->controllers[dim] == controller;
    if (!bound)
        return false;
    vec4 rect = {0};
    _scene_panel_pixel_rect(pending->panel, &rect[0], &rect[1], &rect[2], &rect[3]);
    for (uint32_t i = 0; i < 4; i++)
        if (rect[i] != pending->navigation_panel_rect[i])
            return false;
    DvzMVP current = {0};
    _scene_panel_apply_mvp(pending->panel, &current);
    return _pan_anchor_matrix_equal(&current.model[0][0], &pending->navigation_mvp.model[0][0]) &&
           _pan_anchor_matrix_equal(&current.view[0][0], &pending->navigation_mvp.view[0][0]) &&
           _pan_anchor_matrix_equal(&current.proj[0][0], &pending->navigation_mvp.proj[0][0]);
}



/**
 * Consume private depth before publication and apply the retained cumulative drag.
 *
 * @param pending private pending query
 * @param result rendered result, or NULL when the request became invalid
 */
void _scene_arcball_pan_anchor_finish(
    const DvzPendingQueryRequest* pending, const DvzQueryResult* result)
{
    DvzController* controller = _pan_anchor_controller(pending);
    if (controller == NULL)
        return;
    bool current = _scene_arcball_pan_anchor_valid(pending);
    float clip_w = 0.0f;
    DvzMVP captured = pending->navigation_mvp;
    if (current && result != NULL && result->hit && result->has_depth &&
        isfinite(result->depth) && result->depth >= 0.0 && result->depth <= 1.0)
    {
        const float width = pending->navigation_panel_rect[2];
        const float height = pending->navigation_panel_rect[3];
        if (width > 0 && height > 0)
        {
            mat4 inverse = GLM_MAT4_IDENTITY_INIT;
            glm_mat4_inv(captured.proj, inverse);
            vec4 clip = {
                2.0f * pending->x / width - 1.0f, 1.0f - 2.0f * pending->y / height,
                2.0f * (float)result->depth - 1.0f, 1.0f};
            vec4 eye = {0};
            glm_mat4_mulv(inverse, clip, eye);
            if (isfinite(eye[3]) && fabsf(eye[3]) > 1e-12f)
            {
                glm_vec4_scale(eye, 1.0f / eye[3], eye);
                glm_mat4_mulv(captured.proj, eye, clip);
                if (isfinite(clip[3]) && clip[3] > 0)
                    clip_w = clip[3];
            }
        }
    }
    if (!current)
    {
        // A stale camera/geometry cannot safely reuse even the old fallback plane.
        _dvz_arcball_pan_depth_cancel(controller->arcball);
        return;
    }
    _dvz_arcball_pan_depth_resolve(controller->arcball, clip_w);
    _scene_controller_navigation_pan_changed(controller);
}
