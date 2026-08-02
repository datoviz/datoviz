/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example compares a synthetic molecular aggregate with and without ambient occlusion.
 *
 * What to look for: both panels render the same irregular, multi-lobed aggregate of variable-sized
 * spheres, while the right panel applies AO. Narrow clefts, recessed pockets, and near-contact
 * atom clusters become easier to separate without a floor or external dataset. In live mode, use
 * the GUI and linked arcball to inspect how radius, intensity, thickness, minimum visibility, and
 * quality affect local occlusion.
 *
 * Scenario: features_technique_ao
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/technique_ao
 * Run:    ./build/examples/c/features/technique_ao --live
 * Smoke:  ./build/examples/c/features/technique_ao --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_controller_preview.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define AGGREGATE_TARGET_SPHERES 2600u
#define AGGREGATE_MAX_CANDIDATES 200000u
#define AGGREGATE_HASH_X         27
#define AGGREGATE_HASH_Y         23
#define AGGREGATE_HASH_Z         20
#define AGGREGATE_HASH_CELLS                                                            \
    (AGGREGATE_HASH_X * AGGREGATE_HASH_Y * AGGREGATE_HASH_Z)
#define AGGREGATE_HASH_STEP 0.080f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct AoDemoState
{
    DvzPanel* ao_panel;
    DvzArcball* plain_arcball;
    DvzArcball* ao_arcball;
    DvzExampleGuiAoControls ao;
    vec3 arcball_angles;
    vec2 arcball_pan;
    float arcball_zoom;
    ExampleTuner tuner;
} AoDemoState;



static void _apply_arcball(AoDemoState* state)
{
    if (state == NULL)
        return;

    if (state->plain_arcball != NULL)
    {
        dvz_arcball_set(state->plain_arcball, state->arcball_angles);
        dvz_arcball_zoom(state->plain_arcball, state->arcball_zoom);
        dvz_arcball_pan(state->plain_arcball, state->arcball_pan);
    }
    if (state->ao_arcball != NULL)
    {
        dvz_arcball_set(state->ao_arcball, state->arcball_angles);
        dvz_arcball_zoom(state->ao_arcball, state->arcball_zoom);
        dvz_arcball_pan(state->ao_arcball, state->arcball_pan);
    }
}



/**
 * Return a deterministic pseudo-random 32-bit value.
 *
 * @param value input value
 * @return mixed value
 */
static uint32_t _hash_u32(uint32_t value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    value ^= value >> 16u;
    return value;
}



/**
 * Return a deterministic value in the [0, 1] interval.
 *
 * @param value input value
 * @return normalized mixed value
 */
static float _hash_unit(uint32_t value)
{
    return (float)(_hash_u32(value) & 0x00ffffffu) / 16777215.0f;
}



