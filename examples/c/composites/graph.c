/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* graph - semantic graph composite with curved edges.
 *
 * Scenario: composite_graph_static
 * Style: composites, graphite_cyan, 1280x960 capture target
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

#define WIDTH  1280u
#define HEIGHT  960u
#define NODE_COUNT 10u
#define EDGE_COUNT 17u
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
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.55, 1.55);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.25, 1.25);
    return rc == 0;
}



/**
 * Fill a small static graph layout.
 *
 * @param positions output node positions
 */
static void _make_positions(dvec3 positions[NODE_COUNT])
{
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        const double angle = 2.0 * PI * (double)i / (double)NODE_COUNT + 0.22;
        const double radius = i % 2 == 0 ? 0.98 : 0.68;
        positions[i][0] = radius * cos(angle);
        positions[i][1] = radius * sin(angle);
        positions[i][2] = 0.0;
    }
    positions[0][0] -= 0.08;
    positions[3][1] += 0.10;
    positions[6][0] += 0.12;
    positions[8][1] -= 0.08;
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

    static const uint32_t edges[2 * EDGE_COUNT] = {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9,
        9, 0, 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 0, 3, 5, 8,
    };

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
        const bool hub = i == 0 || i == 5;
        node_colors[i] = hub ? (DvzColor){255, 198, 80, 255} : (DvzColor){46, 190, 210, 255};
        node_sizes[i] = hub ? 34.0f : 23.0f;
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
        const bool chord = i >= 10;
        edge_colors[i] = chord ? (DvzColor){231, 98, 82, 210} : (DvzColor){180, 215, 225, 155};
        edge_widths[i] = chord ? 3.5f : 2.0f;
    }
    rc = dvz_graph_edge_colors(graph, 0, EDGE_COUNT, edge_colors);
    if (rc != 0)
        return false;
    rc = dvz_graph_edge_widths(graph, 0, EDGE_COUNT, edge_widths);
    if (rc != 0)
        return false;

    DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
    edge_style.mode = DVZ_GRAPH_EDGE_MODE_BEZIER;
    edge_style.tessellation.segment_count = 18;
    rc = dvz_graph_set_edge_style(graph, &edge_style);
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

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "composite_graph_static");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    debug = example_debug(win, argc > 0 ? argv[0] : NULL, "composite_graph_static");
    example_debug_panzoom(&debug, "composite_graph_static", panzoom);
    if (example_debug_requested(argc, argv))
    {
        EXAMPLE_CHECK(example_debug_install(&debug, argc, argv), "example_debug_install() failed");
    }

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
