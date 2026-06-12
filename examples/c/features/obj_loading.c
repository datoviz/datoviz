/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* obj_loading - Wavefront OBJ mesh loading through geom/fileio helpers.
 *
 * Scenario: feature_obj_loading
 * Style: features, graphite_cyan, 1600x1200 capture target
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

#define WIDTH  1600u
#define HEIGHT 1200u
#define OBJ_PATH "feature_obj_loading_tmp.obj"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Write a tiny OBJ fixture to the working directory.
 *
 * @return true when the file was written
 */
static bool _write_obj_fixture(void)
{
    static const char OBJ[] =
        "v 0 0 0.55\n"
        "v -0.52 -0.45 -0.24\n"
        "v 0.52 -0.45 -0.24\n"
        "v 0.52 0.45 -0.24\n"
        "v -0.52 0.45 -0.24\n"
        "f 1 2 3\n"
        "f 1 3 4\n"
        "f 1 4 5\n"
        "f 1 5 2\n"
        "f 2 5 4 3\n";
    return dvz_write_bytes(OBJ_PATH, "wb", sizeof(OBJ) - 1u, (const uint8_t*)OBJ) == 0;
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

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 1.35f;
    camera.eye[1] = -1.75f;
    camera.eye[2] = 1.35f;
    camera.target[0] = 0.0f;
    camera.target[1] = 0.0f;
    camera.target[2] = 0.0f;
    camera.up[0] = 0.0f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.66f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (dvz_panel_set_camera(panel, &camera) == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }

    DvzVisual* mesh = dvz_mesh(ctx->scene, 0);
    if (mesh == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    const bool ok = example_apply_default_phong_material(mesh) &&
                    dvz_mesh_set_geometry(mesh, geometry) == 0 &&
                    dvz_panel_add_visual(panel, mesh, NULL) == 0;
    dvz_geometry_destroy(geometry);
    if (!ok)
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.55f, 0.0f, -0.25f});
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
