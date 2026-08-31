/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene request-frame notifications                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"


/*************************************************************************************************/
/*  Request-frame subscriptions                                                                  */
/*************************************************************************************************/


/**
 * Register one scene-level callback used to request a host frame.
 *
 * @param scene the scene
 * @param callback callback pointer
 * @param user_data opaque pointer forwarded to the callback
 * @return true on success, false when the subscription table is full or input is invalid
 */
bool _scene_add_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data)
{
    if (scene == NULL || callback == NULL)
        return false;

    DvzSceneRequestFrameSubscription* free_slot = NULL;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS; i++)
    {
        DvzSceneRequestFrameSubscription* sub = &scene->request_frame_subscriptions[i];
        if (sub->active && sub->callback == callback && sub->user_data == user_data)
            return true;
        if (!sub->active && free_slot == NULL)
            free_slot = sub;
    }

    if (free_slot == NULL)
    {
        log_error("scene request-frame subscription table is full");
        return false;
    }

    free_slot->callback = callback;
    free_slot->user_data = user_data;
    free_slot->active = true;
    return true;
}


/**
 * Remove one scene-level host frame request callback.
 *
 * @param scene the scene
 * @param callback callback pointer
 * @param user_data opaque pointer previously registered with the callback
 */
void _scene_remove_request_frame_callback(
    DvzScene* scene, DvzSceneRequestFrameCallback callback, void* user_data)
{
    if (scene == NULL || callback == NULL)
        return;

    for (uint32_t i = 0; i < DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS; i++)
    {
        DvzSceneRequestFrameSubscription* sub = &scene->request_frame_subscriptions[i];
        if (sub->active && sub->callback == callback && sub->user_data == user_data)
        {
            dvz_memset(sub, sizeof(DvzSceneRequestFrameSubscription), 0,
                       sizeof(DvzSceneRequestFrameSubscription));
            return;
        }
    }
}


/**
 * Notify all scene hosts that one figure needs another frame.
 *
 * @param figure figure requesting a frame
 */
void _scene_notify_request_frame(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    figure->frame_revision = figure->frame_revision == UINT64_MAX ? 1 : figure->frame_revision + 1;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_REQUEST_FRAME_SUBSCRIPTIONS; i++)
    {
        const DvzSceneRequestFrameSubscription* sub = &scene->request_frame_subscriptions[i];
        DvzSceneRequestFrameCallback callback = sub->callback;
        void* user_data = sub->user_data;
        if (sub->active && callback != NULL)
            callback(figure, user_data);
    }
}


/**
 * Notify app hosts for every figure containing an attached visual.
 *
 * @param visual visual whose attached figures should be redrawn
 */
void _scene_notify_visual_changed(DvzVisual* visual)
{
    if (visual == NULL || visual->scene == NULL)
        return;
    DvzScene* scene = visual->scene;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        bool contains_visual = false;
        for (uint32_t pi = 0; pi < figure->panel_count && !contains_visual; pi++)
        {
            const DvzPanel* panel = &figure->panels[pi];
            for (uint32_t vi = 0; vi < panel->visual_count; vi++)
            {
                if (panel->visuals[vi].visual == visual)
                {
                    contains_visual = true;
                    break;
                }
            }
        }
        if (contains_visual)
            _scene_notify_request_frame(figure);
    }
}


/**
 * Return whether one visual consumes a scene buffer.
 *
 * @param visual visual to inspect
 * @param buffer scene buffer to find
 * @return whether the visual references the buffer
 */
static bool _scene_visual_uses_buffer(const DvzVisual* visual, const DvzSceneBuffer* buffer)
{
    if (visual == NULL || buffer == NULL)
        return false;
    const DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state != NULL && state->buffer == buffer)
        return true;
    for (uint32_t ai = 0; ai < visual->attr_count; ai++)
    {
        if (visual->attrs[ai].buffer == buffer)
            return true;
    }
    return false;
}


/**
 * Notify app hosts for every figure containing a visual bound to a scene buffer.
 *
 * @param buffer scene buffer whose consumers should be redrawn
 */
void _scene_notify_buffer_changed(DvzSceneBuffer* buffer)
{
    if (buffer == NULL || buffer->scene == NULL)
        return;
    DvzScene* scene = buffer->scene;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        bool uses_buffer = false;
        for (uint32_t pi = 0; pi < figure->panel_count && !uses_buffer; pi++)
        {
            const DvzPanel* panel = &figure->panels[pi];
            for (uint32_t vi = 0; vi < panel->visual_count; vi++)
            {
                if (_scene_visual_uses_buffer(panel->visuals[vi].visual, buffer))
                {
                    uses_buffer = true;
                    break;
                }
            }
        }
        for (uint32_t ci = 0; ci < figure->compute_count && !uses_buffer; ci++)
        {
            DvzSceneCompute* compute = figure->computes[ci];
            if (compute == NULL)
                continue;
            for (uint32_t bi = 0; bi < compute->binding_count; bi++)
            {
                if (compute->bindings[bi].active && compute->bindings[bi].buffer == buffer)
                {
                    uses_buffer = true;
                    break;
                }
            }
        }
        if (uses_buffer)
            _scene_notify_request_frame(figure);
    }
}
