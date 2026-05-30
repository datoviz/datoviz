/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual draw descriptors                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"

#include "_assertions.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker visual draw-count metadata.
 *
 * @param visual the visual descriptor
 * @param out the output draw descriptor
 * @return whether draw metadata was resolved
 */
bool _scene_marker_visual_draw_desc(
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
    return true;
}
