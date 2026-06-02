/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* graph_edge_modes - compare graph segment, path, and Bezier edge modes.
 *
 * Scenario: composite_graph_edge_modes
 * Style: composites, graphite_cyan, 1600x900 capture target
 *
 * Build:  cmake --build build --target graph_edge_modes
 * Run:    ./build/examples/c/composites/graph_edge_modes
 * Smoke:  ./build/examples/c/composites/graph_edge_modes 1
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
#define HEIGHT 900u
#define NODE_COUNT 8u
#define EDGE_COUNT 12u
#define PI 3.14159265358979323846



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);
    example_graphite_cyan_set_panel_background(panel);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.06f, .right = 0.06f, .bottom = 0.08f,
                                        .top = 0.08f});
    if (!ok)
        return false;
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.25, 1.25);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.10, 1.10);
    return rc == 0;
}



static void _make_positions(dvec3 positions[NODE_COUNT], double x_offset)
{
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        const double angle = 2.0 * PI * (double)i / (double)NODE_COUNT + 0.35;
        const double radius = i % 2 == 0 ? 0.92 : 0.58;
        positions[i][0] = x_offset + radius * cos(angle);
        positions[i][1] = radius * sin(angle);
        positions[i][2] = 0.0;
    }
}



static bool _add_graph(DvzScene* scene, DvzPanel* panel, DvzGraphEdgeMode mode)
{
    ANN(scene);
    ANN(panel);

    dvec3 positions[NODE_COUNT] = {0};
    _make_positions(positions, 0.0);
    static const uint32_t edges[2 * EDGE_COUNT] = {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
        6, 7, 7, 0, 0, 4, 1, 5, 2, 6, 3, 7,
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

    uint64_t ids[NODE_COUNT] = {0};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
        ids[i] = 10 + i;
    rc = dvz_graph_node_ids(graph, 0, NODE_COUNT, ids);
    if (rc != 0)
        return false;

    DvzColor node_colors[NODE_COUNT] = {0};
    float node_sizes[NODE_COUNT] = {0};
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        node_colors[i] = i % 2 == 0 ? (DvzColor){46, 190, 210, 255}
                                    : (DvzColor){255, 198, 80, 255};
        node_sizes[i] = i % 2 == 0 ? 24.0f : 29.0f;
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
        edge_colors[i] = i < 8 ? (DvzColor){180, 215, 225, 155} : (DvzColor){231, 98, 82, 220};
        edge_widths[i] = i < 8 ? 2.0f : 3.5f;
    }
    rc = dvz_graph_edge_colors(graph, 0, EDGE_COUNT, edge_colors);
    if (rc != 0)
        return false;
    rc = dvz_graph_edge_widths(graph, 0, EDGE_COUNT, edge_widths);
    if (rc != 0)
        return false;

    DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
    edge_style.mode = mode;
    edge_style.tessellation.segment_count = 16;
    rc = dvz_graph_set_edge_style(graph, &edge_style);
    if (rc != 0)
        return false;

    DvzComposite* composite = dvz_graph_composite(graph, 0);
    if (composite == NULL)
        return false;
    rc = dvz_panel_add_composite(panel, composite, NULL);
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

    DvzPanel* panels[3] = {
        dvz_panel(figure, (DvzPanelDesc){0.00f, 0.0f, 0.333f, 1.0f}),
        dvz_panel(figure, (DvzPanelDesc){0.333f, 0.0f, 0.334f, 1.0f}),
        dvz_panel(figure, (DvzPanelDesc){0.667f, 0.0f, 0.333f, 1.0f}),
    };
    for (uint32_t i = 0; i < 3; i++)
    {
        EXAMPLE_CHECK(panels[i] != NULL, "dvz_panel() failed");
        EXAMPLE_CHECK(_configure_panel(panels[i]), "panel configuration failed");
    }
    EXAMPLE_CHECK(
        _add_graph(scene, panels[0], DVZ_GRAPH_EDGE_MODE_SEGMENT), "segment graph failed");
    EXAMPLE_CHECK(_add_graph(scene, panels[1], DVZ_GRAPH_EDGE_MODE_PATH), "path graph failed");
    EXAMPLE_CHECK(
        _add_graph(scene, panels[2], DVZ_GRAPH_EDGE_MODE_BEZIER), "Bezier graph failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "composite_graph_edge_modes");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = NULL;
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzPanzoom* pz = dvz_view_panzoom(win, panels[i], NULL);
        EXAMPLE_CHECK(pz != NULL, "failed to create or bind panzoom controller");
        if (panzoom == NULL)
            panzoom = pz;
    }

    debug = example_debug(win, argc > 0 ? argv[0] : NULL, "composite_graph_edge_modes");
    example_debug_panzoom(&debug, "composite_graph_edge_modes", panzoom);
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
