/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon semantic objects and composites                                                       */
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

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_POLYGON_COMPOSITE_FILL_ROLE   "fill"
#define DVZ_POLYGON_COMPOSITE_STROKE_ROLE "stroke"



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
static bool _polygon_allocation_valid(uint32_t count, DvzSize item_size)
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
static void _polygon_color_copy(DvzColor dst, const DvzColor src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
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
static uint32_t _polygon_stored_ring_count(const DvzPolygonStoredRing* ring)
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
static void _polygon_ring_reset(DvzPolygonStoredRing* ring)
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
static bool _polygon_ring_copy(const DvzPolygonRing* src, DvzPolygonStoredRing* dst)
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
 * Build borrowed ring views from one retained polygon.
 *
 * @param polygon retained polygon
 * @param outer output outer ring view
 * @param holes output hole view pointer, or NULL when no holes
 * @return whether the views were built
 */
static bool _polygon_borrowed_desc(
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
 */
static void _polygon_mark_composites_dirty(DvzPolygon* polygon)
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
        composite->dirty = true;
        for (uint32_t j = 0; j < composite->visual_count; j++)
            _scene_notify_visual_changed(composite->visuals[j].visual);
    }
}



/**
 * Allocate one scene-owned polygon slot.
 *
 * @param scene the scene
 * @return the polygon slot, or NULL on capacity exhaustion
 */
static DvzPolygon* _scene_alloc_polygon(DvzScene* scene)
{
    if (scene == NULL || scene->polygon_count >= DVZ_SCENE_MAX_POLYGONS)
        return NULL;

    DvzPolygon* polygon = &scene->polygons[scene->polygon_count++];
    dvz_memset(polygon, sizeof(DvzPolygon), 0, sizeof(DvzPolygon));
    polygon->scene = scene;
    polygon->active = true;
    polygon->fill_color[0] = 255;
    polygon->fill_color[1] = 255;
    polygon->fill_color[2] = 255;
    polygon->fill_color[3] = 255;
    polygon->stroke_color[0] = 0;
    polygon->stroke_color[1] = 0;
    polygon->stroke_color[2] = 0;
    polygon->stroke_color[3] = 255;
    polygon->stroke_width = 1.0f;
    polygon->version = 1;
    return polygon;
}



/**
 * Allocate one scene-owned composite slot.
 *
 * @param scene the scene
 * @return the composite slot, or NULL on capacity exhaustion
 */
static DvzComposite* _scene_alloc_composite(DvzScene* scene)
{
    if (scene == NULL || scene->composite_count >= DVZ_SCENE_MAX_COMPOSITES)
        return NULL;

    DvzComposite* composite = &scene->composites[scene->composite_count++];
    dvz_memset(composite, sizeof(DvzComposite), 0, sizeof(DvzComposite));
    composite->scene = scene;
    composite->active = true;
    composite->dirty = true;
    return composite;
}



/**
 * Add one generated visual role to a composite.
 *
 * @param composite the composite
 * @param role role name
 * @param visual generated visual
 * @param z_offset z-layer offset relative to the composite attachment
 * @return whether the role was added
 */
static bool _composite_add_visual(
    DvzComposite* composite, const char* role, DvzVisual* visual, int32_t z_offset)
{
    if (
        composite == NULL || role == NULL || visual == NULL ||
        composite->visual_count >= DVZ_COMPOSITE_MAX_VISUALS)
    {
        return false;
    }

    DvzCompositeVisual* slot = &composite->visuals[composite->visual_count++];
    dvz_strlcpy(slot->role, role, sizeof(slot->role));
    slot->visual = visual;
    slot->z_offset = z_offset;
    return true;
}



/**
 * Return whether one panel already contains a visual.
 *
 * @param panel the panel
 * @param visual the visual
 * @return whether the visual is already attached
 */
static bool _panel_has_visual(const DvzPanel* panel, const DvzVisual* visual)
{
    if (panel == NULL || visual == NULL)
        return false;

    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual == visual)
            return true;
    }
    return false;
}



