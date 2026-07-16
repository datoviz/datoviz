/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* galaxy - This example renders an animated density-wave spiral galaxy.
 *
 * What to look for: small temperature-colored stars form a warm central bulge while large soft
 * dust sprites reveal nested blue spiral arms. Additive blending lets overlapping particles
 * accumulate light without turning the dense core into an opaque disc. Drag to orbit through the
 * thin stellar and dust layers, and use the wheel to inspect the core or the full disk. A sparse
 * world-space star shell provides depth without competing with the simulated particles.
 *
 * The density-wave equations and rendering composition are adapted from Nicolas P. Rougier's
 * Glumpy galaxy example and Ingo Berg's galaxy simulation. See galaxy_model.c for the retained BSD
 * notice and model details.
 *
 * Scenario: showcases_galaxy
 * Style: showcase, astronomical, interactive 3D
 *
 * Build:  just example-c showcases/galaxy
 * Run:    ./build/examples/c/showcases/galaxy --live
 * Smoke:  ./build/examples/c/showcases/galaxy --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_tuner.h"
#include "galaxy_model.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define SPRITE_SIZE            64u
#define BACKGROUND_STAR_COUNT  1200u
#define BACKGROUND_STAR_RADIUS 48.0f

#define GALAXY_INITIAL_YEARS                    100000.0
#define GALAXY_DEFAULT_MILLION_YEARS_PER_SECOND 1.0f



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_galaxy_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GalaxyState
{
    ExampleTuner tuner;
    GalaxyModel model;
    DvzVisual* visual;
    DvzVisual* background_stars;
    DvzArcball* arcball;
    float* angles;
    DvzSymbolId* symbols;
    double elapsed_years;
    float million_years_per_second;
    bool animate;
    bool show_background_stars;
} GalaxyState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a normalized scalar.
 *
 * @param value input value
 * @return value clamped to [0, 1]
 */
static float _saturate(float value) { return fminf(fmaxf(value, 0.0f), 1.0f); }



/**
 * Return one deterministic pseudo-random integer for the background star shell.
 *
 * @param state generator state
 * @return next pseudo-random integer
 */
static uint32_t _background_random_u32(uint32_t* state)
{
    ANN(state);
    *state = 1664525u * (*state) + 1013904223u;
    return *state;
}



/**
 * Return one deterministic pseudo-random scalar in [0, 1).
 *
 * @param state generator state
 * @return next pseudo-random scalar
 */
static float _background_random_f32(uint32_t* state)
{
    return (float)(_background_random_u32(state) & 0x00FFFFFFu) / (float)0x01000000u;
}



/**
 * Create a sparse, deterministic world-space background star shell.
 *
 * This follows the point-shell technique used by the textured-planet showcase, but deliberately
 * omits its prominent sun and uses dimmer colors so the galaxy remains the focal layer.
 *
 * @param scene scene
 * @return background point visual, or NULL on failure
 */
static DvzVisual* _create_background_stars(DvzScene* scene)
{
    ANN(scene);
    vec3* positions = dvz_calloc(BACKGROUND_STAR_COUNT, sizeof(*positions));
    DvzColor* colors = dvz_calloc(BACKGROUND_STAR_COUNT, sizeof(*colors));
    float* sizes = dvz_calloc(BACKGROUND_STAR_COUNT, sizeof(*sizes));
    if (positions == NULL || colors == NULL || sizes == NULL)
        goto fail;

    uint32_t random = 0xA54FF53Au;
    for (uint32_t i = 0; i < BACKGROUND_STAR_COUNT; i++)
    {
        const float z = 2.0f * _background_random_f32(&random) - 1.0f;
        const float phi = 2.0f * (float)DVZ_PI * _background_random_f32(&random);
        const float radius_xy = sqrtf(fmaxf(0.0f, 1.0f - z * z));
        positions[i][0] = BACKGROUND_STAR_RADIUS * radius_xy * cosf(phi);
        positions[i][1] = BACKGROUND_STAR_RADIUS * radius_xy * sinf(phi);
        positions[i][2] = BACKGROUND_STAR_RADIUS * z;

        const float brightness = 0.35f + 0.65f * _background_random_f32(&random);
        const float warmth = _background_random_f32(&random);
        colors[i] = (DvzColor){
            (uint8_t)(brightness * (150.0f + 45.0f * warmth)),
            (uint8_t)(brightness * (165.0f + 28.0f * warmth)),
            (uint8_t)(brightness * (205.0f - 18.0f * warmth)), 255};
        const float size_random = _background_random_f32(&random);
        sizes[i] = 0.8f + 1.8f * size_random * size_random;
    }

    DvzVisual* stars = dvz_point(scene, 0);
    if (stars == NULL)
        goto fail;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = BACKGROUND_STAR_COUNT},
        {.attr_name = "color", .data = colors, .item_count = BACKGROUND_STAR_COUNT},
        {.attr_name = "size", .data = sizes, .item_count = BACKGROUND_STAR_COUNT},
    };
    if (dvz_visual_set_data_many(stars, updates, 3) != DVZ_OK ||
        dvz_visual_set_depth_test(stars, false) != DVZ_OK)
    {
        goto fail;
    }

    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(positions);
    return stars;

