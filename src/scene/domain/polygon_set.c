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


/**
 * Release a retained polygon set's copied region data.
 *
 * @param set the polygon set
 */
void _scene_polygon_set_reset(DvzPolygonSet* set)
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
DvzPolygonSet* dvz_polygon_set(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (!_scene_visual_mutation_allowed(scene, "create polygon set"))
        return NULL;

    DvzPolygonSet* set = _scene_alloc_polygon_set(scene);
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
void dvz_polygon_set_destroy(DvzPolygonSet* set)
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
uint32_t dvz_polygon_set_add(DvzPolygonSet* set, const DvzPolygonDesc* desc)
{
    if (set == NULL || set->scene == NULL || desc == NULL)
        return UINT32_MAX;
    if (!_scene_visual_mutation_allowed(set->scene, "append polygon set region"))
        return UINT32_MAX;
    if (!_polygon_set_reserve(set, set->polygon_count + 1))
        return UINT32_MAX;

    const uint32_t index = set->polygon_count;
    DvzPolygonSetItem* item = &set->polygons[index];
    dvz_memset(item, sizeof(DvzPolygonSetItem), 0, sizeof(DvzPolygonSetItem));
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
int dvz_polygon_set_region_geometry(
    DvzPolygonSet* set, uint32_t polygon_index, const DvzPolygonDesc* desc)
{
    if (
        set == NULL || set->scene == NULL || polygon_index >= set->polygon_count || desc == NULL)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set geometry"))
        return -1;

    DvzPolygonSetItem* item = &set->polygons[polygon_index];
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
int dvz_polygon_set_region_fill_color(
    DvzPolygonSet* set, uint32_t polygon_index, const DvzColor color)
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
 * Set one polygon region's stroke color.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param color RGBA stroke color
 * @return 0 on success, -1 on error
 */
int dvz_polygon_set_region_stroke_color(
    DvzPolygonSet* set, uint32_t polygon_index, const DvzColor color)
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
 * Set one polygon region's stroke width in pixels.
 *
 * @param set the polygon set
 * @param polygon_index polygon index
 * @param width stroke width in pixels
 * @return 0 on success, -1 on error
 */
int dvz_polygon_set_region_stroke_width(DvzPolygonSet* set, uint32_t polygon_index, float width)
{
    if (
        set == NULL || set->scene == NULL || !isfinite(width) || width < 0.0f ||
        polygon_index >= set->polygon_count)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(set->scene, "update polygon set stroke width"))
        return -1;
    set->polygons[polygon_index].stroke_width = width;
    set->polygons[polygon_index].version++;
    set->version++;
    _polygon_set_mark_composites_dirty(set, false, true);
    return 0;
}
