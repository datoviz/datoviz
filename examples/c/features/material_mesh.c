/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* material_mesh - retained mesh visuals with explicit material parameters.
 *
 * Scenario: feature.material_mesh
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/material_mesh
 * Run:    ./build/examples/c/features/material_mesh --live
 * Smoke:  ./build/examples/c/features/material_mesh --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/geom.h"
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

typedef struct MaterialMeshState
{
    DvzGeometry* geometry;
} MaterialMeshState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Set a visual-local translation transform.
 *
 * @param visual visual to transform
 * @param x translation on X
 * @param z translation on Z
 * @return true when the transform was accepted
 */
static bool _translate_visual(DvzVisual* visual, float x, float z)
{
    mat4 transform = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {x, 0.0f, z, 1.0f},
    };
    return dvz_visual_set_transform(visual, transform) == 0;
}



/**
 * Add one cube mesh with a specific material.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param x visual-local X translation
 * @param z visual-local Z translation
 * @param material material descriptor
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_material_cube(
    DvzScene* scene, DvzPanel* panel, float x, float z, const DvzMaterialDesc* material,
    DvzGeometry** out_geometry)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 0.72, face_roles, out_geometry);
    if (visual == NULL)
        return false;

    if (dvz_visual_set_material(visual, material) != 0)
        return false;
    if (!_translate_visual(visual, x, z))
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the material-mesh feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    MaterialMeshState* state = (MaterialMeshState*)calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.40f;
    camera.eye[2] = 1.30f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.64f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        return false;

    DvzMaterialDesc matte = dvz_phong_material_desc();
    matte.phong.ambient = 0.34f;
    matte.phong.diffuse = 0.84f;
    matte.phong.specular = 0.02f;
    matte.phong.shininess = 8.0f;

    DvzMaterialDesc glossy = dvz_phong_material_desc();
    glossy.phong.ambient = 0.18f;
    glossy.phong.diffuse = 0.70f;
    glossy.phong.specular = 0.48f;
    glossy.phong.shininess = 58.0f;

    DvzMaterialDesc rim = dvz_standard_material_desc();
    rim.standard.roughness = 0.42f;
    rim.standard.specular = 0.46f;
    rim.standard.rim_strength = 0.30f;

    if (!_add_material_cube(ctx->scene, panel, -0.88f, -0.10f, &matte, &state->geometry))
        return false;
    if (!_add_material_cube(ctx->scene, panel, +0.00f, +0.08f, &glossy, &state->geometry))
        return false;
    if (!_add_material_cube(ctx->scene, panel, +0.88f, -0.10f, &rim, &state->geometry))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.14f, +0.26f});
    return true;
}



/**
 * Destroy the material-mesh feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    MaterialMeshState* state = (MaterialMeshState*)user;
    if (state == NULL)
        return;
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    free(state);
}



/**
 * Return the material-mesh scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _material_mesh_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_material_mesh",
        .title = "material_mesh",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the material-mesh feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _material_mesh_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
