/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Labels visual API                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate a retained labels visual for state mutation.
 *
 * @param visual the visual
 * @param action mutation action used in diagnostics
 * @return whether the visual can be mutated
 */
static bool _labels_state_mutation_allowed(DvzVisual* visual, const char* action)
{
    ANN(visual);
    ANN(action);
    if (visual->type != DVZ_VISUAL_TYPE_LABELS)
    {
        log_error("%s requires a labels visual", action);
        return false;
    }
    return _scene_visual_mutation_allowed(visual->scene, action);
}



/**
 * Mark labels presentation state dirty.
 *
 * @param visual the labels visual
 */
static void _labels_state_mark_dirty(DvzVisual* visual)
{
    ANN(visual);
    _visual_bump_version(&visual->labels.version);
    visual->image_gpu.dirty = true;
    _scene_notify_visual_changed(visual);
}


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a labels visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_labels(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_LABELS, flags);
    if (visual != NULL)
    {
        visual->alpha_mode = DVZ_ALPHA_BLENDED;
        visual->depth_test_enabled = false;
    }
    return visual;
}



/**
 * Set the global opacity multiplier on a labels visual.
 *
 * @param visual the labels visual
 * @param opacity opacity multiplier in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_opacity(DvzVisual* visual, float opacity)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels opacity"))
        return -1;
    if (!isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
    {
        log_error("labels opacity must be finite and in [0, 1]");
        return -1;
    }
    visual->labels.opacity = opacity;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the transparent background label ID on a labels visual.
 *
 * @param visual the labels visual
 * @param label_id background label ID
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_background(DvzVisual* visual, DvzCategoryId label_id)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels background"))
        return -1;
    visual->labels.background_id = label_id;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the selected label ID on a labels visual.
 *
 * @param visual the labels visual
 * @param label_id selected label ID
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_selected(DvzVisual* visual, DvzCategoryId label_id)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels selection"))
        return -1;
    visual->labels.selected_enabled = true;
    visual->labels.selected_id = label_id;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Clear the selected label ID on a labels visual.
 *
 * @param visual the labels visual
 * @return 0 on success, -1 on error
 */
int dvz_labels_clear_selected(DvzVisual* visual)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "clear labels selection"))
        return -1;
    visual->labels.selected_enabled = false;
    visual->labels.selected_id = 0;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the hidden label IDs on a labels visual.
 *
 * @param visual the labels visual
 * @param ids hidden label IDs, or NULL when count is 0
 * @param count hidden label ID count
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_hidden(DvzVisual* visual, const DvzCategoryId* ids, uint32_t count)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set hidden labels"))
        return -1;
    if (count > DVZ_LABELS_MAX_HIDDEN)
    {
        log_error("too many hidden labels (%" PRIu32 " > %u)", count, DVZ_LABELS_MAX_HIDDEN);
        return -1;
    }
    if (count > 0 && ids == NULL)
    {
        log_error("hidden labels ids are NULL");
        return -1;
    }
    dvz_memset(
        visual->labels.hidden_ids, sizeof(visual->labels.hidden_ids), 0,
        sizeof(visual->labels.hidden_ids));
    if (count > 0)
        dvz_memcpy(
            visual->labels.hidden_ids, count * sizeof(DvzCategoryId), ids,
            count * sizeof(DvzCategoryId));
    visual->labels.hidden_count = count;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Configure boundary rendering on a labels visual.
 *
 * @param visual the labels visual
 * @param enabled whether boundary rendering is enabled
 * @param width_px boundary width in pixels
 * @param color boundary color
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_boundary(DvzVisual* visual, bool enabled, float width_px, DvzColor color)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels boundary"))
        return -1;
    if (!isfinite(width_px) || width_px < 0.0f)
    {
        log_error("labels boundary width must be finite and non-negative");
        return -1;
    }
    visual->labels.boundary_enabled = enabled;
    visual->labels.boundary_width_px = width_px;
    visual->labels.boundary_color = color;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the deterministic fallback-color seed on a labels visual.
 *
 * @param visual the labels visual
 * @param seed fallback-color seed
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_fallback_seed(DvzVisual* visual, uint32_t seed)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels fallback seed"))
        return -1;
    visual->labels.fallback_seed = seed;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the first-slice axis for a 3D labels visual.
 *
 * @param visual the labels visual
 * @param axis slice axis
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels slice axis"))
        return -1;
    if (axis != DVZ_VOLUME_AXIS_X && axis != DVZ_VOLUME_AXIS_Y && axis != DVZ_VOLUME_AXIS_Z)
    {
        log_error("unsupported labels slice axis %d", (int)axis);
        return -1;
    }
    visual->labels.slice_axis = axis;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the first-slice position for a 3D labels visual.
 *
 * @param visual the labels visual
 * @param position normalized slice position in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_slice_position(DvzVisual* visual, double position)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels slice position"))
        return -1;
    if (!isfinite(position) || position < 0.0 || position > 1.0)
    {
        log_error("labels slice position must be finite and in [0, 1]");
        return -1;
    }
    visual->labels.slice_position = position;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Return the retained labels state for inspection.
 *
 * @param visual the labels visual
 * @return the labels state, or NULL on error
 */
const DvzLabelsState* dvz_labels_state(const DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_LABELS)
        return NULL;
    return &visual->labels;
}
