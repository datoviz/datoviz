/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* graph - deterministic brain-connectivity graph composite.
 *
 * Scenario: composite_graph
 * Style: feature composite, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c composites/graph
 * Run:    ./build/examples/c/composites/graph --live
 * Smoke:  ./build/examples/c/composites/graph --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct BrainCommunity
{
    DvzColor color;
} BrainCommunity;


typedef struct BrainNode
{
    const char* label;
    uint32_t community;
    dvec3 position;
    uint64_t semantic_id;
    float strength;
} BrainNode;


typedef struct BrainEdge
{
    uint32_t source;
    uint32_t target;
    uint64_t semantic_id;
    float weight;
    bool bridge;
} BrainEdge;



/*************************************************************************************************/
/*  Data                                                                                         */
/*************************************************************************************************/

static const BrainCommunity COMMUNITIES[] = {
    {.color = {65, 201, 226, 255}},
    {.color = {246, 185, 72, 255}},
    {.color = {234, 104, 91, 255}},
};


static const BrainNode NODES[] = {
    {.label = "V1", .community = 0, .position = {-1.05, +0.42, 0.0}, .semantic_id = 101, .strength = 0.88f},
    {.label = "V2", .community = 0, .position = {-0.80, +0.62, 0.0}, .semantic_id = 102, .strength = 0.72f},
    {.label = "V4", .community = 0, .position = {-0.55, +0.42, 0.0}, .semantic_id = 103, .strength = 0.76f},
    {.label = "LGN", .community = 0, .position = {-0.92, +0.12, 0.0}, .semantic_id = 104, .strength = 0.66f},
    {.label = "MT", .community = 0, .position = {-0.62, +0.12, 0.0}, .semantic_id = 105, .strength = 0.70f},
    {.label = "M1", .community = 1, .position = {+0.55, +0.45, 0.0}, .semantic_id = 201, .strength = 0.86f},
    {.label = "S1", .community = 1, .position = {+0.82, +0.62, 0.0}, .semantic_id = 202, .strength = 0.78f},
    {.label = "SMA", .community = 1, .position = {+1.05, +0.36, 0.0}, .semantic_id = 203, .strength = 0.70f},
    {.label = "PMd", .community = 1, .position = {+0.70, +0.15, 0.0}, .semantic_id = 204, .strength = 0.67f},
    {.label = "Thal", .community = 1, .position = {+1.00, +0.08, 0.0}, .semantic_id = 205, .strength = 0.80f},
    {.label = "HPC-L", .community = 2, .position = {-0.30, -0.55, 0.0}, .semantic_id = 301, .strength = 0.84f},
    {.label = "HPC-R", .community = 2, .position = {+0.00, -0.78, 0.0}, .semantic_id = 302, .strength = 0.82f},
    {.label = "Ent", .community = 2, .position = {-0.58, -0.70, 0.0}, .semantic_id = 303, .strength = 0.63f},
    {.label = "PCC", .community = 2, .position = {+0.34, -0.50, 0.0}, .semantic_id = 304, .strength = 0.74f},
    {.label = "Amy", .community = 2, .position = {+0.55, -0.72, 0.0}, .semantic_id = 305, .strength = 0.60f},
};


