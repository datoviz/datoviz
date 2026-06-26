/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Point visual draw descriptors                                                                */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "point/internal.h"

#include "_assertions.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve point visual draw-count metadata.
 *
 * @param visual the visual descriptor
 * @param out the output draw descriptor
 * @return whether draw metadata was resolved
 */
bool _scene_point_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out)
{
    ANN(visual);
    ANN(out);
    if (!_scene_visual_default_draw_desc(visual, shader_format, out))
        return false;
    DvzScenePointLikeLoweringDesc lowering = {0};
    if (!_scene_point_like_lowering_desc(
            visual->point_like_kind, shader_format, visual->vertex_count, &lowering))
        return false;
    out->vertex_count = lowering.draw_vertex_count;
    out->instance_count = lowering.draw_instance_count;
    if (visual->has_item_range)
    {
        if (lowering.lowering == DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS)
        {
            out->first_vertex = 0;
            out->first_instance = visual->item_range_first;
            out->instance_count = visual->item_range_count;
        }
        else
        {
            out->first_vertex = visual->item_range_first;
            out->vertex_count = visual->item_range_count;
        }
    }
    return true;
}
