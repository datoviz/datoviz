/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Vector visual shader descriptors                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "vector/internal.h"

#include "_assertions.h"
#include "_visual_pipeline_internal.h"
#include "path/internal.h"
#include "segment/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve vector visual shader metadata through the lowered descriptor kind.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_vector_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    if (_scene_visual_desc_is_segment(visual->kind))
    {
        return _scene_segment_visual_shader_desc(
            visual, picking, wboit_accumulation, format_tag, out);
    }
    if (_scene_visual_desc_is_path(visual->kind))
    {
        return _scene_path_visual_shader_desc(
            visual, picking, wboit_accumulation, format_tag, out);
    }
    return false;
}
