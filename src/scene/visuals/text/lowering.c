/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Text visual lowering                                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "text/internal.h"

#include "_alloc.h"
#include "_assertions.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Reject retained text visual lowering; text synchronizes glyph visuals before render emission.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return false because text visuals are semantic parents, not renderable draws
 */
bool _scene_text_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);
    (void)visual;

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_NONE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_NONE;
    return false;
}
