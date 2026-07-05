/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_cue - depth-dependent fading applied to a regular 3D sphere lattice.
 *
 * Scenario: feature.depth_cue
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_depth_cue
 * Run:    ./build/examples/c/features/technique_depth_cue --live
 * Smoke:  ./build/examples/c/features/technique_depth_cue --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define LATTICE_SIDE  3u
#define SPHERE_COUNT  (LATTICE_SIDE * LATTICE_SIDE * LATTICE_SIDE)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a regular 3D sphere lattice with optional depth cueing.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param cue_enabled whether depth cueing is enabled
 * @return true on success
 */
static bool _add_sphere_lattice(DvzScene* scene, DvzPanel* panel, bool cue_enabled)
{
    vec3 positions[SPHERE_COUNT] = {{0}};
    DvzColor colors[SPHERE_COUNT] = {{0}};
    float radii[SPHERE_COUNT] = {0};

    for (uint32_t z = 0; z < LATTICE_SIDE; z++)
    {
        for (uint32_t y = 0; y < LATTICE_SIDE; y++)
        {
            for (uint32_t x = 0; x < LATTICE_SIDE; x++)
            {
                const uint32_t i = z * LATTICE_SIDE * LATTICE_SIDE + y * LATTICE_SIDE + x;
                positions[i][0] = -0.58f + 0.58f * (float)x;
                positions[i][1] = -0.44f + 0.44f * (float)y;
                positions[i][2] = -0.72f + 0.72f * (float)z;
                colors[i] = example_graphite_cyan_color(
                    z == 0u   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
                    : z == 1u ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                              : EXAMPLE_STYLE_COLOR_WARNING);
                radii[i] = 0.115f;
            }
        }
    }

    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (sphere == NULL)
        return false;
    if (dvz_sphere_set_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(sphere, updates, 3) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.light_direction[0] = -0.32f;
    material.light_direction[1] = +0.55f;
    material.light_direction[2] = +0.76f;
    material.standard.roughness = 0.46f;
    material.standard.specular = 0.44f;
    material.standard.rim_strength = 0.18f;
    if (dvz_visual_set_material(sphere, &material) != 0)
        return false;
    if (cue_enabled)
    {
        DvzDepthCueDesc cue = dvz_depth_cue_desc();
        cue.mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
        cue.metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE;
        cue.falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR;
        cue.near_depth = 1.20f;
        cue.far_depth = 3.80f;
        cue.strength = 0.88f;
        cue.background_color[0] = 0.035f;
        cue.background_color[1] = 0.047f;
        cue.background_color[2] = 0.067f;
        cue.background_color[3] = 1.0f;
        if (dvz_visual_set_depth_cue(sphere, &cue) != 0)
            return false;
    }
    return dvz_panel_add_visual(panel, sphere, NULL) == 0;
}



/**
 * Set the depth-cue camera.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    return example_set_default_3d_camera(panel, 1.0f) != NULL;
}


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
    dvz_arcball_set(arcball, (vec3){+0.48f, -0.18f, +0.20f});
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the depth-cue feature scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}) != DVZ_OK)
        return false;
    if (dvz_grid_set_gutter(grid, 30.0f, 0.0f) != DVZ_OK)
        return false;

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* cued = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || cued == NULL)
        return false;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(cued);

    DvzTextStyle label_style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label_style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    DvzTextPlacement label_placement = dvz_text_placement();
    label_placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_placement.position[0] = EXAMPLE_PANEL_LABEL_X_PX;
    label_placement.position[1] = EXAMPLE_PANEL_LABEL_Y_PX;
    label_placement.text_anchor[0] = 0.0f;
    label_placement.text_anchor[1] = 0.0f;
    label_placement.has_text_anchor = true;
    DvzLabelDesc label = dvz_label_desc();
    label.text = "plain depth";
    DvzAnnotation* annotation = dvz_annotation_label(plain, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "depth cue";
    annotation = dvz_annotation_label(cued, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;

    if (!_set_camera(plain) || !_set_camera(cued))
        return false;
    DvzController* plain_controller = _bind_arcball(ctx, plain);
    DvzController* cued_controller = _bind_arcball(ctx, cued);
    if (plain_controller == NULL || cued_controller == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, plain_controller, cued_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    return _add_sphere_lattice(ctx->scene, plain, false) &&
           _add_sphere_lattice(ctx->scene, cued, true);
}



/**
 * Return the depth-cue scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _depth_cue_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_depth_cue",
        .title = "Depth Cue",
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
 * Run the depth-cue feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _depth_cue_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
