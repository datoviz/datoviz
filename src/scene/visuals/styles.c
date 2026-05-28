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
 * Return default vector/arrow styling.
 *
 * @return default vector style descriptor
 */
DvzVectorStyle dvz_vector_style(void)
{
    DvzVectorStyle style = {
        .scale = 1.0f,
        .anchor = DVZ_VECTOR_ANCHOR_TAIL,
        .start_cap = DVZ_SEGMENT_CAP_NONE,
        .end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT,
        .join = DVZ_PATH_JOIN_ROUND,
        .miter_limit = 4.0f,
    };
    return style;
}


/**
 * Return whether one vector anchor enum value is supported.
 *
 * @param anchor the vector anchor
 * @return whether the anchor is valid
 */
static bool _vector_anchor_valid(DvzVectorAnchor anchor)
{
    return anchor >= DVZ_VECTOR_ANCHOR_TAIL && anchor <= DVZ_VECTOR_ANCHOR_HEAD;
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



/**
 * Configure segment endpoint caps.
 *
 * @param visual the segment visual
 * @param start_cap cap applied to the start endpoint
 * @param end_cap cap applied to the end endpoint
 * @return 0 on success, -1 on validation error
 */
int dvz_segment_set_caps(DvzVisual* visual, DvzSegmentCap start_cap, DvzSegmentCap end_cap)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_SEGMENT)
    {
        log_error("dvz_segment_set_caps requires a segment visual");
        return -1;
    }
    if (!_stroke_cap_valid(start_cap) || !_stroke_cap_valid(end_cap))
    {
        log_error("invalid segment cap");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update segment caps"))
        return -1;

    if (visual->segment.start_cap == start_cap && visual->segment.end_cap == end_cap)
        return 0;
    visual->segment.start_cap = start_cap;
    visual->segment.end_cap = end_cap;
    _segment_sync_params(visual);
    _visual_bump_version(&visual->material.version);
    visual->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}

/**
 * Configure vector/arrow styling.
 *
 * @param visual the vector visual
 * @param style style descriptor, or NULL for defaults
 * @return 0 on success, -1 on validation error
 */
int dvz_vector_set_style(DvzVisual* visual, const DvzVectorStyle* style)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
    {
        log_error("dvz_vector_set_style requires a vector visual");
        return -1;
    }
    DvzVectorStyle defaults = dvz_vector_style();
    if (style == NULL)
        style = &defaults;
    if (!isfinite(style->scale))
    {
        log_error("vector scale must be finite");
        return -1;
    }
    if (!_vector_anchor_valid(style->anchor))
    {
        log_error("invalid vector anchor");
        return -1;
    }
    if (!_stroke_cap_valid(style->start_cap) || !_stroke_cap_valid(style->end_cap))
    {
        log_error("invalid vector cap");
        return -1;
    }
    if (!_stroke_join_valid(style->join))
    {
        log_error("invalid vector path join");
        return -1;
    }
    if (!isfinite(style->miter_limit) || style->miter_limit <= 0.0f)
    {
        log_error("vector miter_limit must be positive and finite");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update vector style"))
        return -1;

    bool changed = visual->vector.scale != style->scale || visual->vector.anchor != style->anchor ||
                   visual->vector.start_cap != style->start_cap ||
                   visual->vector.end_cap != style->end_cap ||
                   visual->vector.join != style->join ||
                   visual->vector.miter_limit != style->miter_limit;
    if (!changed)
        return 0;

    visual->vector.scale = style->scale;
    visual->vector.anchor = style->anchor;
    visual->vector.start_cap = style->start_cap;
    visual->vector.end_cap = style->end_cap;
    visual->vector.join = style->join;
    visual->vector.miter_limit = style->miter_limit;
    _vector_sync_params(visual);
    _visual_bump_version(&visual->material.version);
    visual->material_params_dirty = true;
    visual->vector.stroke_gpu.dirty = true;
    visual->vector.path_gpu.dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Configure path endpoint caps.
 *
 * @param visual the path visual
 * @param start_cap cap applied to each open subpath start
 * @param end_cap cap applied to each open subpath end
 * @return 0 on success, -1 on validation error
 */
int dvz_path_set_caps(DvzVisual* visual, DvzSegmentCap start_cap, DvzSegmentCap end_cap)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_PATH)
    {
        log_error("dvz_path_set_caps requires a path visual");
        return -1;
    }
    if (!_stroke_cap_valid(start_cap) || !_stroke_cap_valid(end_cap))
    {
        log_error("invalid path cap");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update path caps"))
        return -1;

    if (visual->path.cap_start == start_cap && visual->path.cap_end == end_cap)
        return 0;
    visual->path.cap_start = start_cap;
    visual->path.cap_end = end_cap;
    _path_sync_params(visual);
    _visual_bump_version(&visual->material.version);
    visual->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Configure path joins and miter limit.
 *
 * @param visual the path visual
 * @param join the path join style
 * @param miter_limit positive finite miter limit
 * @return 0 on success, -1 on validation error
 */
int dvz_path_set_join(DvzVisual* visual, DvzPathJoin join, float miter_limit)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_PATH)
    {
        log_error("dvz_path_set_join requires a path visual");
        return -1;
    }
    if (!_stroke_join_valid(join))
    {
        log_error("invalid path join");
        return -1;
    }
    if (!isfinite(miter_limit) || miter_limit <= 0.0f)
    {
        log_error("path miter_limit must be positive and finite");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update path join"))
        return -1;

    if (visual->path.join == join && visual->path.miter_limit == miter_limit)
        return 0;
    visual->path.join = join;
    visual->path.miter_limit = miter_limit;
    _path_sync_params(visual);
    _visual_bump_version(&visual->material.version);
    visual->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}
