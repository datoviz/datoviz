/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* sphere - dense analytic sphere impostors with internal SSAO controls.
 *
 * Build:  just example-c visuals/sphere
 * Run:    ./build/examples/c/visuals/sphere
 * Smoke:  ./build/examples/c/visuals/sphere 60
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
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_gui_controls.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1000u
#define HEIGHT      760u
#define CLOUD_GRID  11u
#define MAX_SPHERES (CLOUD_GRID * CLOUD_GRID * CLOUD_GRID)

#define ROTATION_SPEED_RAD_PER_SEC 0.22f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SsaoExampleState
{
    DvzPanel* panel;
    DvzVisual* sphere;
    DvzAnimation* spin;
    float* base_sizes;
    float* live_sizes;
    uint32_t sphere_count;
    bool ssao_enabled;
    bool msaa_enabled;
    bool msaa_alpha_to_coverage;
    bool spin_enabled;
    bool raycast_mode;
    bool standard_material;
    float radius;
    float strength;
    float bias;
    float power;
    float min_visibility;
    float blur_radius;
    float blur_depth_sigma;
    float blur_normal_sigma;
    float sample_count;
    float msaa_sample_count;
    float size_scale;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float roughness;
    float rim_strength;
    bool blur_enabled;
    bool debug_view;
} SsaoExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;
    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/**
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Return a deterministic pseudo-random offset in [-0.5, +0.5].
 *
 * @param seed integer seed
 * @return deterministic offset
 */
static float _jitter(uint32_t seed)
{
    uint32_t x = seed * 747796405u + 2891336453u;
    x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
    x = (x >> 22u) ^ x;
    return ((float)(x & 0xFFFFu) / 65535.0f) - 0.5f;
}



/**
 * Build a deterministic compact cloud of overlapping sphere impostors.
 *
 * @param positions output sphere centers
 * @param colors output sphere colors
 * @param sizes output base sphere radii
 * @param max_count output array capacity
 * @return number of generated spheres
 */
static uint32_t
_build_sphere_cloud(float (*positions)[3], DvzColor* colors, float* sizes, uint32_t max_count)
{
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    uint32_t count = 0;
    const float inv = 1.0f / (float)(CLOUD_GRID - 1u);
    for (uint32_t k = 0; k < CLOUD_GRID; k++)
    {
        for (uint32_t j = 0; j < CLOUD_GRID; j++)
        {
            for (uint32_t i = 0; i < CLOUD_GRID; i++)
            {
                float x = -1.0f + 2.0f * (float)i * inv;
                float y = -1.0f + 2.0f * (float)j * inv;
                float z = -1.0f + 2.0f * (float)k * inv;
                float r = sqrtf(x * x + y * y + z * z);
                if (r > 1.03f || count >= max_count)
                    continue;

                uint32_t seed = i + 23u * j + 521u * k;
                positions[count][0] = 0.92f * x + 0.020f * _jitter(seed + 1u);
                positions[count][1] = 0.82f * y + 0.018f * _jitter(seed + 2u);
                positions[count][2] = 0.92f * z + 0.020f * _jitter(seed + 3u);

                float t = 0.5f + 0.5f * z;
                float band = 0.5f + 0.5f * sinf(9.0f * x + 5.0f * y);
                colors[count][0] = _u8(0.24f + 0.58f * t);
                colors[count][1] = _u8(0.34f + 0.36f * band);
                colors[count][2] = _u8(0.74f - 0.42f * t + 0.12f * band);
                colors[count][3] = 255;

                sizes[count] = 0.060f + 0.018f * (1.0f - r) + 0.010f * band;
                count++;
            }
        }
    }
    return count;
}



/**
 * Apply the current sphere-size multiplier to the retained visual.
 *
 * @param state example state
 */
