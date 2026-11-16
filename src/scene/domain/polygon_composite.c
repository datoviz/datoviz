/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon composite realization                                                       */
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
#include "graph_internal.h"
#include "polygon_internal.h"
#include "_visual_internal.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

/** View-owned triangulations survive color and visibility changes. */
typedef struct
{
    DvzGeometry* geometry;
    uint64_t geometry_version;
} PolygonPaintGeometry;


typedef struct
{
    uint32_t count;
    PolygonPaintGeometry* items;
    bool geometry_dirty;
} PolygonPaintCache;


/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static int _polygon_composite_prepare(DvzComposite* composite);
static int _polygon_set_composite_prepare(DvzComposite* composite);
static bool _polygon_set_item_borrowed_desc(
    const DvzPolygonsItem* item, DvzPolygonRing* outer, DvzPolygonRing** holes);


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
    slot->dirty = true;
    return true;
}



/**
 * Return one generated visual role slot by name.
 *
 * @param composite the composite
 * @param role role name
 * @return the generated visual slot, or NULL when absent
 */
static DvzCompositeVisual* _composite_visual_slot(DvzComposite* composite, const char* role)
{
    if (composite == NULL || !composite->active || role == NULL)
        return NULL;
    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        if (strcmp(composite->visuals[i].role, role) == 0)
            return &composite->visuals[i];
    }
    return NULL;
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
 * Return whether a composite attachment descriptor is valid.
 *
 * @param desc optional attachment descriptor
 * @return whether the descriptor can be applied
 */
static bool _composite_attach_desc_valid(const DvzVisualAttachDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzVisualAttachDesc, 0u))
    {
        log_error("invalid DvzVisualAttachDesc ABI prologue");
        return false;
    }
    if (
        desc->coord_space != DVZ_VISUAL_COORD_VIEW && desc->coord_space != DVZ_VISUAL_COORD_DATA &&
        desc->coord_space != DVZ_VISUAL_COORD_PANEL &&
        desc->coord_space != DVZ_VISUAL_COORD_PANEL_PIXEL)
    {
        log_error("invalid visual coordinate space");
        return false;
    }
    if (
        desc->clip_rect != DVZ_VISUAL_CLIP_AUTO &&
        desc->clip_rect != DVZ_VISUAL_CLIP_PANEL &&
        desc->clip_rect != DVZ_VISUAL_CLIP_PLOT)
    {
        log_error("invalid visual clip rectangle");
        return false;
    }
    if (
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_AUTO &&
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_PANEL &&
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_PLOT &&
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_TARGET)
    {
        log_error("invalid visual viewport rectangle");
        return false;
    }
    return true;
}


/**
 * Return normalized semantic paint data for either polygon source kind.
 *
 * @param composite polygon view
 * @param index region index
 * @param single scratch record for a single polygon
 * @return borrowed normalized region
 */
static const DvzPolygonsItem* _polygon_paint_item(
    const DvzComposite* composite, uint32_t index, DvzPolygonsItem* single)
{
    if (composite->type == DVZ_COMPOSITE_TYPE_POLYGON_SET)
        return &((DvzPolygons*)composite->source)->polygons[index];
    const DvzPolygon* p = composite->source;
    *single = (DvzPolygonsItem){
        .active = p->active, .outer = p->outer, .holes = p->holes,
        .hole_count = p->hole_count, .user_id = p->user_id, .visible = p->visible,
        .fill_color = p->fill_color, .stroke_color = p->stroke_color,
        .stroke_width_px = p->stroke_width_px, .geometry_version = p->geometry_version,
    };
    return single;
}


/**
 * Publish a successfully prepared role's owned ordered draw spans.
 *
 * @param visual generated role
 * @param spans new owned span allocation
 * @param count region count
 */
static void _polygon_paint_publish_spans(
    DvzVisual* visual, DvzVisualDrawSpan* spans, uint32_t count)
{
    dvz_free(visual->draw_spans);
    visual->draw_spans = spans;
    visual->draw_span_count = count;
}


/**
 * Realize shared fill buffers while caching topology separately from paint attributes.
 *
 * @param composite polygon view
 * @param fill fill role
 * @param count retained region count
 * @return success or allocation/triangulation failure
 */
