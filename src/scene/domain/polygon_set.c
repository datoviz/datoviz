/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon set semantic objects                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "polygon_internal.h"
#include "_visual_internal.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


static bool _polygon_set_range_valid(uint32_t count, uint32_t first, uint32_t item_count)
{
    return first <= count && item_count <= count - first;
}


/**
 * Release a retained polygon set's copied region data.
 *
 * @param set the polygon set
 */
void _scene_polygon_set_reset(DvzPolygons* set)
{
    if (set == NULL)
        return;

    if (set->polygons != NULL)
    {
        for (uint32_t i = 0; i < set->polygon_count; i++)
            _polygon_set_item_reset(&set->polygons[i]);
    }
    dvz_free(set->polygons);
    set->polygons = NULL;
    set->polygon_count = 0;
    set->polygon_capacity = 0;
    set->scene = NULL;
    set->active = false;
}


/**
 * Create a scene-owned semantic polygon set object.
 *
 * @param scene the scene
 * @param flags reserved polygon-set flags
 * @return the polygon set, or NULL on allocation failure
 */
DvzPolygons* dvz_polygons(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (!_scene_visual_mutation_allowed(scene, "create polygon set"))
        return NULL;

    DvzPolygons* set = _scene_alloc_polygon_set(scene);
    if (set == NULL)
        return NULL;
    set->flags = flags;
    return set;
}


/**
 * Destroy a scene-owned polygon set object.
 *
 * @param set the polygon set
 */
void dvz_polygons_destroy(DvzPolygons* set)
{
    if (set == NULL || set->scene == NULL)
        return;
    DvzScene* scene = set->scene;
    if (!_scene_visual_mutation_allowed(scene, "destroy polygon set"))
        return;

    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (composite->active && composite->source == set)
            dvz_composite_destroy(composite);
    }
    _scene_polygon_set_reset(set);
}


/**
 * Append one polygon region to a polygon set.
 *
 * @param set the polygon set
 * @param desc borrowed polygon descriptor
 * @return the polygon index, or UINT32_MAX on error
 */
uint32_t dvz_polygons_add_region(DvzPolygons* set, const DvzPolygonDesc* desc)
{
    if (set == NULL || set->scene == NULL || desc == NULL)
        return UINT32_MAX;
    if (!_scene_visual_mutation_allowed(set->scene, "append polygon set region"))
        return UINT32_MAX;
    if (!_polygon_set_reserve(set, set->polygon_count + 1))
        return UINT32_MAX;

    const uint32_t index = set->polygon_count;
    DvzPolygonsItem* item = &set->polygons[index];
    dvz_memset(item, sizeof(DvzPolygonsItem), 0, sizeof(DvzPolygonsItem));
    _polygon_set_item_default_style(item);
    if (_polygon_set_item_set_geometry(item, desc) != 0)
    {
        _polygon_set_item_reset(item);
        return UINT32_MAX;
    }

    set->polygon_count++;
    set->version++;
    _polygon_set_mark_composites_dirty(set, true, true);
    return index;
}


/**
 * Replace one polygon region's rings.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_geometry(
    DvzPolygons* set, uint32_t polygon_index, const DvzPolygonDesc* desc)
{
    if (
        set == NULL || set->scene == NULL || polygon_index >= set->polygon_count || desc == NULL)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set geometry"))
        return -1;

    DvzPolygonsItem* item = &set->polygons[polygon_index];
    if (_polygon_set_item_set_geometry(item, desc) != 0)
        return -1;
    set->version++;
    _polygon_set_mark_composites_dirty(set, true, true);
    return 0;
}


/**
 * Set one polygon region's fill color.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param color RGBA fill color
 * @return 0 on success, -1 on error
 */
