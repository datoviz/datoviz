/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* builtin_shapes_2d - builtin 2D geometry builders rendered through retained meshes.
 *
 * Scenario: feature_builtin_shapes_2d
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/builtin_shapes_2d
 * Run:    ./build/examples/c/features/builtin_shapes_2d --live
 * Smoke:  ./build/examples/c/features/builtin_shapes_2d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_builtin_shapes_2d_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload one geometry as a retained mesh and immediately release the CPU copy.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry geometry to upload
 * @return true when the mesh was added
 */
static bool _add_geometry(DvzScene* scene, DvzPanel* panel, DvzGeometry* geometry)
{
    ANN(scene);
    ANN(panel);

    if (geometry == NULL)
        return false;
    DvzVisual* mesh = dvz_mesh(scene, 0);
    if (mesh == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    const bool ok = dvz_mesh_set_geometry(mesh, geometry) == 0 &&
                    dvz_panel_add_visual(panel, mesh, NULL) == 0;
    dvz_geometry_destroy(geometry);
    return ok;
}



/**
 * Create a triangulated polygon with a hole.
 *
 * @return geometry, or NULL on failure
 */
static DvzGeometry* _hole_polygon(void)
{
    const dvec2 outer[] = {
        {+0.46, -0.56},
        {+0.88, -0.50},
        {+0.84, -0.12},
        {+0.54, -0.02},
        {+0.36, -0.28},
    };
    const dvec2 hole[] = {
        {+0.58, -0.38},
        {+0.72, -0.36},
        {+0.70, -0.22},
        {+0.56, -0.22},
    };
    const DvzPolygonRing holes[] = {{.xy = hole, .count = DVZ_ARRAY_COUNT(hole)}};
    return dvz_triangulate_polygon(
        &(DvzPolygonDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
            .outer = {.xy = outer, .count = DVZ_ARRAY_COUNT(outer)},
            .holes = holes,
            .hole_count = DVZ_ARRAY_COUNT(holes),
        },
        NULL);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the builtin 2D shapes feature scenario.
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
    if (!example_configure_equal_aspect_panel(
            panel, (DvzDataDomain){.min = -1.05, .max = +1.05},
            (DvzDataDomain){.min = -0.72, .max = +0.72}, 0.04, NULL))
        return false;

    const bool ok =
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_plane(&(DvzGeometryPlaneDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryPlaneDesc),
                .center = {-0.66, +0.40, 0.0},
                .width = 0.46,
                .height = 0.30,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_disc(&(DvzGeometryDiscDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryDiscDesc),
                .center = {0.0, +0.40, 0.01},
                .radius = 0.21,
                .segments = 48,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_sector(&(DvzGeometrySectorDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometrySectorDesc),
                .center = {+0.66, +0.40, 0.02},
                .radius = 0.27,
                .start_angle = -0.35,
                .sweep_angle = 4.4,
                .segments = 36,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_regular_polygon(&(DvzGeometryRegularPolygonDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryRegularPolygonDesc),
                .center = {-0.66, -0.30, 0.03},
                .radius = 0.25,
                .sides = 7,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_star(&(DvzGeometryStarDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryStarDesc),
                .center = {0.0, -0.30, 0.04},
                .outer_radius = 0.28,
                .inner_radius = 0.12,
                .points = 5,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
            })) &&
        _add_geometry(ctx->scene, panel, _hole_polygon());

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzController* controller = dvz_panzoom(ctx->scene, &panzoom_desc);
    if (controller == NULL)
        return false;
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    if (panzoom == NULL)
        return false;
    const float aspect = ctx->height > 0 ? (float)ctx->width / (float)ctx->height : 1.0f;
    dvz_panzoom_zoom(panzoom, (vec2){1.0f, aspect});
    dvz_panzoom_end(panzoom);
    return ok && dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XY) == 0;
}



/**
 * Return the builtin 2D shapes scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_builtin_shapes_2d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_builtin_shapes_2d",
        .title = "builtin_shapes_2d",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the builtin 2D shapes feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_builtin_shapes_2d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
