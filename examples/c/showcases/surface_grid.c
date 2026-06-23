/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* surface_grid - generated height-field mesh with a derived wireframe overlay.
 *
 * Scenario: showcase_surface_grid
 * Style: showcase, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c showcases/surface_grid
 * Run:    ./build/examples/c/showcases/surface_grid --live
 * Smoke:  ./build/examples/c/showcases/surface_grid --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_surface_grid_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600u
#define HEIGHT       1200u
#define SURFACE_ROWS 80u
#define SURFACE_COLS 96u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SurfaceGridState
{
    DvzGeometry* geometry;
    DvzGeometryEdges* edges;
} SurfaceGridState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic height field and color ramp.
 *
 * @param heights output heights
 * @param colors output colors
 */
static void _surface_data(double* heights, DvzColor* colors)
{
    ANN(heights);
    ANN(colors);

    for (uint32_t row = 0; row < SURFACE_ROWS; row++)
    {
        const double y = -1.0 + 2.0 * (double)row / (double)(SURFACE_ROWS - 1u);
        for (uint32_t col = 0; col < SURFACE_COLS; col++)
        {
            const double x = -1.0 + 2.0 * (double)col / (double)(SURFACE_COLS - 1u);
            const double r2 = x * x + y * y;
            const double ridge = 0.34 * cos(15.0 * sqrt(r2)) * exp(-1.25 * r2);
            const double saddle = 0.13 * (x * x - 0.70 * y * y) * exp(-0.85 * r2);
            const double ripple = 0.050 * sin(15.5 * x + 2.0 * y) * cos(10.0 * y - 2.5 * x);
            const double z = ridge + saddle + ripple;
            const uint32_t idx = row * SURFACE_COLS + col;
            heights[idx] = z;

            const double t = fmin(1.0, fmax(0.0, (z + 0.26) / 0.58));
            const DvzColor lo = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
            const DvzColor hi = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
            colors[idx] = dvz_color_rgba(
                (uint8_t)((1.0 - t) * lo.r + t * hi.r), (uint8_t)((1.0 - t) * lo.g + t * hi.g),
                (uint8_t)((1.0 - t) * lo.b + t * hi.b), 235);
        }
    }
}



/**
 * Add the lit surface mesh.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @param geometry surface-grid geometry
 * @return true on success
 */
static bool _add_surface(DvzScene* scene, DvzPanel* panel, const DvzGeometry* geometry)
{
    ANN(scene);
    ANN(panel);
    ANN(geometry);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
        return false;
    if (!example_apply_default_phong_material(mesh))
        return false;
    if (dvz_mesh_set_geometry(mesh, geometry) != 0)
        return false;
    return dvz_panel_add_visual(panel, mesh, NULL) == 0;
}



/**
 * Add a segment wireframe derived from unique geometry edges.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @param geometry source geometry
 * @param edges derived edge list
 * @return true on success
 */
static bool _add_wireframe(
    DvzScene* scene, DvzPanel* panel, const DvzGeometry* geometry, const DvzGeometryEdges* edges)
{
    ANN(scene);
    ANN(panel);
    ANN(geometry);
    ANN(edges);

    if (edges->edge_count == 0)
        return false;

    vec3* starts = (vec3*)dvz_calloc(edges->edge_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(edges->edge_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(edges->edge_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(edges->edge_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto error;

    const DvzColor wire_color = dvz_color_rgba(14, 34, 38, 92);
    for (uint32_t i = 0; i < edges->edge_count; i++)
    {
        const DvzGeometryEdge* edge = &edges->edges[i];
        const double* p0 = geometry->positions[edge->v0];
        const double* p1 = geometry->positions[edge->v1];
        starts[i][0] = (float)p0[0];
        starts[i][1] = (float)(p0[1] + 0.003);
        starts[i][2] = (float)p0[2];
        ends[i][0] = (float)p1[0];
        ends[i][1] = (float)(p1[1] + 0.003);
        ends[i][2] = (float)p1[2];
        colors[i] = wire_color;
        widths[i] = 1.10f;
    }

    DvzVisual* wire = dvz_segment(scene, 0);
    if (wire == NULL)
        goto error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = edges->edge_count},
        {.attr_name = "position_end", .data = ends, .item_count = edges->edge_count},
        {.attr_name = "color", .data = colors, .item_count = edges->edge_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = edges->edge_count},
    };
    if (dvz_visual_set_data_many(wire, updates, 4) != 0)
        goto error;
    if (dvz_segment_set_caps(wire, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, wire, NULL) != 0)
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
 * Initialize the surface-grid showcase.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;

    SurfaceGridState* state = (SurfaceGridState*)calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    const uint32_t vertex_count = SURFACE_ROWS * SURFACE_COLS;
    double* heights = (double*)dvz_calloc(vertex_count, sizeof(double));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    if (heights == NULL || colors == NULL)
        goto error;
    _surface_data(heights, colors);

    DvzGeometrySurfaceGridDesc desc = dvz_geometry_surface_grid_desc();
    desc.rows = SURFACE_ROWS;
    desc.cols = SURFACE_COLS;
    desc.heights = heights;
    desc.colors = colors;
    desc.origin[0] = -2.7;
    desc.origin[1] = 0.0;
    desc.origin[2] = +2.00;
    desc.col_basis[0] = 2 * 2.70 / (double)(SURFACE_COLS - 1u);
    desc.col_basis[1] = 0.0;
    desc.col_basis[2] = 0.0;
    desc.row_basis[0] = 0.0;
    desc.row_basis[1] = 0.0;
    desc.row_basis[2] = -2 * 2.00 / (double)(SURFACE_ROWS - 1u);
    desc.height_axis[0] = 0.0;
    desc.height_axis[1] = 1.0;
    desc.height_axis[2] = 0.0;
    desc.height_scale = 1.18;
    state->geometry = dvz_geom_surface_grid(&desc);
    dvz_free(heights);
    dvz_free(colors);
    heights = NULL;
    colors = NULL;
    if (state->geometry == NULL)
        goto error;

    state->edges = dvz_geometry_edges(state->geometry);
    if (state->edges == NULL)
        goto error;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (example_set_default_3d_camera(panel, 1.80f) == NULL)
        goto error;

    if (!_add_surface(ctx->scene, panel, state->geometry))
        goto error;
    if (!_add_wireframe(ctx->scene, panel, state->geometry, state->edges))
        goto error;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        goto error;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        goto error;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        goto error;
    example_set_default_arcball(arcball);
    return true;

error:
    dvz_free(heights);
    dvz_free(colors);
    return false;
}



/**
 * Destroy the surface-grid showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    SurfaceGridState* state = (SurfaceGridState*)user;
    if (state == NULL)
        return;
    if (state->edges != NULL)
        dvz_geometry_edges_destroy(state->edges);
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    free(state);
}



/**
 * Return the surface-grid showcase scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_surface_grid_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcase_surface_grid",
        .title = "surface_grid",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements =
            DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the surface-grid showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_surface_grid_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
