/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon semantic objects                                                       */
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

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


/**
 * Release a retained polygon's copied ring data.
 *
 * @param polygon the polygon
 */
void _scene_polygon_reset(DvzPolygon* polygon)
{
    if (polygon == NULL)
        return;

    _polygon_ring_reset(&polygon->outer);
    if (polygon->holes != NULL)
    {
        for (uint32_t i = 0; i < polygon->hole_count; i++)
            _polygon_ring_reset(&polygon->holes[i]);
    }
    dvz_free(polygon->holes);
    polygon->holes = NULL;
    polygon->hole_count = 0;
    polygon->scene = NULL;
    polygon->active = false;
}


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
 * Create a scene-owned semantic polygon object.
 *
 * @param scene the scene
 * @param flags reserved polygon flags
 * @return the polygon, or NULL on allocation failure
 */
DvzPolygon* dvz_polygon(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (!_scene_visual_mutation_allowed(scene, "create polygon"))
        return NULL;

    DvzPolygon* polygon = _scene_alloc_polygon(scene);
    if (polygon == NULL)
        return NULL;
    polygon->flags = flags;
    return polygon;
}


/**
 * Destroy a scene-owned polygon object and release its copied ring data.
 *
 * @param polygon the polygon
 */
void dvz_polygon_destroy(DvzPolygon* polygon)
{
    if (polygon == NULL || polygon->scene == NULL)
        return;
    DvzScene* scene = polygon->scene;
    if (!_scene_visual_mutation_allowed(scene, "destroy polygon"))
        return;

    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (composite->active && composite->source == polygon)
            dvz_composite_destroy(composite);
    }
    _scene_polygon_reset(polygon);
}


/**
 * Replace all polygon rings from a borrowed descriptor.
 *
 * @param polygon the polygon
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on invalid input or allocation failure
 */
int dvz_polygon_set_geometry(DvzPolygon* polygon, const DvzPolygonDesc* desc)
{
    if (polygon == NULL || polygon->scene == NULL || desc == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon geometry"))
        return -1;

    DvzGeometry* probe = dvz_triangulate_polygon(desc, NULL);
    if (probe == NULL)
        return -1;
    dvz_geometry_destroy(probe);

    DvzPolygonStoredRing outer = {0};
    DvzPolygonStoredRing* holes = NULL;
    if (!_polygon_ring_copy(&desc->outer, &outer))
        return -1;

    if (desc->hole_count > 0)
    {
        if (!_polygon_allocation_valid(desc->hole_count, sizeof(DvzPolygonStoredRing)))
        {
            _polygon_ring_reset(&outer);
            return -1;
        }
        holes = (DvzPolygonStoredRing*)dvz_calloc(desc->hole_count, sizeof(DvzPolygonStoredRing));
        if (holes == NULL)
        {
            _polygon_ring_reset(&outer);
            return -1;
        }
        for (uint32_t i = 0; i < desc->hole_count; i++)
        {
            if (!_polygon_ring_copy(&desc->holes[i], &holes[i]))
            {
                for (uint32_t j = 0; j < i; j++)
                    _polygon_ring_reset(&holes[j]);
                dvz_free(holes);
                _polygon_ring_reset(&outer);
                return -1;
            }
        }
    }

    _polygon_ring_reset(&polygon->outer);
    if (polygon->holes != NULL)
    {
        for (uint32_t i = 0; i < polygon->hole_count; i++)
            _polygon_ring_reset(&polygon->holes[i]);
    }
    dvz_free(polygon->holes);

    polygon->outer = outer;
    polygon->holes = holes;
    polygon->hole_count = desc->hole_count;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, true, true);
    return 0;
}


/**
 * Replace the polygon outer ring while preserving existing holes.
 *
 * @param polygon the polygon
 * @param count number of outer ring vertices
 * @param xy borrowed XY vertex array
 * @return 0 on success, -1 on invalid input or allocation failure
 */
int dvz_polygon_outer(DvzPolygon* polygon, uint32_t count, const dvec2* xy)
{
    if (polygon == NULL || xy == NULL)
        return -1;

    DvzPolygonRing* holes = NULL;
    DvzPolygonRing outer = {.xy = xy, .count = count};
    if (polygon->hole_count > 0)
    {
        if (!_polygon_borrowed_desc(polygon, &(DvzPolygonRing){0}, &holes))
            return -1;
    }
    const int out = dvz_polygon_set_geometry(
        polygon,
        &(DvzPolygonDesc){.outer = outer, .holes = holes, .hole_count = polygon->hole_count});
    dvz_free(holes);
    return out;
}


/**
 * Append or replace one polygon hole ring.
 *
 * @param polygon the polygon
 * @param hole_index hole index to replace, or current hole count to append
 * @param count number of hole ring vertices
 * @param xy borrowed XY vertex array
 * @return 0 on success, -1 on invalid input or allocation failure
 */
int dvz_polygon_hole(DvzPolygon* polygon, uint32_t hole_index, uint32_t count, const dvec2* xy)
{
    if (polygon == NULL || polygon->outer.xy == NULL || xy == NULL)
        return -1;
    if (hole_index > polygon->hole_count)
        return -1;

    const uint32_t new_hole_count =
        hole_index == polygon->hole_count ? polygon->hole_count + 1 : polygon->hole_count;
    if (!_polygon_allocation_valid(new_hole_count, sizeof(DvzPolygonRing)))
        return -1;

    DvzPolygonRing* holes = (DvzPolygonRing*)dvz_calloc(new_hole_count, sizeof(DvzPolygonRing));
    if (holes == NULL)
        return -1;

    for (uint32_t i = 0; i < new_hole_count; i++)
    {
        if (i == hole_index)
            holes[i] = (DvzPolygonRing){.xy = xy, .count = count};
        else
        {
            const uint32_t src = i > hole_index ? i - 1 : i;
            holes[i] = (DvzPolygonRing){
                .xy = (const dvec2*)polygon->holes[src].xy,
                .count = polygon->holes[src].count,
            };
        }
    }

    const DvzPolygonDesc desc = {
        .outer = {.xy = (const dvec2*)polygon->outer.xy, .count = polygon->outer.count},
        .holes = holes,
        .hole_count = new_hole_count,
    };
    const int out = dvz_polygon_set_geometry(polygon, &desc);
    dvz_free(holes);
    return out;
}


/**
 * Set the polygon fill color.
 *
 * @param polygon the polygon
 * @param color RGBA fill color
 * @return 0 on success, -1 on error
 */
int dvz_polygon_fill_color(DvzPolygon* polygon, const DvzColor color)
{
    if (polygon == NULL || polygon->scene == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon fill color"))
        return -1;
    _polygon_color_copy(&polygon->fill_color, color);
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, true, false);
    return 0;
}


/**
 * Set the polygon stroke color.
 *
 * @param polygon the polygon
 * @param color RGBA stroke color
 * @return 0 on success, -1 on error
 */
int dvz_polygon_stroke_color(DvzPolygon* polygon, const DvzColor color)
{
    if (polygon == NULL || polygon->scene == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon stroke color"))
        return -1;
    _polygon_color_copy(&polygon->stroke_color, color);
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, false, true);
    return 0;
}


/**
 * Set the polygon stroke width in pixels.
 *
 * @param polygon the polygon
 * @param width stroke width in pixels
 * @return 0 on success, -1 on invalid input
 */
int dvz_polygon_stroke_width(DvzPolygon* polygon, float width)
{
    if (polygon == NULL || polygon->scene == NULL || !isfinite(width) || width < 0.0f)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon stroke width"))
        return -1;
    polygon->stroke_width = width;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, false, true);
    return 0;
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
