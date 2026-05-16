/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* protein_sphere_ssao_glfw - preprocessed protein atoms rendered as SSAO sphere impostors.
 *
 * Prepare: python tools/preprocess_protein.py 1UBQ
 * Build:   just example-c protein_sphere_ssao_glfw
 * Run:     ./build/examples/c/protein_sphere_ssao_glfw
 * Smoke:   ./build/examples/c/protein_sphere_ssao_glfw ~/.cache/datoviz/proteins/1ubq 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1000u
#define HEIGHT 760u

#define DEFAULT_PDB_ID "1ubq"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ProteinBundle
{
    char path[1024];
    uint32_t atom_count;
    float* positions;
    float* radii;
    DvzColor* colors;
} ProteinBundle;


typedef struct ProteinExampleState
{
    DvzPanel* panel;
    DvzVisual* spheres;
    DvzArcball* arcball;
    const ProteinBundle* bundle;
    float* live_radii;
    bool ssao_enabled;
    bool msaa_enabled;
    bool msaa_alpha_to_coverage;
    float atom_scale;
    float ssao_radius;
    float ssao_strength;
    float ssao_bias;
    float ssao_power;
    float ssao_min_visibility;
    float ssao_samples;
    float msaa_samples;
    bool ssao_blur;
} ProteinExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Parse a bounded frame count from an optional command-line argument.
 *
 * @param text command-line text, or NULL
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(const char* text)
{
    if (text == NULL || text[0] == '\0')
        return 0;

    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/**
 * Return the default user-cache bundle path.
 *
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit in the output buffer
 */
static bool _default_bundle_path(char* out, size_t out_size)
{
    ANN(out);
    const char* home = getenv("HOME");
    if (home == NULL || home[0] == '\0')
        return false;
    int n = dvz_snprintf(
        out, out_size, "%s/.cache/datoviz/proteins/%s", home, DEFAULT_PDB_ID);
    return n > 0 && (size_t)n < out_size;
}



/**
 * Join a bundle directory with one filename.
 *
 * @param dir bundle directory
 * @param name child filename
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit in the output buffer
 */
static bool _join_path(const char* dir, const char* name, char* out, size_t out_size)
{
    ANN(dir);
    ANN(name);
    ANN(out);
    int n = dvz_snprintf(out, out_size, "%s/%s", dir, name);
    return n > 0 && (size_t)n < out_size;
}



/**
 * Return the size of a file in bytes.
 *
 * @param path input file path
 * @param out_size output byte size
 * @return whether the size was read
 */
static bool _file_size(const char* path, uint64_t* out_size)
{
    ANN(path);
    ANN(out_size);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return false;
    }
    long size = ftell(fp);
    fclose(fp);
    if (size < 0)
        return false;
    *out_size = (uint64_t)size;
    return true;
}



/**
 * Read one binary file into an existing buffer.
 *
 * @param path input file path
 * @param dst destination buffer
 * @param byte_size expected byte count
 * @return whether the full file was read
 */
static bool _read_file_exact(const char* path, void* dst, uint64_t byte_size)
{
    ANN(path);
    ANN(dst);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    size_t read = fread(dst, 1, (size_t)byte_size, fp);
    bool ok = read == (size_t)byte_size && ferror(fp) == 0;
    fclose(fp);
    return ok;
}



/**
 * Release all CPU arrays owned by a protein bundle.
 *
 * @param bundle protein bundle
 */
static void _protein_bundle_destroy(ProteinBundle* bundle)
{
    if (bundle == NULL)
        return;
    dvz_free(bundle->colors);
    dvz_free(bundle->radii);
    dvz_free(bundle->positions);
    dvz_memset(bundle, sizeof(ProteinBundle), 0, sizeof(ProteinBundle));
}



/**
 * Load the atom arrays exported by tools/preprocess_protein.py.
 *
 * @param dir protein bundle directory
 * @param out output bundle
 * @return whether the bundle was loaded
 */
static bool _protein_bundle_load(const char* dir, ProteinBundle* out)
{
    ANN(dir);
    ANN(out);
    dvz_memset(out, sizeof(ProteinBundle), 0, sizeof(ProteinBundle));
    dvz_snprintf(out->path, sizeof(out->path), "%s", dir);

    char position_path[1200] = {0};
    char radius_path[1200] = {0};
    char color_path[1200] = {0};
    if (!_join_path(dir, "atom_position.f32", position_path, sizeof(position_path)) ||
        !_join_path(dir, "atom_radius_vdw.f32", radius_path, sizeof(radius_path)) ||
        !_join_path(dir, "atom_color_element.rgba8", color_path, sizeof(color_path)))
    {
        return false;
    }

    uint64_t position_size = 0;
    uint64_t radius_size = 0;
    uint64_t color_size = 0;
    if (!_file_size(position_path, &position_size) || !_file_size(radius_path, &radius_size) ||
        !_file_size(color_path, &color_size))
    {
        return false;
    }
    if (position_size == 0 || position_size % (3u * sizeof(float)) != 0)
        return false;

    uint64_t atom_count64 = position_size / (3u * sizeof(float));
    if (atom_count64 > UINT32_MAX)
        return false;
    if (radius_size != atom_count64 * sizeof(float) || color_size != atom_count64 * sizeof(DvzColor))
        return false;

    out->atom_count = (uint32_t)atom_count64;
    out->positions = (float*)dvz_calloc(out->atom_count * 3u, sizeof(float));
    out->radii = (float*)dvz_calloc(out->atom_count, sizeof(float));
    out->colors = (DvzColor*)dvz_calloc(out->atom_count, sizeof(DvzColor));
    if (out->positions == NULL || out->radii == NULL || out->colors == NULL)
    {
        _protein_bundle_destroy(out);
        return false;
    }
    if (!_read_file_exact(position_path, out->positions, position_size) ||
        !_read_file_exact(radius_path, out->radii, radius_size) ||
        !_read_file_exact(color_path, out->colors, color_size))
    {
        _protein_bundle_destroy(out);
        return false;
    }

    return true;
}