static int _polygon_paint_prepare_fill(
    DvzComposite* composite, DvzVisual* fill, uint32_t count)
{
    if (count == 0)
        return -1;
    PolygonPaintCache* cache = composite->polygon_cache;
    if (cache == NULL)
    {
        cache = dvz_calloc(1, sizeof(PolygonPaintCache));
        if (cache == NULL)
            return -1;
        composite->polygon_cache = cache;
    }
    if (cache->count != count)
    {
        if (!_polygon_allocation_valid(count, sizeof(PolygonPaintGeometry)))
            return -1;
        PolygonPaintGeometry* items = dvz_calloc(count, sizeof(PolygonPaintGeometry));
        if (items == NULL)
            return -1;
        uint32_t retained = cache->count < count ? cache->count : count;
        if (retained > 0)
            dvz_memcpy(items, count * sizeof(*items), cache->items, retained * sizeof(*items));
        for (uint32_t i = retained; i < cache->count; i++)
            dvz_geometry_destroy(cache->items[i].geometry);
        dvz_free(cache->items);
        cache->items = items;
        cache->count = count;
        cache->geometry_dirty = true;
    }
    DvzGeometry** geometries = dvz_calloc(count, sizeof(DvzGeometry*));
    if (geometries == NULL)
        return -1;
    DvzVisualDrawSpan* spans = dvz_calloc(count, sizeof(DvzVisualDrawSpan));
    if (spans == NULL)
    {
        dvz_free(geometries);
        return -1;
    }
    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    bool visible = false;
    for (uint32_t i = 0; i < count; i++)
    {
        DvzPolygonsItem single = {0};
        const DvzPolygonsItem* item = _polygon_paint_item(composite, i, &single);
        PolygonPaintGeometry* entry = &cache->items[i];
        if (entry->geometry == NULL || entry->geometry_version != item->geometry_version)
        {
            DvzPolygonRing outer = {0};
            DvzPolygonRing* holes = NULL;
            if (!_polygon_set_item_borrowed_desc(item, &outer, &holes))
                goto error;
            DvzGeometry* geometry = dvz_triangulate_polygon(
                &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                    .outer = outer, .holes = holes, .hole_count = item->hole_count}, NULL);
            dvz_free(holes);
            if (geometry == NULL)
                goto error;
            dvz_geometry_destroy(entry->geometry);
            entry->geometry = geometry;
            entry->geometry_version = item->geometry_version;
            cache->geometry_dirty = true;
        }
        DvzGeometry* geometry = entry->geometry;
        if (geometry == NULL || vertex_count + geometry->vertex_count > UINT32_MAX ||
            index_count + geometry->index_count > UINT32_MAX)
            goto error;
        bool painted = item->active && item->visible && item->fill_color.a > 0;
        spans[i] = (DvzVisualDrawSpan){
            .item_index = composite->polygon_order == DVZ_POLYGON_ORDER_ITEMS ? i : 0,
            .first_index = (uint32_t)index_count,
            .index_count = painted ? geometry->index_count : 0,
        };
        visible |= painted;
        for (uint32_t j = 0; j < geometry->vertex_count; j++)
            _polygon_color_copy(&geometry->colors[j], item->fill_color);
        vertex_count += geometry->vertex_count;
        index_count += geometry->index_count;
        geometries[i] = geometry;
    }
    int out = 0;
    if (cache->geometry_dirty)
    {
        DvzGeometry* merged = dvz_geometry_merge(count, (const DvzGeometry* const*)geometries);
        if (merged == NULL)
            goto error;
        out = dvz_mesh_set_geometry(fill, merged);
        dvz_geometry_destroy(merged);
        if (out == 0)
            cache->geometry_dirty = false;
    }
    else
    {
        if (!_polygon_allocation_valid((uint32_t)vertex_count, sizeof(DvzColor)))
            goto error;
        DvzColor* colors = dvz_calloc((size_t)vertex_count, sizeof(DvzColor));
        if (colors == NULL)
            goto error;
        uint32_t offset = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            DvzGeometry* geometry = geometries[i];
            size_t bytes = geometry->vertex_count * sizeof(DvzColor);
            dvz_memcpy(colors + offset, bytes, geometry->colors, bytes);
            offset += geometry->vertex_count;
        }
        out = dvz_visual_set_data(fill, "color", colors, vertex_count);
        dvz_free(colors);
    }
    if (out == 0)
    {
        _polygon_paint_publish_spans(fill, spans, count);
        fill->visible = visible;
    }
    else
        dvz_free(spans);
    dvz_free(geometries);
    return out;