/**
 * Upload the polygon fill role visual.
 *
 * @param polygon source polygon
 * @param fill fill mesh visual
 * @return 0 on success, -1 on error
 */
static int _polygon_prepare_fill(DvzPolygon* polygon, DvzVisual* fill)
{
    DvzPolygonRing outer = {0};
    DvzPolygonRing* holes = NULL;
    if (!_polygon_borrowed_desc(polygon, &outer, &holes))
        return -1;

    DvzGeometry* geometry = dvz_triangulate_polygon(
        &(DvzPolygonDesc){.outer = outer, .holes = holes, .hole_count = polygon->hole_count}, NULL);
    dvz_free(holes);
    if (geometry == NULL)
        return -1;

    for (uint32_t i = 0; i < geometry->vertex_count; i++)
        _polygon_color_copy(geometry->colors[i], polygon->fill_color);

    const int out = dvz_mesh_set_geometry(fill, geometry);
    fill->visible = polygon->fill_color[3] > 0;
    dvz_geometry_destroy(geometry);
    return out;
}



/**
 * Append one closed stroke ring into retained path upload arrays.
 *
 * @param ring stored ring
 * @param polygon source polygon
 * @param positions path positions
 * @param colors path colors
 * @param widths path stroke widths
 * @param offset current write offset
 * @return updated write offset
 */
static uint32_t _polygon_append_stroke_ring(
    const DvzPolygonStoredRing* ring, const DvzPolygon* polygon, vec3* positions, DvzColor* colors,
    float* widths, uint32_t offset)
{
    const uint32_t count = _polygon_stored_ring_count(ring);
    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t j = offset + i;
        positions[j][0] = (float)ring->xy[i][0];
        positions[j][1] = (float)ring->xy[i][1];
        positions[j][2] = 0.0f;
        _polygon_color_copy(colors[j], polygon->stroke_color);
        widths[j] = polygon->stroke_width;
    }

    const uint32_t close = offset + count;
    positions[close][0] = (float)ring->xy[0][0];
    positions[close][1] = (float)ring->xy[0][1];
    positions[close][2] = 0.0f;
    _polygon_color_copy(colors[close], polygon->stroke_color);
    widths[close] = polygon->stroke_width;
    return close + 1;
}



/**
 * Upload the polygon stroke role visual.
 *
 * @param polygon source polygon
 * @param stroke stroke path visual
 * @return 0 on success, -1 on error
 */
