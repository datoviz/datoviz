/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Splat visual API                                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a Gaussian splat visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_splat(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_SPLAT, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    visual->alpha_mode = DVZ_ALPHA_BLENDED;
    visual->depth_test_enabled = true;
    return visual;
}