/**
 * Add a protein-like aggregate made from irregular lobes and recessed pockets.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_sphere_cluster(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[AGGREGATE_TARGET_SPHERES] = {{0}};
    float radii[AGGREGATE_TARGET_SPHERES] = {0};
    DvzColor colors[AGGREGATE_TARGET_SPHERES] = {{0}};
    int32_t hash_heads[AGGREGATE_HASH_CELLS] = {0};
    int32_t hash_next[AGGREGATE_TARGET_SPHERES] = {0};
    uint32_t count = 0;

    const vec3 lobe_centers[5] = {
        {-0.40f, +0.12f, -0.02f},
        {+0.18f, +0.17f, +0.06f},
        {+0.55f, -0.24f, -0.08f},
        {-0.26f, -0.38f, +0.10f},
        {+0.08f, -0.18f, -0.30f},
    };
    const vec3 lobe_radii[5] = {
        {0.58f, 0.45f, 0.43f},
        {0.54f, 0.49f, 0.46f},
        {0.40f, 0.34f, 0.36f},
        {0.43f, 0.32f, 0.35f},
        {0.38f, 0.32f, 0.34f},
    };

    for (int32_t i = 0; i < AGGREGATE_HASH_CELLS; i++)
        hash_heads[i] = -1;

    for (uint32_t candidate = 0;
         candidate < AGGREGATE_MAX_CANDIDATES && count < AGGREGATE_TARGET_SPHERES;
         candidate++)
    {
        const float px = -1.05f + 2.10f * _hash_unit(4u * candidate + 0u);
        const float py = -0.90f + 1.80f * _hash_unit(4u * candidate + 1u);
        const float pz = -0.80f + 1.60f * _hash_unit(4u * candidate + 2u);
        const float radius = 0.024f + 0.012f * _hash_unit(4u * candidate + 3u);

        bool inside = false;
        for (uint32_t i = 0; i < 5; i++)
        {
            const float dx = (px - lobe_centers[i][0]) / lobe_radii[i][0];
            const float dy = (py - lobe_centers[i][1]) / lobe_radii[i][1];
            const float dz = (pz - lobe_centers[i][2]) / lobe_radii[i][2];
            if (dx * dx + dy * dy + dz * dz <= 1.0f)
            {
                inside = true;
                break;
            }
        }

        const float pocket0_x = px + 0.03f;
        const float pocket0_y = py - 0.31f;
        const float pocket0_z = pz - 0.48f;
        const bool pocket0 = pocket0_x * pocket0_x + pocket0_y * pocket0_y +
                             pocket0_z * pocket0_z < 0.070f;
        const float pocket1_x = px - 0.48f;
        const float pocket1_y = py - 0.08f;
        const float pocket1_z = pz - 0.24f;
        const bool pocket1 = pocket1_x * pocket1_x + pocket1_y * pocket1_y +
                             pocket1_z * pocket1_z < 0.030f;
        if (!inside || pocket0 || pocket1)
            continue;

        const int32_t cell_x = (int32_t)floorf((px + 1.05f) / AGGREGATE_HASH_STEP);
        const int32_t cell_y = (int32_t)floorf((py + 0.90f) / AGGREGATE_HASH_STEP);
        const int32_t cell_z = (int32_t)floorf((pz + 0.80f) / AGGREGATE_HASH_STEP);
        if (
            cell_x < 0 || cell_x >= AGGREGATE_HASH_X || cell_y < 0 ||
            cell_y >= AGGREGATE_HASH_Y || cell_z < 0 || cell_z >= AGGREGATE_HASH_Z)
        {
            continue;
        }

        bool overlaps = false;
        for (int32_t dz = -1; dz <= 1 && !overlaps; dz++)
        {
            const int32_t nz = cell_z + dz;
            if (nz < 0 || nz >= AGGREGATE_HASH_Z)
                continue;
            for (int32_t dy = -1; dy <= 1 && !overlaps; dy++)
            {
                const int32_t ny = cell_y + dy;
                if (ny < 0 || ny >= AGGREGATE_HASH_Y)
                    continue;
                for (int32_t dx = -1; dx <= 1 && !overlaps; dx++)
                {
                    const int32_t nx = cell_x + dx;
                    if (nx < 0 || nx >= AGGREGATE_HASH_X)
                        continue;
                    const int32_t hash_cell =
                        nx + AGGREGATE_HASH_X * (ny + AGGREGATE_HASH_Y * nz);
                    for (int32_t j = hash_heads[hash_cell]; j >= 0; j = hash_next[j])
                    {
                        const float sx = px - positions[j][0];
                        const float sy = py - positions[j][1];
                        const float sz = pz - positions[j][2];
                        const float minimum = radius + radii[j] + 0.002f;
                        if (sx * sx + sy * sy + sz * sz < minimum * minimum)
                        {
                            overlaps = true;
                            break;
                        }
                    }
                }
            }
        }
        if (overlaps)
            continue;

        positions[count][0] = px;
        positions[count][1] = py;
        positions[count][2] = pz;
        radii[count] = radius;
        colors[count] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        const int32_t hash_cell =
            cell_x + AGGREGATE_HASH_X * (cell_y + AGGREGATE_HASH_Y * cell_z);
        hash_next[count] = hash_heads[hash_cell];
        hash_heads[hash_cell] = (int32_t)count;
        count++;
    }
    if (count == 0)
        return false;

    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (spheres == NULL)
        return false;
    if (dvz_sphere_set_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "radius", .data = radii, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
    };
    if (dvz_visual_set_data_many(spheres, updates, 3) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.light_direction[0] = -0.38f;
    material.light_direction[1] = +0.52f;
    material.light_direction[2] = +0.76f;
    material.standard.roughness = 0.72f;
    material.standard.specular = 0.22f;
    material.standard.rim_strength = 0.10f;
    if (dvz_visual_set_material(spheres, &material) != 0)
        return false;
    return dvz_panel_add_visual(panel, spheres, NULL) == 0;
}



static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel, vec3 angles)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    if (dvz_arcball_initial(arcball, angles) != DVZ_OK)
        return NULL;
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the AO feature scenario.
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

    AoDemoState* state = (AoDemoState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("AO calibration");

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

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* ao_panel = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || ao_panel == NULL)
        return false;
    state->ao_panel = ao_panel;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(ao_panel);

    DvzTextStyle label_style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label_style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    label_style.size_px = EXAMPLE_PANEL_LABEL_LARGE_SIZE;
    DvzTextPlacement label_placement = dvz_text_placement();
    label_placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_placement.position[0] = EXAMPLE_PANEL_LABEL_LARGE_X_PX;
    label_placement.position[1] = EXAMPLE_PANEL_LABEL_LARGE_Y_PX;
    label_placement.text_anchor[0] = 0.0f;
    label_placement.text_anchor[1] = 0.0f;
    label_placement.has_text_anchor = true;
    DvzLabelDesc label = dvz_label_desc();
    label.text = "plain lighting";
    DvzAnnotation* annotation = dvz_annotation_label(plain, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    label.text = "ambient occlusion";
    annotation = dvz_annotation_label(ao_panel, &label);
    if (annotation == NULL || dvz_annotation_set_style(annotation, &label_style) != 0 ||
        dvz_annotation_set_placement(annotation, &label_placement) != 0)
        return false;
    if (example_set_default_3d_camera(plain, 1.0f) == NULL ||
        example_set_default_3d_camera(ao_panel, 1.0f) == NULL)
        return false;
    if (!_add_sphere_cluster(ctx->scene, plain) || !_add_sphere_cluster(ctx->scene, ao_panel))
        return false;

    state->arcball_angles[0] = -0.239547f;
    state->arcball_angles[1] = -0.407204f;
    state->arcball_angles[2] = +0.824558f;
    state->arcball_zoom = 0.505017f;
    state->arcball_pan[0] = +0.000f;
    state->arcball_pan[1] = -0.020f;
    DvzController* plain_controller = _bind_arcball(ctx, plain, state->arcball_angles);
    DvzController* ao_controller = _bind_arcball(ctx, ao_panel, state->arcball_angles);
    if (plain_controller == NULL || ao_controller == NULL)
        return false;
    state->plain_arcball = dvz_controller_arcball(plain_controller);
    state->ao_arcball = dvz_controller_arcball(ao_controller);
    if (state->plain_arcball == NULL || state->ao_arcball == NULL)
        return false;
    if (dvz_controller_link(
            ctx->scene, plain_controller, ao_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
            DVZ_CONTROLLER_LINK_TWO_WAY) == NULL)
        return false;
    _apply_arcball(state);

    state->ao = (DvzExampleGuiAoControls){
        .enabled = true,
        .show_debug_view = true,
        .radius = 0.560f,
        .intensity = 3.318f,
        .thickness = 0.128f,
        .min_visibility = 0.000f,
        .quality = (float)DVZ_AO_QUALITY_ULTRA,
        .debug_mode = DVZ_AO_DEBUG_NONE,
    };
    example_tuner_ao(&state->tuner, "Occlusion", state->ao_panel, &state->ao);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->plain_arcball, state->arcball_angles,
        state->arcball_zoom, state->arcball_pan);
    return true;
}


static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    AoDemoState* state = (AoDemoState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    AoDemoState* state = (AoDemoState*)user;
    if (state != NULL)
        example_tuner_detach(&state->tuner);
    dvz_free(state);
}


static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    AoDemoState* state = (AoDemoState*)user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL)
        return;
    ExamplePreviewArcballDesc desc = example_preview_arcball_cube_desc();
    example_preview_arcball(
        state->plain_arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
    example_preview_arcball(
        state->ao_arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
}



/**
 * Return the AO scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _ao_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_technique_ao",
        .title = "View-Space Ambient Occlusion",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the AO feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _ao_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
