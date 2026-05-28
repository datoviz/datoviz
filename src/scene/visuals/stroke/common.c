/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene stroke visual helpers                                                                  */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_visual_internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one stroke cap enum value is supported by the first slice.
 *
 * @param cap the stroke cap
 * @return whether the cap is valid
 */
bool _stroke_cap_valid(DvzSegmentCap cap)
{
    return cap >= DVZ_SEGMENT_CAP_NONE && cap <= DVZ_SEGMENT_CAP_BUTT;
}



/**
 * Return whether one stroke join enum value is supported by the first slice.
 *
 * @param join the stroke join
 * @return whether the join is valid
 */
bool _stroke_join_valid(DvzPathJoin join)
{
    return join >= DVZ_PATH_JOIN_MITER && join <= DVZ_PATH_JOIN_BEVEL;
}



/**
 * Release one stroke-quad visual's derived GPU upload cache.
 *
 * @param cache the stroke-quad GPU cache
 */
void _stroke_quad_gpu_cache_free(DvzSegmentGpuCache* cache)
{
    if (cache == NULL)
        return;
    dvz_free(cache->position_start);
    dvz_free(cache->position_end);
    dvz_free(cache->color);
    dvz_free(cache->line_width);
    dvz_free(cache->indices);
    dvz_memset(cache, sizeof(DvzSegmentGpuCache), 0, sizeof(DvzSegmentGpuCache));
}



/**
 * Release one path-stroke visual's derived GPU upload cache.
 *
 * @param cache the path-stroke GPU cache
 */
void _path_stroke_gpu_cache_free(DvzPathGpuCache* cache)
{
    if (cache == NULL)
        return;
    dvz_free(cache->position_prev);
    dvz_free(cache->position_curr);
    dvz_free(cache->position_next);
    dvz_free(cache->color);
    dvz_free(cache->line_width);
    dvz_free(cache->path_flags);
    dvz_free(cache->path_distance);
    dvz_free(cache->indices);
    dvz_memset(cache, sizeof(DvzPathGpuCache), 0, sizeof(DvzPathGpuCache));
}