error:
    dvz_free(spans);
    dvz_free(geometries);
    return -1;
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
        _polygon_color_copy(&colors[j], polygon->stroke_color);
        widths[j] = polygon->stroke_width_px;
    }

    const uint32_t close = offset + count;
    positions[close][0] = (float)ring->xy[0][0];
    positions[close][1] = (float)ring->xy[0][1];
    positions[close][2] = 0.0f;
    _polygon_color_copy(&colors[close], polygon->stroke_color);
    widths[close] = polygon->stroke_width_px;
    return close + 1;
}


/**
 * Upload the polygon stroke role visual.
 *
 * @param polygon source polygon
 * @param stroke stroke path visual
 * @return 0 on success, -1 on error
 */
static int _polygon_prepare_stroke(
    DvzComposite* composite, DvzPolygon* polygon, DvzVisual* stroke)
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

    if (total - (polygon->hole_count + 1) > UINT32_MAX / 6u)
        return -1;
    (void)composite;
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
    DvzVisualDrawSpan* spans = dvz_calloc(1, sizeof(DvzVisualDrawSpan));
    if (positions == NULL || colors == NULL || widths == NULL || lengths == NULL || spans == NULL)
    {
        dvz_free(positions);
        dvz_free(colors);
        dvz_free(widths);
        dvz_free(lengths);
        dvz_free(spans);
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
        {.attr_name = "stroke_width_px", .data = widths, .item_count = point_count},
    };
    int out = dvz_visual_set_data_many(stroke, updates, 3);
    if (out == 0)
        out = dvz_path_set_subpaths(stroke, subpath_count, lengths);
    if (out == 0)
        out = dvz_path_set_caps(stroke, polygon->stroke_cap_start, polygon->stroke_cap_end);
    if (out == 0)
        out = dvz_path_set_join(stroke, polygon->stroke_join, polygon->stroke_miter_limit);
    if (out == 0)
    {
        bool painted = polygon->visible && polygon->stroke_color.a > 0 &&
                       polygon->stroke_width_px > 0;
        spans[0] = (DvzVisualDrawSpan){
            .item_index = 0, .first_index = 0,
            .index_count = painted ? 6u * (uint32_t)(total - subpath_count) : 0,
        };
        _polygon_paint_publish_spans(stroke, spans, 1);
        stroke->visible = painted;
    }
    else
        dvz_free(spans);

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(widths);
    dvz_free(lengths);
    return out;
}


/**
 * Build borrowed ring views from one retained polygon-set item.
 *
 * @param item retained polygon-set item
 * @param outer output outer ring view
 * @param holes output hole view pointer, or NULL when no holes
 * @return whether the views were built
 */
static bool _polygon_set_item_borrowed_desc(
    const DvzPolygonsItem* item, DvzPolygonRing* outer, DvzPolygonRing** holes)
{
    if (item == NULL || outer == NULL || holes == NULL || item->outer.xy == NULL)
        return false;

    *outer = (DvzPolygonRing){.xy = (const dvec2*)item->outer.xy, .count = item->outer.count};
    *holes = NULL;
    if (item->hole_count == 0)
        return true;

    if (!_polygon_allocation_valid(item->hole_count, sizeof(DvzPolygonRing)))
        return false;
    DvzPolygonRing* out = (DvzPolygonRing*)dvz_calloc(item->hole_count, sizeof(DvzPolygonRing));
    if (out == NULL)
        return false;

    for (uint32_t i = 0; i < item->hole_count; i++)
        out[i] = (DvzPolygonRing){
            .xy = (const dvec2*)item->holes[i].xy,
            .count = item->holes[i].count,
        };
    *holes = out;
    return true;
}


/**
 * Append one polygon-set stroke ring into path upload arrays.
 *
 * @param ring stored ring
 * @param item source polygon-set item
 * @param positions path positions
 * @param colors path colors
 * @param widths path stroke widths
 * @param offset current write offset
 * @return updated write offset
 */
static uint32_t _polygon_set_append_stroke_ring(
    const DvzPolygonStoredRing* ring, const DvzPolygonsItem* item, vec3* positions,
    DvzColor* colors, float* widths, uint32_t offset)
{
    const uint32_t count = _polygon_stored_ring_count(ring);
    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t j = offset + i;
        positions[j][0] = (float)ring->xy[i][0];
        positions[j][1] = (float)ring->xy[i][1];
        positions[j][2] = 0.0f;
        _polygon_color_copy(&colors[j], item->stroke_color);
        widths[j] = item->stroke_width_px;
    }

    const uint32_t close = offset + count;
    positions[close][0] = (float)ring->xy[0][0];
    positions[close][1] = (float)ring->xy[0][1];
    positions[close][2] = 0.0f;
    _polygon_color_copy(&colors[close], item->stroke_color);
    widths[close] = item->stroke_width_px;
    return close + 1;
}


