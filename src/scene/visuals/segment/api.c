/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Segment visual API                                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

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
 * Create a segment visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_segment(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_SEGMENT, flags);
    if (visual == NULL)
        return NULL;
    _visual_family_state(visual)->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    _visual_family_state(visual)->material_params_dirty = true;
    _visual_family_state(visual)->segment.gpu.dirty = true;
    return visual;
}



/**
 * Configure segment endpoint caps.
 *
 * @param visual the segment visual
 * @param start_cap cap applied to the start endpoint
 * @param end_cap cap applied to the end endpoint
 * @return 0 on success, -1 on validation error
 */
DvzResult dvz_segment_set_caps(DvzVisual* visual, DvzSegmentCap start_cap, DvzSegmentCap end_cap)
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

    if (_visual_family_state(visual)->segment.start_cap == start_cap && _visual_family_state(visual)->segment.end_cap == end_cap)
        return 0;
    _visual_family_state(visual)->segment.start_cap = start_cap;
    _visual_family_state(visual)->segment.end_cap = end_cap;
    _segment_sync_params(visual);
    _visual_bump_version(&visual->material.version);
    _visual_family_state(visual)->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}
