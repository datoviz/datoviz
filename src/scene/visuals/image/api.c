/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual API                                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an image visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_image(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    return _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_IMAGE, flags);
}


/**
 * Set the sampler filter mode for an image visual.
 *
 * @param visual the image visual
 * @param sampling the image sampler filter mode
 * @return 0 on success, -1 on error
 */
int dvz_image_set_sampling(DvzVisual* visual, DvzImageSampling sampling)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_image_set_sampling requires an image visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update image sampling"))
        return -1;

    switch (sampling)
    {
    case DVZ_IMAGE_SAMPLING_LINEAR:
        _visual_family_state(visual)->image_nearest_sampler = false;
        break;
    case DVZ_IMAGE_SAMPLING_NEAREST:
        _visual_family_state(visual)->image_nearest_sampler = true;
        break;
    default:
        log_error("invalid image sampling mode");
        return -1;
    }

    _visual_bump_version(&_visual_family_state(visual)->image_sampling_version);
    return 0;
}
