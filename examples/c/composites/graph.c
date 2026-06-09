/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* graph - semantic two-community graph composite with bridge edges.
 *
 * Scenario: composite_graph
 * Style: feature composite, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c composites/graph
 * Run:    ./build/examples/c/composites/graph --live
 * Smoke:  ./build/examples/c/composites/graph --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u
#define COMMUNITY_COUNT 2u
#define COMMUNITY_SIZE  11u
#define RING_COUNT      (COMMUNITY_SIZE - 1u)
#define BRIDGE_COUNT    2u
#define NODE_COUNT      (COMMUNITY_COUNT * COMMUNITY_SIZE + BRIDGE_COUNT)
#define INTRA_EDGE_COUNT  (COMMUNITY_COUNT * RING_COUNT * 2u)
#define BRIDGE_EDGE_COUNT 8u
#define EDGE_COUNT        (INTRA_EDGE_COUNT + BRIDGE_EDGE_COUNT)
#define PI 3.14159265358979323846



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Configure the panel used by the graph composite example.
 *
 * @param panel target panel
 * @return whether setup succeeded
 */
static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);
    example_graphite_cyan_set_panel_background(panel);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.06f, .right = 0.06f, .bottom = 0.06f,
                                        .top = 0.06f});
    if (!ok)
        return false;
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.60, 1.60);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.10, 1.10);
    return rc == 0;
}



/**
 * Fill a deterministic clustered graph layout.
 *
 * @param positions output node positions
 */
static void _make_positions(dvec3 positions[NODE_COUNT])
{
    static const dvec2 centers[COMMUNITY_COUNT] = {
        {-0.70, +0.00},
        {+0.70, +0.00},
    };

    for (uint32_t c = 0; c < COMMUNITY_COUNT; c++)
    {
        const uint32_t base = c * COMMUNITY_SIZE;
        positions[base][0] = centers[c][0];
        positions[base][1] = centers[c][1];
        positions[base][2] = 0.0;
        for (uint32_t i = 0; i < RING_COUNT; i++)
        {
            const double angle = 2.0 * PI * (double)i / (double)RING_COUNT + 0.18 * (double)c;
            const double radius = 0.38;
            positions[base + 1 + i][0] = centers[c][0] + radius * cos(angle);
            positions[base + 1 + i][1] = centers[c][1] + radius * sin(angle);
            positions[base + 1 + i][2] = 0.0;
        }
    }

    positions[22][0] = -0.08;
    positions[22][1] = +0.36;
    positions[23][0] = +0.08;
    positions[23][1] = -0.36;
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
    for (uint32_t c = 0; c < COMMUNITY_COUNT; c++)
    {
        const uint32_t base = c * COMMUNITY_SIZE;
        for (uint32_t i = 0; i < RING_COUNT; i++)
        {
            _push_edge(endpoints, &edge_count, base, base + 1 + i);
            _push_edge(endpoints, &edge_count, base + 1 + i, base + 1 + ((i + 1) % RING_COUNT));
        }
    }

    *bridge_first_edge = edge_count;

    static const uint32_t bridge_edges[2 * BRIDGE_EDGE_COUNT] = {
        0, 22, 11, 23, 22, 23, 22, 11, //
        23, 0,  1,  12, 6,  17, 22, 17,
    };
    for (uint32_t i = 0; i < BRIDGE_EDGE_COUNT; i++)
        _push_edge(endpoints, &edge_count, bridge_edges[2 * i], bridge_edges[2 * i + 1]);

    ASSERT(edge_count == EDGE_COUNT);
}



