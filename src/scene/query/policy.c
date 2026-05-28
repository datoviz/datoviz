/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query policy                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the capability bit required by one query target.
 *
 * @param target the requested scene target
 * @return required capability bit, or zero for unsupported targets
 */
uint32_t _dvz_scene_query_target_capability(DvzSceneTargetKind target)
{
    switch (target)
    {
    case DVZ_SCENE_TARGET_NONE:
        return DVZ_QUERY_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_OBJECT:
        return DVZ_QUERY_CAPABILITY_OBJECT;
    case DVZ_SCENE_TARGET_ITEM:
        return DVZ_QUERY_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_VERTEX:
        return DVZ_QUERY_CAPABILITY_VERTEX;
    case DVZ_SCENE_TARGET_FACE:
    case DVZ_SCENE_TARGET_TRIANGLE:
        return DVZ_QUERY_CAPABILITY_FACE;
    case DVZ_SCENE_TARGET_PIXEL:
        return DVZ_QUERY_CAPABILITY_PIXEL;
    case DVZ_SCENE_TARGET_SAMPLE:
        return DVZ_QUERY_CAPABILITY_SAMPLE;
    case DVZ_SCENE_TARGET_STRIP:
        return DVZ_QUERY_CAPABILITY_GROUP;
    case DVZ_SCENE_TARGET_SEGMENT:
        return DVZ_QUERY_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_TEXT:
    case DVZ_SCENE_TARGET_ANNOTATION:
        return DVZ_QUERY_CAPABILITY_OBJECT;
    default:
        return 0;
    }
}



/**
 * Return whether a query profile is supported by the capability snapshot.
 *
 * @param profile the query profile
 * @param caps the capability snapshot
 * @return true when supported
 */
static bool _query_profile_supported(DvzQueryProfile profile, const DvzCapabilitySnapshot* caps)
{
    ANN(caps);
    if (!caps->supports_readback)
        return false;

    switch (profile)
    {
    case DVZ_QUERY_PROFILE_U32_R32:
        return caps->query_profile_u32_r32;
    case DVZ_QUERY_PROFILE_U64_RG32:
        return caps->query_profile_u64_rg32;
    case DVZ_QUERY_PROFILE_U64_2XR32:
        return caps->query_profile_u64_2xr32;
    case DVZ_QUERY_PROFILE_UNSUPPORTED:
    default:
        return false;
    }
}


/**
 * Resolve the effective profile for one query request.
 *
 * @param request the query request
 * @param caps the capability snapshot
 * @return a supported profile, or unsupported
 */
DvzQueryProfile
_dvz_scene_query_select_profile(const DvzQueryRequest* request, const DvzCapabilitySnapshot* caps)
{
    ANN(request);
    ANN(caps);
    if (request->profile != DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        if (_query_profile_supported(request->profile, caps))
            return request->profile;
        return DVZ_QUERY_PROFILE_UNSUPPORTED;
    }
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U32_R32, caps))
        return DVZ_QUERY_PROFILE_U32_R32;
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U64_RG32, caps))
        return DVZ_QUERY_PROFILE_U64_RG32;
    /* Two-attachment query readback remains an explicit future profile until family builders
     * and FramePlan emission can produce and consume both output attachments. */
    return DVZ_QUERY_PROFILE_UNSUPPORTED;
}



/**
 * Return one currently drawable query candidate for a panel request.
 *
 * @param panel the panel
 * @param capability required query capability
 * @return the topmost matching visual, or NULL
 */
DvzVisual* _dvz_scene_query_candidate_visual(const DvzPanel* panel, uint32_t capability)
{
    ANN(panel);
    if (capability == 0)
        return NULL;

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        const DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        if ((visual->query_capabilities & capability) != 0)
            return visual;
    }
    return NULL;
}



/**
 * Return the family query operation table eligible for one visual.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request the query request
 * @return operation table, or NULL when no native family path is eligible
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_family_ops_for_visual(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    for (uint32_t i = 0; i < _dvz_scene_query_registry_count(); i++)
    {
        const DvzSceneQueryFamilyOps* ops = _dvz_scene_query_registry_get(i);
        if (ops == NULL || ops->eligible == NULL)
            continue;
        if (ops->eligible(panel, visual, request))
            return ops;
    }
    return NULL;
}



/**
 * Resolve one panel-local query coordinate to a figure framebuffer coordinate.
 *
 * @param figure the figure
 * @param panel the panel
 * @param x panel-local logical x coordinate
 * @param y panel-local logical y coordinate
 * @param out_position output framebuffer coordinate
 * @return true when the framebuffer coordinate was written
 */
bool _dvz_scene_query_framebuffer_position(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y,
    uint32_t out_position[2])
{
    ANN(figure);
    ANN(panel);
    ANN(out_position);
    if (figure->width == 0 || figure->height == 0 || x < 0.0 || y < 0.0)
        return false;

    double fb_x = (double)figure->width * (double)panel->desc.x + x;
    double fb_y = (double)figure->height * (double)panel->desc.y + y;
    if (fb_x < 0.0 || fb_y < 0.0)
        return false;

    uint32_t max_x = figure->width > 0 ? figure->width - 1 : 0;
    uint32_t max_y = figure->height > 0 ? figure->height - 1 : 0;
    out_position[0] = fb_x >= (double)figure->width ? max_x : (uint32_t)fb_x;
    out_position[1] = fb_y >= (double)figure->height ? max_y : (uint32_t)fb_y;
    return true;
}
