/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon semantic storage helpers                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "polygon_internal.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a count-sized allocation is representable.
 *
 * @param count number of items
 * @param item_size item size in bytes
 * @return whether the allocation size is valid
 */
bool _polygon_allocation_valid(uint32_t count, DvzSize item_size)
{
    uint64_t total = 0;
    return !_dvz_mul_u64_overflows((uint64_t)count, (uint64_t)item_size, &total);
}



/**
 * Copy one color value.
 *
 * @param dst output color
 * @param src input color
 */
void _polygon_color_copy(DvzColor* dst, const DvzColor src)
{
    ANN(dst);
    *dst = src;
}



/**
 * Return whether two 2D points are exactly equal.
 *
 * @param a first point
 * @param b second point
 * @return whether the points have equal coordinates
 */
static bool _polygon_points_equal(const dvec2 a, const dvec2 b)
{
    return a != NULL && b != NULL && a[0] == b[0] && a[1] == b[1];
}



/**
 * Return whether a 2D point can be represented as finite F32 coordinates.
 *
 * @param xy input point
 * @return whether both coordinates are finite and in float range
 */
static bool _polygon_point_f32_valid(const dvec2 xy)
{
    return xy != NULL && isfinite(xy[0]) && isfinite(xy[1]) && xy[0] >= -(double)FLT_MAX &&
           xy[0] <= (double)FLT_MAX && xy[1] >= -(double)FLT_MAX && xy[1] <= (double)FLT_MAX;
}



/**
 * Return the render vertex count after dropping an optional repeated closing point.
 *
 * @param ring stored ring
 * @return normalized vertex count
 */
uint32_t _polygon_stored_ring_count(const DvzPolygonStoredRing* ring)
{
    if (ring == NULL || ring->xy == NULL || ring->count == 0)
        return 0;

    uint32_t count = ring->count;
    if (count >= 2 && _polygon_points_equal(ring->xy[0], ring->xy[count - 1]))
        count--;
    return count;
}



/**
 * Release one stored ring.
 *
 * @param ring ring to release
 */
void _polygon_ring_reset(DvzPolygonStoredRing* ring)
{
    if (ring == NULL)
        return;
    dvz_free(ring->xy);
    ring->xy = NULL;
    ring->count = 0;
}



/**
 * Copy one borrowed ring into retained storage.
 *
 * @param src borrowed ring descriptor
 * @param dst output stored ring
 * @return whether the ring was copied
 */
bool _polygon_ring_copy(const DvzPolygonRing* src, DvzPolygonStoredRing* dst)
{
    if (src == NULL || dst == NULL || src->xy == NULL || src->count == 0)
        return false;
    if (!_polygon_allocation_valid(src->count, sizeof(dvec2)))
        return false;

    for (uint32_t i = 0; i < src->count; i++)
    {
        if (!_polygon_point_f32_valid(src->xy[i]))
            return false;
    }

    dvec2* xy = (dvec2*)dvz_calloc(src->count, sizeof(dvec2));
    if (xy == NULL)
        return false;

    const size_t byte_size = (size_t)src->count * sizeof(dvec2);
    dvz_memcpy(xy, byte_size, src->xy, byte_size);
    dst->xy = xy;
    dst->count = src->count;
    return true;
}



/**
 * Release all retained data in one polygon-set item.
 *
 * @param item polygon-set item
 */
void _polygon_set_item_reset(DvzPolygonSetItem* item)
{
    if (item == NULL)
        return;
    _polygon_ring_reset(&item->outer);
    if (item->holes != NULL)
    {
        for (uint32_t i = 0; i < item->hole_count; i++)
            _polygon_ring_reset(&item->holes[i]);
    }
    dvz_free(item->holes);
    dvz_memset(item, sizeof(DvzPolygonSetItem), 0, sizeof(DvzPolygonSetItem));
}



/**
 * Initialize default style on one polygon-set item.
 *
 * @param item polygon-set item
 */
void _polygon_set_item_default_style(DvzPolygonSetItem* item)
{
    ANN(item);
    DvzPolygonStyle style = dvz_polygon_style();
    item->visible = style.visible;
    item->fill_color = style.fill_color;
    item->stroke_color = style.stroke_color;
    item->stroke_width_px = style.stroke_width_px;
}



/**
 * Copy a borrowed polygon descriptor into retained ring storage.
 *
 * @param desc borrowed polygon descriptor
 * @param outer output outer ring
 * @param holes output hole array
 * @param hole_count output hole count
 * @return 0 on success, -1 on invalid input or allocation failure
 */
