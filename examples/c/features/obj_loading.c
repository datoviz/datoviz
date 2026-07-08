/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* obj_loading loads a Wavefront OBJ fixture and displays it as a retained mesh.
 *
 * What to look for: the example writes a compact OBJ file, loads it through dvz_geometry_obj(),
 * assigns one mesh color, then uploads the geometry into a lit mesh visual. In the live preview,
 * rotate the low-poly object with the arcball controller and compare its faceted silhouette with
 * the source fixture's vertices and faces. OBJ loading is useful when scientific geometry comes
 * from external meshing or modeling tools.
 *
 * Scenario: features_obj_loading
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

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/fileio.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"


DvzScenarioSpec dvz_example_obj_loading_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define OBJ_PATH "feature_obj_loading_tmp.obj"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ObjLoadingState
{
    DvzVisual* visual;
    DvzMaterialDesc material;
    DvzArcball* arcball;
    ExampleTuner tuner;
} ObjLoadingState;



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
        "# Compact low-poly bunny fixture. The loader ignores comments and object names.\n"
        "o datoviz_low_poly_bunny\n"
        "v 0.000 0.350 0.000\n"
        "v 0.000 0.150 0.340\n"
        "v 0.330 0.150 0.240\n"
        "v 0.470 0.150 0.000\n"
        "v 0.330 0.150 -0.240\n"
        "v 0.000 0.150 -0.340\n"
        "v -0.330 0.150 -0.240\n"
        "v -0.470 0.150 0.000\n"
        "v -0.330 0.150 0.240\n"
        "v 0.000 -0.550 0.280\n"
        "v 0.280 -0.550 0.200\n"
        "v 0.400 -0.550 0.000\n"
        "v 0.280 -0.550 -0.200\n"
        "v 0.000 -0.550 -0.280\n"
        "v -0.280 -0.550 -0.200\n"
        "v -0.400 -0.550 0.000\n"
        "v -0.280 -0.550 0.200\n"
        "v 0.000 -0.780 0.000\n"
        "v 0.000 0.780 0.220\n"
        "v 0.000 0.200 0.220\n"
        "v 0.000 0.490 0.500\n"
        "v 0.270 0.490 0.220\n"
        "v 0.000 0.490 -0.040\n"
        "v -0.270 0.490 0.220\n"
        "v -0.160 0.560 0.200\n"
        "v -0.350 1.050 0.200\n"
        "v -0.080 1.040 0.200\n"
        "v -0.230 1.350 0.200\n"
        "v -0.160 0.560 0.080\n"
        "v -0.350 1.050 0.080\n"
        "v -0.080 1.040 0.080\n"
        "v -0.230 1.350 0.080\n"
        "v 0.160 0.560 0.200\n"
        "v 0.080 1.040 0.200\n"
        "v 0.350 1.050 0.200\n"
        "v 0.230 1.350 0.200\n"
        "v 0.160 0.560 0.080\n"
        "v 0.080 1.040 0.080\n"
        "v 0.350 1.050 0.080\n"
        "v 0.230 1.350 0.080\n"
        "v 0.000 -0.200 -0.520\n"
        "v 0.000 -0.500 -0.520\n"
        "v 0.000 -0.350 -0.360\n"
        "v 0.180 -0.350 -0.520\n"
        "v 0.000 -0.350 -0.680\n"
        "v -0.180 -0.350 -0.520\n"
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
        "f 8 16 17 9\n"
        "f 9 17 10 2\n"
        "f 18 11 10\n"
        "f 18 12 11\n"
        "f 18 13 12\n"
        "f 18 14 13\n"
        "f 18 15 14\n"
        "f 18 16 15\n"
        "f 18 17 16\n"
        "f 18 10 17\n"
        "f 19 21 22\n"
        "f 19 22 23\n"
        "f 19 23 24\n"
        "f 19 24 21\n"
        "f 20 22 21\n"
        "f 20 23 22\n"
        "f 20 24 23\n"
        "f 20 21 24\n"
        "f 25 26 28 27\n"
        "f 29 31 32 30\n"
        "f 25 29 30 26\n"
        "f 27 28 32 31\n"
        "f 26 30 32 28\n"
        "f 25 27 31 29\n"
        "f 33 34 36 35\n"
        "f 37 39 40 38\n"
        "f 33 37 38 34\n"
        "f 35 36 40 39\n"
        "f 34 38 40 36\n"
        "f 33 35 39 37\n"
        "f 41 43 44\n"
        "f 41 44 45\n"
        "f 41 45 46\n"
        "f 41 46 43\n"
        "f 42 44 43\n"
        "f 42 45 44\n"
        "f 42 46 45\n"
        "f 42 43 46\n";
    return dvz_write_bytes(OBJ_PATH, "wb", sizeof(OBJ) - 1u, (const uint8_t*)OBJ) == 0;
}


/**
 * Upload a geometry as one lit retained mesh and release the CPU copy.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param geometry geometry to upload
 * @param material material descriptor
 * @param out_visual optional output visual pointer
 * @return true when the mesh was added
 */
static bool _add_obj_mesh(
    DvzScene* scene,
    DvzPanel* panel,
    DvzGeometry* geometry,
    DvzMaterialDesc* material,
    DvzVisual** out_visual)
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
    const bool ok = dvz_visual_set_material(mesh, material) == 0 &&
                    dvz_mesh_set_geometry(mesh, geometry) == 0 &&
                    dvz_panel_add_visual(panel, mesh, NULL) == 0;
    if (ok && out_visual != NULL)
        *out_visual = mesh;
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

    ObjLoadingState* state = (ObjLoadingState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("OBJ loading settings");
    state->material = example_default_phong_material_desc();

    if (!_write_obj_fixture())
        return false;
    DvzGeometry* geometry = dvz_geometry_obj(
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
    example_tuner_figure(&state->tuner, ctx->figure);
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = example_default_3d_camera_desc(1.0f);
    if (dvz_panel_set_camera_desc(panel, &camera) != 0)
    {
        dvz_geometry_destroy(geometry);
        return false;
    }
    if (!_add_obj_mesh(ctx->scene, panel, geometry, &state->material, &state->visual))
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
    state->arcball = arcball;

    vec3 arcball_angles = {0.0f, 0.0f, 0.0f};
    vec2 arcball_pan = {0.0f, 0.0f};
    example_tuner_camera(&state->tuner, "Camera", panel, &camera);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, arcball_angles, 1.0f, arcball_pan);
    example_tuner_material(&state->tuner, "Material", state->visual, &state->material);
    return true;
}


static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    ObjLoadingState* state = (ObjLoadingState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}


static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ObjLoadingState* state = (ObjLoadingState*)user;
    if (state != NULL)
        example_tuner_detach(&state->tuner);
    dvz_free(state);
}



/**
 * Return the OBJ-loading scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_obj_loading_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_obj_loading",
        .title = "OBJ Loading",
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
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