fail:
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(positions);
    return NULL;
}



/**
 * Fill a soft white particle texture matching the original 64-pixel Glumpy sprite profile.
 *
 * RGB carries the radial intensity while alpha stays opaque. The particle's retained color alpha
 * therefore controls additive brightness without squaring the sampled intensity.
 *
 * @param rgba output RGBA8 sprite
 */
static void _fill_particle_sprite(DvzColor rgba[SPRITE_SIZE * SPRITE_SIZE])
{
    ANN(rgba);
    for (uint32_t y = 0; y < SPRITE_SIZE; y++)
    {
        for (uint32_t x = 0; x < SPRITE_SIZE; x++)
        {
            const float px = 2.0f * ((float)x + 0.5f) / (float)SPRITE_SIZE - 1.0f;
            const float py = 2.0f * ((float)y + 0.5f) / (float)SPRITE_SIZE - 1.0f;
            const float radius2 = px * px + py * py;
            const float core = 0.70f * expf(-radius2 / 0.080f);
            const float glow = 0.17f * expf(-radius2 / 0.450f);
            const float halo = 0.05f * expf(-radius2 / 0.800f);
            const uint8_t intensity = (uint8_t)(255.0f * _saturate(core + glow + halo) + 0.5f);
            rgba[y * SPRITE_SIZE + x] = (DvzColor){intensity, intensity, intensity, 255};
        }
    }
}



/**
 * Free scenario-owned model and attribute arrays.
 *
 * @param state galaxy scenario state
 */
static void _free_state(GalaxyState* state)
{
    if (state == NULL)
        return;
    example_tuner_detach(&state->tuner);
    dvz_free(state->symbols);
    dvz_free(state->angles);
    galaxy_model_destroy(&state->model);
    dvz_free(state);
}



/**
 * Restore the live galaxy controls to their showcase defaults.
 *
 * @param user galaxy scenario state
 */
static void _galaxy_controls_reset(void* user)
{
    GalaxyState* state = (GalaxyState*)user;
    if (state == NULL)
        return;
    state->animate = true;
    state->show_background_stars = true;
    state->million_years_per_second = GALAXY_DEFAULT_MILLION_YEARS_PER_SECOND;
    if (state->background_stars != NULL)
        (void)dvz_visual_set_visible(state->background_stars, true);
}



/**
 * Draw the live simulation and background controls.
 *
 * @param gui GUI
 * @param user galaxy scenario state
 * @return whether a control changed
 */
static bool _galaxy_controls_gui(DvzGui* gui, void* user)
{
    GalaxyState* state = (GalaxyState*)user;
    if (gui == NULL || state == NULL)
        return false;

    bool changed = false;
    changed |= dvz_gui_checkbox(gui, "Animate galaxy", &state->animate);
    changed |= dvz_gui_slider_float_format(
        gui, "Simulation speed", &state->million_years_per_second, 0.0f, 6.0f,
        "%.2f million years/s");
    changed |= dvz_gui_checkbox(gui, "Background stars", &state->show_background_stars);
    return changed;
}



/**
 * Apply live galaxy controls that affect retained visual state.
 *
 * @param user galaxy scenario state
 */
