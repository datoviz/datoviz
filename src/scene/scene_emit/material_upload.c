/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual material upload emission                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "core/panel_layout_internal.h"
#include "scene_emit/internal.h"
#include "scene_emit/visual_lowering.h"
#include "_visual_internal.h"
#include "datoviz/drp2/runtime.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit material parameter uploads for one visual.
 *
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @return whether emission can continue for this visual
 */
bool _scene_emit_visual_material_upload(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    if (!visual->material_params_dirty)
        return true;

    char material_resource_id[128];
    if (!_scene_visual_attr_resource_key(
            figure, visual, visual_index, "material_params", material_resource_id,
            sizeof(material_resource_id)))
    {
        return false;
    }
    DvzSceneMaterialParams* params =
        (DvzSceneMaterialParams*)dvz_malloc(sizeof(DvzSceneMaterialParams));
    if (params == NULL)
        return false;
    DvzVisualLowering lowering = {0};
    bool has_lowering = _scene_visual_lowering_resolve(visual, &lowering);
    bool point_style_scaled =
        has_lowering && lowering.point_style_enabled &&
        (lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_POINT ||
         lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER);
    _material_params_upload_payload(
        visual, point_style_scaled, _scene_screen_scale(figure), params);
    if (!dvz_frame_plan_upload_bytes(
            plan, material_resource_id, 0, sizeof(DvzSceneMaterialParams), "material_params",
            params))
    {
        dvz_free(params);
        return false;
    }
    plan->nodes[plan->count - 1].u.upload.owned_data = params;
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX, 1);
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    return true;
}
