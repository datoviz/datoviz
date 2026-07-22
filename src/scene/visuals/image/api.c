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
DvzResult dvz_image_set_sampling(DvzVisual* visual, DvzImageSampling sampling)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_image_set_sampling requires an image visual");
        return -1;
    }
    DvzFieldSamplingDesc desc = dvz_field_sampling_desc();
    switch (sampling)
    {
    case DVZ_IMAGE_SAMPLING_LINEAR:
        desc.min_filter = DVZ_FIELD_FILTER_LINEAR;
        desc.mag_filter = DVZ_FIELD_FILTER_LINEAR;
        break;
    case DVZ_IMAGE_SAMPLING_NEAREST:
        desc.min_filter = DVZ_FIELD_FILTER_NEAREST;
        desc.mag_filter = DVZ_FIELD_FILTER_NEAREST;
        break;
    default:
        log_error("invalid image sampling mode");
        return -1;
    }

    return dvz_visual_set_field_sampling(visual, "field", &desc);
}
