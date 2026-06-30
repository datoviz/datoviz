/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#ifndef DVZ_SCENE_GENERATED_VISUAL_POLICY_HEADER
#define DVZ_SCENE_GENERATED_VISUAL_POLICY_HEADER

/*************************************************************************************************/
/*  Scene generated visual role policy                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "datoviz/scene.h"
#include "frame_plan/frame_plan.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_GENERATED_VISUAL_GUIDE_FILL = 0,
    DVZ_GENERATED_VISUAL_AXIS_GRID,
    DVZ_GENERATED_VISUAL_DATA_DEFAULT,
    DVZ_GENERATED_VISUAL_GUIDE_LINE,
    DVZ_GENERATED_VISUAL_GUIDE_OUTLINE,
    DVZ_GENERATED_VISUAL_AXIS_MARKS,
    DVZ_GENERATED_VISUAL_AXIS_TEXT,
} DvzGeneratedVisualRole;


typedef struct
{
    DvzGeneratedVisualRole role;
    int32_t z_layer;
    DvzControllerMode controller_mode;
    DvzVisualCoordSpace coord_space;
    DvzFramePlanClipRect clip_rect;
    DvzFramePlanViewportRect viewport_rect;
    bool depth_test;
    DvzAlphaMode alpha_mode;
} DvzGeneratedVisualPolicy;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static inline DvzGeneratedVisualPolicy
_scene_generated_visual_policy(DvzGeneratedVisualRole role)
{
    switch (role)
    {
    case DVZ_GENERATED_VISUAL_GUIDE_FILL:
        return (DvzGeneratedVisualPolicy){
            .role = role,
            .z_layer = -20,
            .controller_mode = DVZ_CONTROLLER_APPLY,
            .coord_space = DVZ_COORD_DATA,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT,
            .depth_test = false,
            .alpha_mode = DVZ_ALPHA_OPAQUE,
        };
    case DVZ_GENERATED_VISUAL_AXIS_GRID:
        return (DvzGeneratedVisualPolicy){
            .role = role,
            .z_layer = -10,
            .controller_mode = DVZ_CONTROLLER_APPLY,
            .coord_space = DVZ_COORD_VIEW,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT,
            .depth_test = false,
            .alpha_mode = DVZ_ALPHA_OPAQUE,
        };
    case DVZ_GENERATED_VISUAL_GUIDE_LINE:
        return (DvzGeneratedVisualPolicy){
            .role = role,
            .z_layer = 10,
            .controller_mode = DVZ_CONTROLLER_APPLY,
            .coord_space = DVZ_COORD_DATA,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT,
            .depth_test = false,
            .alpha_mode = DVZ_ALPHA_OPAQUE,
        };
    case DVZ_GENERATED_VISUAL_GUIDE_OUTLINE:
        return (DvzGeneratedVisualPolicy){
            .role = role,
            .z_layer = 11,
            .controller_mode = DVZ_CONTROLLER_APPLY,
            .coord_space = DVZ_COORD_DATA,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT,
            .depth_test = false,
            .alpha_mode = DVZ_ALPHA_OPAQUE,
        };
    case DVZ_GENERATED_VISUAL_AXIS_MARKS:
        return (DvzGeneratedVisualPolicy){
            .role = role,
            .z_layer = 1000,
            .controller_mode = DVZ_CONTROLLER_FIXED,
            .coord_space = DVZ_COORD_VIEW,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PANEL,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PANEL,
            .depth_test = false,
            .alpha_mode = DVZ_ALPHA_OPAQUE,
        };
    case DVZ_GENERATED_VISUAL_AXIS_TEXT:
        return (DvzGeneratedVisualPolicy){
            .role = role,
            .z_layer = 1001,
            .controller_mode = DVZ_CONTROLLER_FIXED,
            .coord_space = DVZ_COORD_VIEW,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PANEL,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PANEL,
            .depth_test = false,
            .alpha_mode = DVZ_ALPHA_BLENDED,
        };
    case DVZ_GENERATED_VISUAL_DATA_DEFAULT:
    default:
        return (DvzGeneratedVisualPolicy){
            .role = DVZ_GENERATED_VISUAL_DATA_DEFAULT,
            .z_layer = 0,
            .controller_mode = DVZ_CONTROLLER_APPLY,
            .coord_space = DVZ_COORD_DATA,
            .clip_rect = DVZ_FRAME_PLAN_CLIP_RECT_PLOT,
            .viewport_rect = DVZ_FRAME_PLAN_VIEWPORT_PLOT,
            .depth_test = true,
            .alpha_mode = DVZ_ALPHA_OPAQUE,
        };
    }
}


static inline DvzVisualAttachDesc
_scene_generated_visual_attach_desc(const DvzGeneratedVisualPolicy* policy, int32_t z_offset)
{
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    if (policy == NULL)
        return attach;
    attach.z_layer = policy->z_layer + z_offset;
    attach.controller_mode = policy->controller_mode;
    attach.coord_space = policy->coord_space;
    return attach;
}


static inline DvzAlphaMode
_scene_generated_visual_alpha_mode(const DvzGeneratedVisualPolicy* policy, uint8_t alpha)
{
    DvzAlphaMode mode = policy != NULL ? policy->alpha_mode : DVZ_ALPHA_OPAQUE;
    if (alpha < 255)
        mode = DVZ_ALPHA_BLENDED;
    return mode;
}


static inline bool _scene_generated_visual_rgba_is_translucent(const uint8_t rgba[4])
{
    return rgba != NULL && rgba[3] < 255;
}


static inline int _scene_generated_visual_apply_defaults(
    DvzVisual* visual, const DvzGeneratedVisualPolicy* policy, uint8_t alpha)
{
    if (visual == NULL || policy == NULL)
        return -1;
    if (visual->depth_test_enabled != policy->depth_test &&
        dvz_visual_set_depth_test(visual, policy->depth_test) != 0)
    {
        return -1;
    }
    DvzAlphaMode alpha_mode = _scene_generated_visual_alpha_mode(policy, alpha);
    if (visual->alpha_mode != alpha_mode && dvz_visual_set_alpha_mode(visual, alpha_mode) != 0)
        return -1;
    return 0;
}

#endif
