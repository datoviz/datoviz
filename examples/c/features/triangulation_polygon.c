/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* triangulation_polygon - polygon triangulation with derived edge overlay.
 *
 * Scenario: feature_triangulation_polygon
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/triangulation_polygon
 * Run:    ./build/examples/c/features/triangulation_polygon --live
 * Smoke:  ./build/examples/c/features/triangulation_polygon --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_triangulation_polygon_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add the triangulated polygon fill as a primitive triangle list.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry triangulated polygon geometry
 * @return true when the visual was added
 */
static bool _add_fill(DvzScene* scene, DvzPanel* panel, const DvzGeometry* geometry)
{
    ANN(scene);
    ANN(panel);
    ANN(geometry);

    if (geometry->index_count == 0 || geometry->indices == NULL || geometry->positions == NULL)
        return false;

    vec3* positions = (vec3*)dvz_calloc(geometry->index_count, sizeof(vec3));
    vec3* normals = (vec3*)dvz_calloc(geometry->index_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(geometry->index_count, sizeof(DvzColor));
    if (positions == NULL || normals == NULL || colors == NULL)
        goto error;

    for (uint32_t i = 0; i < geometry->index_count; i++)
    {
        const DvzIndex src = geometry->indices[i];
        if (src >= geometry->vertex_count)
            goto error;
        positions[i][0] = (float)geometry->positions[src][0];
        positions[i][1] = (float)geometry->positions[src][1];
        positions[i][2] = 0.0f;
        normals[i][2] = 1.0f;
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        colors[i].a = 118u;
    }

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        goto error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = geometry->index_count},
        {.attr_name = "color", .data = colors, .item_count = geometry->index_count},
        {.attr_name = "normal", .data = normals, .item_count = geometry->index_count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        goto error;

    dvz_free(positions);
    dvz_free(normals);
    dvz_free(colors);
    return true;

error:
    dvz_free(positions);
    dvz_free(normals);
    dvz_free(colors);
    return false;
}



/**
 * Add unique triangulation edges as segment overlays.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry triangulated polygon geometry
 * @param edges derived edge list
 * @return true when the visual was added
 */
static bool
_add_edges(DvzScene* scene, DvzPanel* panel, const DvzGeometry* geometry, const DvzGeometryEdges* edges)
{
    ANN(scene);
    ANN(panel);
    ANN(geometry);
    ANN(edges);

    if (edges->edge_count == 0 || edges->edges == NULL || geometry->positions == NULL)
        return false;

    vec3* starts = (vec3*)dvz_calloc(edges->edge_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(edges->edge_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(edges->edge_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(edges->edge_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto error;

    for (uint32_t i = 0; i < edges->edge_count; i++)
    {
        const DvzGeometryEdge* edge = &edges->edges[i];
        if (edge->v0 >= geometry->vertex_count || edge->v1 >= geometry->vertex_count)
            goto error;
        starts[i][0] = (float)geometry->positions[edge->v0][0];
        starts[i][1] = (float)geometry->positions[edge->v0][1];
        starts[i][2] = 0.02f;
        ends[i][0] = (float)geometry->positions[edge->v1][0];
        ends[i][1] = (float)geometry->positions[edge->v1][1];
        ends[i][2] = 0.02f;
        const bool boundary = (edge->flags & DVZ_GEOMETRY_EDGE_BOUNDARY) != 0;
        colors[i] = boundary ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT)
                             : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
        widths[i] = boundary ? 4.0f : 2.0f;
    }

    DvzVisual* segment = dvz_segment(scene, 0);
    if (segment == NULL)
        goto error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = edges->edge_count},
        {.attr_name = "position_end", .data = ends, .item_count = edges->edge_count},
        {.attr_name = "color", .data = colors, .item_count = edges->edge_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = edges->edge_count},
    };
    if (dvz_visual_set_data_many(segment, updates, 4) != 0)
        goto error;
    if (dvz_segment_set_caps(segment, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, segment, NULL) != 0)
        goto error;

    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return true;

error:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return false;
}



/**
 * Add input polygon rings as plain boundary segments.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param outer outer polygon ring
 * @param holes optional hole rings
 * @param hole_count number of hole rings
 * @return true when the visual was added
 */
static bool _add_input_rings(
    DvzScene* scene, DvzPanel* panel, const DvzPolygonRing* outer, const DvzPolygonRing* holes,
    uint32_t hole_count)
{
    ANN(scene);
    ANN(panel);
    ANN(outer);
    if (outer->xy == NULL || outer->count < 3)
        return false;

    uint32_t line_count = outer->count;
    for (uint32_t h = 0; h < hole_count; h++)
        line_count += holes[h].count;
    if (line_count == 0)
        return false;

    vec3* starts = (vec3*)dvz_calloc(line_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(line_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(line_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(line_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto error;

    uint32_t line = 0;
    for (uint32_t r = 0; r < hole_count + 1u; r++)
    {
        const DvzPolygonRing* ring = r == 0 ? outer : &holes[r - 1u];
        ANN(ring);
        for (uint32_t i = 0; i < ring->count; i++)
        {
            const uint32_t j = (i + 1u) % ring->count;
            starts[line][0] = (float)ring->xy[i][0];
            starts[line][1] = (float)ring->xy[i][1];
            starts[line][2] = 0.02f;
            ends[line][0] = (float)ring->xy[j][0];
            ends[line][1] = (float)ring->xy[j][1];
            ends[line][2] = 0.02f;
            colors[line] = r == 0 ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT)
                                  : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
            widths[line] = r == 0 ? 4.0f : 3.0f;
            line++;
        }
    }

    DvzVisual* segment = dvz_segment(scene, 0);
    if (segment == NULL)
        goto error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = line_count},
        {.attr_name = "position_end", .data = ends, .item_count = line_count},
        {.attr_name = "color", .data = colors, .item_count = line_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = line_count},
    };
    if (dvz_visual_set_data_many(segment, updates, 4) != 0)
        goto error;
    if (dvz_segment_set_caps(segment, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, segment, NULL) != 0)
        goto error;

    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return true;

error:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return false;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the polygon triangulation feature scenario.
 *
 * @param ctx scenario context
 * @param out_user unused scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){.left_px = 58.0f, .right_px = 58.0f, .top_px = 54.0f,
                                     .bottom_px = 54.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 42.0f, 0.0f))
        return false;

    DvzPanel* input_panel = dvz_grid_panel(grid, 0, 0);
    DvzPanel* result_panel = dvz_grid_panel(grid, 0, 1);
    if (input_panel == NULL || result_panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(input_panel);
    example_graphite_cyan_set_panel_background(result_panel);

    const dvec2 outer[] = {
        {-0.88, -0.34}, {-0.70, +0.22}, {-0.42, +0.64}, {+0.02, +0.78},
        {+0.48, +0.60}, {+0.84, +0.20}, {+0.74, -0.34}, {+0.38, -0.70},
        {-0.04, -0.54}, {-0.42, -0.80},
    };
    const dvec2 hole_a[] = {
        {-0.42, -0.22},
        {-0.14, -0.18},
        {-0.18, +0.12},
        {-0.50, +0.10},
    };
    const dvec2 hole_b[] = {
        {+0.18, -0.06},
        {+0.46, -0.02},
        {+0.40, +0.28},
        {+0.12, +0.20},
    };
    const DvzPolygonRing holes[] = {
        {.xy = hole_a, .count = DVZ_ARRAY_COUNT(hole_a)},
        {.xy = hole_b, .count = DVZ_ARRAY_COUNT(hole_b)},
    };
    const DvzPolygonRing outer_ring = {.xy = outer, .count = DVZ_ARRAY_COUNT(outer)};
    DvzGeometry* geometry = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = outer_ring,
            .holes = holes,
            .hole_count = DVZ_ARRAY_COUNT(holes),
        },
        NULL);
    if (geometry == NULL)
        return false;
    DvzGeometryEdges* edges = dvz_geometry_edges(geometry);
    if (edges == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }

    const bool ok = _add_input_rings(
                        ctx->scene, input_panel, &outer_ring, holes, DVZ_ARRAY_COUNT(holes)) &&
                    _add_fill(ctx->scene, result_panel, geometry) &&
                    _add_edges(ctx->scene, result_panel, geometry, edges);
    dvz_geometry_edges_destroy(edges);
    dvz_geometry_destroy(geometry);
    return ok;
}



/**
 * Return the polygon triangulation scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_triangulation_polygon_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_triangulation_polygon",
        .title = "triangulation_polygon",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the polygon triangulation feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_triangulation_polygon_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
