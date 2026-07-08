/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* msaa compares single-sample rendering with 8x multisample antialiasing.
 *
 * What to look for: both panels draw the same translated cube cluster with linked arcball
 * controls, but only the right panel enables an MSAA descriptor with sample_count set to 8. Inspect
 * diagonal cube edges and small silhouettes in the screenshot or live preview; MSAA is useful when
 * scientific meshes or glyphs need smoother boundaries without changing their geometry.
 *
 * Scenario: features_technique_msaa
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_msaa
 * Run:    ./build/examples/c/features/technique_msaa --live
 * Smoke:  ./build/examples/c/features/technique_msaa --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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
#define CUBE_COUNT 4u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MsaaDemoState
{
    DvzPanel* multisample_panel;
    DvzArcball* arcball;
    DvzExampleGuiMsaaControls msaa;
    ExampleTuner tuner;
} MsaaDemoState;



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
 * Add one slanted cube cluster whose edges make MSAA differences visible.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_cube_cluster(DvzScene* scene, DvzPanel* panel)
{
    static const vec3 positions[CUBE_COUNT] = {
        {-0.54f, -0.30f, -0.05f},
        {+0.04f, -0.08f, +0.12f},
        {+0.52f, +0.16f, +0.00f},
        {-0.10f, +0.45f, +0.18f},
    };
    const double sizes[CUBE_COUNT] = {0.50, 0.62, 0.44, 0.34};

    const ExampleStyleColorRole roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY, EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,           EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };

    for (uint32_t i = 0; i < CUBE_COUNT; i++)
    {
        DvzVisual* cube = example_graphite_cyan_cube_mesh(scene, sizes[i], roles, NULL);
        if (cube == NULL)
            return false;

        DvzMaterialDesc material = dvz_standard_material_desc();
        material.standard.roughness = 0.46f;
        material.standard.specular = 0.34f;
        material.standard.rim_strength = 0.18f;
        if (dvz_visual_set_material(cube, &material) != 0)
            return false;

        mat4 transform = {{0}};
        _translation(positions[i][0], positions[i][1], positions[i][2], transform);
        if (dvz_visual_set_transform(cube, transform) != 0)
            return false;
        if (dvz_panel_add_visual(panel, cube, NULL) != 0)
            return false;
    }
    return true;
}



/**
 * Set the shared arcball orientation used by both panels.
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
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.24f, +0.18f});
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the MSAA feature scenario.
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

    MsaaDemoState* state = (MsaaDemoState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("MSAA settings");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    example_tuner_figure(&state->tuner, ctx->figure);

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}) != DVZ_OK)
        return false;
    if (dvz_grid_set_gutter(grid, 30.0f, 0.0f) != DVZ_OK)
        return false;

    DvzPanel* single = dvz_grid_panel(grid, 0, 0);
    DvzPanel* multisample = dvz_grid_panel(grid, 0, 1);
    if (single == NULL || multisample == NULL)
        return false;
    state->multisample_panel = multisample;
    example_graphite_cyan_set_panel_background(single);
    example_graphite_cyan_set_panel_background(multisample);

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
    label.text = "single sample";
    DvzAnnotation* annotation = dvz_annotation_label(single, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "8x MSAA";
    annotation = dvz_annotation_label(multisample, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;

    if (example_set_default_3d_camera(single, 1.0f) == NULL ||
        example_set_default_3d_camera(multisample, 1.0f) == NULL)
        return false;
    DvzController* single_controller = _bind_arcball(ctx, single);
    DvzController* multisample_controller = _bind_arcball(ctx, multisample);
    if (single_controller == NULL || multisample_controller == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, single_controller, multisample_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    state->arcball = dvz_controller_arcball(single_controller);
    if (state->arcball == NULL)
        return false;
    if (!_add_cube_cluster(ctx->scene, single) || !_add_cube_cluster(ctx->scene, multisample))
        return false;

    state->msaa = (DvzExampleGuiMsaaControls){
        .enabled = true,
        .alpha_to_coverage = false,
        .samples = 8.0f,
        .min_samples = 2.0f,
        .max_samples = 16.0f,
    };
    vec3 arcball_angles = {+0.58f, -0.24f, +0.18f};
    vec2 arcball_pan = {0.0f, 0.0f};
    example_tuner_msaa(&state->tuner, "MSAA", state->multisample_panel, &state->msaa);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, arcball_angles, 1.0f, arcball_pan);
    return true;
}


static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    MsaaDemoState* state = (MsaaDemoState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}


static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    MsaaDemoState* state = (MsaaDemoState*)user;
    if (state != NULL)
        example_tuner_detach(&state->tuner);
    dvz_free(state);
}



/**
 * Return the MSAA scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _msaa_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_technique_msaa",
        .title = "Multisample Antialiasing",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the MSAA feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _msaa_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