/**
 * Upload a polygon-set stroke role visual.
 *
 * @param set source polygon set
 * @param stroke stroke path visual
 * @return 0 on success, -1 on error
 */
static int _polygon_set_prepare_stroke(
    DvzComposite* composite, DvzPolygons* set, DvzVisual* stroke)
{
    uint64_t total_points = 0;
    uint64_t total_subpaths = 0;
    bool any_visible_stroke = false;
    for (uint32_t i = 0; i < set->polygon_count; i++)
    {
        const DvzPolygonsItem* item = &set->polygons[i];
        if (!item->active)
            continue;
        any_visible_stroke =
            any_visible_stroke ||
            (item->visible && item->stroke_width_px > 0.0f && item->stroke_color.a > 0);

        uint64_t next = 0;
        if (_dvz_add_u64_overflows(
                total_points, (uint64_t)_polygon_stored_ring_count(&item->outer) + 1, &next))
        {
            return -1;
        }
        total_points = next;
        if (_dvz_add_u64_overflows(total_subpaths, 1, &next))
            return -1;
        total_subpaths = next;

        for (uint32_t h = 0; h < item->hole_count; h++)
        {
            if (_dvz_add_u64_overflows(
                    total_points, (uint64_t)_polygon_stored_ring_count(&item->holes[h]) + 1,
                    &next))
            {
                return -1;
            }
            total_points = next;
            if (_dvz_add_u64_overflows(total_subpaths, 1, &next))
                return -1;
            total_subpaths = next;
        }
    }
    if (total_points == 0)
    {
        stroke->visible = false;
        return 0;
    }
    if (total_points > UINT32_MAX || total_subpaths > UINT32_MAX ||
        total_points - total_subpaths > UINT32_MAX / 6u ||
        !_polygon_allocation_valid(set->polygon_count, sizeof(DvzVisualDrawSpan)))
        return -1;

    const uint32_t point_count = (uint32_t)total_points;
    const uint32_t subpath_count = (uint32_t)total_subpaths;
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
    DvzVisualDrawSpan* spans = dvz_calloc(set->polygon_count, sizeof(DvzVisualDrawSpan));
    if (positions == NULL || colors == NULL || widths == NULL || lengths == NULL || spans == NULL)
    {
        dvz_free(positions);
        dvz_free(colors);
        dvz_free(widths);
        dvz_free(lengths);
        dvz_free(spans);
        return -1;
    }

    uint32_t offset = 0;
    uint32_t subpath = 0;
    uint32_t first_index = 0;
    for (uint32_t i = 0; i < set->polygon_count; i++)
    {
        const DvzPolygonsItem* item = &set->polygons[i];
        if (!item->active)
            continue;

        uint64_t segment_count = _polygon_stored_ring_count(&item->outer);
        for (uint32_t h = 0; h < item->hole_count; h++)
            segment_count += _polygon_stored_ring_count(&item->holes[h]);
        uint32_t index_count = 6u * (uint32_t)segment_count;
        spans[i] = (DvzVisualDrawSpan){
            .item_index = composite->polygon_order == DVZ_POLYGON_ORDER_ITEMS ? i : 0,
            .first_index = first_index,
            .index_count = item->visible && item->stroke_color.a > 0 &&
                                   item->stroke_width_px > 0 ? index_count : 0,
        };
        first_index += index_count;
        lengths[subpath++] = _polygon_stored_ring_count(&item->outer) + 1;
        offset = _polygon_set_append_stroke_ring(
            &item->outer, item, positions, colors, widths, offset);
        for (uint32_t h = 0; h < item->hole_count; h++)
        {
            lengths[subpath++] = _polygon_stored_ring_count(&item->holes[h]) + 1;
            offset = _polygon_set_append_stroke_ring(
                &item->holes[h], item, positions, colors, widths, offset);
        }
    }

    DvzVisualDataUpdate updates[3] = {
        {.attr_name = "position", .data = positions, .item_count = point_count},
        {.attr_name = "color", .data = colors, .item_count = point_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = point_count},
    };
    int out = dvz_visual_set_data_many(stroke, updates, 3);
    if (out == 0)
        out = dvz_path_set_subpaths(stroke, subpath_count, lengths);
    if (out == 0)
        out = dvz_path_set_caps(stroke, set->stroke_cap_start, set->stroke_cap_end);
    if (out == 0)
        out = dvz_path_set_join(stroke, set->stroke_join, set->stroke_miter_limit);
    if (out == 0)
    {
        _polygon_paint_publish_spans(stroke, spans, set->polygon_count);
        stroke->visible = any_visible_stroke;
    }
    else
        dvz_free(spans);

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(widths);
    dvz_free(lengths);
    return out;
}


