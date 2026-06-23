/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Point visual lowering                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "point/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve point visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_point_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_POINT;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
    out->has_point_like_kind = true;
    out->point_style_enabled = visual->material.point_style_enabled;
    out->needs_material_params =
        visual->material.depth_cue_enabled || visual->material.point_style_enabled ||
        _scene_visual_has_dense_attr(visual, "item_state");
    if (visual->material.point_style_enabled)
    {
        out->material_param_fields[out->material_param_field_count++] =
            (DvzScenePayloadFieldDesc){
                .name = "point_style.stroke_width_px",
                .offset = offsetof(DvzSceneMaterialParams, params),
                .count = 1,
                .authored_unit = DVZ_SCENE_PAYLOAD_UNIT_LOGICAL_PX,
                .runtime_unit = DVZ_SCENE_PAYLOAD_UNIT_PHYSICAL_PX,
            };
    }
    return true;
}



/**
 * Resolve point visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_point_like_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualBindDesc), 0, sizeof(DvzSceneVisualBindDesc));
    out->uses_scene_occlusion_set2 = visual->scene_occluded;
    out->scene_occlusion = visual->scene_occlusion;
    out->controller_mode = controller_mode;

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, DVZ_ALPHA_OPAQUE, controller_mode, &caps))
        return false;
    out->uses_common_set0 = caps.uses_common_set;
    out->uses_fixed_common = caps.fixed_controller;
    out->uses_material_set1 = caps.uses_material_set;
    out->material_buffer_id = visual->material_buffer_id;
    out->uses_item_state_style_set1 = visual->has_item_state;
    out->item_state_style_buffer_id = visual->item_state_style_buffer_id;
    return true;
}



/**
 * Resolve point visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_point_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    return _scene_point_like_visual_bind_desc(visual, controller_mode, out);
}
