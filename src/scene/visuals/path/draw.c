/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Path visual draw descriptors                                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "path/internal.h"

#include "_assertions.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve path visual draw-count metadata.
 *
 * @param visual the visual descriptor
 * @param out the output draw descriptor
 * @return whether draw metadata was resolved
 */
bool _scene_path_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneVisualDrawDesc* out)
{
    ANN(visual);
    ANN(out);
    return _scene_visual_default_draw_desc(visual, out);
}