static void _galaxy_controls_apply(void* user)
{
    GalaxyState* state = (GalaxyState*)user;
    if (state == NULL || state->background_stars == NULL)
        return;
    (void)dvz_visual_set_visible(state->background_stars, state->show_background_stars);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained density-wave galaxy scenario.
 *
 * @param ctx scenario context
 * @param out_user output scenario state
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    GalaxyState* state = dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("Galaxy settings");
    _galaxy_controls_reset(state);

    ok = galaxy_model_init(&state->model, 0x6A09E667u);
    EXAMPLE_CHECK(ok, "galaxy model initialization failed");
    state->angles = dvz_calloc(state->model.particle_count, sizeof(*state->angles));
    state->symbols = dvz_calloc(state->model.particle_count, sizeof(*state->symbols));
    EXAMPLE_CHECK(
        state->angles != NULL && state->symbols != NULL,
        "galaxy marker attribute allocation failed");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    example_tuner_figure(&state->tuner, ctx->figure);
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    dvz_panel_set_background_color(panel, dvz_color_rgba(0, 0, 8, 255));

    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 0.0f;
    camera.view.eye[1] = -3.0f;
    camera.view.eye[2] = +4.8f;
    camera.view.target[0] = 0.0f;
    camera.view.target[1] = 0.0f;
    camera.view.target[2] = 0.0f;
    camera.view.up[0] = 0.0f;
    camera.view.up[1] = 1.0f;
    camera.view.up[2] = 0.0f;
    camera.projection.fov_y = 0.55f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
    DvzResult rc = dvz_panel_set_camera_desc(panel, &camera);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy camera setup failed");

    state->background_stars = _create_background_stars(ctx->scene);
    EXAMPLE_CHECK(state->background_stars != NULL, "galaxy background star creation failed");
    rc = dvz_panel_add_visual(panel, state->background_stars, NULL);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy background star attachment failed");

    DvzSymbolSet* symbol_set = dvz_symbol_set(ctx->scene, 0);
    EXAMPLE_CHECK(symbol_set != NULL, "dvz_symbol_set() failed");
    DvzColor sprite[SPRITE_SIZE * SPRITE_SIZE] = {{0}};
    _fill_particle_sprite(sprite);
    DvzSymbolId sprite_id = dvz_symbol_bitmap(
        symbol_set, "galaxy_soft_particle", sprite, SPRITE_SIZE, SPRITE_SIZE, NULL);
    EXAMPLE_CHECK(sprite_id != DVZ_SYMBOL_ID_INVALID, "galaxy particle symbol creation failed");
    for (uint32_t i = 0; i < state->model.particle_count; i++)
        state->symbols[i] = sprite_id;

    state->visual = dvz_marker(ctx->scene, 0);
    EXAMPLE_CHECK(state->visual != NULL, "dvz_marker() failed");
    rc = dvz_marker_set_symbols(state->visual, symbol_set);
    EXAMPLE_CHECK(rc == DVZ_OK, "dvz_marker_set_symbols() failed");
    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    rc = dvz_marker_set_style(state->visual, &style);
    EXAMPLE_CHECK(rc == DVZ_OK, "dvz_marker_set_style() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position",
         .data = state->model.positions,
         .item_count = state->model.particle_count},
        {.attr_name = "color",
         .data = state->model.colors,
         .item_count = state->model.particle_count},
        {.attr_name = "diameter_px",
         .data = state->model.sizes,
         .item_count = state->model.particle_count},
        {.attr_name = "angle", .data = state->angles, .item_count = state->model.particle_count},
        {.attr_name = "symbol", .data = state->symbols, .item_count = state->model.particle_count},
    };
    rc = dvz_visual_set_data_many(state->visual, updates, 5);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy marker upload failed");
    rc = dvz_visual_set_alpha_mode(state->visual, DVZ_ALPHA_BLENDED);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy alpha mode setup failed");
    rc = dvz_visual_set_blend_mode(state->visual, DVZ_BLEND_ADDITIVE);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy additive blend setup failed");
    rc = dvz_visual_set_depth_test(state->visual, false);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy depth setup failed");
    rc = dvz_panel_add_visual(panel, state->visual, NULL);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy visual attachment failed");

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(controller != NULL, "dvz_arcball() failed");
    state->arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(state->arcball != NULL, "dvz_controller_arcball() failed");
    rc = dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy arcball binding failed");
    vec3 initial_angles = {-0.302710f, +0.044938f, -0.017917f};
    vec2 initial_pan = {0.0f, 0.0f};
    rc = dvz_arcball_initial(state->arcball, initial_angles);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy arcball initialization failed");
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, initial_angles, 7.389051f, initial_pan);
    example_tuner_camera(&state->tuner, "Camera", panel, &camera);
    (void)example_tuner_add_component(
        &state->tuner, "Simulation", state, NULL, _galaxy_controls_gui, _galaxy_controls_apply,
        _galaxy_controls_reset, NULL);

    state->elapsed_years = GALAXY_INITIAL_YEARS;
    ok = true;
cleanup:
    if (!ok && out_user == NULL)
        _free_state(state);
    return ok;
}



/**
 * Advance the density-wave orbital phases and update dynamic marker attributes.
 *
 * @param ctx scenario context
 * @param user_data galaxy scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user_data)
{
    GalaxyState* state = (GalaxyState*)user_data;
    if (ctx == NULL || state == NULL || state->visual == NULL)
        return;

    if (ctx->preview_mode)
    {
        state->elapsed_years = GALAXY_INITIAL_YEARS + 1000000.0 *
                                                          GALAXY_DEFAULT_MILLION_YEARS_PER_SECOND *
                                                          dvz_scenario_preview_time(ctx);
    }
    else if (state->animate)
    {
        const double dt = fmin(fmax(ctx->dt, 0.0), 0.1);
        state->elapsed_years += 1000000.0 * (double)state->million_years_per_second * dt;
    }
    galaxy_model_update(&state->model, state->elapsed_years);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position",
         .data = state->model.positions,
         .item_count = state->model.particle_count},
        {.attr_name = "diameter_px",
         .data = state->model.sizes,
         .item_count = state->model.particle_count},
    };
    (void)dvz_visual_set_data_many(state->visual, updates, 2);
}



/**
 * Destroy scenario-owned CPU state.
 *
 * @param ctx scenario context
 * @param user_data galaxy scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    _free_state((GalaxyState*)user_data);
}



/**
 * Attach the live-only galaxy tuner to the native view.
 *
 * @param ctx scenario context
 * @param app app
 * @param view native view
 * @param user_data galaxy scenario state
 * @return whether the tuner was attached or intentionally skipped
 */
static bool
_scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user_data)
{
    (void)app;
    GalaxyState* state = (GalaxyState*)user_data;
    if (ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
    {
        return true;
    }
    return example_tuner_attach(&state->tuner, view);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the density-wave galaxy scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_galaxy_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcases_galaxy",
        .title = "Density-Wave Galaxy",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_MARKER_VISUAL |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL | DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_galaxy_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