/**
 * Apply a visual scale to protein atom radii.
 *
 * @param bundle protein bundle
 * @param scale radius scale factor
 * @return newly allocated scaled radii, or NULL on error
 */
static float* _scaled_radii(const ProteinBundle* bundle, float scale)
{
    ANN(bundle);
    float* out = (float*)dvz_calloc(bundle->atom_count, sizeof(float));
    if (out == NULL)
        return NULL;
    for (uint32_t i = 0; i < bundle->atom_count; i++)
        out[i] = bundle->radii[i] * scale;
    return out;
}



/**
 * Update the retained sphere radii from the current atom scale.
 *
 * @param state example state
 */
static void _apply_atom_scale(ProteinExampleState* state)
{
    ANN(state);
    ANN(state->bundle);
    ANN(state->live_radii);

    if (state->atom_scale < 0.15f)
        state->atom_scale = 0.15f;
    if (state->atom_scale > 2.5f)
        state->atom_scale = 2.5f;
    for (uint32_t i = 0; i < state->bundle->atom_count; i++)
        state->live_radii[i] = state->bundle->radii[i] * state->atom_scale;
    if (dvz_sphere_size(state->spheres, 0, state->bundle->atom_count, state->live_radii) != 0)
        dvz_fprintf(stderr, "dvz_sphere_size() failed\n");
}



/**
 * Update the panel SSAO state from live controls.
 *
 * @param state example state
 */
static void _apply_ssao(ProteinExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->ssao_enabled)
    {
        (void)dvz_panel_set_ssao(state->panel, NULL);
        return;
    }
    if (state->ssao_samples < 4.0f)
        state->ssao_samples = 4.0f;
    if (state->ssao_samples > 32.0f)
        state->ssao_samples = 32.0f;

    (void)dvz_panel_set_ssao(
        state->panel,
        &(DvzSsaoDesc){
            .radius = state->ssao_radius,
            .strength = state->ssao_strength,
            .bias = state->ssao_bias,
            .power = state->ssao_power,
            .min_visibility = state->ssao_min_visibility,
            .blur_radius = 2.0f,
            .blur_depth_sigma = 0.65f,
            .blur_normal_sigma = 0.35f,
            .sample_count = (uint32_t)(state->ssao_samples + 0.5f),
            .blur_enabled = state->ssao_blur,
        });
}



/**
 * Update the panel MSAA state from live controls.
 *
 * @param state example state
 */
