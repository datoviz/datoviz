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
 * thin stellar and dust layers, and use the wheel to inspect the core or the full disk.
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
#include "galaxy_model.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define SPRITE_SIZE             64u
#define GALAXY_YEARS_PER_SECOND 6000000.0
#define GALAXY_INITIAL_YEARS    100000.0



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_galaxy_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GalaxyState
{
    GalaxyModel model;
    DvzVisual* visual;
    float* angles;
    DvzSymbolId* symbols;
    double elapsed_years;
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
    dvz_free(state->symbols);
    dvz_free(state->angles);
    galaxy_model_destroy(&state->model);
    dvz_free(state);
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

    ok = galaxy_model_init(&state->model, 0x6A09E667u);
    EXAMPLE_CHECK(ok, "galaxy model initialization failed");
    state->angles = dvz_calloc(state->model.particle_count, sizeof(*state->angles));
    state->symbols = dvz_calloc(state->model.particle_count, sizeof(*state->symbols));
    EXAMPLE_CHECK(
        state->angles != NULL && state->symbols != NULL,
        "galaxy marker attribute allocation failed");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
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
    DvzArcball* arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(arcball != NULL, "dvz_controller_arcball() failed");
    rc = dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ);
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy arcball binding failed");
    rc = dvz_arcball_initial(arcball, (vec3){0.0f, 0.0f, 0.0f});
    EXAMPLE_CHECK(rc == DVZ_OK, "galaxy arcball initialization failed");

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
        state->elapsed_years =
            GALAXY_INITIAL_YEARS + GALAXY_YEARS_PER_SECOND * dvz_scenario_preview_time(ctx);
    }
    else
    {
        const double dt = fmin(fmax(ctx->dt, 0.0), 0.1);
        state->elapsed_years += GALAXY_YEARS_PER_SECOND * dt;
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
        .requirements = DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL |
                        DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_galaxy_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
