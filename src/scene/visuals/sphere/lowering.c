/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Sphere visual lowering                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "sphere/internal.h"

#include "_alloc.h"
#include "_assertions.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve sphere visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_sphere_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);
    (void)visual;

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
    out->needs_material_params = true;
    return true;
}