static const BrainEdge EDGES[] = {
    {.source = 0, .target = 1, .semantic_id = 1001, .weight = 0.84f, .bridge = false},
    {.source = 1, .target = 2, .semantic_id = 1002, .weight = 0.76f, .bridge = false},
    {.source = 0, .target = 3, .semantic_id = 1003, .weight = 0.62f, .bridge = false},
    {.source = 3, .target = 4, .semantic_id = 1004, .weight = 0.58f, .bridge = false},
    {.source = 2, .target = 4, .semantic_id = 1005, .weight = 0.71f, .bridge = false},
    {.source = 0, .target = 4, .semantic_id = 1006, .weight = 0.66f, .bridge = false},
    {.source = 5, .target = 6, .semantic_id = 2001, .weight = 0.82f, .bridge = false},
    {.source = 6, .target = 7, .semantic_id = 2002, .weight = 0.70f, .bridge = false},
    {.source = 5, .target = 8, .semantic_id = 2003, .weight = 0.64f, .bridge = false},
    {.source = 8, .target = 9, .semantic_id = 2004, .weight = 0.68f, .bridge = false},
    {.source = 7, .target = 9, .semantic_id = 2005, .weight = 0.74f, .bridge = false},
    {.source = 5, .target = 9, .semantic_id = 2006, .weight = 0.60f, .bridge = false},
    {.source = 10, .target = 11, .semantic_id = 3001, .weight = 0.86f, .bridge = false},
    {.source = 10, .target = 12, .semantic_id = 3002, .weight = 0.72f, .bridge = false},
    {.source = 11, .target = 13, .semantic_id = 3003, .weight = 0.66f, .bridge = false},
    {.source = 12, .target = 14, .semantic_id = 3004, .weight = 0.57f, .bridge = false},
    {.source = 13, .target = 14, .semantic_id = 3005, .weight = 0.61f, .bridge = false},
    {.source = 10, .target = 13, .semantic_id = 3006, .weight = 0.64f, .bridge = false},
    {.source = 2, .target = 5, .semantic_id = 4001, .weight = 0.48f, .bridge = true},
    {.source = 4, .target = 8, .semantic_id = 4002, .weight = 0.42f, .bridge = true},
    {.source = 1, .target = 6, .semantic_id = 4003, .weight = 0.35f, .bridge = true},
    {.source = 3, .target = 12, .semantic_id = 4004, .weight = 0.44f, .bridge = true},
    {.source = 4, .target = 10, .semantic_id = 4005, .weight = 0.39f, .bridge = true},
    {.source = 8, .target = 13, .semantic_id = 4006, .weight = 0.50f, .bridge = true},
    {.source = 9, .target = 14, .semantic_id = 4007, .weight = 0.46f, .bridge = true},
    {.source = 5, .target = 13, .semantic_id = 4008, .weight = 0.37f, .bridge = true},
};


typedef struct BrainBounds
{
    double xmin;
    double xmax;
    double ymin;
    double ymax;
} BrainBounds;


enum
{
    COMMUNITY_COUNT = sizeof(COMMUNITIES) / sizeof(COMMUNITIES[0]),
    NODE_COUNT = sizeof(NODES) / sizeof(NODES[0]),
    EDGE_COUNT = sizeof(EDGES) / sizeof(EDGES[0]),
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _bounds_add(BrainBounds* bounds, double x, double y)
{
    ANN(bounds);
    if (x < bounds->xmin)
        bounds->xmin = x;
    if (x > bounds->xmax)
        bounds->xmax = x;
    if (y < bounds->ymin)
        bounds->ymin = y;
    if (y > bounds->ymax)
        bounds->ymax = y;
}



static void _default_edge_controls(const dvec3 p0, const dvec3 p3, dvec3 c0, dvec3 c1)
{
    ANN(p0);
    ANN(p3);
    ANN(c0);
    ANN(c1);

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



static void _bridge_edge_controls(uint32_t edge_index, uint32_t bridge_index, dvec3 c0, dvec3 c1)
{
    ASSERT(edge_index < EDGE_COUNT);
    ANN(c0);
    ANN(c1);

    const BrainEdge* edge = &EDGES[edge_index];
    const dvec3* source = &NODES[edge->source].position;
    const dvec3* target = &NODES[edge->target].position;
    const double sx = (*source)[0];
    const double sy = (*source)[1];
    const double tx = (*target)[0];
    const double ty = (*target)[1];
    const double dx = tx - sx;
    const double dy = ty - sy;
    const double bend = (bridge_index % 2 == 0 ? 1.0 : -1.0) * (0.14 + 0.08 * edge->weight);

    c0[0] = sx + 0.36 * dx - bend * dy;
    c0[1] = sy + 0.36 * dy + bend * dx;
    c0[2] = 0.0;
    c1[0] = sx + 0.64 * dx - bend * dy;
    c1[1] = sy + 0.64 * dy + bend * dx;
    c1[2] = 0.0;
}



static void _bounds_add_cubic(
    BrainBounds* bounds, const dvec3 p0, const dvec3 c0, const dvec3 c1, const dvec3 p3)
{
    ANN(bounds);
    ANN(p0);
    ANN(c0);
    ANN(c1);
    ANN(p3);

    for (uint32_t i = 0; i <= 32; i++)
    {
        const double t = (double)i / 32.0;
        const double u = 1.0 - t;
        const double uu = u * u;
        const double tt = t * t;
        const double x =
            uu * u * p0[0] + 3.0 * uu * t * c0[0] + 3.0 * u * tt * c1[0] + tt * t * p3[0];
        const double y =
            uu * u * p0[1] + 3.0 * uu * t * c0[1] + 3.0 * u * tt * c1[1] + tt * t * p3[1];
        _bounds_add(bounds, x, y);
    }
}



static BrainBounds _graph_bounds(void)
{
    BrainBounds bounds = {.xmin = DBL_MAX, .xmax = -DBL_MAX, .ymin = DBL_MAX, .ymax = -DBL_MAX};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
        _bounds_add(&bounds, NODES[i].position[0], NODES[i].position[1]);

    uint32_t bridge_index = 0;
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
    {
        const BrainEdge* edge = &EDGES[i];
        const dvec3* p0 = &NODES[edge->source].position;
        const dvec3* p3 = &NODES[edge->target].position;
        dvec3 c0 = {0};
        dvec3 c1 = {0};
        if (edge->bridge)
            _bridge_edge_controls(i, bridge_index++, c0, c1);
        else
            _default_edge_controls(*p0, *p3, c0, c1);
        _bounds_add_cubic(&bounds, *p0, c0, c1, *p3);
    }
    return bounds;
}



/**
 * Configure the panel used by the graph composite example.
 *
 * @param panel target panel
 * @return whether setup succeeded
 */
static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);
    const BrainBounds bounds = _graph_bounds();
    if (dvz_panel_set_padding(
            panel, &(DvzPanelReserve){
                       .left_px = 24.0f, .right_px = 24.0f, .bottom_px = 18.0f,
                       .top_px = 18.0f}) != DVZ_OK)
        return false;
    return example_configure_equal_aspect_panel(
        panel, (DvzDataDomain){.min = bounds.xmin, .max = bounds.xmax},
        (DvzDataDomain){.min = bounds.ymin, .max = bounds.ymax}, 0.14);
}



