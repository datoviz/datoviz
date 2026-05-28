/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Path visual lowering                                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "path/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve path visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_path_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = _scene_visual_has_dense_attr(visual, "line_width")
                               ? DVZ_RENDERABLE_PATH_STROKE
                               : DVZ_RENDERABLE_INDEXED_MESH;
    out->desc_kind = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE
                         ? DVZ_SCENE_VISUAL_DESC_PATH
                         : DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    out->needs_material_params = out->renderable_kind == DVZ_RENDERABLE_PATH_STROKE;
    out->path_stroke_cache = &visual->path.gpu;
    return true;
}
