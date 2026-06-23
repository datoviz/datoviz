/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual lowering                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "image/internal.h"

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve image visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_image_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
    if (_scene_visual_has_dense_attr(visual, "position_px") &&
        _scene_visual_has_dense_attr(visual, "extent_px"))
    {
        out->draw_position_attr = "position_px";
    }
    return true;
}



/**
 * Fill image visual FramePlan metadata.
 *
 * @param visual the retained visual
 * @param lowering resolved lowering facts
 * @param metadata the metadata being built
 * @return whether metadata was filled
 */
bool _scene_image_visual_fill_metadata(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata)
{
    ANN(visual);
    ANN(lowering);
    ANN(metadata);

    if (lowering->desc_kind != DVZ_SCENE_VISUAL_DESC_IMAGE)
        return true;
    metadata->image_nearest_sampler = _visual_family_state(visual)->image_nearest_sampler;
    if (!_image_uses_generated_quads(visual))
        return true;
    if (_visual_family_state(visual)->image_gpu.vertex_count > UINT32_MAX)
        return false;
    if (_visual_family_state(visual)->image_gpu.vertex_count > 0)
        metadata->vertex_count = (uint32_t)_visual_family_state(visual)->image_gpu.vertex_count;
    metadata->image_pixel_space = _visual_family_state(visual)->image_gpu.pixel_space;
    return true;
}



/**
 * Resolve image visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_image_visual_bind_desc(
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
    out->uses_image_set1 = caps.uses_image_set;
    out->image_texture_id = visual->image_texture_id;
    out->image_nearest_sampler = visual->image_nearest_sampler;
    out->image_color_role = visual->image_color_role;
    return true;
}