/**
 * Set one polygon region's stable user id.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param id stable user id
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_id(DvzPolygons* set, uint32_t polygon_index, uint64_t id)
{
    if (set == NULL || set->scene == NULL || polygon_index >= set->polygon_count)
        return -1;
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set region id"))
        return -1;
    set->polygons[polygon_index].user_id = id;
    set->polygons[polygon_index].version++;
    set->version++;
    return 0;
}



/**
 * Set a contiguous range of polygon region stable user ids.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param ids borrowed stable user id array
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_ids(
    DvzPolygons* set, uint32_t first_polygon, uint32_t polygon_count, const uint64_t* ids)
{
    if (
        set == NULL || set->scene == NULL || ids == NULL ||
        !_polygon_set_range_valid(set->polygon_count, first_polygon, polygon_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set region ids"))
        return -1;
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        DvzPolygonsItem* region = &set->polygons[first_polygon + i];
        region->user_id = ids[i];
        region->version++;
    }
    set->version++;
    return 0;
}



/**
 * Set one polygon region's visibility.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param visible whether the region should render
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_visible(DvzPolygons* set, uint32_t polygon_index, bool visible)
{
    if (set == NULL || set->scene == NULL || polygon_index >= set->polygon_count)
        return -1;
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set region visibility"))
        return -1;
    DvzPolygonsItem* region = &set->polygons[polygon_index];
    if (region->visible == visible)
        return 0;
    region->visible = visible;
    region->version++;
    set->version++;
    _polygon_set_mark_composites_dirty(set, true, true);
    return 0;
}



/**
 * Set a contiguous range of polygon region visibilities.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param visible borrowed visibility array
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_visibilities(
    DvzPolygons* set, uint32_t first_polygon, uint32_t polygon_count, const bool* visible)
{
    if (
        set == NULL || set->scene == NULL || visible == NULL ||
        !_polygon_set_range_valid(set->polygon_count, first_polygon, polygon_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set region visibilities"))
        return -1;

    bool changed = false;
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        DvzPolygonsItem* region = &set->polygons[first_polygon + i];
        if (region->visible == visible[i])
            continue;
        region->visible = visible[i];
        region->version++;
        changed = true;
    }
    if (!changed)
        return 0;
    set->version++;
    _polygon_set_mark_composites_dirty(set, true, true);
    return 0;
}



/**
 * Set one polygon region's fill color.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param color RGBA fill color
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_fill_color(
    DvzPolygons* set, uint32_t polygon_index, const DvzColor color)
{
    if (
        set == NULL || set->scene == NULL || polygon_index >= set->polygon_count)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set fill color"))
        return -1;
    _polygon_color_copy(&set->polygons[polygon_index].fill_color, color);
    set->polygons[polygon_index].version++;
    set->version++;
    _polygon_set_mark_composites_dirty(set, true, false);
    return 0;
}


/**
 * Set a contiguous range of polygon region fill colors.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param colors RGBA fill colors
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_fill_colors(
    DvzPolygons* set, uint32_t first_polygon, uint32_t polygon_count, const DvzColor* colors)
{
    if (
        set == NULL || set->scene == NULL || colors == NULL ||
        first_polygon > set->polygon_count || polygon_count > set->polygon_count - first_polygon)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set fill colors"))
        return -1;
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        DvzPolygonsItem* region = &set->polygons[first_polygon + i];
        _polygon_color_copy(&region->fill_color, colors[i]);
        region->version++;
    }
    set->version++;
    _polygon_set_mark_composites_dirty(set, true, false);
    return 0;
}



/**
 * Set one polygon region's stroke color.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param color RGBA stroke color
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_stroke_color(
    DvzPolygons* set, uint32_t polygon_index, const DvzColor color)
{
    if (
        set == NULL || set->scene == NULL || polygon_index >= set->polygon_count)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke color"))
        return -1;
    _polygon_color_copy(&set->polygons[polygon_index].stroke_color, color);
    set->polygons[polygon_index].version++;
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}


/**
 * Set a contiguous range of polygon region stroke colors.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param colors RGBA stroke colors
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_stroke_colors(
    DvzPolygons* set, uint32_t first_polygon, uint32_t polygon_count, const DvzColor* colors)
{
    if (
        set == NULL || set->scene == NULL || colors == NULL ||
        first_polygon > set->polygon_count || polygon_count > set->polygon_count - first_polygon)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke colors"))
        return -1;
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        DvzPolygonsItem* region = &set->polygons[first_polygon + i];
        _polygon_color_copy(&region->stroke_color, colors[i]);
        region->version++;
    }
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}



/**
 * Set one polygon region's stroke width in pixels.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param width stroke width in pixels
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_stroke_width_px(DvzPolygons* set, uint32_t polygon_index, float width)
{
    if (
        set == NULL || set->scene == NULL || !isfinite(width) || width < 0.0f ||
        polygon_index >= set->polygon_count)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke width"))
        return -1;
    set->polygons[polygon_index].stroke_width_px = width;
    set->polygons[polygon_index].version++;
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}



/**
 * Set a contiguous range of polygon region stroke widths.
 *
 * @param set the polygon set
 * @param first_polygon first polygon index
 * @param polygon_count number of regions to update
 * @param widths stroke widths in pixels
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_region_stroke_widths_px(
    DvzPolygons* set, uint32_t first_polygon, uint32_t polygon_count, const float* widths)
{
    if (
        set == NULL || set->scene == NULL || widths == NULL ||
        first_polygon > set->polygon_count || polygon_count > set->polygon_count - first_polygon)
    {
        return -1;
    }
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        if (!isfinite(widths[i]) || widths[i] < 0.0f)
            return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke widths"))
        return -1;
    for (uint32_t i = 0; i < polygon_count; i++)
    {
        DvzPolygonsItem* region = &set->polygons[first_polygon + i];
        region->stroke_width_px = widths[i];
        region->version++;
    }
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}



/**
 * Return whether one stroke cap value is valid.
 *
 * @param cap stroke cap
 * @return whether the cap is valid
 */
