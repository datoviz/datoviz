/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Vector visual API                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
 * Create a vector/arrow visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_vector(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_VECTOR, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    visual->material_params_dirty = true;
    visual->vector.stroke_gpu.dirty = true;
    visual->vector.path_gpu.dirty = true;
    return visual;
}



/**
 * Create an arrow visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_arrow(DvzScene* scene, uint32_t flags)
{
    return dvz_vector(scene, flags);
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
 * Set explicit subpath lengths for a curved vector visual.
 *
 * @param visual the vector visual
 * @param subpath_count number of subpaths
 * @param lengths point count for each subpath
 * @return 0 on success, -1 on error
 */
int dvz_vector_set_subpaths(DvzVisual* visual, uint32_t subpath_count, const uint32_t* lengths)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
    {
        log_error("dvz_vector_set_subpaths requires a vector visual");
        return -1;
    }
    return _stroke_set_path_subpaths(
        visual, subpath_count, lengths, "vector", &visual->vector.subpath_lengths,
        &visual->vector.subpath_count, &visual->vector.path_gpu);
}
