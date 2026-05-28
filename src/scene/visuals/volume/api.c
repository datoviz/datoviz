/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Volume visual API                                                                            */
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
#include "sample_profile.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Apply the current retained volume bounds to the proxy cube attributes.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
static int _volume_apply_bounds_geometry(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return -1;

    static const float texcoords[36][3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
    };
    float positions[36][3] = {0};
    for (uint32_t i = 0; i < 36; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            double min_value = visual->volume.bounds_min[j];
            double max_value = visual->volume.bounds_max[j];
            positions[i][j] = (float)(min_value + (max_value - min_value) * texcoords[i][j]);
        }
    }

    if (dvz_visual_set_data(visual, "position", positions, 36) != 0 ||
        dvz_visual_set_data(visual, "texcoords", texcoords, 36) != 0)
    {
        log_error("dvz_volume: failed to initialize default box geometry");
        return -1;
    }
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a volume visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_volume(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_VOLUME, flags);
    if (visual == NULL)
        return NULL;

    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (_volume_apply_bounds_geometry(visual) != 0)
        log_error("dvz_volume: failed to apply default bounds");
    return visual;
}



/**
 * Set the global opacity multiplier on a volume visual.
 *
 * @param visual the volume visual
 * @param opacity opacity multiplier in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_opacity(DvzVisual* visual, float opacity)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_opacity requires a volume visual");
        return -1;
    }
    if (!isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
    {
        log_error("volume opacity must be finite and in [0, 1]");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume opacity"))
        return -1;
    visual->volume.opacity = opacity;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the texture sampling mode on a volume visual.
 *
 * @param visual the volume visual
 * @param sampling the sampling mode
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_sampling(DvzVisual* visual, DvzVolumeSamplingMode sampling)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_sampling requires a volume visual");
        return -1;
    }
    if (sampling != DVZ_VOLUME_SAMPLING_LINEAR && sampling != DVZ_VOLUME_SAMPLING_NEAREST)
    {
        log_error("unsupported volume sampling mode %d", (int)sampling);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume sampling"))
        return -1;
    visual->volume.sampling = sampling;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the volume render mode.
 *
 * @param visual the volume visual
 * @param mode the render mode
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_render_mode(DvzVisual* visual, DvzVolumeRenderMode mode)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_render_mode requires a volume visual");
        return -1;
    }
    if (mode != DVZ_VOLUME_RENDER_SLICE && mode != DVZ_VOLUME_RENDER_MIP &&
        mode != DVZ_VOLUME_RENDER_COMPOSITE)
    {
        log_error("unsupported volume render mode %d", (int)mode);
        return -1;
    }
    DvzSceneSampleProfile profile = {0};
    if (
        mode == DVZ_VOLUME_RENDER_MIP && visual->field != NULL &&
        _scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile) &&
        _scene_sample_profile_is_integer_label(&profile))
    {
        log_error("label volumes only support slice and composite render modes");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume render mode"))
        return -1;
    visual->volume.render_mode = mode;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the slice axis used by slice rendering.
 *
 * @param visual the volume visual
 * @param axis axis normal for slice plane selection
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_slice_axis requires a volume visual");
        return -1;
    }
    if (axis < DVZ_VOLUME_AXIS_X || axis > DVZ_VOLUME_AXIS_Z)
    {
        log_error("unsupported volume slice axis %d", (int)axis);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume slice axis"))
        return -1;
    visual->volume.slice_axis = axis;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the normalized slice position for slice rendering.
 *
 * @param visual the volume visual
 * @param position normalized slice coordinate in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_slice_position(DvzVisual* visual, double position)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_slice_position requires a volume visual");
        return -1;
    }
    if (!isfinite(position) || position < 0.0 || position > 1.0)
    {
        log_error("volume slice position must be finite and in [0, 1]");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume slice position"))
        return -1;
    visual->volume.slice_position = position;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the volume raymarch step count used by MIP rendering.
 *
 * @param visual the volume visual
 * @param step_count number of raymarch samples
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_step_count(DvzVisual* visual, uint32_t step_count)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_step_count requires a volume visual");
        return -1;
    }
    if (step_count == 0 || step_count > 1024)
    {
        log_error("volume step count must be in [1, 1024]");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume step count"))
        return -1;
    visual->volume.step_count = step_count;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the object-space volume proxy bounds.
 *
 * @param visual the volume visual
 * @param bounds_min minimum object-space coordinate
 * @param bounds_max maximum object-space coordinate
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_bounds(
    DvzVisual* visual, const double bounds_min[3], const double bounds_max[3])
{
    ANN(visual);
    ANN(bounds_min);
    ANN(bounds_max);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_bounds requires a volume visual");
        return -1;
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(bounds_min[i]) || !isfinite(bounds_max[i]) || bounds_min[i] >= bounds_max[i])
        {
            log_error("volume bounds must be finite and satisfy min < max");
            return -1;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume bounds"))
        return -1;
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.bounds_min[i] = bounds_min[i];
        visual->volume.bounds_max[i] = bounds_max[i];
    }
    if (_volume_apply_bounds_geometry(visual) != 0)
        return -1;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the mapping from normalized volume coordinates to texture UVW coordinates.
 *
 * @param visual the volume visual
 * @param axis_order texture-axis source order
 * @param axis_flip optional per-texture-axis flips
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_axis_mapping(
    DvzVisual* visual, const uint32_t axis_order[3], const bool axis_flip[3])
{
    ANN(visual);
    ANN(axis_order);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_axis_mapping requires a volume visual");
        return -1;
    }
    bool seen[3] = {false, false, false};
    for (uint32_t i = 0; i < 3; i++)
    {
        if (axis_order[i] > 2 || seen[axis_order[i]])
        {
            log_error("volume axis order must be a permutation of 0, 1, 2");
            return -1;
        }
        seen[axis_order[i]] = true;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume axis mapping"))
        return -1;
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.axis_order[i] = axis_order[i];
        visual->volume.axis_flip[i] = axis_flip != NULL ? axis_flip[i] : false;
    }
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the scalar value range used before transfer texture lookup.
 *
 * @param visual the volume visual
 * @param min minimum scalar value mapped to 0
 * @param max maximum scalar value mapped to 1
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_value_range(DvzVisual* visual, double min, double max)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_value_range requires a volume visual");
        return -1;
    }
    if (!isfinite(min) || !isfinite(max) || min >= max)
    {
        log_error("volume value range must be finite and satisfy min < max");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume value range"))
        return -1;
    visual->volume.value_min = min;
    visual->volume.value_max = max;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set piecewise-linear opacity stops for scalar volume transfer.
 *
 * @param visual the volume visual
 * @param stops alpha stops
 * @param count number of stops
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_alpha_stops(DvzVisual* visual, const DvzVolumeAlphaStop* stops, uint32_t count)
{
    ANN(visual);
    ANN(stops);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_alpha_stops requires a volume visual");
        return -1;
    }
    if (count == 0 || count > 8)
    {
        log_error("volume alpha stop count must be in [1, 8]");
        return -1;
    }
    DvzVolumeAlphaStop sorted[8] = {0};
    for (uint32_t i = 0; i < count; i++)
    {
        if (!isfinite(stops[i].position) || !isfinite(stops[i].alpha) ||
            stops[i].position < 0.0 || stops[i].position > 1.0 || stops[i].alpha < 0.0f ||
            stops[i].alpha > 1.0f)
        {
            log_error("volume alpha stops require finite position and alpha in [0, 1]");
            return -1;
        }
        sorted[i] = stops[i];
    }
    for (uint32_t i = 1; i < count; i++)
    {
        DvzVolumeAlphaStop stop = sorted[i];
        uint32_t j = i;
        while (j > 0 && sorted[j - 1].position > stop.position)
        {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = stop;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume alpha stops"))
        return -1;
    for (uint32_t i = 0; i < count; i++)
        visual->volume.alpha_stops[i] = sorted[i];
    visual->volume.alpha_stop_count = count;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Enable axis-aligned clipping on a volume visual.
 *
 * @param visual the volume visual
 * @param clip_min minimum normalized clip coordinate
 * @param clip_max maximum normalized clip coordinate
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_clipping_box(
    DvzVisual* visual, const double clip_min[3], const double clip_max[3])
{
    ANN(visual);
    ANN(clip_min);
    ANN(clip_max);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_clipping_box requires a volume visual");
        return -1;
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(clip_min[i]) || !isfinite(clip_max[i]) || clip_min[i] < 0.0 ||
            clip_max[i] > 1.0 || clip_min[i] > clip_max[i])
        {
            log_error("volume clipping box coordinates must satisfy 0 <= min <= max <= 1");
            return -1;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume clipping box"))
        return -1;
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.clip_min[i] = clip_min[i];
        visual->volume.clip_max[i] = clip_max[i];
    }
    visual->volume.clipping_enabled = true;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Enable arbitrary plane clipping on a volume visual.
 *
 * @param visual the volume visual
 * @param point point on the clipping plane, in normalized volume coordinates
 * @param normal non-zero clipping plane normal
 * @param keep_positive whether to keep the positive side of the plane
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_clipping_plane(
    DvzVisual* visual, const double point[3], const double normal[3], bool keep_positive)
{
    ANN(visual);
    ANN(point);
    ANN(normal);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_clipping_plane requires a volume visual");
        return -1;
    }
    double norm2 = 0.0;
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(point[i]) || !isfinite(normal[i]) || point[i] < 0.0 || point[i] > 1.0)
        {
            log_error("volume clipping plane point must be finite and in [0, 1]");
            return -1;
        }
        norm2 += normal[i] * normal[i];
    }
    if (norm2 <= 0.0 || !isfinite(norm2))
    {
        log_error("volume clipping plane normal must be finite and non-zero");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume clipping plane"))
        return -1;
    double inv_norm = 1.0 / sqrt(norm2);
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.clip_plane_point[i] = point[i];
        visual->volume.clip_plane_normal[i] = normal[i] * inv_norm;
    }
    visual->volume.clip_plane_keep_positive = keep_positive;
    visual->volume.clip_plane_enabled = true;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Disable arbitrary plane clipping on a volume visual.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
int dvz_volume_clear_clipping_plane(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_clear_clipping_plane requires a volume visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "clear volume clipping plane"))
        return -1;
    visual->volume.clip_plane_enabled = false;
    visual->volume.clip_plane_keep_positive = false;
    visual->volume.clip_plane_point[0] = 0.5;
    visual->volume.clip_plane_point[1] = 0.5;
    visual->volume.clip_plane_point[2] = 0.5;
    visual->volume.clip_plane_normal[0] = 1.0;
    visual->volume.clip_plane_normal[1] = 0.0;
    visual->volume.clip_plane_normal[2] = 0.0;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Disable all clipping on a volume visual.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
int dvz_volume_clear_clipping(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_clear_clipping requires a volume visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "clear volume clipping"))
        return -1;
    visual->volume.clipping_enabled = false;
    visual->volume.clip_min[0] = 0.0;
    visual->volume.clip_min[1] = 0.0;
    visual->volume.clip_min[2] = 0.0;
    visual->volume.clip_max[0] = 1.0;
    visual->volume.clip_max[1] = 1.0;
    visual->volume.clip_max[2] = 1.0;
    visual->volume.clip_plane_enabled = false;
    visual->volume.clip_plane_keep_positive = false;
    visual->volume.clip_plane_point[0] = 0.5;
    visual->volume.clip_plane_point[1] = 0.5;
    visual->volume.clip_plane_point[2] = 0.5;
    visual->volume.clip_plane_normal[0] = 1.0;
    visual->volume.clip_plane_normal[1] = 0.0;
    visual->volume.clip_plane_normal[2] = 0.0;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return the retained volume state for inspection.
 *
 * @param visual the volume visual
 * @return the volume state, or NULL on error
 */
const DvzVolumeState* dvz_volume_state(const DvzVisual* visual)
{
    if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return NULL;
    return &visual->volume;
}
