/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene technique planning                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzSceneGBufferPlan
{
    bool enabled;
    bool needs_depth;
    bool needs_normal;
    bool needs_object_id;
    uint32_t producer_count;
} DvzSceneGBufferPlan;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_alpha_mode_is_wboit(DvzAlphaMode mode);

bool _scene_alpha_mode_is_depth_peel(DvzAlphaMode mode);

bool _scene_alpha_mode_is_blended(DvzAlphaMode mode);

bool _scene_visual_writes_depth(const DvzVisual* visual, const DvzPanelAttach* attach);

bool _scene_transparent_visual_needs_depth(
    const DvzVisual* visual, const DvzPanelAttach* attach);

bool _scene_technique_emit_wboit_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth);

bool _scene_technique_emit_blended_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth);

bool _scene_technique_emit_depth_peel_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth);

bool _scene_technique_emit_opaque_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool needs_depth);

void _scene_technique_gbuffer_plan_init(DvzSceneGBufferPlan* plan);

bool _scene_technique_gbuffer_plan_add_visual(
    DvzSceneGBufferPlan* plan, const DvzVisual* visual, const DvzPanelAttach* attach);

bool _scene_technique_emit_gbuffer_frame_graph(
    DvzFramePlan* plan, const char* panel_id, const DvzSceneGBufferPlan* gbuffer);
