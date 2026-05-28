/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Vector visual lowering                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "vector/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a vector visual lowers through path-stroke geometry.
 *
 * @param visual the retained visual
 * @return whether the vector has path-style point data
 */
static bool _vector_uses_path_stroke(const DvzVisual* visual)
{
    ANN(visual);
    return visual->type == DVZ_VISUAL_TYPE_VECTOR &&
           !_scene_visual_has_dense_attr(visual, "vector") &&
           _scene_visual_has_dense_attr(visual, "position") &&
           _scene_visual_has_dense_attr(visual, "color") &&
           _scene_visual_has_dense_attr(visual, "line_width");
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve vector visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_vector_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = _vector_uses_path_stroke(visual) ? DVZ_RENDERABLE_PATH_STROKE
                                                            : DVZ_RENDERABLE_STROKE_QUAD;
    out->desc_kind = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE
                         ? DVZ_SCENE_VISUAL_DESC_PATH
                         : DVZ_SCENE_VISUAL_DESC_SEGMENT;
    out->needs_material_params = true;
    out->needs_vector_params_sync = true;
    out->stroke_quad_cache = &visual->vector.stroke_gpu;
    out->path_stroke_cache = &visual->vector.path_gpu;
    return true;
}
