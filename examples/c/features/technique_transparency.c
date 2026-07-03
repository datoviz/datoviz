/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* transparency_order - source-over, WBOIT, and depth-peel transparency on overlapping cubes.
 *
 * Scenario: feature.transparency_order
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_transparency
 * Run:    ./build/examples/c/features/technique_transparency --live
 * Smoke:  ./build/examples/c/features/technique_transparency --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one visual-local translation transform.
 *
 * @param x translation x
 * @param y translation y
 * @param z translation z
 * @param out output matrix
 */
static void _translation(float x, float y, float z, mat4 out)
{
    if (out == NULL)
        return;
    memset(out, 0, sizeof(mat4));
    out[0][0] = 1.0f;
    out[1][1] = 1.0f;
    out[2][2] = 1.0f;
    out[3][3] = 1.0f;
    out[3][0] = x;
    out[3][1] = y;
    out[3][2] = z;
}



/**
 * Add one translucent cube visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param size cube edge length
 * @param position visual-local translation
 * @param alpha vertex alpha
 * @param mode alpha technique
 * @param primary primary face color role
 * @return true on success
 */
static bool _add_transparent_cube(
    DvzScene* scene,
    DvzPanel* panel,
    double size,
    vec3 position,
    uint8_t alpha,
    DvzAlphaMode mode,
    ExampleStyleColorRole primary)
{
    const ExampleStyleColorRole roles[6] = {
        primary,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,
    };
    DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {{0}};
    for (uint32_t i = 0; i < DVZ_GEOM_CUBE_FACE_COUNT; i++)
    {
        face_colors[i] = example_graphite_cyan_color(roles[i]);
        face_colors[i].a = alpha;
    }

    DvzGeometry* cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = size,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return false;

    bool ok = false;
    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        goto cleanup;
    if (dvz_mesh_set_geometry(visual, cube) != 0)
        goto cleanup;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.standard.roughness = 0.42f;
    material.standard.specular = 0.30f;
    material.standard.rim_strength = 0.20f;
    material.alpha_mode = mode;
    if (dvz_visual_set_material(visual, &material) != 0)
        goto cleanup;
    if (dvz_visual_set_alpha_mode(visual, mode) != 0)
        goto cleanup;

    mat4 transform = {{0}};
    _translation(position[0], position[1], position[2], transform);
    if (dvz_visual_set_transform(visual, transform) != 0)
        goto cleanup;

    ok = dvz_panel_add_visual(panel, visual, NULL) == 0;

cleanup:
    dvz_geometry_destroy(cube);
    return ok;
}



/**
 * Add the same overlapping translucent cubes to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param mode alpha technique
 * @return true on success
 */
static bool _add_transparent_cubes(DvzScene* scene, DvzPanel* panel, DvzAlphaMode mode)
{
    return _add_transparent_cube(
               scene, panel, 1.06, (vec3){-0.20f, -0.02f, +0.00f}, 112u, mode,
               EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY) &&
           _add_transparent_cube(
               scene, panel, 0.78, (vec3){+0.26f, +0.10f, +0.18f}, 146u, mode,
               EXAMPLE_STYLE_COLOR_WARNING);
}



/**
 * Set the shared 3D camera used by all panels.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = +0.10f;
    camera.view.eye[1] = +1.25f;
    camera.view.eye[2] = +3.25f;
    camera.projection.fov_y = 0.58f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
    return dvz_panel_set_camera_desc(panel, &camera) == 0;
}



/**
 * Set the shared arcball orientation used by all panels.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @return true on success
 */
static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    dvz_arcball_set(arcball, (vec3){+0.50f, -0.18f, +0.22f});
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the transparency-order feature scenario.
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

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 3);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 24.0f, 0.0f))
        return false;

    DvzPanel* blended = dvz_grid_panel(grid, 0, 0);
    DvzPanel* wboit = dvz_grid_panel(grid, 0, 1);
    DvzPanel* peel = dvz_grid_panel(grid, 0, 2);
    if (blended == NULL || wboit == NULL || peel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(blended);
    example_graphite_cyan_set_panel_background(wboit);
    example_graphite_cyan_set_panel_background(peel);

    DvzLabelDesc label = dvz_label_desc();
    label.style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    label.placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label.placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label.placement.position[0] = EXAMPLE_PANEL_LABEL_X_PX;
    label.placement.position[1] = EXAMPLE_PANEL_LABEL_Y_PX;
    label.placement.text_anchor[0] = 0.0f;
    label.placement.text_anchor[1] = 0.0f;
    label.placement.has_text_anchor = true;
    label.text = "source-over";
    if (dvz_annotation_label(blended, &label) == NULL)
        return false;
    label.text = "weighted OIT";
    if (dvz_annotation_label(wboit, &label) == NULL)
        return false;
    label.text = "depth peel";
    if (dvz_annotation_label(peel, &label) == NULL)
        return false;

    if (!_set_camera(blended) || !_set_camera(wboit) || !_set_camera(peel))
        return false;
    DvzController* controllers[3] = {
        _bind_arcball(ctx, blended),
        _bind_arcball(ctx, wboit),
        _bind_arcball(ctx, peel),
    };
    if (controllers[0] == NULL || controllers[1] == NULL || controllers[2] == NULL)
        return false;
    for (uint32_t i = 1; i < 3u; i++)
    {
        if (dvz_controller_link(
                ctx->scene, controllers[0], controllers[i],
                DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
                DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
            return false;
    }

    return _add_transparent_cubes(ctx->scene, blended, DVZ_ALPHA_BLENDED) &&
           _add_transparent_cubes(ctx->scene, wboit, DVZ_ALPHA_WBOIT) &&
           _add_transparent_cubes(ctx->scene, peel, DVZ_ALPHA_DEPTH_PEEL);
}



/**
 * Return the transparency-order scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _transparency_order_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_transparency",
        .title = "Transparency Order",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the transparency-order feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _transparency_order_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
