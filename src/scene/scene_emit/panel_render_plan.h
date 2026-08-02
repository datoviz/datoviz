/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene panel render planning                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_technique.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_PANEL_RENDER_INVALID_INDEX UINT32_MAX



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzVisual* visual;
    DvzPanelAttach* attach;
    uint32_t visual_index;
    uint32_t blend_group;
    uint32_t authored_order;
    DvzSceneVisualLayer layer;
    DvzSceneVisualPassCaps caps;
    bool needs_depth;
    bool writes_depth;
} DvzPanelRenderVisualPlan;

typedef enum
{
    DVZ_PANEL_RENDER_TRANSPARENT_BLENDED = 0,
    DVZ_PANEL_RENDER_TRANSPARENT_DEPTH_PEEL,
    DVZ_PANEL_RENDER_TRANSPARENT_WBOIT,
} DvzPanelRenderTransparentKind;

typedef struct
{
    DvzPanelRenderTransparentKind kind;
    uint32_t index;
} DvzPanelRenderTransparentPassPlan;

typedef struct
{
    char panel_id[64];
    uint32_t drawable_count;
    uint32_t order[DVZ_SCENE_MAX_VISUALS];
    DvzPanelRenderVisualPlan visuals[DVZ_SCENE_MAX_VISUALS];
    uint32_t visual_count;
    DvzPanelCompositionSnapshot composition;

    bool scene_occlusion_enabled;
    bool volume_occlusion_enabled;
    bool has_volume_occluder;
    DvzPanelAttach volume_occluder_attach;
    uint32_t volume_occluder_visual_index;

    bool gbuffer_enabled;
    bool ssao_enabled;
    bool gbuffer_required;
    bool edl_enabled;
    bool edl_has_depth_producer;
    bool has_transparent;
    bool unsupported_noncontiguous_oit;
    bool opaque_needs_depth;
    bool transparent_needs_depth;

    const DvzSceneSsaoTechniqueState* ssao_state;
    const DvzSceneMsaaTechniqueState* msaa_state;
    const DvzSceneEdlTechniqueState* edl_state;
    DvzSceneGBufferPlan gbuffer;

    DvzPanelRenderVisualPlan scene_occlusion[DVZ_SCENE_MAX_VISUALS];
    uint32_t scene_occlusion_count;

    DvzPanelRenderVisualPlan gbuffer_visuals[DVZ_SCENE_MAX_VISUALS];
    uint32_t gbuffer_visual_count;

    DvzPanelRenderVisualPlan opaque_visuals[DVZ_SCENE_MAX_VISUALS];
    uint32_t opaque_visual_count;

    DvzPanelRenderVisualPlan blended_visuals[DVZ_SCENE_MAX_VISUALS];
    uint32_t blended_visual_count;
    bool blended_needs_depth[DVZ_SCENE_MAX_RENDER_VISUALS];
    bool blended_writes_depth[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t blended_group_count;

    DvzPanelRenderVisualPlan depth_peel_visuals[DVZ_SCENE_MAX_VISUALS];
    uint32_t depth_peel_visual_count;

    DvzPanelRenderVisualPlan wboit_visuals[DVZ_SCENE_MAX_VISUALS];
    uint32_t wboit_visual_count;

    DvzPanelRenderTransparentPassPlan transparent_passes[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t transparent_pass_count;
} DvzPanelRenderPlan;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const char* _scene_panel_render_visual_draw_position_attr(const DvzVisual* visual);

bool _scene_panel_render_visual_is_visible_drawable(const DvzVisual* visual);

bool _scene_panel_render_plan_build(
    DvzFigure* figure, uint32_t panel_index, const char* figure_id,
    const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report, DvzPanelRenderPlan* out);

bool _scene_panel_composition_resolve(
    const DvzPanelRenderPlan* render_plan, const DvzCapabilitySnapshot* caps,
    DvzPanelCompositionSnapshot* out, DvzDiagnosticReport* report);

bool _scene_bind_panel_composition(
    DvzFramePlan* plan, const char* panel_id, const DvzPanelCompositionSnapshot* snapshot,
    DvzDiagnosticReport* report);

bool _scene_panel_composition_lower_graph(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report);