/**
 * Fill node positions from the fixed brain-region table.
 *
 * @param positions output node positions
 */
static void _make_positions(dvec3 positions[NODE_COUNT])
{
    ANN(positions);
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        positions[i][0] = NODES[i].position[0];
        positions[i][1] = NODES[i].position[1];
        positions[i][2] = NODES[i].position[2];
    }
}



static void _push_edge(uint32_t endpoints[2 * EDGE_COUNT], uint32_t* edge_count, uint32_t source,
                       uint32_t target)
{
    ANN(endpoints);
    ANN(edge_count);
    ASSERT(*edge_count < EDGE_COUNT);
    endpoints[2 * *edge_count + 0] = source;
    endpoints[2 * *edge_count + 1] = target;
    *edge_count += 1;
}



static void _make_edges(uint32_t endpoints[2 * EDGE_COUNT], uint32_t* bridge_first_edge)
{
    ANN(endpoints);
    ANN(bridge_first_edge);

    uint32_t edge_count = 0;
    *bridge_first_edge = EDGE_COUNT;
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
    {
        if (EDGES[i].bridge && *bridge_first_edge == EDGE_COUNT)
            *bridge_first_edge = i;
        ASSERT(!EDGES[i].bridge || i >= *bridge_first_edge);
        _push_edge(endpoints, &edge_count, EDGES[i].source, EDGES[i].target);
    }

    ASSERT(edge_count == EDGE_COUNT);
}



static void _make_bridge_controls(
    dvec3 positions[NODE_COUNT], const uint32_t endpoints[2 * EDGE_COUNT],
    uint32_t bridge_first_edge, uint32_t bridge_count, dvec3 control0[EDGE_COUNT],
    dvec3 control1[EDGE_COUNT])
{
    ANN(positions);
    ANN(endpoints);
    ANN(control0);
    ANN(control1);

    for (uint32_t i = 0; i < bridge_count; i++)
    {
        const uint32_t edge_index = bridge_first_edge + i;
        ASSERT(endpoints[2 * edge_index + 0] == EDGES[edge_index].source);
        ASSERT(endpoints[2 * edge_index + 1] == EDGES[edge_index].target);
        _bridge_edge_controls(edge_index, i, control0[i], control1[i]);
    }
}



/**
 * Add a semantic graph composite.
 *
 * @param scene scene owning the graph
 * @param panel panel receiving the composite
 * @return whether the graph was created and attached
 */