static bool _polygon_set_stroke_cap_valid(DvzSegmentCap cap)
{
    return cap == DVZ_SEGMENT_CAP_NONE || cap == DVZ_SEGMENT_CAP_ROUND ||
           cap == DVZ_SEGMENT_CAP_TRIANGLE_IN || cap == DVZ_SEGMENT_CAP_TRIANGLE_OUT ||
           cap == DVZ_SEGMENT_CAP_SQUARE || cap == DVZ_SEGMENT_CAP_BUTT;
}



/**
 * Return whether one path join value is valid.
 *
 * @param join path join
 * @return whether the join is valid
 */
static bool _polygon_set_stroke_join_valid(DvzPathJoin join)
{
    return join == DVZ_PATH_JOIN_MITER || join == DVZ_PATH_JOIN_ROUND ||
           join == DVZ_PATH_JOIN_BEVEL;
}



/**
 * Configure polygon-set stroke endpoint caps.
 *
 * @param set the polygon set
 * @param start_cap cap applied to each ring start
 * @param end_cap cap applied to each ring end
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_stroke_caps(
    DvzPolygons* set, DvzSegmentCap start_cap, DvzSegmentCap end_cap)
{
    if (
        set == NULL || set->scene == NULL || !_polygon_set_stroke_cap_valid(start_cap) ||
        !_polygon_set_stroke_cap_valid(end_cap))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke caps"))
        return -1;
    if (set->stroke_cap_start == start_cap && set->stroke_cap_end == end_cap)
        return 0;
    set->stroke_cap_start = start_cap;
    set->stroke_cap_end = end_cap;
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}



/**
 * Configure polygon-set stroke joins.
 *
 * @param set the polygon set
 * @param join join style
 * @param miter_limit positive finite miter limit
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygons_set_stroke_join(DvzPolygons* set, DvzPathJoin join, float miter_limit)
{
    if (
        set == NULL || set->scene == NULL || !_polygon_set_stroke_join_valid(join) ||
        !isfinite(miter_limit) || miter_limit <= 0.0f)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke join"))
        return -1;
    if (set->stroke_join == join && set->stroke_miter_limit == miter_limit)
        return 0;
    set->stroke_join = join;
    set->stroke_miter_limit = miter_limit;
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}