static int _polygon_prepare_stroke(DvzPolygon* polygon, DvzVisual* stroke)
{
    uint64_t total = (uint64_t)_polygon_stored_ring_count(&polygon->outer) + 1;
    for (uint32_t i = 0; i < polygon->hole_count; i++)
    {
        uint64_t next = 0;
        if (_dvz_add_u64_overflows(
                total, (uint64_t)_polygon_stored_ring_count(&polygon->holes[i]) + 1, &next))
        {
            return -1;
        }
        total = next;
    }
    if (total == 0 || total > UINT32_MAX)
        return -1;

    const uint32_t point_count = (uint32_t)total;
    const uint32_t subpath_count = polygon->hole_count + 1;
    if (
        !_polygon_allocation_valid(point_count, sizeof(vec3)) ||
        !_polygon_allocation_valid(point_count, sizeof(DvzColor)) ||
        !_polygon_allocation_valid(point_count, sizeof(float)) ||
        !_polygon_allocation_valid(subpath_count, sizeof(uint32_t)))
    {
        return -1;
    }

    vec3* positions = (vec3*)dvz_calloc(point_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(point_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(point_count, sizeof(float));
    uint32_t* lengths = (uint32_t*)dvz_calloc(subpath_count, sizeof(uint32_t));
    if (positions == NULL || colors == NULL || widths == NULL || lengths == NULL)
    {
        dvz_free(positions);
        dvz_free(colors);
        dvz_free(widths);
        dvz_free(lengths);
        return -1;
    }

    uint32_t offset = 0;
    lengths[0] = _polygon_stored_ring_count(&polygon->outer) + 1;
    offset = _polygon_append_stroke_ring(&polygon->outer, polygon, positions, colors, widths, offset);
    for (uint32_t i = 0; i < polygon->hole_count; i++)
    {
        lengths[i + 1] = _polygon_stored_ring_count(&polygon->holes[i]) + 1;
        offset = _polygon_append_stroke_ring(
            &polygon->holes[i], polygon, positions, colors, widths, offset);
    }

    DvzVisualDataUpdate updates[3] = {
        {.attr_name = "position", .data = positions, .item_count = point_count},
        {.attr_name = "color", .data = colors, .item_count = point_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = point_count},
    };
    int out = dvz_visual_set_data_many(stroke, updates, 3);
    if (out == 0)
        out = dvz_path_set_subpaths(stroke, subpath_count, lengths);
    stroke->visible = polygon->stroke_color[3] > 0 && polygon->stroke_width > 0.0f;

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(widths);
    dvz_free(lengths);
    return out;
}



/**
 * Realize or refresh one polygon composite.
 *
 * @param composite polygon composite
 * @return 0 on success, -1 on error
 */
static int _polygon_composite_prepare(DvzComposite* composite)
{
    if (
        composite == NULL || !composite->active || composite->type != DVZ_COMPOSITE_TYPE_POLYGON ||
        composite->source == NULL)
    {
        return -1;
    }

    DvzPolygon* polygon = (DvzPolygon*)composite->source;
    if (!polygon->active || polygon->outer.xy == NULL)
        return -1;
    if (!composite->dirty && composite->source_version_seen == polygon->version)
        return 0;

    DvzVisual* fill = dvz_composite_visual(composite, DVZ_POLYGON_COMPOSITE_FILL_ROLE);
    DvzVisual* stroke = dvz_composite_visual(composite, DVZ_POLYGON_COMPOSITE_STROKE_ROLE);
    if (fill == NULL || stroke == NULL)
        return -1;

    if (_polygon_prepare_fill(polygon, fill) != 0)
        return -1;
    if (_polygon_prepare_stroke(polygon, stroke) != 0)
        return -1;

    composite->dirty = false;
    composite->source_version_seen = polygon->version;
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
 * Reset a scene composite slot.
 *
 * @param composite the composite
 */
void _scene_composite_reset(DvzComposite* composite)
{
    if (composite == NULL)
        return;

    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        if (composite->visuals[i].visual != NULL)
            composite->visuals[i].visual->visible = false;
    }
    composite->scene = NULL;
    composite->active = false;
    composite->source = NULL;
    composite->visual_count = 0;
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
    _polygon_mark_composites_dirty(polygon);
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
    if (polygon == NULL || polygon->scene == NULL || color == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon fill color"))
        return -1;
    _polygon_color_copy(polygon->fill_color, color);
    polygon->version++;
    _polygon_mark_composites_dirty(polygon);
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
    if (polygon == NULL || polygon->scene == NULL || color == NULL)
        return -1;
    if (!_scene_visual_mutation_allowed(polygon->scene, "update polygon stroke color"))
        return -1;
    _polygon_color_copy(polygon->stroke_color, color);
    polygon->version++;
    _polygon_mark_composites_dirty(polygon);
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
    _polygon_mark_composites_dirty(polygon);
    return 0;
}



/**
 * Create a scene-owned composite render view for a polygon.
 *
 * @param polygon the source polygon
 * @param flags reserved composite flags
 * @return the composite, or NULL on allocation failure
 */
DvzComposite* dvz_polygon_composite(DvzPolygon* polygon, uint32_t flags)
{
    if (polygon == NULL || polygon->scene == NULL || !polygon->active)
        return NULL;
    DvzScene* scene = polygon->scene;
    if (!_scene_visual_mutation_allowed(scene, "create polygon composite"))
        return NULL;

    DvzComposite* composite = _scene_alloc_composite(scene);
    if (composite == NULL)
        return NULL;
    composite->type = DVZ_COMPOSITE_TYPE_POLYGON;
    composite->flags = flags;
    composite->source = polygon;

    DvzVisual* fill = dvz_mesh(scene, 0);
    DvzVisual* stroke = dvz_path(scene, 0);
    if (
        fill == NULL || stroke == NULL ||
        !_composite_add_visual(composite, DVZ_POLYGON_COMPOSITE_FILL_ROLE, fill, 0) ||
        !_composite_add_visual(composite, DVZ_POLYGON_COMPOSITE_STROKE_ROLE, stroke, 1))
    {
        if (fill != NULL)
            fill->visible = false;
        if (stroke != NULL)
            stroke->visible = false;
        _scene_composite_reset(composite);
        return NULL;
    }

    return composite;
}



/**
 * Destroy a scene-owned composite render view.
 *
 * @param composite the composite
 */
void dvz_composite_destroy(DvzComposite* composite)
{
    if (composite == NULL || composite->scene == NULL)
        return;
    if (!_scene_visual_mutation_allowed(composite->scene, "destroy composite"))
        return;
    _scene_composite_reset(composite);
}



/**
 * Return the number of generated visuals owned by a composite.
 *
 * @param composite the composite
 * @return generated visual count
 */
uint32_t dvz_composite_visual_count(const DvzComposite* composite)
{
    return composite != NULL && composite->active ? composite->visual_count : 0;
}



/**
 * Return a generated visual by role index.
 *
 * @param composite the composite
 * @param index role index
 * @return the generated visual, or NULL when out of range
 */
DvzVisual* dvz_composite_visual_at(DvzComposite* composite, uint32_t index)
{
    if (composite == NULL || !composite->active || index >= composite->visual_count)
        return NULL;
    return composite->visuals[index].visual;
}



/**
 * Return a generated visual by role name.
 *
 * @param composite the composite
 * @param role role name
 * @return the generated visual, or NULL when absent
 */
DvzVisual* dvz_composite_visual(DvzComposite* composite, const char* role)
{
    if (composite == NULL || !composite->active || role == NULL)
        return NULL;
    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        if (strcmp(composite->visuals[i].role, role) == 0)
            return composite->visuals[i].visual;
    }
    return NULL;
}



/**
 * Add all generated visual roles of a composite to a panel.
 *
 * @param panel the panel
 * @param composite the composite
 * @param desc attachment options applied to the composite roles
 * @return 0 on success, -1 on error
 */
int dvz_panel_add_composite(
    DvzPanel* panel, DvzComposite* composite, const DvzVisualAttachDesc* desc)
{
    if (
        panel == NULL || composite == NULL || !composite->active || panel->figure == NULL ||
        panel->figure->scene == NULL || composite->scene != panel->figure->scene)
    {
        return -1;
    }
    if (_polygon_composite_prepare(composite) != 0)
        return -1;

    uint32_t missing_count = 0;
    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        DvzVisual* visual = composite->visuals[i].visual;
        if (visual == NULL || visual->scene != composite->scene)
            return -1;
        if (!_panel_has_visual(panel, visual))
            missing_count++;
    }
    if (missing_count > DVZ_SCENE_MAX_VISUALS - panel->visual_count)
        return -1;

    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        DvzVisual* visual = composite->visuals[i].visual;
        if (_panel_has_visual(panel, visual))
            continue;

        DvzPanelAttach* slot = &panel->visuals[panel->visual_count];
        slot->visual = visual;
        slot->z_layer = (desc != NULL ? desc->z_layer : 0) + composite->visuals[i].z_offset;
        slot->controller_mode = desc != NULL ? desc->controller_mode : DVZ_CONTROLLER_APPLY;
        slot->insertion_index = panel->visual_count;
        panel->visual_count++;
    }
    _scene_notify_request_frame(panel->figure);
    return 0;
}



/**
 * Prepare all active composite visuals before upload/render emission.
 *
 * @param figure figure being emitted
 */
void _scene_prepare_composite_visuals(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;

    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (!composite->active)
            continue;
        if (composite->type == DVZ_COMPOSITE_TYPE_POLYGON)
            (void)_polygon_composite_prepare(composite);
    }
}
