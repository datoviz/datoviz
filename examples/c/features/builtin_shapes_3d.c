/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* builtin_shapes_3d - builtin 3D geometry builders rendered through retained meshes.
 *
 * Scenario: feature_builtin_shapes_3d
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/builtin_shapes_3d
 * Run:    ./build/examples/c/features/builtin_shapes_3d --live
 * Smoke:  ./build/examples/c/features/builtin_shapes_3d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

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
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.phong.ambient = 0.34f;
    material.phong.diffuse = 0.76f;
    material.phong.specular = 0.20f;
    material.phong.shininess = 28.0f;
    const bool ok = dvz_visual_set_material(mesh, &material) == 0 &&
                    dvz_mesh_set_geometry(mesh, geometry) == 0 &&
                    dvz_panel_add_visual(panel, mesh, NULL) == 0;
    dvz_geometry_destroy(geometry);
    return ok;
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

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.35f;
    camera.eye[1] = -4.10f;
    camera.eye[2] = 1.70f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.68f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        return false;

    const bool ok =
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_cube(&(DvzGeometryCubeDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
                .center = {-1.25, +0.05, 0.00},
                .size = 0.42,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_sphere(&(DvzGeometrySphereDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
                .center = {-0.55, +0.05, 0.02},
                .radius = 0.27,
                .rings = 18,
                .sectors = 36,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_cylinder(&(DvzGeometryCylinderDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryCylinderDesc),
                .center = {+0.18, +0.05, 0.00},
                .radius = 0.18,
                .height = 0.62,
                .sectors = 32,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_cone(&(DvzGeometryConeDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryConeDesc),
                .center = {+0.86, +0.05, 0.00},
                .radius = 0.25,
                .height = 0.66,
                .sectors = 32,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_torus(&(DvzGeometryTorusDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryTorusDesc),
                .center = {-0.34, -0.70, 0.05},
                .major_radius = 0.28,
                .minor_radius = 0.08,
                .rings = 36,
                .sectors = 16,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
            })) &&
        _add_geometry(
            ctx->scene, panel,
            dvz_geom_arrow(&(DvzGeometryArrowDesc){
                DVZ_STRUCT_INIT_FIELDS(DvzGeometryArrowDesc),
                .center = {+0.56, -0.70, 0.02},
                .length = 0.78,
                .shaft_radius = 0.055,
                .head_radius = 0.15,
                .head_length = 0.25,
                .sectors = 32,
                .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
            }));

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.62f, -0.08f, +0.26f});
    return ok;
}



/**
 * Return the builtin 3D shapes scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _builtin_shapes_3d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_builtin_shapes_3d",
        .title = "builtin_shapes_3d",
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
 * Run the builtin 3D shapes feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _builtin_shapes_3d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