int _polygon_copy_desc(
    const DvzPolygonDesc* desc, DvzPolygonStoredRing* outer, DvzPolygonStoredRing** holes,
    uint32_t* hole_count)
{
    if (desc == NULL || outer == NULL || holes == NULL || hole_count == NULL)
        return -1;

    DvzGeometry* probe = dvz_triangulate_polygon(desc, NULL);
    if (probe == NULL)
        return -1;
    dvz_geometry_destroy(probe);

    DvzPolygonStoredRing copied_outer = {0};
    DvzPolygonStoredRing* copied_holes = NULL;
    if (!_polygon_ring_copy(&desc->outer, &copied_outer))
        return -1;

    if (desc->hole_count > 0)
    {
        if (!_polygon_allocation_valid(desc->hole_count, sizeof(DvzPolygonStoredRing)))
        {
            _polygon_ring_reset(&copied_outer);
            return -1;
        }
        copied_holes =
            (DvzPolygonStoredRing*)dvz_calloc(desc->hole_count, sizeof(DvzPolygonStoredRing));
        if (copied_holes == NULL)
        {
            _polygon_ring_reset(&copied_outer);
            return -1;
        }
        for (uint32_t i = 0; i < desc->hole_count; i++)
        {
            if (!_polygon_ring_copy(&desc->holes[i], &copied_holes[i]))
            {
                for (uint32_t j = 0; j < i; j++)
                    _polygon_ring_reset(&copied_holes[j]);
                dvz_free(copied_holes);
                _polygon_ring_reset(&copied_outer);
                return -1;
            }
        }
    }

    *outer = copied_outer;
    *holes = copied_holes;
    *hole_count = desc->hole_count;
    return 0;
}



/**
 * Build borrowed ring views from one retained polygon.
 *
 * @param polygon retained polygon
 * @param outer output outer ring view
 * @param holes output hole view pointer, or NULL when no holes
 * @return whether the views were built
 */
bool _polygon_borrowed_desc(
    const DvzPolygon* polygon, DvzPolygonRing* outer, DvzPolygonRing** holes)
{
    if (polygon == NULL || outer == NULL || holes == NULL || polygon->outer.xy == NULL)
        return false;

    *outer = (DvzPolygonRing){.xy = (const dvec2*)polygon->outer.xy, .count = polygon->outer.count};
    *holes = NULL;
    if (polygon->hole_count == 0)
        return true;

    if (!_polygon_allocation_valid(polygon->hole_count, sizeof(DvzPolygonRing)))
        return false;
    DvzPolygonRing* out = (DvzPolygonRing*)dvz_calloc(polygon->hole_count, sizeof(DvzPolygonRing));
    if (out == NULL)
        return false;

    for (uint32_t i = 0; i < polygon->hole_count; i++)
        out[i] = (DvzPolygonRing){
            .xy = (const dvec2*)polygon->holes[i].xy,
            .count = polygon->holes[i].count,
        };
    *holes = out;
    return true;
}



/**
 * Notify generated visuals for every composite depending on one polygon.
 *
 * @param polygon changed polygon
 * @param fill_dirty whether the fill role needs refresh
 * @param stroke_dirty whether the stroke role needs refresh
 */
void _polygon_mark_composites_dirty(DvzPolygon* polygon, bool fill_dirty, bool stroke_dirty)
{
    if (polygon == NULL || polygon->scene == NULL)
        return;

    DvzScene* scene = polygon->scene;
    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (!composite->active || composite->type != DVZ_COMPOSITE_TYPE_POLYGON ||
            composite->source != polygon)
        {
            continue;
        }
        for (uint32_t j = 0; j < composite->visual_count; j++)
        {
            DvzCompositeVisual* composite_visual = &composite->visuals[j];
            bool notify_fill =
                fill_dirty && strcmp(composite_visual->role, DVZ_POLYGON_COMPOSITE_FILL_ROLE) == 0;
            bool notify_stroke = stroke_dirty &&
                                 strcmp(composite_visual->role,
                                        DVZ_POLYGON_COMPOSITE_STROKE_ROLE) == 0;
            if (notify_fill || notify_stroke)
            {
                composite->dirty = true;
                composite_visual->dirty = true;
                _scene_notify_visual_changed(composite_visual->visual);
            }
        }
    }
}



/**
 * Notify generated visuals for every composite depending on one polygon set.
 *
 * @param set changed polygon set
 * @param fill_dirty whether the fill role needs refresh
 * @param stroke_dirty whether the stroke role needs refresh
 */
void _polygon_set_mark_composites_dirty(
    DvzPolygonSet* set, bool fill_dirty, bool stroke_dirty)
{
    if (set == NULL || set->scene == NULL)
        return;

    DvzScene* scene = set->scene;
    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (!composite->active || composite->type != DVZ_COMPOSITE_TYPE_POLYGON_SET ||
            composite->source != set)
        {
            continue;
        }
        for (uint32_t j = 0; j < composite->visual_count; j++)
        {
            DvzCompositeVisual* composite_visual = &composite->visuals[j];
            bool notify_fill =
                fill_dirty && strcmp(composite_visual->role, DVZ_POLYGON_COMPOSITE_FILL_ROLE) == 0;
            bool notify_stroke = stroke_dirty &&
                                 strcmp(composite_visual->role,
                                        DVZ_POLYGON_COMPOSITE_STROKE_ROLE) == 0;
            if (notify_fill || notify_stroke)
            {
                composite->dirty = true;
                composite_visual->dirty = true;
                _scene_notify_visual_changed(composite_visual->visual);
            }
        }
    }
}



