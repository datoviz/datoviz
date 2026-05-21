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

DvzPointStyleDesc dvz_point_style_desc(void)
{
    DvzPointStyleDesc desc = {
        .edge_color = {0, 0, 0, 255},
        .stroke_width = 0.0f,
        .filled = true,
        .stroke = false,
        .outline = false,
    };
    return desc;
}


/**
 * Return default marker styling.
 *
 * @return default marker style descriptor
 */
DvzMarkerStyle dvz_marker_style(void)
{
    DvzMarkerStyle style = {
        .edge_color = {0, 0, 0, 255},
        .stroke_width = 0.0f,
        .filled = true,
        .stroke = false,
        .outline = false,
    };
    return style;
}


/**
 * Return whether one point style needs the style shader path.
 *
 * @param style the point style descriptor
 * @return whether style parameters differ from the shader defaults
 */
bool _point_style_enabled(const DvzPointStyleDesc* style)
{
    ANN(style);
    return style->outline || !style->filled || style->stroke;
}


/**
 * Convert a marker style to the shared point-like material payload.
 *
 * @param style the marker style
 * @return equivalent point style descriptor
 */
DvzPointStyleDesc _marker_style_to_point_style(const DvzMarkerStyle* style)
{
    ANN(style);
    DvzPointStyleDesc out = {
        .edge_color =
            {
                style->edge_color[0],
                style->edge_color[1],
                style->edge_color[2],
                style->edge_color[3],
            },
        .stroke_width = style->stroke_width,
        .filled = style->filled,
        .stroke = style->stroke,
        .outline = style->outline,
    };
    return out;
}


/**
 * Store point style data into the shared material payload used by point shaders.
 *
 * @param params the material parameter payload
 * @param style the point style descriptor
 */
void _point_style_sync_params(DvzSceneMaterialParams* params, const DvzPointStyleDesc* style)
{
    ANN(params);
    ANN(style);
    const bool stroke_enabled = style->stroke || style->outline;
    params->params[0] = stroke_enabled && style->stroke_width > 0.0f ? style->stroke_width : 0.0f;
    params->params[1] = style->filled ? 1.0f : 0.0f;
    params->params[2] = style->stroke ? 1.0f : 0.0f;
    params->params[3] = style->outline ? 1.0f : 0.0f;
    params->base_color_factor[0] = (float)style->edge_color[0] / 255.0f;
    params->base_color_factor[1] = (float)style->edge_color[1] / 255.0f;
    params->base_color_factor[2] = (float)style->edge_color[2] / 255.0f;
    params->base_color_factor[3] = (float)style->edge_color[3] / 255.0f;
}


/**
 * Return whether one segment cap enum value is supported by the first slice.
 *
 * @param cap the segment cap
 * @return whether the cap is valid
 */
bool _segment_cap_valid(DvzSegmentCap cap)
{
    return cap >= DVZ_SEGMENT_CAP_NONE && cap <= DVZ_SEGMENT_CAP_BUTT;
}


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
 * Release one segment visual's derived GPU upload cache.
 *
 * @param cache the segment GPU cache
 */
void _segment_gpu_cache_free(DvzSegmentGpuCache* cache)
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
 * Release one path visual's derived GPU upload cache.
 *
 * @param cache the path GPU cache
 */
void _path_gpu_cache_free(DvzPathGpuCache* cache)
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
 * Configure circular point fill/stroke styling.
 *
 * @param visual the point visual
 * @param desc the point style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
int dvz_point_set_style(DvzVisual* visual, const DvzPointStyleDesc* desc)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_POINT)
    {
        log_error("dvz_point_set_style requires a point visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update point style"))
        return -1;

    DvzPointStyleDesc style = desc != NULL ? *desc : dvz_point_style_desc();
    if (!isfinite(style.stroke_width) || style.stroke_width < 0.0f)
    {
        log_error("point stroke_width must be finite and nonnegative");
        return -1;
    }

    visual->material.point_style = style;
    visual->material.point_style_enabled = _point_style_enabled(&style);
    _visual_material_mark_dirty(visual);
    return 0;
}


/**
 * Configure marker fill/stroke styling.
 *
 * @param visual the marker visual
 * @param style the marker style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
int dvz_marker_set_style(DvzVisual* visual, const DvzMarkerStyle* style)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MARKER)
    {
        log_error("dvz_marker_set_style requires a marker visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update marker style"))
        return -1;

    DvzMarkerStyle marker_style = style != NULL ? *style : dvz_marker_style();
    if (!isfinite(marker_style.stroke_width) || marker_style.stroke_width < 0.0f)
    {
        log_error("marker stroke_width must be finite and nonnegative");
        return -1;
    }

    DvzPointStyleDesc point_style = _marker_style_to_point_style(&marker_style);
    visual->material.point_style = point_style;
    visual->material.point_style_enabled = _point_style_enabled(&point_style);
    _visual_material_mark_dirty(visual);
    return 0;
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
    if (!_segment_cap_valid(start_cap) || !_segment_cap_valid(end_cap))
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
 * Return whether one path join enum value is supported by the first slice.
 *
 * @param join the path join
 * @return whether the join is valid
 */
static bool _path_join_valid(DvzPathJoin join)
{
    return join >= DVZ_PATH_JOIN_MITER && join <= DVZ_PATH_JOIN_BEVEL;
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
    if (!_segment_cap_valid(start_cap) || !_segment_cap_valid(end_cap))
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
    if (!_path_join_valid(join))
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
 * Set the sphere impostor rendering mode.
 *
 * @param visual the sphere visual
 * @param mode the rendering mode
 * @return 0 on success, -1 on error
 */
int dvz_sphere_mode(DvzVisual* visual, DvzSphereMode mode)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_SPHERE)
    {
        log_error("dvz_sphere_mode requires a sphere visual");
        return -1;
    }
    if (mode != DVZ_SPHERE_MODE_FAST_IMPOSTOR && mode != DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR)
    {
        log_error("invalid sphere rendering mode");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update sphere mode"))
        return -1;

    if (visual->sphere_mode == mode)
        return 0;
    visual->sphere_mode = mode;
    _sphere_params_sync_mode(visual);
    _visual_bump_version(&visual->material.version);
    visual->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
    return 0;
}
