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
#include "_visual_internal.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


#define DVZ_POLYGON_STYLE_KNOWN_FLAGS 0u


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
 * Return whether one stroke cap value is valid.
 *
 * @param cap stroke cap
 * @return whether the cap is valid
 */
static bool _polygon_stroke_cap_valid(DvzSegmentCap cap)
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
static bool _polygon_stroke_join_valid(DvzPathJoin join)
{
    return join == DVZ_PATH_JOIN_MITER || join == DVZ_PATH_JOIN_ROUND ||
           join == DVZ_PATH_JOIN_BEVEL;
}



static bool _polygon_style_valid(const DvzPolygonStyle* style)
{
    if (style == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(style, DvzPolygonStyle, DVZ_POLYGON_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzPolygonStyle ABI prologue");
        return false;
    }
    if (!isfinite(style->stroke_width_px) || style->stroke_width_px < 0.0f)
        return false;
    if (
        !_polygon_stroke_cap_valid(style->stroke_start_cap) ||
        !_polygon_stroke_cap_valid(style->stroke_end_cap))
    {
        return false;
    }
    if (!_polygon_stroke_join_valid(style->stroke_join))
        return false;
    if (!isfinite(style->stroke_miter_limit) || style->stroke_miter_limit <= 0.0f)
        return false;
    return true;
}



/**
 * Return the default polygon style descriptor.
 *
 * @return default polygon style
 */
DvzPolygonStyle dvz_polygon_style(void)
{
    return (DvzPolygonStyle){
        DVZ_STRUCT_INIT_FIELDS(DvzPolygonStyle),
        .visible = true,
        .fill_color = {255, 255, 255, 255},
        .stroke_color = {0, 0, 0, 255},
        .stroke_width_px = 1.0f,
        .stroke_start_cap = DVZ_SEGMENT_CAP_BUTT,
        .stroke_end_cap = DVZ_SEGMENT_CAP_BUTT,
        .stroke_join = DVZ_PATH_JOIN_ROUND,
        .stroke_miter_limit = 4.0f,
    };
}



/**
 * Replace all polygon rings from a borrowed descriptor.
 *
 * @param polygon the polygon
 * @param desc borrowed polygon descriptor
 * @return 0 on success, -1 on invalid input or allocation failure
 */
DvzResult dvz_polygon_geometry(DvzPolygon* polygon, const DvzPolygonDesc* desc)
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
DvzResult dvz_polygon_outer(DvzPolygon* polygon, uint32_t count, const dvec2* xy)
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
    const int out = dvz_polygon_geometry(
        polygon,
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = outer,
            .holes = holes,
            .hole_count = polygon->hole_count,
        });
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
DvzResult dvz_polygon_hole(DvzPolygon* polygon, uint32_t hole_index, uint32_t count, const dvec2* xy)
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
        DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
        .outer = {.xy = (const dvec2*)polygon->outer.xy, .count = polygon->outer.count},
        .holes = holes,
        .hole_count = new_hole_count,
    };
    const int out = dvz_polygon_geometry(polygon, &desc);
    dvz_free(holes);
    return out;
}


/**
 * Set the stable user id associated with a polygon.
 *
 * @param polygon the polygon
 * @param id stable user id
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygon_id(DvzPolygon* polygon, uint64_t id)
{
    if (polygon == NULL || polygon->scene == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon id"))
        return -1;
    polygon->user_id = id;
    polygon->version++;
    return 0;
}



/**
 * Set polygon visibility.
 *
 * @param polygon the polygon
 * @param visible whether the polygon should render
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygon_visible(DvzPolygon* polygon, bool visible)
{
    if (polygon == NULL || polygon->scene == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon visibility"))
        return -1;
    if (polygon->visible == visible)
        return 0;
    polygon->visible = visible;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, true, true);
    return 0;
}



/**
 * Apply polygon render style.
 *
 * @param polygon the polygon
 * @param style polygon style descriptor
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygon_set_style(DvzPolygon* polygon, const DvzPolygonStyle* style)
{
    if (polygon == NULL || polygon->scene == NULL || !_polygon_style_valid(style))
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon style"))
        return -1;

    polygon->visible = style->visible;
    _polygon_color_copy(&polygon->fill_color, style->fill_color);
    _polygon_color_copy(&polygon->stroke_color, style->stroke_color);
    polygon->stroke_width_px = style->stroke_width_px;
    polygon->stroke_cap_start = style->stroke_start_cap;
    polygon->stroke_cap_end = style->stroke_end_cap;
    polygon->stroke_join = style->stroke_join;
    polygon->stroke_miter_limit = style->stroke_miter_limit;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, true, true);
    return 0;
}



/**
 * Set the polygon fill color.
 *
 * @param polygon the polygon
 * @param color RGBA fill color
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygon_fill_color(DvzPolygon* polygon, const DvzColor color)
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
DvzResult dvz_polygon_stroke_color(DvzPolygon* polygon, const DvzColor color)
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
DvzResult dvz_polygon_stroke_width_px(DvzPolygon* polygon, float width)
{
    if (polygon == NULL || polygon->scene == NULL || !isfinite(width) || width < 0.0f)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon stroke width"))
        return -1;
    polygon->stroke_width_px = width;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, false, true);
    return 0;
}



/**
 * Configure polygon stroke endpoint caps.
 *
 * @param polygon the polygon
 * @param start_cap cap applied to each ring start
 * @param end_cap cap applied to each ring end
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygon_stroke_caps(DvzPolygon* polygon, DvzSegmentCap start_cap, DvzSegmentCap end_cap)
{
    if (
        polygon == NULL || polygon->scene == NULL || !_polygon_stroke_cap_valid(start_cap) ||
        !_polygon_stroke_cap_valid(end_cap))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon stroke caps"))
        return -1;
    if (polygon->stroke_cap_start == start_cap && polygon->stroke_cap_end == end_cap)
        return 0;
    polygon->stroke_cap_start = start_cap;
    polygon->stroke_cap_end = end_cap;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, false, true);
    return 0;
}



/**
 * Configure polygon stroke joins.
 *
 * @param polygon the polygon
 * @param join join style
 * @param miter_limit positive finite miter limit
 * @return 0 on success, -1 on error
 */
DvzResult dvz_polygon_stroke_join(DvzPolygon* polygon, DvzPathJoin join, float miter_limit)
{
    if (
        polygon == NULL || polygon->scene == NULL || !_polygon_stroke_join_valid(join) ||
        !isfinite(miter_limit) || miter_limit <= 0.0f)
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon stroke join"))
        return -1;
    if (polygon->stroke_join == join && polygon->stroke_miter_limit == miter_limit)
        return 0;
    polygon->stroke_join = join;
    polygon->stroke_miter_limit = miter_limit;
    polygon->version++;
    _polygon_mark_composites_dirty(polygon, false, true);
    return 0;
}