/**
 * Realize or refresh one polygon-set composite.
 *
 * @param composite polygon-set composite
 * @return 0 on success, -1 on error
 */
static int _polygon_set_composite_prepare(DvzComposite* composite)
{
    if (
        composite == NULL || !composite->active ||
        composite->type != DVZ_COMPOSITE_TYPE_POLYGON_SET || composite->source == NULL)
    {
        return -1;
    }

    DvzPolygons* set = (DvzPolygons*)composite->source;
    if (!set->active || set->polygon_count == 0)
        return -1;
    if (!composite->dirty && composite->source_version_seen == set->version)
        return 0;

    DvzCompositeVisual* fill_role =
        _composite_visual_slot(composite, DVZ_POLYGON_COMPOSITE_FILL_ROLE);
    DvzCompositeVisual* stroke_role =
        _composite_visual_slot(composite, DVZ_POLYGON_COMPOSITE_STROKE_ROLE);
    if (fill_role == NULL || stroke_role == NULL)
        return -1;

    if (fill_role->dirty && _polygon_paint_prepare_fill(composite, fill_role->visual, set->polygon_count) != 0)
        return -1;
    if (stroke_role->dirty && _polygon_set_prepare_stroke(composite, set, stroke_role->visual) != 0)
        return -1;

    composite->dirty = false;
    fill_role->dirty = false;
    stroke_role->dirty = false;
    composite->source_version_seen = set->version;
    return 0;
}


/**
 * Realize or refresh one composite.
 *
 * @param composite composite
 * @return 0 on success, -1 on error
 */
static int _composite_prepare(DvzComposite* composite)
{
    if (composite == NULL)
        return -1;
    if (composite->type == DVZ_COMPOSITE_TYPE_POLYGON)
        return _polygon_composite_prepare(composite);
    if (composite->type == DVZ_COMPOSITE_TYPE_POLYGON_SET)
        return _polygon_set_composite_prepare(composite);
    if (composite->type == DVZ_COMPOSITE_TYPE_GRAPH)
        return _graph_composite_prepare(composite);
    return -1;
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

    DvzCompositeVisual* fill_role =
        _composite_visual_slot(composite, DVZ_POLYGON_COMPOSITE_FILL_ROLE);
    DvzCompositeVisual* stroke_role =
        _composite_visual_slot(composite, DVZ_POLYGON_COMPOSITE_STROKE_ROLE);
    if (fill_role == NULL || stroke_role == NULL)
        return -1;

    if (fill_role->dirty && _polygon_paint_prepare_fill(composite, fill_role->visual, 1) != 0)
        return -1;
    if (stroke_role->dirty && _polygon_prepare_stroke(composite, polygon, stroke_role->visual) != 0)
        return -1;

    composite->dirty = false;
    fill_role->dirty = false;
    stroke_role->dirty = false;
    composite->source_version_seen = polygon->version;
    return 0;
}


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
        DvzVisual* visual = composite->visuals[i].visual;
        if (visual != NULL)
        {
            visual->visible = false;
            dvz_free(visual->draw_spans);
            visual->draw_spans = NULL;
            visual->draw_span_count = 0;
        }
    }
    PolygonPaintCache* cache = composite->polygon_cache;
    if (cache != NULL)
    {
        for (uint32_t i = 0; i < cache->count; i++)
            dvz_geometry_destroy(cache->items[i].geometry);
        dvz_free(cache->items);
        dvz_free(cache);
        composite->polygon_cache = NULL;
    }
    composite->scene = NULL;
    composite->active = false;
    composite->source = NULL;
    composite->visual_count = 0;
}


/**
 * Initialize persistent paint defaults once when a generated role is created.
 *
 * @param visual generated role
 * @param fill whether this is the mesh fill role
 * @return whether initialization succeeded
 */
