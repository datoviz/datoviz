/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Graph semantic objects                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "graph_internal.h"
#include "polygon_internal.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GRAPH_EDGE_STYLE_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _graph_dvec3_finite(const dvec3 v)
{
    return v != NULL && isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}



static bool _graph_range_valid(uint32_t count, uint32_t first, uint32_t item_count)
{
    return first <= count && item_count <= count - first;
}



static void _graph_node_default(DvzGraphNode* node)
{
    ANN(node);
    dvz_memset(node, sizeof(DvzGraphNode), 0, sizeof(DvzGraphNode));
    node->color = dvz_color_rgba(35, 190, 210, 255);
    node->size = 18.0f;
    node->shape = DVZ_MARKER_SHAPE_DISC;
    node->visible = true;
}



static void _graph_edge_default(DvzGraphEdgeRecord* edge)
{
    ANN(edge);
    dvz_memset(edge, sizeof(DvzGraphEdgeRecord), 0, sizeof(DvzGraphEdgeRecord));
    edge->color = dvz_color_rgba(190, 205, 215, 210);
    edge->width = 3.0f;
    edge->visible = true;
}



static bool _graph_edge_mode_valid(DvzGraphEdgeMode mode)
{
    return mode == DVZ_GRAPH_EDGE_MODE_SEGMENT || mode == DVZ_GRAPH_EDGE_MODE_PATH ||
           mode == DVZ_GRAPH_EDGE_MODE_BEZIER;
}



static bool _graph_segment_cap_valid(DvzSegmentCap cap)
{
    return cap == DVZ_SEGMENT_CAP_NONE || cap == DVZ_SEGMENT_CAP_ROUND ||
           cap == DVZ_SEGMENT_CAP_TRIANGLE_IN || cap == DVZ_SEGMENT_CAP_TRIANGLE_OUT ||
           cap == DVZ_SEGMENT_CAP_SQUARE || cap == DVZ_SEGMENT_CAP_BUTT;
}



static bool _graph_path_join_valid(DvzPathJoin join)
{
    return join == DVZ_PATH_JOIN_MITER || join == DVZ_PATH_JOIN_ROUND ||
           join == DVZ_PATH_JOIN_BEVEL;
}



static bool _graph_tessellation_valid(const DvzBezierTessellationDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzBezierTessellationDesc, DVZ_GRAPH_EDGE_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzBezierTessellationDesc ABI prologue");
        return false;
    }
    if (desc->segment_count > 65535)
    {
        log_error("graph Bezier tessellation segment_count exceeds the supported maximum");
        return false;
    }
    if (desc->tolerance < 0 || !isfinite(desc->tolerance))
    {
        log_error("graph Bezier tessellation tolerance must be finite and non-negative");
        return false;
    }
    return true;
}



