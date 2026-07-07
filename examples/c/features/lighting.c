/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* lighting compares the same sphere cluster under three material and light configurations.
 *
 * What to look for: each panel uploads the same sphere position, radius, and color arrays, but
 * changes the material light_direction, roughness, specular, and rim_strength values. Rotate any
 * panel in the live preview and the linked arcball controllers keep the views aligned, making it
 * easier to compare matte lighting, glossy highlights, and rim emphasis on identical data.
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

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SPHERE_COUNT 9u
#define LABEL_SIZE   18.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LightingState
{
    DvzVisual* visuals[3];
    DvzMaterialDesc materials[3];
    DvzArcball* arcball;
    ExampleTuner tuner;
} LightingState;



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
 * @param material material descriptor
 * @param out_visual optional output visual pointer
 * @return true on success
 */
static bool _add_lit_spheres(
    DvzScene* scene, DvzPanel* panel, const DvzMaterialDesc* material, DvzVisual** out_visual)
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

    if (dvz_visual_set_material(visual, material) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        return false;
    if (out_visual != NULL)
        *out_visual = visual;
    return true;
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

    LightingState* state = (LightingState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("Lighting settings");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    example_tuner_figure(&state->tuner, ctx->figure);

    state->materials[0] = dvz_standard_material_desc();
    state->materials[0].light_direction[0] = +0.34f;
    state->materials[0].light_direction[1] = +0.46f;
    state->materials[0].light_direction[2] = +0.82f;
    state->materials[0].standard.roughness = 0.86f;
    state->materials[0].standard.specular = 0.12f;
    state->materials[0].standard.rim_strength = 0.05f;

    state->materials[1] = dvz_standard_material_desc();
    state->materials[1].light_direction[0] = +0.12f;
    state->materials[1].light_direction[1] = +0.70f;
    state->materials[1].light_direction[2] = +0.62f;
    state->materials[1].standard.roughness = 0.42f;
    state->materials[1].standard.specular = 0.60f;
    state->materials[1].standard.rim_strength = 0.18f;

    state->materials[2] = dvz_standard_material_desc();
    state->materials[2].light_direction[0] = +0.62f;
    state->materials[2].light_direction[1] = +0.18f;
    state->materials[2].light_direction[2] = +0.76f;
    state->materials[2].standard.roughness = 0.24f;
    state->materials[2].standard.specular = 0.78f;
    state->materials[2].standard.rim_strength = 0.42f;

    DvzCameraDesc camera = example_default_3d_camera_desc(1.0f);
    DvzController* controllers[3] = {0};
    const char* labels[3] = {"Matte key light", "Glossy side light", "Rim highlight"};
    DvzPanel* panels[3] = {0};
    const DvzPanelDesc panel_descs[3] = {
        {.x = 0.0000f, .y = 0.0f, .width = 0.3340f, .height = 1.0f},
        {.x = 0.3330f, .y = 0.0f, .width = 0.3340f, .height = 1.0f},
        {.x = 0.6660f, .y = 0.0f, .width = 0.3340f, .height = 1.0f},
    };
    for (uint32_t i = 0; i < 3u; i++)
    {
        DvzPanel* panel = dvz_panel(ctx->figure, &panel_descs[i]);
        if (panel == NULL)
            return false;
        panels[i] = panel;
        example_graphite_cyan_set_panel_background(panel);
        if (!_add_lighting_label(panel, labels[i]))
            return false;
        if (dvz_panel_set_camera_desc(panel, &camera) != 0)
            return false;
        if (!_add_lit_spheres(ctx->scene, panel, &state->materials[i], &state->visuals[i]))
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
        if (i == 0u)
            state->arcball = arcball;
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

    vec3 arcball_angles = {0.0f, 0.0f, 0.0f};
    vec2 arcball_pan = {0.0f, 0.0f};
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, arcball_angles, 1.0f, arcball_pan);
    for (uint32_t i = 0; i < 3u; i++)
    {
        example_tuner_camera(&state->tuner, labels[i], panels[i], &camera);
        example_tuner_material(&state->tuner, labels[i], state->visuals[i], &state->materials[i]);
    }
    return true;
}


static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    LightingState* state = (LightingState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}


static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    LightingState* state = (LightingState*)user;
    if (state != NULL)
        example_tuner_detach(&state->tuner);
    dvz_free(state);
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
        .destroy = _scenario_destroy,
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
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