static void _make_bridge_controls(
    dvec3 positions[NODE_COUNT], const uint32_t endpoints[2 * EDGE_COUNT],
    uint32_t bridge_first_edge, dvec3 control0[BRIDGE_EDGE_COUNT], dvec3 control1[BRIDGE_EDGE_COUNT])
{
    ANN(positions);
    ANN(endpoints);
    ANN(control0);
    ANN(control1);

    for (uint32_t i = 0; i < BRIDGE_EDGE_COUNT; i++)
    {
        const uint32_t edge_index = bridge_first_edge + i;
        const uint32_t source = endpoints[2 * edge_index + 0];
        const uint32_t target = endpoints[2 * edge_index + 1];
        const double sx = positions[source][0];
        const double sy = positions[source][1];
        const double tx = positions[target][0];
        const double ty = positions[target][1];
        const double dx = tx - sx;
        const double dy = ty - sy;
        const double bend = (i % 2 == 0 ? 1.0 : -1.0) * 0.18;

        control0[i][0] = sx + 0.36 * dx - bend * dy;
        control0[i][1] = sy + 0.36 * dy + bend * dx;
        control0[i][2] = 0.0;
        control1[i][0] = sx + 0.64 * dx - bend * dy;
        control1[i][1] = sy + 0.64 * dy + bend * dx;
        control1[i][2] = 0.0;
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
    int rc = dvz_graph_node_count(graph, NODE_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_graph_node_positions(graph, 0, NODE_COUNT, (const dvec3*)positions);
    if (rc != 0)
        return false;
    rc = dvz_graph_edge_count(graph, EDGE_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_graph_edges(graph, 0, EDGE_COUNT, edges);
    if (rc != 0)
        return false;
    uint64_t node_ids[NODE_COUNT] = {0};
    uint64_t edge_ids[EDGE_COUNT] = {0};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
        node_ids[i] = 1000 + i;
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
        edge_ids[i] = 2000 + i;
    rc = dvz_graph_node_ids(graph, 0, NODE_COUNT, node_ids);
    if (rc != 0)
        return false;
    rc = dvz_graph_edge_ids(graph, 0, EDGE_COUNT, edge_ids);
    if (rc != 0)
        return false;

    DvzColor node_colors[NODE_COUNT] = {0};
    float node_sizes[NODE_COUNT] = {0};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        const bool hub = i == 0 || i == COMMUNITY_SIZE;
        const bool bridge = i >= COMMUNITY_COUNT * COMMUNITY_SIZE;
        node_colors[i] = bridge ? (DvzColor){231, 98, 82, 255} :
                         hub    ? (DvzColor){255, 198, 80, 255} :
                                  (DvzColor){46, 190, 210, 255};
        node_sizes[i] = bridge ? 34.0f : hub ? 38.0f : 22.0f;
    }
    rc = dvz_graph_node_colors(graph, 0, NODE_COUNT, node_colors);
    if (rc != 0)
        return false;
    rc = dvz_graph_node_sizes(graph, 0, NODE_COUNT, node_sizes);
    if (rc != 0)
        return false;

    DvzColor edge_colors[EDGE_COUNT] = {0};
    float edge_widths[EDGE_COUNT] = {0};
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
    {
        const bool bridge = i >= bridge_first_edge;
        edge_colors[i] =
            bridge ? (DvzColor){231, 98, 82, 215} : (DvzColor){150, 220, 235, 115};
        edge_widths[i] = bridge ? 3.6f : 1.7f;
    }
    rc = dvz_graph_edge_colors(graph, 0, EDGE_COUNT, edge_colors);
    if (rc != 0)
        return false;
    rc = dvz_graph_edge_widths(graph, 0, EDGE_COUNT, edge_widths);
    if (rc != 0)
        return false;

    DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
    edge_style.mode = DVZ_GRAPH_EDGE_MODE_BEZIER;
    edge_style.tessellation.segment_count = 22;
    rc = dvz_graph_set_edge_style(graph, &edge_style);
    if (rc != 0)
        return false;

    dvec3 control0[BRIDGE_EDGE_COUNT] = {0};
    dvec3 control1[BRIDGE_EDGE_COUNT] = {0};
    _make_bridge_controls(positions, edges, bridge_first_edge, control0, control1);
    rc = dvz_graph_edge_controls(
        graph, bridge_first_edge, BRIDGE_EDGE_COUNT, (const dvec3*)control0,
        (const dvec3*)control1);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_graph_composite(graph, 0);
    if (composite == NULL)
        return false;
    rc = dvz_panel_add_composite(
        panel, composite, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
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

    DvzPanel* panel = dvz_panel(ctx->figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    EXAMPLE_CHECK(_configure_panel(panel), "panel configuration failed");
    EXAMPLE_CHECK(_add_graph(ctx->scene, panel), "graph setup failed");

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
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
        .title = "composite_graph",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
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
