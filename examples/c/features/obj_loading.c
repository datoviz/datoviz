/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* obj_loading - Wavefront OBJ mesh loading through geom/fileio helpers.
 *
 * Scenario: feature_obj_loading
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/obj_loading
 * Run:    ./build/examples/c/features/obj_loading --live
 * Smoke:  ./build/examples/c/features/obj_loading --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_assertions.h"
#include "datoviz/fileio.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"


DvzScenarioSpec dvz_example_obj_loading_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define OBJ_PATH "feature_obj_loading_tmp.obj"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Write a compact low-poly OBJ fixture to the working directory.
 *
 * @return true when the file was written
 */
static bool _write_obj_fixture(void)
{
    static const char OBJ[] =
        "v 0.000 0.720 0.000\n"
        "v 0.260 0.300 0.000\n"
        "v 0.130 0.300 0.225\n"
        "v -0.130 0.300 0.225\n"
        "v -0.260 0.300 0.000\n"
        "v -0.130 0.300 -0.225\n"
        "v 0.130 0.300 -0.225\n"
        "v 0.310 -0.430 0.000\n"
        "v 0.155 -0.430 0.268\n"
        "v -0.155 -0.430 0.268\n"
        "v -0.310 -0.430 0.000\n"
        "v -0.155 -0.430 -0.268\n"
        "v 0.155 -0.430 -0.268\n"
        "v 0.000 -0.680 0.000\n"
        "v -0.520 0.390 0.100\n"
        "v -0.300 0.090 0.100\n"
        "v -0.410 0.090 0.291\n"
        "v -0.630 0.090 0.291\n"
        "v -0.740 0.090 0.100\n"
        "v -0.630 0.090 -0.091\n"
        "v -0.410 0.090 -0.091\n"
        "v -0.240 -0.470 0.130\n"
        "v -0.370 -0.470 0.355\n"
        "v -0.630 -0.470 0.355\n"
        "v -0.760 -0.470 0.130\n"
        "v -0.630 -0.470 -0.095\n"
        "v -0.370 -0.470 -0.095\n"
        "v -0.430 -0.640 0.120\n"
        "v 0.470 0.330 -0.070\n"
        "v 0.660 0.040 -0.070\n"
        "v 0.565 0.040 0.095\n"
        "v 0.375 0.040 0.095\n"
        "v 0.280 0.040 -0.070\n"
        "v 0.375 0.040 -0.235\n"
        "v 0.565 0.040 -0.235\n"
        "v 0.700 -0.500 -0.050\n"
        "v 0.585 -0.500 0.149\n"
        "v 0.355 -0.500 0.149\n"
        "v 0.240 -0.500 -0.050\n"
        "v 0.355 -0.500 -0.249\n"
        "v 0.585 -0.500 -0.249\n"
        "v 0.560 -0.660 -0.040\n"
        "f 1 2 3\n"
        "f 1 3 4\n"
        "f 1 4 5\n"
        "f 1 5 6\n"
        "f 1 6 7\n"
        "f 1 7 8\n"
        "f 1 8 9\n"
        "f 1 9 2\n"
        "f 2 10 11 3\n"
        "f 3 11 12 4\n"
        "f 4 12 13 5\n"
        "f 5 13 14 6\n"
        "f 6 14 15 7\n"
        "f 7 15 16 8\n"
        "f 8 14 9\n"
        "f 9 14 10\n"
        "f 10 14 11\n"
        "f 11 14 12\n"
        "f 12 14 13\n"
        "f 13 14 8\n"
        "f 15 16 17\n"
        "f 15 17 18\n"
        "f 15 18 19\n"
        "f 15 19 20\n"
        "f 15 20 21\n"
        "f 15 21 16\n"
        "f 16 22 23 17\n"
        "f 17 23 24 18\n"
        "f 18 24 25 19\n"
        "f 19 25 26 20\n"
        "f 20 26 27 21\n"
        "f 21 27 22 16\n"
        "f 22 28 23\n"
        "f 23 28 24\n"
        "f 24 28 25\n"
        "f 25 28 26\n"
        "f 26 28 27\n"
        "f 27 28 22\n"
        "f 29 30 31\n"
        "f 29 31 32\n"
        "f 29 32 33\n"
        "f 29 33 34\n"
        "f 29 34 35\n"
        "f 29 35 30\n"
        "f 30 36 37 31\n"
        "f 31 37 38 32\n"
        "f 32 38 39 33\n"
        "f 33 39 40 34\n"
        "f 34 40 41 35\n"
        "f 35 41 36 30\n"
        "f 36 42 37\n"
        "f 37 42 38\n"
        "f 38 42 39\n"
        "f 39 42 40\n"
        "f 40 42 41\n"
        "f 41 42 36\n";
    return dvz_write_bytes(OBJ_PATH, "wb", sizeof(OBJ) - 1u, (const uint8_t*)OBJ) == 0;
}


/**
 * Upload a geometry as one lit retained mesh and release the CPU copy.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry geometry to upload
 * @return true when the mesh was added
 */
static bool _add_obj_mesh(DvzScene* scene, DvzPanel* panel, DvzGeometry* geometry)
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



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the OBJ-loading feature scenario.
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

    if (!_write_obj_fixture())
        return false;
    DvzGeometry* geometry = dvz_geom_obj(
        OBJ_PATH, &(DvzGeometryObjDesc){
                      DVZ_STRUCT_INIT_FIELDS(DvzGeometryObjDesc),
                      .color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
                  });
    remove(OBJ_PATH);
    if (geometry == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    example_graphite_cyan_set_panel_background(panel);

    if (example_set_default_3d_camera(panel, 1.0f) == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    if (!_add_obj_mesh(ctx->scene, panel, geometry))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){0.0f, 0.0f, 0.0f});
    return true;
}



/**
 * Return the OBJ-loading scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_obj_loading_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_obj_loading",
        .title = "obj_loading",
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
 * Run the OBJ-loading feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_obj_loading_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
