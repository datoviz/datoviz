/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* graph - deterministic brain-connectivity graph composite with community labels.
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

#include <stdint.h>
#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum BrainLabelSide
{
    BRAIN_LABEL_SIDE_TOP,
    BRAIN_LABEL_SIDE_BOTTOM,
} BrainLabelSide;


typedef struct BrainCommunity
{
    const char* label;
    DvzColor color;
    BrainLabelSide label_side;
    double label_gap_ratio;
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
    {.label = "Visual",
     .color = {65, 201, 226, 255},
     .label_side = BRAIN_LABEL_SIDE_TOP,
     .label_gap_ratio = 0.20},
    {.label = "Motor",
     .color = {246, 185, 72, 255},
     .label_side = BRAIN_LABEL_SIDE_TOP,
     .label_gap_ratio = 0.20},
    {.label = "Memory",
     .color = {234, 104, 91, 255},
     .label_side = BRAIN_LABEL_SIDE_BOTTOM,
     .label_gap_ratio = 1.20},
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


enum
{
    COMMUNITY_COUNT = sizeof(COMMUNITIES) / sizeof(COMMUNITIES[0]),
    NODE_COUNT = sizeof(NODES) / sizeof(NODES[0]),
    EDGE_COUNT = sizeof(EDGES) / sizeof(EDGES[0]),
};



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

    DvzPanelDomainFit fit = dvz_panel_domain_fit();
    fit.aspect = DVZ_PANEL_DOMAIN_ASPECT_EQUAL;
    fit.x = (DvzDataDomain){.min = -1.34, .max = +1.34};
    fit.y = (DvzDataDomain){.min = -1.18, .max = +0.92};
    fit.padding = 0.03;
    return dvz_panel_set_domain_fit(panel, &fit) == 0;
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



static void _community_label_placement(uint32_t community, double position[3])
{
    ASSERT(community < COMMUNITY_COUNT);
    ANN(position);

    bool found = false;
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
    for (uint32_t i = 0; i < NODE_COUNT; i++)
    {
        if (NODES[i].community != community)
            continue;
        const double x = NODES[i].position[0];
        const double y = NODES[i].position[1];
        if (!found)
        {
            min_x = max_x = x;
            min_y = max_y = y;
            found = true;
        }
        else
        {
            if (x < min_x)
                min_x = x;
            if (x > max_x)
                max_x = x;
            if (y < min_y)
                min_y = y;
            if (y > max_y)
                max_y = y;
        }
    }
    ASSERT(found);

    const BrainCommunity* community_desc = &COMMUNITIES[community];
    const double height = max_y - min_y;
    const double gap = height * community_desc->label_gap_ratio;
    position[0] = 0.5 * (min_x + max_x);
    position[1] =
        community_desc->label_side == BRAIN_LABEL_SIDE_BOTTOM ? min_y - gap : max_y + gap;
    position[2] = 0.0;
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
        const uint32_t source = endpoints[2 * edge_index + 0];
        const uint32_t target = endpoints[2 * edge_index + 1];
        const double sx = positions[source][0];
        const double sy = positions[source][1];
        const double tx = positions[target][0];
        const double ty = positions[target][1];
        const double dx = tx - sx;
        const double dy = ty - sy;
        const double bend = (i % 2 == 0 ? 1.0 : -1.0) * (0.14 + 0.08 * EDGES[edge_index].weight);

        control0[i][0] = sx + 0.36 * dx - bend * dy;
        control0[i][1] = sy + 0.36 * dy + bend * dx;
        control0[i][2] = 0.0;
        control1[i][0] = sx + 0.64 * dx - bend * dy;
        control1[i][1] = sy + 0.64 * dy + bend * dx;
        control1[i][2] = 0.0;
    }
}



/**
 * Add fixed labels describing the graph dataset and communities.
 *
 * @param panel panel receiving labels
 * @return whether all labels were added
 */
static bool _add_graph_labels(DvzPanel* panel)
{
    ANN(panel);

    DvzLabelDesc title = dvz_label_desc();
    title.text = "Brain connectivity: 15 regions / 26 weighted links";
    title.style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_TITLE);
    title.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    title.style.size_px = 27.0f;
    title.placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    title.placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    title.placement.position[0] = 28.0f;
    title.placement.position[1] = 24.0f;
    title.placement.text_anchor[0] = 0.0f;
    title.placement.text_anchor[1] = 0.0f;
    title.placement.has_text_anchor = true;
    if (dvz_annotation_label(panel, &title) == NULL)
        return false;

    for (uint32_t i = 0; i < COMMUNITY_COUNT; i++)
    {
        DvzLabelDesc desc = dvz_label_desc();
        desc.text = COMMUNITIES[i].label;
        desc.style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_DATA_LABEL);
        desc.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
        desc.style.size_px = 26.0f;
        desc.style.color[0] = COMMUNITIES[i].color.r;
        desc.style.color[1] = COMMUNITIES[i].color.g;
        desc.style.color[2] = COMMUNITIES[i].color.b;
        desc.style.color[3] = 245u;
        desc.placement.mode = DVZ_TEXT_PLACEMENT_DATA;
        desc.placement.anchor = DVZ_SCENE_ANCHOR_DATA;
        _community_label_placement(i, desc.placement.position);
        desc.placement.text_anchor[0] = 0.5f;
        desc.placement.text_anchor[1] = 0.5f;
        desc.placement.has_text_anchor = true;
        if (dvz_annotation_label(panel, &desc) == NULL)
            return false;
    }
    return true;
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
        node_ids[i] = NODES[i].semantic_id;
    for (uint32_t i = 0; i < EDGE_COUNT; i++)
        edge_ids[i] = EDGES[i].semantic_id;
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
        const uint32_t community = NODES[i].community;
        ASSERT(community < COMMUNITY_COUNT);
        node_colors[i] = COMMUNITIES[community].color;
        node_sizes[i] = 18.0f + 24.0f * NODES[i].strength;
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
        const bool bridge = EDGES[i].bridge;
        const uint32_t community = NODES[EDGES[i].source].community;
        edge_colors[i] = bridge ? (DvzColor){222, 236, 244, 180} : COMMUNITIES[community].color;
        edge_colors[i].a = bridge ? 185u : 105u;
        edge_widths[i] = 1.1f + 4.2f * EDGES[i].weight;
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

    const uint32_t bridge_count = EDGE_COUNT - bridge_first_edge;
    if (bridge_count > 0)
    {
        dvec3 control0[EDGE_COUNT] = {0};
        dvec3 control1[EDGE_COUNT] = {0};
        _make_bridge_controls(positions, edges, bridge_first_edge, bridge_count, control0, control1);
        rc = dvz_graph_edge_controls(
            graph, bridge_first_edge, bridge_count, (const dvec3*)control0, (const dvec3*)control1);
        if (rc != 0)
            return false;
    }

    DvzComposite* composite = dvz_graph_composite(graph, 0);
    if (composite == NULL)
        return false;
    rc = dvz_panel_add_composite(
        panel, composite, &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
                                                .z_layer = 0});
    if (rc != 0)
        return false;
    return _add_graph_labels(panel);
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

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
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
        .title = "composite_graph",
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
