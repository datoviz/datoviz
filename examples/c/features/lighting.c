/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* lighting - compare lit sphere clusters with different material and light settings.
 *
 * Scenario: feature.lighting
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/lighting
 * Run:    ./build/examples/c/features/lighting --live
 * Smoke:  ./build/examples/c/features/lighting --png
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
#define SPHERE_COUNT 9u
#define LABEL_SIZE   18.0f



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_lighting_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one sphere visual with deterministic positions and one material/light variant.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param variant material/light variant index
 * @return true on success
 */
static bool _add_lit_spheres(DvzScene* scene, DvzPanel* panel, uint32_t variant)
{
    vec3 positions[SPHERE_COUNT] = {
        {-0.56f, -0.20f, -0.20f}, {-0.29f, -0.20f, +0.05f}, {+0.00f, -0.20f, +0.18f},
        {+0.29f, -0.20f, +0.05f}, {+0.56f, -0.20f, -0.20f}, {-0.40f, +0.15f, -0.06f},
        {-0.11f, +0.15f, +0.16f}, {+0.19f, +0.15f, +0.12f}, {+0.47f, +0.15f, -0.08f},
    };
    const float c = 0.82f;
    const float radii[SPHERE_COUNT] = {0.150f * c, 0.170f * c, 0.190f * c, 0.170f * c, 0.150f * c,
                                       0.165f * c, 0.205f * c, 0.185f * c, 0.155f * c};
    DvzColor colors[SPHERE_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
    };

    DvzVisual* visual = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (visual == NULL)
        return false;
    if (dvz_sphere_set_mode(visual, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    if (variant == 0u)
    {
        material.light_direction[0] = +0.34f;
        material.light_direction[1] = +0.46f;
        material.light_direction[2] = +0.82f;
        material.standard.roughness = 0.86f;
        material.standard.specular = 0.12f;
        material.standard.rim_strength = 0.05f;
    }
    else if (variant == 1u)
    {
        material.light_direction[0] = +0.12f;
        material.light_direction[1] = +0.70f;
        material.light_direction[2] = +0.62f;
        material.standard.roughness = 0.42f;
        material.standard.specular = 0.60f;
        material.standard.rim_strength = 0.18f;
    }
    else
    {
        material.light_direction[0] = +0.62f;
        material.light_direction[1] = +0.18f;
        material.light_direction[2] = +0.76f;
        material.standard.roughness = 0.24f;
        material.standard.specular = 0.78f;
        material.standard.rim_strength = 0.42f;
    }
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}


/**
 * Add one high-quality MSDF label to a lighting comparison panel.
 *
 * @param panel panel receiving the label
 * @param label label text
 * @return true on success
 */
static bool _add_lighting_label(DvzPanel* panel, const char* label)
{
    if (panel == NULL || label == NULL || label[0] == '\0')
        return false;

    DvzLabelDesc desc = dvz_label_desc();
    desc.text = label;
    DvzTextStyle style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    style.size_px = LABEL_SIZE;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[3] = 255u;
    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 20.0f;
    placement.position[1] = 20.0f;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.0f;
    placement.has_text_anchor = true;
    DvzAnnotation* annotation = dvz_annotation_label(panel, &desc);
    return annotation != NULL && dvz_annotation_set_style(annotation, &style) == 0 &&
           dvz_annotation_set_placement(annotation, &placement) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the lighting feature scenario.
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
                      .left_px = 34.0f, .right_px = 34.0f, .top_px = 40.0f, .bottom_px = 40.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 24.0f, 0.0f))
        return false;

    DvzCameraDesc camera = example_default_3d_camera_desc(1.0f);
    DvzController* controllers[3] = {0};
    const char* labels[3] = {"Matte key light", "Glossy side light", "Rim highlight"};
    for (uint32_t i = 0; i < 3u; i++)
    {
        DvzPanel* panel = dvz_grid_panel(grid, 0, i);
        if (panel == NULL)
            return false;
        example_graphite_cyan_set_panel_background(panel);
        if (!_add_lighting_label(panel, labels[i]))
            return false;
        if (dvz_panel_set_camera_desc(panel, &camera) != 0)
            return false;
        if (!_add_lit_spheres(ctx->scene, panel, i))
            return false;

        DvzController* controller = dvz_arcball(ctx->scene, NULL);
        if (controller == NULL)
            return false;
        DvzArcball* arcball = dvz_controller_arcball(controller);
        if (arcball == NULL)
            return false;
        if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
            return false;
        // dvz_arcball_set(arcball, (vec3){+0.46f, -0.12f, +0.18f});
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
 * Return the lighting scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_lighting_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_lighting",
        .title = "Lighting",
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
 * Run the lighting feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_lighting_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