static bool _polygon_paint_defaults(DvzVisual* visual, bool fill)
{
    if (visual == NULL)
        return false;
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_UNLIT;
    material.alpha_mode = DVZ_ALPHA_BLENDED;
    if (fill && dvz_visual_set_material(visual, &material) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0 ||
        dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    visual->paint = true;
    visual->paint_required = true;
    return true;
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
        !_polygon_paint_defaults(fill, true) || !_polygon_paint_defaults(stroke, false) ||
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
 * Create a scene-owned composite render view for a polygon set.
 *
 * @param set the polygon set
 * @param flags reserved composite flags
 * @return the composite, or NULL on allocation failure
 */
DvzComposite* dvz_polygons_composite(DvzPolygons* set, uint32_t flags)
{
    if (set == NULL || set->scene == NULL || !set->active)
        return NULL;
    DvzScene* scene = set->scene;
    if (!_scene_visual_mutation_allowed(scene, "create polygon set composite"))
        return NULL;

    DvzComposite* composite = _scene_alloc_composite(scene);
    if (composite == NULL)
        return NULL;
    composite->type = DVZ_COMPOSITE_TYPE_POLYGON_SET;
    composite->flags = flags;
    composite->source = set;

    DvzVisual* fill = dvz_mesh(scene, 0);
    DvzVisual* stroke = dvz_path(scene, 0);
    if (
        !_polygon_paint_defaults(fill, true) || !_polygon_paint_defaults(stroke, false) ||
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
 * Change persistent overlap ordering for a polygon view.
 *
 * @param composite polygon or polygon-set view
 * @param order item or collection composition
 * @return success or invalid argument/mutation error
 */
DvzResult dvz_polygon_composite_set_order(DvzComposite* composite, DvzPolygonOrder order)
{
    if (composite == NULL || !composite->active || composite->scene == NULL ||
        (composite->type != DVZ_COMPOSITE_TYPE_POLYGON &&
         composite->type != DVZ_COMPOSITE_TYPE_POLYGON_SET) ||
        (order != DVZ_POLYGON_ORDER_ITEMS && order != DVZ_POLYGON_ORDER_COLLECTION))
        return -1;
    if (!_scene_visual_mutation_allowed(composite->scene, "change polygon paint order"))
        return -1;
    composite->polygon_order = (uint32_t)order;
    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        DvzVisual* visual = composite->visuals[i].visual;
        for (uint32_t j = 0; j < visual->draw_span_count; j++)
            visual->draw_spans[j].item_index = order == DVZ_POLYGON_ORDER_ITEMS ? j : 0;
        _scene_notify_visual_changed(visual);
    }
    return 0;
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
    DvzCompositeVisual* slot = _composite_visual_slot(composite, role);
    return slot != NULL ? slot->visual : NULL;
}


/**
 * Add all generated visual roles of a composite to a panel.
 *
 * @param panel the panel
 * @param composite the composite
 * @param desc attachment options applied to the composite roles
 * @return 0 on success, -1 on error
 */
DvzResult dvz_panel_add_composite(
    DvzPanel* panel, DvzComposite* composite, const DvzVisualAttachDesc* desc)
{
    if (
        panel == NULL || composite == NULL || !composite->active || panel->figure == NULL ||
        panel->figure->scene == NULL || composite->scene != panel->figure->scene)
    {
        return -1;
    }
    if (!_composite_attach_desc_valid(desc))
        return -1;
    if (_composite_prepare(composite) != 0)
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

    const uint32_t parent_insertion_index = panel->visual_count;
    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        DvzVisual* visual = composite->visuals[i].visual;
        if (_panel_has_visual(panel, visual))
            continue;

        DvzPanelAttach* slot = &panel->visuals[panel->visual_count];
        slot->visual = visual;
        slot->z_layer = (desc != NULL ? desc->z_layer : 0) +
                        (visual->paint ? 0 : composite->visuals[i].z_offset);
        if (visual->paint)
        {
            slot->has_parent_order = true;
            slot->parent_insertion_index = parent_insertion_index;
            slot->role_order = i;
        }
        slot->controller_mode = desc != NULL ? desc->controller_mode : DVZ_CONTROLLER_APPLY;
        slot->coord_space = desc != NULL ? desc->coord_space : DVZ_VISUAL_COORD_VIEW;
        slot->clip_rect = desc != NULL ? desc->clip_rect : DVZ_VISUAL_CLIP_AUTO;
        slot->viewport_rect = desc != NULL ? desc->viewport_rect : DVZ_VISUAL_VIEWPORT_AUTO;
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
        (void)_composite_prepare(composite);
    }
}
