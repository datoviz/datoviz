/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* lighting - compare lit sphere clusters with different material and light settings.
 *
 * Scenario: feature.lighting
 * Style: features, graphite_cyan, 1600x1200 capture target
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

#define WIDTH        1600u
#define HEIGHT       1200u
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
        {-1.02f, -0.36f, -0.20f}, {-0.52f, -0.36f, +0.05f}, {+0.00f, -0.36f, +0.18f},
        {+0.52f, -0.36f, +0.05f}, {+1.02f, -0.36f, -0.20f}, {-0.72f, +0.28f, -0.06f},
        {-0.20f, +0.28f, +0.16f}, {+0.34f, +0.28f, +0.12f}, {+0.86f, +0.28f, -0.08f},
    };
    const float c = 0.35;
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
    if (dvz_sphere_mode(visual, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
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

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = 1.10f;
    camera.eye[2] = 3.45f;
    camera.up[1] = 1.0f;
    camera.up[2] = 0.0f;
    camera.fov_y = 0.66f;
    camera.near = 0.05f;
    camera.far = 100.0f;
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
        if (!dvz_panel_set_camera(panel, &camera))
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
        if (!example_link_controllers_bidirectional(
                ctx->scene, controllers[0], controllers[i],
                DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM))
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
        .title = "lighting",
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
