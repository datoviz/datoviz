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
#include "core/scene_notify_internal.h"
#include "_visual_internal.h"
#include "stroke/internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_VECTOR_STYLE_KNOWN_FLAGS 0u

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
 * Return default vector styling.
 *
 * @return default vector style descriptor
 */
DvzVectorStyle dvz_vector_style(void)
{
    DvzVectorStyle style = {
        DVZ_STRUCT_INIT_FIELDS(DvzVectorStyle),
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
 * Create a vector visual.
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
    _visual_family_state(visual)->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    _visual_family_state(visual)->material_params_dirty = true;
    _visual_family_state(visual)->vector.stroke_gpu.dirty = true;
    _visual_family_state(visual)->vector.path_gpu.dirty = true;
    return visual;
}



/**
 * Configure vector styling.
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
    if (!DVZ_STRUCT_VALID(style, DvzVectorStyle, DVZ_VECTOR_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzVectorStyle ABI prologue");
        return -1;
    }
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

    bool changed = _visual_family_state(visual)->vector.scale != style->scale || _visual_family_state(visual)->vector.anchor != style->anchor ||
                   _visual_family_state(visual)->vector.start_cap != style->start_cap ||
                   _visual_family_state(visual)->vector.end_cap != style->end_cap ||
                   _visual_family_state(visual)->vector.join != style->join ||
                   _visual_family_state(visual)->vector.miter_limit != style->miter_limit;
    if (!changed)
        return 0;

    _visual_family_state(visual)->vector.scale = style->scale;
    _visual_family_state(visual)->vector.anchor = style->anchor;
    _visual_family_state(visual)->vector.start_cap = style->start_cap;
    _visual_family_state(visual)->vector.end_cap = style->end_cap;
    _visual_family_state(visual)->vector.join = style->join;
    _visual_family_state(visual)->vector.miter_limit = style->miter_limit;
    _vector_sync_params(visual);
    _visual_bump_version(&visual->material.version);
    _visual_family_state(visual)->material_params_dirty = true;
    _visual_family_state(visual)->vector.stroke_gpu.dirty = true;
    _visual_family_state(visual)->vector.path_gpu.dirty = true;
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
        visual, subpath_count, lengths, "vector", &_visual_family_state(visual)->vector.subpath_lengths,
        &_visual_family_state(visual)->vector.subpath_count, &_visual_family_state(visual)->vector.path_gpu);
}