static void _apply_msaa(ProteinExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->msaa_enabled)
    {
        (void)dvz_panel_set_msaa(state->panel, NULL);
        return;
    }
    if (state->msaa_samples < 2.0f)
        state->msaa_samples = 2.0f;
    if (state->msaa_samples > 8.0f)
        state->msaa_samples = 8.0f;

    uint32_t sample_count = (uint32_t)(state->msaa_samples + 0.5f);
    if (sample_count <= 2)
        sample_count = 2;
    else if (sample_count <= 4)
        sample_count = 4;
    else
        sample_count = 8;
    state->msaa_samples = (float)sample_count;

    DvzMsaaDesc desc = dvz_msaa_desc();
    desc.sample_count = sample_count;
    desc.alpha_to_coverage = state->msaa_alpha_to_coverage;
    if (!dvz_panel_set_msaa(state->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_msaa() failed\n");
}



/**
 * Build the live protein controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void _protein_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    ANN(gui);
    ANN(win);
    ProteinExampleState* state = (ProteinExampleState*)user_data;
    ANN(state);

    if (dvz_gui_begin(gui, "Protein", NULL, 0))
    {
        dvz_gui_text(gui, state->bundle->path);

        bool atom_changed = false;
        atom_changed |= dvz_gui_slider_float(gui, "Atom scale", &state->atom_scale, 0.15f, 2.5f);
        if (atom_changed)
            _apply_atom_scale(state);

        bool msaa_changed = false;
        msaa_changed |= dvz_gui_checkbox(gui, "Enable MSAA", &state->msaa_enabled);
        msaa_changed |=
            dvz_gui_slider_float(gui, "MSAA samples", &state->msaa_samples, 2.0f, 8.0f);
        msaa_changed |=
            dvz_gui_checkbox(gui, "Alpha-to-coverage", &state->msaa_alpha_to_coverage);
        if (msaa_changed)
            _apply_msaa(state);

        bool ssao_changed = false;
        ssao_changed |= dvz_gui_checkbox(gui, "SSAO", &state->ssao_enabled);
        ssao_changed |= dvz_gui_slider_float(gui, "Radius", &state->ssao_radius, 0.05f, 4.0f);
        ssao_changed |= dvz_gui_slider_float(gui, "Strength", &state->ssao_strength, 0.0f, 6.0f);
        ssao_changed |= dvz_gui_slider_float(gui, "Bias", &state->ssao_bias, 0.0f, 0.12f);
        ssao_changed |= dvz_gui_slider_float(gui, "Power", &state->ssao_power, 0.1f, 8.0f);
        ssao_changed |= dvz_gui_slider_float(
            gui, "Min visibility", &state->ssao_min_visibility, 0.0f, 1.0f);
        ssao_changed |= dvz_gui_slider_float(gui, "Samples", &state->ssao_samples, 4.0f, 32.0f);
        ssao_changed |= dvz_gui_checkbox(gui, "Blur", &state->ssao_blur);
        if (ssao_changed)
            _apply_ssao(state);

        if (dvz_gui_button(gui, "Reset view"))
            dvz_arcball_reset(state->arcball);
    }
    dvz_gui_end(gui);
}



/**
 * Print a short message explaining how to create the default bundle.
 *
 * @param path expected bundle path
 */
static void _print_prepare_hint(const char* path)
{
    dvz_fprintf(stderr, "failed to load protein bundle at '%s'\n", path);
    dvz_fprintf(stderr, "prepare one with:\n");
    dvz_fprintf(stderr, "  python tools/preprocess_protein.py 1UBQ\n");
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the protein sphere SSAO example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    char default_path[1024] = {0};
    const char* bundle_path = NULL;
    const char* frame_arg = NULL;
    if (argc >= 2)
        bundle_path = argv[1];
    if (argc >= 3)
        frame_arg = argv[2];
    if (bundle_path == NULL)
    {
        if (!_default_bundle_path(default_path, sizeof(default_path)))
        {
            dvz_fprintf(stderr, "HOME is not set; pass a protein bundle path explicitly\n");
            return 1;
        }
        bundle_path = default_path;
    }

    ProteinBundle bundle = {0};
    if (!_protein_bundle_load(bundle_path, &bundle))
    {
        _print_prepare_hint(bundle_path);
        return 1;
    }

    float atom_scale = 0.535f;
    float* scaled_radii = _scaled_radii(&bundle, atom_scale);
    if (scaled_radii == NULL)
    {
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (figure == NULL || panel == NULL || spheres == NULL)
    {
        dvz_fprintf(stderr, "scene setup failed\n");
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.35f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.68f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    if (dvz_sphere_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0 ||
        dvz_sphere_position(spheres, 0, bundle.atom_count, bundle.positions) != 0 ||
        dvz_sphere_color(spheres, 0, bundle.atom_count, bundle.colors) != 0 ||
        dvz_sphere_size(spheres, 0, bundle.atom_count, scaled_radii) != 0 ||
        dvz_panel_add_visual(panel, spheres, NULL) != 0)
    {
        dvz_fprintf(stderr, "sphere visual setup failed\n");
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    dvz_visual_set_primitive_shading(
        spheres,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.25f, 0.65f, 0.72f},
            .ambient = 0.20f,
            .diffuse = 0.76f,
            .specular = 0.55f,
            .shininess = 80.0f,
        });
    dvz_panel_set_background_color(panel, 0.030f, 0.034f, 0.044f, 1.0f);
    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "protein_sphere_ssao_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }
    dvz_arcball_initial(arcball, (vec3){+0.70f, 0.0f, +0.30f});

    ProteinExampleState state = {
        .panel = panel,
        .spheres = spheres,
        .arcball = arcball,
        .bundle = &bundle,
        .live_radii = scaled_radii,
        .ssao_enabled = true,
        .msaa_enabled = true,
        .msaa_alpha_to_coverage = true,
        .atom_scale = atom_scale,
        .ssao_radius = 0.496f,
        .ssao_strength = 1.458f,
        .ssao_bias = 0.012f,
        .ssao_power = 2.153f,
        .ssao_min_visibility = 0.582f,
        .ssao_samples = 32.0f,
        .msaa_samples = 8.0f,
        .ssao_blur = true,
    };
    _apply_msaa(&state);
    _apply_ssao(&state);

    DvzGui* gui = dvz_app_window_gui(win, NULL);
    if (gui == NULL)
        dvz_fprintf(stderr, "warning: failed to attach GUI overlay\n");
    else
        dvz_app_window_set_gui_callback(win, _protein_gui, &state);

    dvz_fprintf(
        stderr, "loaded %" PRIu32 " atoms from %s\n", bundle.atom_count, bundle.path);
    dvz_app_run(app, _frame_count(frame_arg));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    dvz_free(scaled_radii);
    _protein_bundle_destroy(&bundle);
    return 0;
}
