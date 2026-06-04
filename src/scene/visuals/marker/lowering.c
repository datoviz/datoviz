/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual lowering                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"
#include "point/internal.h"

#include "_alloc.h"
#include "_assertions.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_marker_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_MARKER;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_MARKER;
    out->has_point_like_kind = true;
    out->needs_material_params = true;
    return true;
}



/**
 * Resolve marker visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_marker_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    if (!_scene_point_like_visual_bind_desc(visual, controller_mode, out))
        return false;
    if (visual->image_texture_id != 0)
    {
        out->uses_material_set1 = false;
        out->material_buffer_id = 0;
        out->uses_image_set1 = true;
        out->image_texture_id = visual->image_texture_id;
        out->image_nearest_sampler = visual->image_nearest_sampler;
    }
    return true;
}
