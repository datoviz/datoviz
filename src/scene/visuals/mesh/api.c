/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Mesh visual API                                                                              */
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
 * Create a mesh visual.
 *
 * First-slice mesh visuals reuse the indexed primitive triangle-list execution path.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_MESH, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    visual->material_params_dirty = true;
    return visual;
}
