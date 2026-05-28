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
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
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



/**
 * Set explicit subpath lengths for a path-backed visual.
 *
 * @param visual the path-backed visual
 * @param subpath_count number of subpaths
 * @param lengths point count for each subpath
 * @param label label used in diagnostics
 * @param out_lengths owner pointer updated with the copied subpath lengths
 * @param out_count owner count updated with the subpath count
 * @param cache derived path-stroke cache dirtied by the subpath change
 * @return 0 on success, -1 on error
 */
int _stroke_set_path_subpaths(
    DvzVisual* visual, uint32_t subpath_count, const uint32_t* lengths, const char* label,
    uint32_t** out_lengths, uint32_t* out_count, DvzPathGpuCache* cache)
{
    ANN(visual);
    ANN(label);
    ANN(out_lengths);
    ANN(out_count);
    ANN(cache);
    if (!_scene_visual_mutation_allowed(visual->scene, "update path-backed subpaths"))
        return -1;
    if (subpath_count > 0 && lengths == NULL)
    {
        log_error("%s subpath lengths are required when subpath_count > 0", label);
        return -1;
    }

    uint32_t* copy = NULL;
    if (subpath_count > 0)
    {
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(subpath_count, sizeof(uint32_t), &byte_size) ||
            byte_size > SIZE_MAX)
        {
            log_error("%s subpath length byte size overflow", label);
            return -1;
        }
        copy = dvz_malloc((size_t)byte_size);
        if (copy == NULL)
        {
            log_error("%s subpath length allocation failed", label);
            return -1;
        }
        dvz_memcpy(copy, (size_t)byte_size, lengths, (size_t)byte_size);
        for (uint32_t i = 0; i < subpath_count; i++)
        {
            if (copy[i] == 0)
            {
                dvz_free(copy);
                log_error("%s subpath lengths must be greater than zero", label);
                return -1;
            }
        }
    }

    dvz_free(*out_lengths);
    *out_lengths = copy;
    *out_count = subpath_count;
    cache->dirty = true;
    if (visual->type == DVZ_VISUAL_TYPE_VECTOR)
        _visual_material_mark_dirty(visual);
    _scene_notify_visual_changed(visual);
    return 0;
}
