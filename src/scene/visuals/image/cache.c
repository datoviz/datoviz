/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual cache                                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "image/internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Release one image visual's derived rectangle upload cache.
 *
 * @param cache the image GPU cache
 */
void _image_gpu_cache_free(DvzImageGpuCache* cache)
{
    if (cache == NULL)
        return;
    dvz_free(cache->position);
    dvz_free(cache->texcoords);
    dvz_memset(cache, sizeof(DvzImageGpuCache), 0, sizeof(DvzImageGpuCache));
}
