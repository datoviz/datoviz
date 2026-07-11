/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example maps an RGBA8 sampled field onto a UV sphere mesh.
 *
 * What to look for: the texture pixels are generated as a 1024x512 color field, attached to the
 * mesh as the "texture" field, and combined with the sphere geometry's UV coordinates. Rotate the
 * live sphere with the arcball controller and check that longitude waves wrap cleanly while the
 * poles avoid radial artifacts. Textures are useful for scientific surfaces, maps, and instrument
 * images that belong on geometry instead of in a flat panel.
 *
 * Scenario: features_mesh_texture
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/mesh_texture
 * Run:    ./build/examples/c/features/mesh_texture --live
 * Smoke:  ./build/examples/c/features/mesh_texture --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/geom.h"
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
#define TEXTURE_WIDTH  1024u
#define TEXTURE_HEIGHT 512u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MeshTextureState
{
    DvzGeometry* geometry;
    DvzVisual* visual;
    DvzMaterialDesc material;
    DvzArcball* arcball;
    ExampleTuner tuner;
} MeshTextureState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a high-resolution procedural texture whose U coordinate wraps cleanly.
 *
 * UV-sphere poles collapse all longitudes to a single vertex. Fade longitude-dependent detail near
 * the poles so interpolation does not produce radial wedges there.
 *
 * @param pixels output RGBA8 texture
 */
static void _fill_texture(uint8_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4])
{
    for (uint32_t y = 0; y < TEXTURE_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < TEXTURE_WIDTH; x++)
        {
            const float u = (float)x / (float)TEXTURE_WIDTH;
            const float v = (float)y / (float)(TEXTURE_HEIGHT - 1u);
            const float pole_fade = powf(sinf((float)DVZ_PI * v), 1.75f);
            const float lon = pole_fade * sinf(TAU * 8.0f * u);
            const float wave = pole_fade * sinf(TAU * (3.0f * u + 1.5f * v));
            const float lat = 0.5f + 0.5f * cosf(TAU * 4.0f * v);
            const float polar_tint = 1.0f - 0.32f * pole_fade;
            const float value = polar_tint * (0.48f + 0.28f * lat + 0.16f * lon + 0.08f * wave);
            const uint32_t i = 4u * (y * TEXTURE_WIDTH + x);
            pixels[i + 0u] = (uint8_t)(18.0f + 58.0f * value);
            pixels[i + 1u] = (uint8_t)(58.0f + 170.0f * value);
            pixels[i + 2u] = (uint8_t)(96.0f + 144.0f * value);
            pixels[i + 3u] = 255u;
        }
    }
}



/**
 * Create a texture field.
 *
 * @param scene scene owning the field
 * @param pixels texture pixels
 * @return sampled field, or NULL on error
 */
static DvzSampledField* _add_texture(
    DvzScene* scene, const uint8_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4])
{
    DvzSampledField* texture = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = TEXTURE_WIDTH,
                   .height = TEXTURE_HEIGHT,
                   .depth = 1});
    if (texture == NULL)
        return NULL;
    if (dvz_sampled_field_set_data(
            texture, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                         .data = pixels,
                         .bytes_per_row = TEXTURE_WIDTH * 4u,
                         .rows_per_image = TEXTURE_HEIGHT}) != DVZ_OK)
        return NULL;
    return texture;
}



/**
 * Add one textured sphere mesh.
 *
 * @param scene scene owning objects
 * @param panel panel receiving the visual
 * @param texture sampled RGBA8 texture
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_textured_mesh(
    DvzScene* scene,
    DvzPanel* panel,
    DvzSampledField* texture,
    DvzMaterialDesc* material,
    DvzGeometry** out_geometry,
    DvzVisual** out_visual)
{
    DvzGeometry* sphere = dvz_geometry_sphere(&(DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = 0.82,
        .sectors = 128,
        .rings = 64,
        .color = {255, 255, 255, 255},
    });
    if (sphere == NULL)
        return false;
    if (out_geometry != NULL)
        *out_geometry = sphere;

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return false;
    if (dvz_mesh_set_geometry(visual, sphere) != 0)
        return false;
    dvz_geometry_destroy(sphere);
    if (out_geometry != NULL)
        *out_geometry = NULL;

    if (dvz_visual_set_material(visual, material) != 0)
        return false;
    if (dvz_visual_set_field(visual, "texture", texture) != DVZ_OK)
        return false;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        return false;
    if (out_visual != NULL)
        *out_visual = visual;
    return true;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the textured-mesh feature scenario.
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

    MeshTextureState* state = (MeshTextureState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;
    state->tuner = example_tuner("Textured mesh settings");
    state->material = example_default_phong_material_desc();

    uint8_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4] = {0};

    _fill_texture(pixels);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;
    example_tuner_figure(&state->tuner, ctx->figure);

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = example_default_3d_camera_desc(1.0f);
    if (dvz_panel_set_camera_desc(panel, &camera) != 0)
        return false;

    DvzSampledField* texture = _add_texture(ctx->scene, pixels);
    if (texture == NULL)
        return false;
    if (!_add_textured_mesh(
            ctx->scene, panel, texture, &state->material, &state->geometry, &state->visual))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    state->arcball = dvz_controller_arcball(controller);
    if (state->arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;

    vec3 arcball_angles = {0.0f, 0.0f, 0.0f};
    vec2 arcball_pan = {0.0f, 0.0f};
    example_tuner_camera(&state->tuner, "Camera", panel, &camera);
    example_tuner_arcball(
        &state->tuner, "Arcball", state->arcball, arcball_angles, 1.0f, arcball_pan);
    example_tuner_material(&state->tuner, "Material", state->visual, &state->material);
    return true;
}


static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    MeshTextureState* state = (MeshTextureState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}



/**
 * Destroy the textured-mesh feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    MeshTextureState* state = (MeshTextureState*)user;
    if (state == NULL)
        return;
    example_tuner_detach(&state->tuner);
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    dvz_free(state);
}



/**
 * Return the textured-mesh scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _mesh_texture_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_mesh_texture",
        .title = "Textured Mesh",
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
 * Run the textured-mesh feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _mesh_texture_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