static void _apply_sphere_sizes(SsaoExampleState* state)
{
    ANN(state);
    if (state->sphere == NULL || state->base_sizes == NULL || state->live_sizes == NULL ||
        state->sphere_count == 0)
    {
        return;
    }
    if (state->size_scale < 0.35f)
        state->size_scale = 0.35f;
    if (state->size_scale > 2.5f)
        state->size_scale = 2.5f;
    for (uint32_t i = 0; i < state->sphere_count; i++)
        state->live_sizes[i] = state->base_sizes[i] * state->size_scale;
    if (dvz_visual_set_data(state->sphere, "radius", state->live_sizes, state->sphere_count) != 0)
        dvz_fprintf(stderr, "sphere radius update failed\n");
}



/**
 * Apply the internal SSAO state to the panel.
 *
 * @param state example state
 */
static void _apply_ssao(SsaoExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->ssao_enabled)
    {
        (void)dvz_panel_set_ssao(state->panel, NULL);
        return;
    }
    if (state->sample_count < 4.0f)
        state->sample_count = 4.0f;
    if (state->sample_count > 32.0f)
        state->sample_count = 32.0f;

    DvzSsaoDesc desc = {
        .radius = state->radius,
        .strength = state->strength,
        .bias = state->bias,
        .power = state->power,
        .min_visibility = state->min_visibility,
        .blur_radius = state->blur_radius,
        .blur_depth_sigma = state->blur_depth_sigma,
        .blur_normal_sigma = state->blur_normal_sigma,
        .sample_count = (uint32_t)(state->sample_count + 0.5f),
        .blur_enabled = state->blur_enabled,
        .debug_view = state->debug_view,
    };
    if (!dvz_panel_set_ssao(state->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_ssao() failed\n");
}



/**
 * Apply the retained MSAA control to the panel.
 *
 * @param state example state
 */
static void _apply_msaa(SsaoExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->msaa_enabled)
    {
        (void)dvz_panel_set_msaa(state->panel, NULL);
        return;
    }
    if (state->msaa_sample_count < 2.0f)
        state->msaa_sample_count = 2.0f;
    if (state->msaa_sample_count > 8.0f)
        state->msaa_sample_count = 8.0f;
    uint32_t sample_count = (uint32_t)(state->msaa_sample_count + 0.5f);
    if (sample_count <= 2)
        sample_count = 2;
    else if (sample_count <= 4)
        sample_count = 4;
    else
        sample_count = 8;
    state->msaa_sample_count = (float)sample_count;

    DvzMsaaDesc desc = dvz_msaa_desc();
    desc.sample_count = sample_count;
    desc.alpha_to_coverage = state->msaa_alpha_to_coverage;
    if (!dvz_panel_set_msaa(state->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_msaa() failed\n");
}



/**
 * Apply the current sphere rendering mode to the retained visual.
 *
 * @param state example state
 */
static void _apply_sphere_mode(SsaoExampleState* state)
{
    ANN(state);
    if (state->sphere == NULL)
        return;
    DvzSphereMode mode = state->raycast_mode ? DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR :
                                               DVZ_SPHERE_MODE_FAST_IMPOSTOR;
    if (dvz_sphere_mode(state->sphere, mode) != 0)
        dvz_fprintf(stderr, "dvz_sphere_mode() failed\n");
}



/**
 * Apply the current material controls to the retained sphere visual.
 *
 * @param state example state
 */
static void _apply_material(SsaoExampleState* state)
{
    ANN(state);
    if (state->sphere == NULL)
        return;
    DvzMaterialDesc material = dvz_material_desc();
    material.light_direction[0] = 0.35f;
    material.light_direction[1] = 0.70f;
    material.light_direction[2] = 0.62f;
    if (state->standard_material)
    {
        material.model = DVZ_MATERIAL_MODEL_STANDARD;
        material.standard.roughness = state->roughness;
        material.standard.specular = state->specular;
        material.standard.rim_strength = state->rim_strength;
    }
    else
    {
        material.model = DVZ_MATERIAL_MODEL_PHONG;
        material.phong.ambient = state->ambient;
        material.phong.diffuse = state->diffuse;
        material.phong.specular = state->specular;
        material.phong.shininess = state->shininess;
    }
    if (dvz_visual_set_material(state->sphere, &material) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_material() failed\n");
}



/**
 * Apply the retained spin control to the scene animation.
 *
 * @param state example state
 */
static void _apply_spin(SsaoExampleState* state)
{
    ANN(state);
    if (state->spin == NULL)
        return;
    if (state->spin_enabled)
        dvz_anim_start(state->spin, 0.0);
    else
        dvz_anim_stop(state->spin);
}



/**
 * Reset the live controls to useful defaults.
 *
 * @param state example state
 */
static void _reset_controls(SsaoExampleState* state)
{
    ANN(state);
    state->ssao_enabled = true;
    state->msaa_enabled = true;
    state->msaa_alpha_to_coverage = true;
    state->spin_enabled = false;
    state->raycast_mode = true;
    state->standard_material = true;
    state->radius = 0.374f;
    state->strength = 1.061f;
    state->bias = 0.037f;
    state->power = 3.344f;
    state->min_visibility = 0.888f;
    state->blur_radius = 6.903f;
    state->blur_depth_sigma = 1.701f;
    state->blur_normal_sigma = 0.638f;
    state->sample_count = 32.0f;
    state->msaa_sample_count = 8.0f;
    state->size_scale = 0.607f;
    state->ambient = 0.18f;
    state->diffuse = 0.76f;
    state->specular = 0.85f;
    state->shininess = 96.0f;
    state->roughness = 0.30f;
    state->rim_strength = 0.12f;
    state->blur_enabled = true;
    state->debug_view = false;
    _apply_sphere_sizes(state);
    _apply_sphere_mode(state);
    _apply_material(state);
    _apply_msaa(state);
    _apply_ssao(state);
    _apply_spin(state);
}



/**
 * Build the live SSAO controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void _ssao_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    SsaoExampleState* state = (SsaoExampleState*)user_data;
    if (state == NULL)
        return;

    bool ssao_changed = false;
    bool msaa_changed = false;
    bool spin_changed = false;
    bool size_changed = false;
    bool mode_changed = false;
    bool material_changed = false;
    if (dvz_gui_begin(gui, "SSAO", NULL, 0))
    {
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->spin_enabled);
        mode_changed |= dvz_gui_checkbox(gui, "Raycast impostor", &state->raycast_mode);

        dvz_gui_separator_text(gui, "Sphere");
        size_changed |= dvz_gui_slider_float(gui, "Sphere size", &state->size_scale, 0.35f, 2.5f);

        dvz_gui_separator_text(gui, "Material");
        DvzExampleGuiMaterialControls material = {
            .standard_material = state->standard_material,
            .ambient = state->ambient,
            .diffuse = state->diffuse,
            .specular = state->specular,
            .shininess = state->shininess,
            .roughness = state->roughness,
            .rim_strength = state->rim_strength,
        };
        material_changed |= dvz_example_gui_material(gui, &material);
        state->standard_material = material.standard_material;
        state->ambient = material.ambient;
        state->diffuse = material.diffuse;
        state->specular = material.specular;
        state->shininess = material.shininess;
        state->roughness = material.roughness;
        state->rim_strength = material.rim_strength;

        dvz_gui_separator_text(gui, "MSAA");
        DvzExampleGuiMsaaControls msaa = {
            .enabled = state->msaa_enabled,
            .alpha_to_coverage = state->msaa_alpha_to_coverage,
            .samples = state->msaa_sample_count,
            .min_samples = 2.0f,
            .max_samples = 8.0f,
        };
        msaa_changed |= dvz_example_gui_msaa(gui, &msaa);
        state->msaa_enabled = msaa.enabled;
        state->msaa_alpha_to_coverage = msaa.alpha_to_coverage;
        state->msaa_sample_count = msaa.samples;

        dvz_gui_separator_text(gui, "SSAO");
        DvzExampleGuiSsaoControls ssao = {
            .enabled = state->ssao_enabled,
            .blur = state->blur_enabled,
            .debug_view = state->debug_view,
            .show_blur_sigmas = true,
            .show_debug_view = true,
            .radius = state->radius,
            .strength = state->strength,
            .bias = state->bias,
            .power = state->power,
            .min_visibility = state->min_visibility,
            .samples = state->sample_count,
            .min_samples = 4.0f,
            .max_samples = 32.0f,
            .blur_radius = state->blur_radius,
            .blur_radius_max = 8.0f,
            .blur_depth_sigma = state->blur_depth_sigma,
            .blur_normal_sigma = state->blur_normal_sigma,
        };
        ssao_changed |= dvz_example_gui_ssao(gui, &ssao);
        state->ssao_enabled = ssao.enabled;
        state->blur_enabled = ssao.blur;
        state->debug_view = ssao.debug_view;
        state->radius = ssao.radius;
        state->strength = ssao.strength;
        state->bias = ssao.bias;
        state->power = ssao.power;
        state->min_visibility = ssao.min_visibility;
        state->sample_count = ssao.samples;
        state->blur_radius = ssao.blur_radius;
        state->blur_depth_sigma = ssao.blur_depth_sigma;
        state->blur_normal_sigma = ssao.blur_normal_sigma;

        if (dvz_gui_button(gui, "Reset"))
        {
            _reset_controls(state);
            ssao_changed = false;
            msaa_changed = false;
            spin_changed = false;
            size_changed = false;
            mode_changed = false;
            material_changed = false;
        }
    }
    dvz_gui_end(gui);

    if (mode_changed)
        _apply_sphere_mode(state);
    if (material_changed)
        _apply_material(state);
    if (size_changed)
        _apply_sphere_sizes(state);
    if (ssao_changed)
        _apply_ssao(state);
    if (msaa_changed)
        _apply_msaa(state);
    if (spin_changed)
        _apply_spin(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.45f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float(*positions)[3] = (float(*)[3])dvz_calloc(MAX_SPHERES, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(MAX_SPHERES, sizeof(DvzColor));
    float* base_sizes = (float*)dvz_calloc(MAX_SPHERES, sizeof(float));
    float* live_sizes = (float*)dvz_calloc(MAX_SPHERES, sizeof(float));
    if (positions == NULL || colors == NULL || base_sizes == NULL || live_sizes == NULL)
    {
        dvz_fprintf(stderr, "sphere cloud allocation failed\n");
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    uint32_t sphere_count = _build_sphere_cloud(positions, colors, base_sizes, MAX_SPHERES);
    if (sphere_count == 0)
    {
        dvz_fprintf(stderr, "sphere cloud generation failed\n");
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* visual = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_sphere() failed\n");
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_visual_set_data(visual, "position", &positions[0][0], sphere_count) != 0 ||
        dvz_visual_set_data(visual, "color", colors, sphere_count) != 0 ||
        dvz_visual_set_data(visual, "radius", live_sizes, sphere_count) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "sphere visual setup failed\n");
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.035f, 0.040f, 0.052f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "sphere");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    if (arcball == NULL ||
        dvz_panel_bind_controller(panel, arcball_controller, DVZ_DIM_MASK_XYZ) != 0)
    {
        dvz_fprintf(stderr, "failed to create or bind arcball controller\n");
        dvz_app_destroy(app);
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_connect_input(panel, dvz_app_window_input(win));
    dvz_arcball_set(arcball, (vec3){+0.92f, 0.0f, +0.18f});
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    SsaoExampleState state = {
        .panel = panel,
        .sphere = visual,
        .spin = spin,
        .base_sizes = base_sizes,
        .live_sizes = live_sizes,
        .sphere_count = sphere_count,
    };
    _reset_controls(&state);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_free(live_sizes);
        dvz_free(base_sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _ssao_gui, &state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_free(live_sizes);
    dvz_free(base_sizes);
    dvz_free(colors);
    dvz_free(positions);
    dvz_scene_destroy(scene);
    return 0;
}
