/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* graph - semantic graph composite with clustered curved edges.
 *
 * Scenario: composite_graph
 * Style: composites, graphite_cyan, 1600x1200 capture target
 *
 * Build:  cmake --build build --target graph
 * Run:    ./build/examples/c/composites/graph
 * Smoke:  ./build/examples/c/composites/graph 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_debug.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u
#define CLUSTER_COUNT 3u
#define CLUSTER_SIZE  9u
#define RING_COUNT    (CLUSTER_SIZE - 1u)
#define BRIDGE_COUNT  3u
#define NODE_COUNT    (CLUSTER_COUNT * CLUSTER_SIZE + BRIDGE_COUNT)
#define INTRA_EDGE_COUNT  (CLUSTER_COUNT * RING_COUNT * 2u)
#define BRIDGE_EDGE_COUNT 12u
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
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.75, 1.75);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.35, 1.35);
    return rc == 0;
}



/**
 * Fill a deterministic clustered graph layout.
 *
 * @param positions output node positions
 */
static void _make_positions(dvec3 positions[NODE_COUNT])
{
    static const dvec2 centers[CLUSTER_COUNT] = {
        {-0.86, +0.42},
        {+0.86, +0.38},
        {+0.02, -0.64},
    };

    for (uint32_t c = 0; c < CLUSTER_COUNT; c++)
    {
        const uint32_t base = c * CLUSTER_SIZE;
        positions[base][0] = centers[c][0];
        positions[base][1] = centers[c][1];
        positions[base][2] = 0.0;
        for (uint32_t i = 0; i < RING_COUNT; i++)
        {
            const double angle = 2.0 * PI * (double)i / (double)RING_COUNT + 0.24 * (double)c;
            const double radius = i % 2 == 0 ? 0.36 : 0.25;
            positions[base + 1 + i][0] = centers[c][0] + radius * cos(angle);
            positions[base + 1 + i][1] = centers[c][1] + radius * sin(angle);
            positions[base + 1 + i][2] = 0.0;
        }
    }

    positions[27][0] = -0.31;
    positions[27][1] = +0.11;
    positions[28][0] = +0.31;
    positions[28][1] = +0.10;
    positions[29][0] = +0.00;
    positions[29][1] = -0.18;
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
    for (uint32_t c = 0; c < CLUSTER_COUNT; c++)
    {
        const uint32_t base = c * CLUSTER_SIZE;
        for (uint32_t i = 0; i < RING_COUNT; i++)
        {
            _push_edge(endpoints, &edge_count, base, base + 1 + i);
            _push_edge(endpoints, &edge_count, base + 1 + i, base + 1 + ((i + 1) % RING_COUNT));
        }
    }

    *bridge_first_edge = edge_count;

    static const uint32_t bridge_edges[2 * BRIDGE_EDGE_COUNT] = {
        0,  27, 9,  28, 18, 29, //
        27, 28, 28, 29, 29, 27, //
        27, 9,  28, 18, 29, 0,  //
        0,  9,  9,  18, 18, 0,
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
        const double bend = (i % 2 == 0 ? 1.0 : -1.0) * (i >= 9 ? 0.42 : 0.24);

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
        const bool hub = i == 0 || i == 9 || i == 18;
        const bool bridge = i >= 27;
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



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    ExampleDebug debug = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    EXAMPLE_CHECK(_configure_panel(panel), "panel configuration failed");
    EXAMPLE_CHECK(_add_graph(scene, panel), "graph setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "composite_graph");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    EXAMPLE_CHECK(
        example_debug_setup(&debug, win, argc, argv, "composite_graph"),
        "example_debug_setup() failed");
    example_debug_panzoom(&debug, "composite_graph", panzoom);

    dvz_app_run(app, example_frame_count_any(argc, argv));
    ret = 0;

cleanup:
    example_debug_uninstall(&debug);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
