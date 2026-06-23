/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Graph composite realization                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_scene.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "graph_internal.h"
#include "polygon_internal.h"
#include "_visual_internal.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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



static DvzCompositeVisual* _composite_visual_type(DvzComposite* composite, DvzVisualType type)
{
    if (composite == NULL || !composite->active)
        return NULL;
    for (uint32_t i = 0; i < composite->visual_count; i++)
    {
        if (composite->visuals[i].visual != NULL && composite->visuals[i].visual->type == type)
            return &composite->visuals[i];
    }
    return NULL;
}



static void _graph_copy_position(vec3 dst, const dvec3 src)
{
    dst[0] = (float)src[0];
    dst[1] = (float)src[1];
    dst[2] = (float)src[2];
}



static void _graph_update_edge_roles(DvzComposite* composite, const DvzGraph* graph)
{
    DvzCompositeVisual* segment = _composite_visual_type(composite, DVZ_VISUAL_TYPE_SEGMENT);
    DvzCompositeVisual* path = _composite_visual_type(composite, DVZ_VISUAL_TYPE_PATH);
    if (segment == NULL || path == NULL || graph == NULL)
        return;

    const bool use_segment = graph->edge_mode == DVZ_GRAPH_EDGE_MODE_SEGMENT;
    dvz_strlcpy(
        segment->role,
        use_segment ? DVZ_GRAPH_COMPOSITE_EDGES_ROLE : DVZ_GRAPH_COMPOSITE_SEGMENTS_INTERNAL_ROLE,
        sizeof(segment->role));
    dvz_strlcpy(
        path->role,
        use_segment ? DVZ_GRAPH_COMPOSITE_PATH_INTERNAL_ROLE : DVZ_GRAPH_COMPOSITE_EDGES_ROLE,
        sizeof(path->role));
}



static int _graph_prepare_nodes(DvzGraph* graph, DvzVisual* nodes)
{
    if (graph == NULL || nodes == NULL || nodes->type != DVZ_VISUAL_TYPE_MARKER)
        return -1;
    if (graph->node_count == 0)
    {
        nodes->visible = false;
        return 0;
    }

    const uint32_t count = graph->node_count;
    if (
        !_polygon_allocation_valid(count, sizeof(vec3)) ||
        !_polygon_allocation_valid(count, sizeof(DvzColor)) ||
        !_polygon_allocation_valid(count, sizeof(float)) ||
        !_polygon_allocation_valid(count, sizeof(uint32_t)))
    {
        return -1;
    }

    vec3* positions = (vec3*)dvz_calloc(count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(count, sizeof(DvzColor));
    float* sizes = (float*)dvz_calloc(count, sizeof(float));
    float* angles = (float*)dvz_calloc(count, sizeof(float));
    uint32_t* shapes = (uint32_t*)dvz_calloc(count, sizeof(uint32_t));
    if (
        positions == NULL || colors == NULL || sizes == NULL || angles == NULL ||
        shapes == NULL)
    {
        dvz_free(positions);
        dvz_free(colors);
        dvz_free(sizes);
        dvz_free(angles);
        dvz_free(shapes);
        return -1;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const DvzGraphNode* node = &graph->nodes[i];
        _graph_copy_position(positions[i], node->position);
        colors[i] = node->visible ? node->color : dvz_color_rgba(0, 0, 0, 0);
        sizes[i] = node->visible ? node->size : 0;
        angles[i] = node->angle;
        shapes[i] = (uint32_t)node->shape;
    }

    const DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter_px", .data = sizes, .item_count = count},
        {.attr_name = "angle", .data = angles, .item_count = count},
        {.attr_name = "shape", .data = shapes, .item_count = count},
    };
    const int rc = dvz_visual_set_data_many(nodes, updates, 5);
    nodes->visible = rc == 0;

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(sizes);
    dvz_free(angles);
    dvz_free(shapes);
    return rc;
}



static int _graph_prepare_segments(DvzGraph* graph, DvzVisual* segments)
{
    if (graph == NULL || segments == NULL || segments->type != DVZ_VISUAL_TYPE_SEGMENT)
        return -1;
    segments->visible = false;
    if (graph->edge_mode != DVZ_GRAPH_EDGE_MODE_SEGMENT || graph->edge_count == 0)
        return 0;

    const uint32_t count = graph->edge_count;
    if (
        !_polygon_allocation_valid(count, sizeof(vec3)) ||
        !_polygon_allocation_valid(count, sizeof(DvzColor)) ||
        !_polygon_allocation_valid(count, sizeof(float)))
    {
        return -1;
    }

    vec3* starts = (vec3*)dvz_calloc(count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
    {
        dvz_free(starts);
        dvz_free(ends);
        dvz_free(colors);
        dvz_free(widths);
        return -1;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const DvzGraphEdgeRecord* edge = &graph->edges[i];
        _graph_copy_position(starts[i], graph->nodes[edge->source].position);
        _graph_copy_position(ends[i], graph->nodes[edge->target].position);
        colors[i] = edge->visible ? edge->color : dvz_color_rgba(0, 0, 0, 0);
        widths[i] = edge->visible ? edge->width : 0;
    }

    const DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = count},
        {.attr_name = "position_end", .data = ends, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = count},
    };
    int rc = dvz_segment_set_caps(segments, graph->edge_cap_start, graph->edge_cap_end);
    if (rc == 0)
        rc = dvz_visual_set_data_many(segments, updates, 4);
    segments->visible = rc == 0;

    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return rc;
}



