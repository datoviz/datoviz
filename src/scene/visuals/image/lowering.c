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
