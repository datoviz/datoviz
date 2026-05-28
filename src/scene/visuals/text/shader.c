/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Text visual shader descriptors                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "text/internal.h"

#include "_assertions.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Reject retained text visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return false because retained text visuals lower through generated glyph visuals
 */
bool _scene_text_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    (void)picking;
    (void)wboit_accumulation;
    return false;
}