/**
 * Allocate one scene-owned polygon slot.
 *
 * @param scene the scene
 * @return the polygon slot, or NULL on capacity exhaustion
 */
DvzPolygon* _scene_alloc_polygon(DvzScene* scene)
{
    if (scene == NULL || scene->polygon_count >= DVZ_SCENE_MAX_POLYGONS)
        return NULL;

    DvzPolygon* polygon = &scene->polygons[scene->polygon_count++];
    dvz_memset(polygon, sizeof(DvzPolygon), 0, sizeof(DvzPolygon));
    polygon->scene = scene;
    polygon->active = true;
    DvzPolygonStyle style = dvz_polygon_style();
    polygon->visible = style.visible;
    polygon->fill_color = style.fill_color;
    polygon->stroke_color = style.stroke_color;
    polygon->stroke_width_px = style.stroke_width_px;
    polygon->stroke_cap_start = style.stroke_start_cap;
    polygon->stroke_cap_end = style.stroke_end_cap;
    polygon->stroke_join = style.stroke_join;
    polygon->stroke_miter_limit = style.stroke_miter_limit;
    polygon->version = 1;
    return polygon;
}



/**
 * Allocate one scene-owned polygon-set slot.
 *
 * @param scene the scene
 * @return the polygon set slot, or NULL on capacity exhaustion
 */
DvzPolygonSet* _scene_alloc_polygon_set(DvzScene* scene)
{
    if (scene == NULL || scene->polygon_set_count >= DVZ_SCENE_MAX_POLYGON_SETS)
        return NULL;

    DvzPolygonSet* set = &scene->polygon_sets[scene->polygon_set_count++];
    dvz_memset(set, sizeof(DvzPolygonSet), 0, sizeof(DvzPolygonSet));
    set->scene = scene;
    set->active = true;
    set->stroke_cap_start = DVZ_SEGMENT_CAP_BUTT;
    set->stroke_cap_end = DVZ_SEGMENT_CAP_BUTT;
    set->stroke_join = DVZ_PATH_JOIN_ROUND;
    set->stroke_miter_limit = 4.0f;
    set->version = 1;
    return set;
}



/**
 * Ensure a polygon set can store at least capacity polygon items.
 *
 * @param set polygon set
 * @param capacity required capacity
 * @return whether the reserve succeeded
 */
bool _polygon_set_reserve(DvzPolygonSet* set, uint32_t capacity)
{
    if (set == NULL)
        return false;
    if (capacity <= set->polygon_capacity)
        return true;
    if (!_polygon_allocation_valid(capacity, sizeof(DvzPolygonSetItem)))
        return false;

    uint32_t next_capacity =
        set->polygon_capacity == 0 ? DVZ_POLYGON_SET_INITIAL_CAPACITY : set->polygon_capacity * 2;
    if (next_capacity < capacity)
        next_capacity = capacity;

    DvzPolygonSetItem* next =
        (DvzPolygonSetItem*)dvz_calloc(next_capacity, sizeof(DvzPolygonSetItem));
    if (next == NULL)
        return false;

    if (set->polygons != NULL && set->polygon_count > 0)
    {
        const size_t byte_size = (size_t)set->polygon_count * sizeof(DvzPolygonSetItem);
        dvz_memcpy(next, (size_t)next_capacity * sizeof(DvzPolygonSetItem), set->polygons,
                   byte_size);
    }
    dvz_free(set->polygons);
    set->polygons = next;
    set->polygon_capacity = next_capacity;
    return true;
}



/**
 * Replace one polygon-set item's retained geometry.
 *
 * @param item polygon-set item
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on error
 */
int _polygon_set_item_set_geometry(DvzPolygonSetItem* item, const DvzPolygonDesc* desc)
{
    if (item == NULL || desc == NULL)
        return -1;

    DvzPolygonStoredRing outer = {0};
    DvzPolygonStoredRing* holes = NULL;
    uint32_t hole_count = 0;
    if (_polygon_copy_desc(desc, &outer, &holes, &hole_count) != 0)
        return -1;

    _polygon_ring_reset(&item->outer);
    if (item->holes != NULL)
    {
        for (uint32_t i = 0; i < item->hole_count; i++)
            _polygon_ring_reset(&item->holes[i]);
    }
    dvz_free(item->holes);

    item->outer = outer;
    item->holes = holes;
    item->hole_count = hole_count;
    item->active = true;
    item->version++;
    return 0;
}
