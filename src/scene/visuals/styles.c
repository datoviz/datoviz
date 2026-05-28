/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual styles */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "stroke/internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Store segment cap state into the shared material payload used by segment shaders.
 *
 * @param visual the segment visual
 */
void _segment_sync_params(DvzVisual* visual)
{
    ANN(visual);
    visual->material_params.params[0] = (float)visual->segment.start_cap;
    visual->material_params.params[1] = (float)visual->segment.end_cap;
}


/**
 * Store path cap/join state into the shared material payload used by path shaders.
 *
 * @param visual the path visual
 */
void _path_sync_params(DvzVisual* visual)
{
    ANN(visual);
    visual->material_params.params[0] = (float)visual->path.cap_start;
    visual->material_params.params[1] = (float)visual->path.cap_end;
    visual->material_params.params[2] = (float)visual->path.join;
    visual->material_params.params[3] = visual->path.miter_limit;
}


/**
 * Store vector-owned cap/join state into the shared material payload used by vector lowerings.
 *
 * @param visual the vector visual
 */
void _vector_sync_params(DvzVisual* visual)
{
    ANN(visual);
    visual->material_params.params[0] = (float)visual->vector.start_cap;
    visual->material_params.params[1] = (float)visual->vector.end_cap;
    visual->material_params.params[2] = (float)visual->vector.join;
    visual->material_params.params[3] = visual->vector.miter_limit;
}


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
