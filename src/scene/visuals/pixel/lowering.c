/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Pixel visual lowering                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "pixel/internal.h"
#include "point/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve pixel visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_pixel_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_PIXEL;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_PIXEL;
    out->has_point_like_kind = true;
    out->needs_material_params =
        visual->material.depth_cue_enabled || _scene_visual_has_dense_attr(visual, "item_state");
    return true;
}



/**
 * Resolve pixel visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_pixel_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    return _scene_point_like_visual_bind_desc(visual, controller_mode, out);
}
