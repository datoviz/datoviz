/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* builtin_shapes_3d - This example shows built-in 3D geometry rendered as lit meshes.
 *
 * Scenario: features_builtin_shapes_3d
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/builtin_shapes_3d
 * Run:    ./build/examples/c/features/builtin_shapes_3d --live
 * Smoke:  ./build/examples/c/features/builtin_shapes_3d --png
 *
 * What to look for: the scene uses geometry builders for common solids such as a cube, sphere,
 * cylinder, cone, capsule, and arrow-like shapes, then uploads each result to a retained mesh with
 * a Phong material. Some Z-axis builders are transformed so their local axis follows scene +Y.
 * Compare the lighting, normals, and orientation of each shape; these primitives are useful for
 * reference objects, probes, direction markers, and simple 3D scientific diagrams.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/math/vec.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"


DvzScenarioSpec dvz_example_builtin_shapes_3d_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



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
    DvzMaterialDesc material = example_default_phong_material_desc();
    const bool ok = dvz_visual_set_material(mesh, &material) == 0 &&
                    dvz_mesh_set_geometry(mesh, geometry) == 0 &&
                    dvz_panel_add_visual(panel, mesh, NULL) == 0;
    dvz_geometry_destroy(geometry);
    return ok;
}


/**
 * Orient one Z-axis builtin geometry so its local axis follows scene +Y.
 *
 * @param geometry target geometry
 * @param center final scene-space center
 * @return true on success
 */
static bool _orient_z_axis_shape_y_up(DvzGeometry* geometry, const dvec3 center)
{
    if (geometry == NULL)
        return false;

    dmat4 transform = _DMAT4_IDENTITY_INIT;
    transform[1][1] = 0.0;
    transform[1][2] = -1.0;
    transform[2][1] = +1.0;
    transform[2][2] = 0.0;
    transform[3][0] = center[0];
    transform[3][1] = center[1];
    transform[3][2] = center[2];
    return dvz_geometry_transform(geometry, transform) == 0;
}



/**
 * Orient one Z-axis builtin geometry to scene +Y, upload it, and release the CPU copy.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry origin-centered Z-axis geometry to orient and upload
 * @param center final scene-space center
 * @return true when the mesh was added
 */
static bool
_add_y_up_geometry(DvzScene* scene, DvzPanel* panel, DvzGeometry* geometry, const dvec3 center)
{
    if (!_orient_z_axis_shape_y_up(geometry, center))
    {
        if (geometry != NULL)
            dvz_geometry_destroy(geometry);
        return false;
    }
    return _add_geometry(scene, panel, geometry);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the builtin 3D shapes feature scenario.
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

    if (example_set_default_3d_camera(panel, 1.0f) == NULL)
        return false;

    const bool ok =
        _add_geometry(
            ctx->scene, panel,
            dvz_geometry_cube(&(DvzGeometryCubeDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
                .center = {-0.92, 0.00, +0.42},
                .size = 0.58,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geometry_sphere(&(DvzGeometrySphereDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
                .center = {+0.00, 0.02, +0.42},
                .radius = 0.36,
                .rings = 36,
                .sectors = 72,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
            })) &&
        _add_y_up_geometry(
            ctx->scene, panel,
            dvz_geometry_cylinder(&(DvzGeometryCylinderDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryCylinderDesc),
                .radius = 0.24,
                .height = 0.82,
                .sectors = 128,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
            }),
            (dvec3){+0.92, 0.00, +0.42}) &&
        _add_y_up_geometry(
            ctx->scene, panel,
            dvz_geometry_cone(&(DvzGeometryConeDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryConeDesc),
                .radius = 0.34,
                .height = 0.86,
                .sectors = 128,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
            }),
            (dvec3){-0.92, 0.00, -0.42}) &&
        _add_y_up_geometry(
            ctx->scene, panel,
            dvz_geometry_torus(&(DvzGeometryTorusDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryTorusDesc),
                .major_radius = 0.36,
                .minor_radius = 0.105,
                .rings = 72,
                .sectors = 32,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
            }),
            (dvec3){+0.00, 0.05, -0.42}) &&
        _add_y_up_geometry(
            ctx->scene, panel,
            dvz_geometry_arrow(&(DvzGeometryArrowDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryArrowDesc),
                .length = 1.00,
                .shaft_radius = 0.075,
                .head_radius = 0.20,
                .head_length = 0.32,
                .sectors = 128,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
            }),
            (dvec3){+0.92, 0.02, -0.42});

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){0.0f, 0.0f, 0.0f});
    return ok;
}



/**
 * Return the builtin 3D shapes scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_builtin_shapes_3d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_builtin_shapes_3d",
        .title = "Builtin Shapes 3D",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements =
            DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Run the builtin 3D shapes feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_builtin_shapes_3d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
