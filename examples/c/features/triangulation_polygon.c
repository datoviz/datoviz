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
        colors[i] = i / 3u % 2u == 0
                        ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)
                        : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
        colors[i].a = 170u;
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

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    const dvec2 outer[] = {
        {-0.82, -0.58}, {-0.62, +0.54}, {-0.08, +0.78}, {+0.72, +0.46},
        {+0.88, -0.22}, {+0.28, -0.74}, {-0.42, -0.70},
    };
    const dvec2 hole[] = {
        {-0.20, -0.16},
        {+0.28, -0.10},
        {+0.20, +0.28},
        {-0.30, +0.22},
    };
    const DvzPolygonRing holes[] = {{.xy = hole, .count = DVZ_ARRAY_COUNT(hole)}};
    DvzGeometry* geometry = dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = outer, .count = DVZ_ARRAY_COUNT(outer)},
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

    const bool ok = _add_fill(ctx->scene, panel, geometry) &&
                    _add_edges(ctx->scene, panel, geometry, edges);
    dvz_geometry_edges_destroy(edges);
    dvz_geometry_destroy(geometry);
    return ok;
}



/**
 * Return the polygon triangulation scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _triangulation_polygon_scenario(void)
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
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _triangulation_polygon_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
