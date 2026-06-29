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
#define LABEL_SIZE 18.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MaterialMeshState
{
    DvzGeometry* geometry;
} MaterialMeshState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_material_mesh_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one cube mesh with a specific material.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param material material descriptor
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_material_cube(
    DvzScene* scene, DvzPanel* panel, const DvzMaterialDesc* material, DvzGeometry** out_geometry)
{
    const ExampleStyleColorRole face_roles[DVZ_GEOM_CUBE_FACE_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {0};
    for (uint32_t i = 0; i < DVZ_GEOM_CUBE_FACE_COUNT; i++)
        face_colors[i] = example_graphite_cyan_color(face_roles[i]);

    DvzGeometry* cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = 0.72,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return false;
    if (out_geometry != NULL)
        *out_geometry = cube;

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return false;
    if (dvz_mesh_set_geometry(visual, cube) != 0)
        return false;
    dvz_geometry_destroy(cube);
    if (out_geometry != NULL)
        *out_geometry = NULL;
    if (dvz_visual_set_material(visual, material) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}


/**
 * Add one high-quality MSDF label to a material comparison panel.
 *
 * @param panel panel receiving the label
 * @param label label text
 * @return true on success
 */
static bool _add_material_label(DvzPanel* panel, const char* label)
{
    if (panel == NULL || label == NULL || label[0] == '\0')
        return false;

    DvzLabelDesc desc = dvz_label_desc();
    desc.text = label;
    desc.style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    desc.style.size_px = LABEL_SIZE;
    desc.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    desc.style.color[3] = 255u;
    desc.placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    desc.placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    desc.placement.position[0] = 20.0f;
    desc.placement.position[1] = 20.0f;
    desc.placement.text_anchor[0] = 0.0f;
    desc.placement.text_anchor[1] = 0.0f;
    desc.placement.has_text_anchor = true;
    return dvz_annotation_label(panel, &desc) != NULL;
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 3);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){.left_px = 34.0f, .right_px = 34.0f, .top_px = 40.0f,
                                     .bottom_px = 40.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 24.0f, 0.0f))
        return false;

    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 0.0f;
    camera.view.eye[1] = 1.22f;
    camera.view.eye[2] = 3.25f;
    camera.projection.fov_y = 0.64f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
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

    const DvzMaterialDesc* materials[3] = {&matte, &glossy, &rim};
    const char* labels[3] = {"Matte Phong", "Glossy Phong", "Standard rim"};
    DvzController* controllers[3] = {0};
    for (uint32_t i = 0; i < 3u; i++)
    {
        DvzPanel* panel = dvz_grid_panel(grid, 0, i);
        if (panel == NULL)
            return false;
        example_graphite_cyan_set_panel_background(panel);
        if (!_add_material_label(panel, labels[i]))
            return false;
        if (!dvz_panel_set_camera(panel, &camera))
            return false;
        if (!_add_material_cube(ctx->scene, panel, materials[i], &state->geometry))
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
        controllers[i] = controller;
    }
    for (uint32_t i = 1; i < 3u; i++)
    {
        if (dvz_controller_link(
                ctx->scene, controllers[0], controllers[i],
                DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
                DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
            return false;
    }
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
DvzScenarioSpec dvz_example_material_mesh_scenario(void)
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
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_material_mesh_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
