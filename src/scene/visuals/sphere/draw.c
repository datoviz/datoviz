/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Sphere visual draw descriptors                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "sphere/internal.h"

#include "_assertions.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve sphere visual draw-count metadata.
 *
 * @param visual the visual descriptor
 * @param out the output draw descriptor
 * @return whether draw metadata was resolved
 */
bool _scene_sphere_visual_draw_desc(
    const DvzSceneVisualDesc* visual, DvzSceneShaderFormat shader_format,
    DvzSceneVisualDrawDesc* out)
{
    ANN(visual);
    ANN(out);
    return _scene_visual_default_draw_desc(visual, shader_format, out);
}