static bool _graph_edge_style_valid(const DvzGraphEdgeStyle* style)
{
    if (style == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(style, DvzGraphEdgeStyle, DVZ_GRAPH_EDGE_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid DvzGraphEdgeStyle ABI prologue");
        return false;
    }
    if (!_graph_edge_mode_valid(style->mode))
        return false;
    if (!_graph_tessellation_valid(&style->tessellation))
        return false;
    if (!_graph_segment_cap_valid(style->start_cap) || !_graph_segment_cap_valid(style->end_cap))
        return false;
    if (!_graph_path_join_valid(style->join))
        return false;
    if (style->miter_limit <= 0 || !isfinite(style->miter_limit))
        return false;
    return true;
}



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

/**
 * Allocate one scene-owned graph slot.
 *
 * @param scene the scene
 * @return the graph slot, or NULL on capacity exhaustion
 */
DvzGraph* _scene_alloc_graph(DvzScene* scene)
{
    if (scene == NULL || scene->graph_count >= DVZ_SCENE_MAX_GRAPHS)
        return NULL;

    DvzGraph* graph = &scene->graphs[scene->graph_count++];
    dvz_memset(graph, sizeof(DvzGraph), 0, sizeof(DvzGraph));
    graph->scene = scene;
    graph->active = true;
    DvzGraphEdgeStyle style = dvz_graph_edge_style();
    graph->edge_mode = style.mode;
    graph->tessellation = style.tessellation;
    graph->edge_cap_start = style.start_cap;
    graph->edge_cap_end = style.end_cap;
    graph->edge_join = style.join;
    graph->edge_miter_limit = style.miter_limit;
    graph->version = 1;
    return graph;
}



/**
 * Reset a scene graph slot and release retained node/edge data.
 *
 * @param graph the graph
 */
void _scene_graph_reset(DvzGraph* graph)
{
    if (graph == NULL)
        return;

    dvz_free(graph->nodes);
    dvz_free(graph->edges);
    graph->nodes = NULL;
    graph->edges = NULL;
    graph->node_count = 0;
    graph->edge_count = 0;
    graph->scene = NULL;
    graph->active = false;
}



/**
 * Notify generated visuals for every composite depending on one graph.
 *
 * @param graph changed graph
 * @param nodes_dirty whether the node role needs refresh
 * @param edges_dirty whether the edge role needs refresh
 */
void _graph_mark_composites_dirty(DvzGraph* graph, bool nodes_dirty, bool edges_dirty)
{
    if (graph == NULL || graph->scene == NULL)
        return;

    DvzScene* scene = graph->scene;
    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (!composite->active || composite->type != DVZ_COMPOSITE_TYPE_GRAPH ||
            composite->source != graph)
        {
            continue;
        }
        for (uint32_t j = 0; j < composite->visual_count; j++)
        {
            DvzCompositeVisual* composite_visual = &composite->visuals[j];
            const bool notify_nodes =
                nodes_dirty && strcmp(composite_visual->role, DVZ_GRAPH_COMPOSITE_NODES_ROLE) == 0;
            const bool notify_edges =
                edges_dirty &&
                (strcmp(composite_visual->role, DVZ_GRAPH_COMPOSITE_EDGES_ROLE) == 0 ||
                 strcmp(composite_visual->role, DVZ_GRAPH_COMPOSITE_SEGMENTS_INTERNAL_ROLE) == 0 ||
                 strcmp(composite_visual->role, DVZ_GRAPH_COMPOSITE_PATH_INTERNAL_ROLE) == 0);
            if (notify_nodes || notify_edges)
            {
                composite->dirty = true;
                composite_visual->dirty = true;
                _scene_notify_visual_changed(composite_visual->visual);
            }
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned semantic graph object.
 *
 * @param scene the scene
 * @param flags reserved graph flags
 * @return the graph, or NULL on allocation failure
 */
DvzGraph* dvz_graph(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (!_scene_visual_mutation_allowed(scene, "create graph"))
        return NULL;

    DvzGraph* graph = _scene_alloc_graph(scene);
    if (graph == NULL)
        return NULL;
    graph->flags = flags;
    return graph;
}



/**
 * Destroy a scene-owned graph object.
 *
 * @param graph the graph
 */
void dvz_graph_destroy(DvzGraph* graph)
{
    if (graph == NULL || graph->scene == NULL)
        return;
    DvzScene* scene = graph->scene;
    if (!_scene_visual_mutation_allowed(scene, "destroy graph"))
        return;

    for (uint32_t i = 0; i < scene->composite_count; i++)
    {
        DvzComposite* composite = &scene->composites[i];
        if (composite->active && composite->source == graph)
            dvz_composite_destroy(composite);
    }
    _scene_graph_reset(graph);
}



/**
 * Return the default graph edge style descriptor.
 *
 * @return default graph edge style
 */
DvzGraphEdgeStyle dvz_graph_edge_style(void)
{
    return (DvzGraphEdgeStyle){
        DVZ_STRUCT_INIT_FIELDS(DvzGraphEdgeStyle),
        .mode = DVZ_GRAPH_EDGE_MODE_SEGMENT,
        .tessellation = dvz_bezier_tessellation_desc(),
        .start_cap = DVZ_SEGMENT_CAP_ROUND,
        .end_cap = DVZ_SEGMENT_CAP_ROUND,
        .join = DVZ_PATH_JOIN_ROUND,
        .miter_limit = 4.0f,
    };
}



/**
 * Replace the graph node array and reset node style defaults.
 *
 * @param graph the graph
 * @param node_count number of nodes
 * @return 0 on success, -1 on error
 */
DvzResult dvz_graph_set_node_count(DvzGraph* graph, uint32_t node_count)
{
    if (graph == NULL || graph->scene == NULL || node_count == 0)
        return -1;
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph nodes"))
        return -1;
    if (!_polygon_allocation_valid(node_count, sizeof(DvzGraphNode)))
        return -1;

    DvzGraphNode* nodes = (DvzGraphNode*)dvz_calloc(node_count, sizeof(DvzGraphNode));
    if (nodes == NULL)
        return -1;
    for (uint32_t i = 0; i < node_count; i++)
        _graph_node_default(&nodes[i]);

    dvz_free(graph->nodes);
    dvz_free(graph->edges);
    graph->nodes = nodes;
    graph->node_count = node_count;
    graph->edges = NULL;
    graph->edge_count = 0;
    graph->version++;
    _graph_mark_composites_dirty(graph, true, true);
    return 0;
}



/**
 * Replace the graph node positions without changing node styles or edges.
 *
 * @param graph the graph
 * @param first_node first node index
 * @param node_count number of node positions to update
 * @param positions borrowed node positions
 * @return 0 on success, -1 on error
 */
DvzResult dvz_graph_node_positions(
    DvzGraph* graph, uint32_t first_node, uint32_t node_count, const dvec3* positions)
{
    if (
        graph == NULL || graph->scene == NULL || positions == NULL ||
        !_graph_range_valid(graph->node_count, first_node, node_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph node positions"))
        return -1;
    for (uint32_t i = 0; i < node_count; i++)
    {
        if (!_graph_dvec3_finite(positions[i]))
            return -1;
    }
    for (uint32_t i = 0; i < node_count; i++)
        memcpy(graph->nodes[first_node + i].position, positions[i], sizeof(dvec3));
    graph->version++;
    _graph_mark_composites_dirty(graph, true, true);
    return 0;
}



/**
 * Replace the graph edge array and reset edge style defaults.
 *
 * @param graph the graph
 * @param edge_count number of edges
 * @return 0 on success, -1 on invalid endpoints or allocation failure
 */
DvzResult dvz_graph_set_edge_count(DvzGraph* graph, uint32_t edge_count)
{
    if (graph == NULL || graph->scene == NULL || graph->node_count == 0)
        return -1;
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edges"))
        return -1;
    if (!_polygon_allocation_valid(edge_count, sizeof(DvzGraphEdgeRecord)))
        return -1;

    DvzGraphEdgeRecord* records = NULL;
    if (edge_count > 0)
    {
        records = (DvzGraphEdgeRecord*)dvz_calloc(edge_count, sizeof(DvzGraphEdgeRecord));
        if (records == NULL)
            return -1;
    }
    for (uint32_t i = 0; i < edge_count; i++)
        _graph_edge_default(&records[i]);

    dvz_free(graph->edges);
    graph->edges = records;
    graph->edge_count = edge_count;
    graph->version++;
    _graph_mark_composites_dirty(graph, false, true);
    return 0;
}



/**
 * Update graph edge endpoints.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edges
 * @param endpoints borrowed packed endpoint array: source0, target0, source1, target1, ...
 * @return 0 on success, -1 on invalid endpoints or allocation failure
 */
DvzResult dvz_graph_edges(
    DvzGraph* graph, uint32_t first_edge, uint32_t edge_count, const uint32_t* endpoints)
{
    if (
        graph == NULL || graph->scene == NULL || endpoints == NULL ||
        !_graph_range_valid(graph->edge_count, first_edge, edge_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edge endpoints"))
        return -1;

    for (uint32_t i = 0; i < edge_count; i++)
    {
        const uint32_t source = endpoints[2 * i + 0];
        const uint32_t target = endpoints[2 * i + 1];
        if (source >= graph->node_count || target >= graph->node_count)
            return -1;
    }
    for (uint32_t i = 0; i < edge_count; i++)
    {
        DvzGraphEdgeRecord* edge = &graph->edges[first_edge + i];
        edge->source = endpoints[2 * i + 0];
        edge->target = endpoints[2 * i + 1];
        edge->has_controls = false;
    }
    graph->version++;
    _graph_mark_composites_dirty(graph, false, true);
    return 0;
}



/**
 * Set stable graph node user ids.
 *
 * @param graph the graph
 * @param first_node first node index
 * @param node_count number of nodes
 * @param ids borrowed user-id array
 * @return 0 on success, -1 on error
 */
DvzResult dvz_graph_node_ids(
    DvzGraph* graph, uint32_t first_node, uint32_t node_count, const uint64_t* ids)
{
    if (
        graph == NULL || graph->scene == NULL || ids == NULL ||
        !_graph_range_valid(graph->node_count, first_node, node_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph node ids"))
        return -1;
    for (uint32_t i = 0; i < node_count; i++)
        graph->nodes[first_node + i].user_id = ids[i];
    graph->version++;
    return 0;
}



/**
 * Set stable graph edge user ids.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edges
 * @param ids borrowed user-id array
 * @return 0 on success, -1 on error
 */
DvzResult dvz_graph_edge_ids(
    DvzGraph* graph, uint32_t first_edge, uint32_t edge_count, const uint64_t* ids)
{
    if (
        graph == NULL || graph->scene == NULL || ids == NULL ||
        !_graph_range_valid(graph->edge_count, first_edge, edge_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edge ids"))
        return -1;
    for (uint32_t i = 0; i < edge_count; i++)
        graph->edges[first_edge + i].user_id = ids[i];
    graph->version++;
    return 0;
}



/**
 * Configure graph edge rendering.
 *
 * @param graph the graph
 * @param style edge style descriptor
 * @return 0 on success, -1 on error
 */
DvzResult dvz_graph_set_edge_style(DvzGraph* graph, const DvzGraphEdgeStyle* style)
{
    if (graph == NULL || graph->scene == NULL || !_graph_edge_style_valid(style))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edge style"))
        return -1;

    graph->edge_mode = style->mode;
    graph->tessellation = style->tessellation;
    graph->edge_cap_start = style->start_cap;
    graph->edge_cap_end = style->end_cap;
    graph->edge_join = style->join;
    graph->edge_miter_limit = style->miter_limit;
    graph->version++;
    _graph_mark_composites_dirty(graph, false, true);
    return 0;
}



/**
 * Configure explicit cubic Bezier control points for graph edges.
 *
 * @param graph the graph
 * @param first_edge first edge index
 * @param edge_count number of edge controls to update
 * @param control0 borrowed first control point array
 * @param control1 borrowed second control point array
 * @return 0 on success, -1 on error
 */
DvzResult dvz_graph_edge_controls(
    DvzGraph* graph, uint32_t first_edge, uint32_t edge_count, const dvec3* control0,
    const dvec3* control1)
{
    if (
        graph == NULL || graph->scene == NULL || control0 == NULL || control1 == NULL ||
        !_graph_range_valid(graph->edge_count, first_edge, edge_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edge controls"))
        return -1;
    for (uint32_t i = 0; i < edge_count; i++)
    {
        if (!_graph_dvec3_finite(control0[i]) || !_graph_dvec3_finite(control1[i]))
            return -1;
    }
    for (uint32_t i = 0; i < edge_count; i++)
    {
        DvzGraphEdgeRecord* edge = &graph->edges[first_edge + i];
        memcpy(edge->control0, control0[i], sizeof(dvec3));
        memcpy(edge->control1, control1[i], sizeof(dvec3));
        edge->has_controls = true;
    }
    graph->version++;
    _graph_mark_composites_dirty(graph, false, true);
    return 0;
}



DvzResult dvz_graph_node_colors(
    DvzGraph* graph, uint32_t first_node, uint32_t node_count, const DvzColor* colors)
{
    if (
        graph == NULL || graph->scene == NULL || colors == NULL ||
        !_graph_range_valid(graph->node_count, first_node, node_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph node colors"))
        return -1;
    for (uint32_t i = 0; i < node_count; i++)
        graph->nodes[first_node + i].color = colors[i];
    graph->version++;
    _graph_mark_composites_dirty(graph, true, false);
    return 0;
}



DvzResult dvz_graph_node_sizes(
    DvzGraph* graph, uint32_t first_node, uint32_t node_count, const float* sizes)
{
    if (
        graph == NULL || graph->scene == NULL || sizes == NULL ||
        !_graph_range_valid(graph->node_count, first_node, node_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph node sizes"))
        return -1;
    for (uint32_t i = 0; i < node_count; i++)
    {
        if (sizes[i] < 0 || !isfinite(sizes[i]))
            return -1;
    }
    for (uint32_t i = 0; i < node_count; i++)
        graph->nodes[first_node + i].size = sizes[i];
    graph->version++;
    _graph_mark_composites_dirty(graph, true, false);
    return 0;
}



DvzResult dvz_graph_edge_colors(
    DvzGraph* graph, uint32_t first_edge, uint32_t edge_count, const DvzColor* colors)
{
    if (
        graph == NULL || graph->scene == NULL || colors == NULL ||
        !_graph_range_valid(graph->edge_count, first_edge, edge_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edge colors"))
        return -1;
    for (uint32_t i = 0; i < edge_count; i++)
        graph->edges[first_edge + i].color = colors[i];
    graph->version++;
    _graph_mark_composites_dirty(graph, false, true);
    return 0;
}



DvzResult dvz_graph_edge_widths(
    DvzGraph* graph, uint32_t first_edge, uint32_t edge_count, const float* widths)
{
    if (
        graph == NULL || graph->scene == NULL || widths == NULL ||
        !_graph_range_valid(graph->edge_count, first_edge, edge_count))
    {
        return -1;
    }
    if (!_scene_visual_mutation_allowed(graph->scene, "update graph edge widths"))
        return -1;
    for (uint32_t i = 0; i < edge_count; i++)
    {
        if (widths[i] < 0 || !isfinite(widths[i]))
            return -1;
    }
    for (uint32_t i = 0; i < edge_count; i++)
        graph->edges[first_edge + i].width = widths[i];
    graph->version++;
    _graph_mark_composites_dirty(graph, false, true);
    return 0;
}