static void _graph_default_controls(const DvzGraph* graph, const DvzGraphEdgeRecord* edge, dvec3 c0,
                                    dvec3 c1)
{
    const double* p0 = graph->nodes[edge->source].position;
    const double* p3 = graph->nodes[edge->target].position;
    const double dx = p3[0] - p0[0];
    const double dy = p3[1] - p0[1];
    const double dz = p3[2] - p0[2];
    const double length = sqrt(dx * dx + dy * dy);
    const double bend = length > 0 ? 0.18 * length : 0.0;
    const double nx = length > 0 ? -dy / length : 0.0;
    const double ny = length > 0 ? dx / length : 0.0;

    c0[0] = p0[0] + dx / 3.0 + nx * bend;
    c0[1] = p0[1] + dy / 3.0 + ny * bend;
    c0[2] = p0[2] + dz / 3.0;
    c1[0] = p0[0] + 2.0 * dx / 3.0 + nx * bend;
    c1[1] = p0[1] + 2.0 * dy / 3.0 + ny * bend;
    c1[2] = p0[2] + 2.0 * dz / 3.0;
}



static int _graph_prepare_paths(DvzGraph* graph, DvzVisual* path)
{
    if (graph == NULL || path == NULL || path->type != DVZ_VISUAL_TYPE_PATH)
        return -1;
    path->visible = false;
    if (graph->edge_mode == DVZ_GRAPH_EDGE_MODE_SEGMENT || graph->edge_count == 0)
        return 0;

    const bool bezier = graph->edge_mode == DVZ_GRAPH_EDGE_MODE_BEZIER;
    const uint32_t points_per_edge =
        bezier ? (graph->tessellation.segment_count == 0 ? 33 : graph->tessellation.segment_count + 1)
               : 2;
    uint64_t point_count_u64 = (uint64_t)graph->edge_count * (uint64_t)points_per_edge;
    if (point_count_u64 == 0 || point_count_u64 > UINT32_MAX)
        return -1;
    const uint32_t point_count = (uint32_t)point_count_u64;
    if (
        !_polygon_allocation_valid(point_count, sizeof(vec3)) ||
        !_polygon_allocation_valid(point_count, sizeof(DvzColor)) ||
        !_polygon_allocation_valid(point_count, sizeof(float)) ||
        !_polygon_allocation_valid(graph->edge_count, sizeof(uint32_t)))
    {
        return -1;
    }

    vec3* positions = (vec3*)dvz_calloc(point_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(point_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(point_count, sizeof(float));
    uint32_t* lengths = (uint32_t*)dvz_calloc(graph->edge_count, sizeof(uint32_t));
    if (positions == NULL || colors == NULL || widths == NULL || lengths == NULL)
    {
        dvz_free(positions);
        dvz_free(colors);
        dvz_free(widths);
        dvz_free(lengths);
        return -1;
    }

    uint32_t offset = 0;
    for (uint32_t i = 0; i < graph->edge_count; i++)
    {
        const DvzGraphEdgeRecord* edge = &graph->edges[i];
        const double* p0 = graph->nodes[edge->source].position;
        const double* p3 = graph->nodes[edge->target].position;
        lengths[i] = points_per_edge;

        if (!bezier)
        {
            _graph_copy_position(positions[offset + 0], p0);
            _graph_copy_position(positions[offset + 1], p3);
        }
        else
        {
            dvec3 c0 = {0};
            dvec3 c1 = {0};
            if (edge->has_controls)
            {
                memcpy(c0, edge->control0, sizeof(dvec3));
                memcpy(c1, edge->control1, sizeof(dvec3));
            }
            else
            {
                _graph_default_controls(graph, edge, c0, c1);
            }
            DvzTessellatedPath* tess =
                dvz_tessellate_cubic_bezier(p0, c0, c1, p3, &graph->tessellation);
            if (tess == NULL || tess->point_count != points_per_edge)
            {
                dvz_tessellated_path_destroy(tess);
                dvz_free(positions);
                dvz_free(colors);
                dvz_free(widths);
                dvz_free(lengths);
                return -1;
            }
            for (uint32_t j = 0; j < tess->point_count; j++)
                _graph_copy_position(positions[offset + j], tess->points[j]);
            dvz_tessellated_path_destroy(tess);
        }

        for (uint32_t j = 0; j < points_per_edge; j++)
        {
            colors[offset + j] = edge->visible ? edge->color : dvz_color_rgba(0, 0, 0, 0);
            widths[offset + j] = edge->visible ? edge->width : 0;
        }
        offset += points_per_edge;
    }

    int rc = dvz_path_set_caps(path, graph->edge_cap_start, graph->edge_cap_end);
    if (rc == 0)
        rc = dvz_path_set_join(path, graph->edge_join, graph->edge_miter_limit);
    if (rc == 0)
        rc = dvz_path_set_subpaths(path, graph->edge_count, lengths);
    if (rc == 0)
    {
        const DvzVisualDataUpdate updates[] = {
            {.attr_name = "position", .data = positions, .item_count = point_count},
            {.attr_name = "color", .data = colors, .item_count = point_count},
            {.attr_name = "stroke_width_px", .data = widths, .item_count = point_count},
        };
        rc = dvz_visual_set_data_many(path, updates, 3);
    }
    path->visible = rc == 0;

    dvz_free(positions);
    dvz_free(colors);
    dvz_free(widths);
    dvz_free(lengths);
    return rc;
}



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

/**
 * Realize or refresh one graph composite.
 *
 * @param composite graph composite
 * @return 0 on success, -1 on error
 */
int _graph_composite_prepare(DvzComposite* composite)
{
    if (
        composite == NULL || !composite->active || composite->type != DVZ_COMPOSITE_TYPE_GRAPH ||
        composite->source == NULL)
    {
        return -1;
    }

    DvzGraph* graph = (DvzGraph*)composite->source;
    if (!graph->active || graph->nodes == NULL || graph->node_count == 0)
        return -1;
    if (!composite->dirty && composite->source_version_seen == graph->version)
        return 0;

    _graph_update_edge_roles(composite, graph);
    DvzCompositeVisual* nodes_role =
        _composite_visual_slot(composite, DVZ_GRAPH_COMPOSITE_NODES_ROLE);
    DvzCompositeVisual* segment_role =
        _composite_visual_type(composite, DVZ_VISUAL_TYPE_SEGMENT);
    DvzCompositeVisual* path_role = _composite_visual_type(composite, DVZ_VISUAL_TYPE_PATH);
    if (nodes_role == NULL || segment_role == NULL || path_role == NULL)
        return -1;

    if (nodes_role->dirty && _graph_prepare_nodes(graph, nodes_role->visual) != 0)
        return -1;
    if (segment_role->dirty && _graph_prepare_segments(graph, segment_role->visual) != 0)
        return -1;
    if (path_role->dirty && _graph_prepare_paths(graph, path_role->visual) != 0)
        return -1;

    composite->dirty = false;
    nodes_role->dirty = false;
    segment_role->dirty = false;
    path_role->dirty = false;
    composite->source_version_seen = graph->version;
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned composite render view for a graph.
 *
 * @param graph the source graph
 * @param flags reserved composite flags
 * @return the composite, or NULL on allocation failure
 */
DvzComposite* dvz_graph_composite(DvzGraph* graph, uint32_t flags)
{
    if (graph == NULL || graph->scene == NULL || !graph->active)
        return NULL;
    DvzScene* scene = graph->scene;
    if (!_scene_visual_mutation_allowed(scene, "create graph composite"))
        return NULL;

    DvzComposite* composite = _scene_alloc_composite(scene);
    if (composite == NULL)
        return NULL;
    composite->type = DVZ_COMPOSITE_TYPE_GRAPH;
    composite->flags = flags;
    composite->source = graph;

    DvzVisual* edges_segment = dvz_segment(scene, 0);
    DvzVisual* edges_path = dvz_path(scene, 0);
    DvzVisual* nodes = dvz_marker(scene, 0);
    if (edges_segment == NULL || edges_path == NULL || nodes == NULL)
    {
        if (edges_segment != NULL)
            edges_segment->visible = false;
        if (edges_path != NULL)
            edges_path->visible = false;
        if (nodes != NULL)
            nodes->visible = false;
        _scene_composite_reset(composite);
        return NULL;
    }

    const bool use_segment = graph->edge_mode == DVZ_GRAPH_EDGE_MODE_SEGMENT;
    if (
        !_composite_add_visual(
            composite,
            use_segment ? DVZ_GRAPH_COMPOSITE_EDGES_ROLE
                        : DVZ_GRAPH_COMPOSITE_SEGMENTS_INTERNAL_ROLE,
            edges_segment, 0) ||
        !_composite_add_visual(
            composite,
            use_segment ? DVZ_GRAPH_COMPOSITE_PATH_INTERNAL_ROLE : DVZ_GRAPH_COMPOSITE_EDGES_ROLE,
            edges_path, 0) ||
        !_composite_add_visual(composite, DVZ_GRAPH_COMPOSITE_NODES_ROLE, nodes, 1))
    {
        edges_segment->visible = false;
        edges_path->visible = false;
        nodes->visible = false;
        _scene_composite_reset(composite);
        return NULL;
    }

    return composite;
}
