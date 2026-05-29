/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Path visual API                                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_visual_internal.h"
#include "stroke/internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a path visual.
 *
 * Path visuals use primitive line-strip rendering unless a per-point `line_width` attribute is
 * present, in which case scene emission lowers them to path-native screen-space strokes.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_path(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_PATH, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    visual->material_params_dirty = true;
    return visual;
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



/**
 * Set explicit subpath lengths for a path visual.
 *
 * @param visual the path visual
 * @param subpath_count number of subpaths
 * @param lengths point count for each subpath
 * @return 0 on success, -1 on error
 */
int dvz_path_set_subpaths(DvzVisual* visual, uint32_t subpath_count, const uint32_t* lengths)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_PATH)
    {
        log_error("dvz_path_set_subpaths requires a path visual");
        return -1;
    }
    return _stroke_set_path_subpaths(
        visual, subpath_count, lengths, "path", &visual->path.subpath_lengths,
        &visual->path.subpath_count, &visual->path.gpu);
}