static bool _add_graph(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    dvec3 positions[NODE_COUNT] = {0};
    _make_positions(positions);

    uint32_t edges[2 * EDGE_COUNT] = {0};
    uint32_t bridge_first_edge = 0;
    _make_edges(edges, &bridge_first_edge);

    DvzGraph* graph = dvz_graph(scene, 0);
    if (graph == NULL)
        return false;
    int rc = dvz_graph_set_node_count(graph, NODE_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_graph_set_node_positions(graph, 0, NODE_COUNT, (const dvec3*)positions);
    if (rc != 0)
        return false;
    rc = dvz_graph_set_edge_count(graph, EDGE_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_graph_set_edge_endpoints(graph, 0, EDGE_COUNT, edges);
    if (rc != 0)
        return false;
    uint64_t node_ids[NODE_COUNT] = {0};
    uint64_t edge_ids[EDGE_COUNT] = {0};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
        node_ids[i] = NODES[i].semantic_id;
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
        edge_ids[i] = EDGES[i].semantic_id;
    rc = dvz_graph_set_node_ids(graph, 0, NODE_COUNT, node_ids);
    if (rc != 0)
        return false;
    rc = dvz_graph_set_edge_ids(graph, 0, EDGE_COUNT, edge_ids);
    if (rc != 0)
        return false;

    DvzColor node_colors[NODE_COUNT] = {0};
    float node_sizes[NODE_COUNT] = {0};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        const uint32_t community = NODES[i].community;
        ASSERT(community < COMMUNITY_COUNT);
        node_colors[i] = COMMUNITIES[community].color;
        node_sizes[i] = 18.0f + 24.0f * NODES[i].strength;
    }
    rc = dvz_graph_set_node_colors(graph, 0, NODE_COUNT, node_colors);
    if (rc != 0)
        return false;
    rc = dvz_graph_set_node_sizes(graph, 0, NODE_COUNT, node_sizes);
    if (rc != 0)
        return false;

    DvzColor edge_colors[EDGE_COUNT] = {0};
    float edge_widths[EDGE_COUNT] = {0};
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
    {
        const bool bridge = EDGES[i].bridge;
        const uint32_t community = NODES[EDGES[i].source].community;
        edge_colors[i] = bridge ? (DvzColor){222, 236, 244, 180} : COMMUNITIES[community].color;
        edge_colors[i].a = bridge ? 185u : 105u;
        edge_widths[i] = 1.1f + 4.2f * EDGES[i].weight;
    }
    rc = dvz_graph_set_edge_colors(graph, 0, EDGE_COUNT, edge_colors);
    if (rc != 0)
        return false;
    rc = dvz_graph_set_edge_widths(graph, 0, EDGE_COUNT, edge_widths);
    if (rc != 0)
        return false;

    DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
    edge_style.mode = DVZ_GRAPH_EDGE_MODE_BEZIER;
    edge_style.tessellation_segment_count = 22;
    rc = dvz_graph_set_edge_style(graph, &edge_style);
    if (rc != 0)
        return false;

    const uint32_t bridge_count = EDGE_COUNT - bridge_first_edge;
    if (bridge_count > 0)
    {
        dvec3 control0[EDGE_COUNT] = {0};
        dvec3 control1[EDGE_COUNT] = {0};
        _make_bridge_controls(positions, edges, bridge_first_edge, bridge_count, control0, control1);
        rc = dvz_graph_set_edge_controls(
            graph, bridge_first_edge, bridge_count, (const dvec3*)control0, (const dvec3*)control1);
        if (rc != 0)
            return false;
    }

    DvzComposite* composite = dvz_graph_composite(graph, 0);
    if (composite == NULL)
        return false;
    rc = dvz_panel_add_composite(
        panel, composite, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
                                                .coord_space = DVZ_VISUAL_COORD_DATA,
                                                .z_layer = 0});
    return rc == 0;
}



static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(ctx->figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    EXAMPLE_CHECK(_configure_panel(panel), "panel configuration failed");
    EXAMPLE_CHECK(_add_graph(ctx->scene, panel), "graph setup failed");

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, &panzoom_desc, DVZ_DIM_MASK_XY);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    (void)panzoom;

    ok = true;
cleanup:
    return ok;
}



static DvzScenarioSpec _graph_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "composite_graph",
        .title = "Graph Composite",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _graph_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
