/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* protein_arcball_viewer_glfw - preprocessed protein atoms rendered with an arcball camera.
 *
 * Prepare: uv run --with rs-dssp tools/preprocess_protein.py 1UBQ --dssp
 * Build:   cmake --build build --target protein_arcball_viewer_glfw
 * Run:     ./build/examples/c/protein_arcball_viewer_glfw
 * Smoke:   ./build/examples/c/protein_arcball_viewer_glfw ~/.cache/datoviz/proteins/1ubq 60
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
#include "datoviz/imgui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1000u
#define HEIGHT 760u

#define DEFAULT_PDB_ID "1ubq"

#define ROTATION_SPEED_RAD_PER_SEC 0.22f

#define PROTEIN_RENDER_SPHERES 0
#define PROTEIN_RENDER_RIBBON  1

#define PROTEIN_ATOM_COLOR_ELEMENT 0
#define PROTEIN_ATOM_COLOR_CHAIN   1
#define PROTEIN_ATOM_COLOR_BFACTOR 2

#define PROTEIN_RIBBON_COLOR_CHAIN 0
#define PROTEIN_RIBBON_COLOR_SS    1



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ProteinBundle
{
    char path[1024];
    uint32_t atom_count;
    float* positions;
    float* radii;
    DvzColor* atom_colors_element;
    DvzColor* atom_colors_chain;
    DvzColor* atom_colors_bfactor;
    bool has_ribbon;
    uint32_t ribbon_vertex_count;
    uint32_t ribbon_index_count;
    float* ribbon_positions;
    float* ribbon_normals;
    DvzColor* ribbon_colors_chain;
    DvzColor* ribbon_colors_ss;
    DvzIndex* ribbon_indices;
} ProteinBundle;


typedef struct ProteinExampleState
{
    DvzPanel* panel;
    DvzVisual* spheres;
    DvzVisual* ribbon;
    DvzArcball* arcball;
    DvzAnimation* spin;
    const ProteinBundle* bundle;
    float* live_radii;
    int render_mode;
    int atom_color_mode;
    int ribbon_color_mode;
    bool standard_material;
    bool ssao_enabled;
    bool msaa_enabled;
    bool msaa_alpha_to_coverage;
    bool spin_enabled;
    float atom_scale;
    float ssao_radius;
    float ssao_strength;
    float ssao_bias;
    float ssao_power;
    float ssao_min_visibility;
    float ssao_samples;
    float msaa_samples;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float roughness;
    float rim_strength;
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
    dvz_free(bundle->ribbon_indices);
    dvz_free(bundle->ribbon_colors_ss);
    dvz_free(bundle->ribbon_colors_chain);
    dvz_free(bundle->ribbon_normals);
    dvz_free(bundle->ribbon_positions);
    dvz_free(bundle->atom_colors_bfactor);
    dvz_free(bundle->atom_colors_chain);
    dvz_free(bundle->atom_colors_element);
    dvz_free(bundle->radii);
    dvz_free(bundle->positions);
    dvz_memset(bundle, sizeof(ProteinBundle), 0, sizeof(ProteinBundle));
}



/**
 * Read an optional ribbon mesh from the exported bundle.
 *
 * @param dir protein bundle directory
 * @param out output bundle
 */
static void _protein_bundle_load_ribbon(const char* dir, ProteinBundle* out)
{
    ANN(dir);
    ANN(out);

    char position_path[1200] = {0};
    char normal_path[1200] = {0};
    char color_chain_path[1200] = {0};
    char color_ss_path[1200] = {0};
    char index_path[1200] = {0};
    if (!_join_path(dir, "ribbon_position.f32", position_path, sizeof(position_path)) ||
        !_join_path(dir, "ribbon_normal.f32", normal_path, sizeof(normal_path)) ||
        !_join_path(dir, "ribbon_color_chain.rgba8", color_chain_path, sizeof(color_chain_path)) ||
        !_join_path(dir, "ribbon_color_ss.rgba8", color_ss_path, sizeof(color_ss_path)) ||
        !_join_path(dir, "ribbon_index.u32", index_path, sizeof(index_path)))
    {
        return;
    }

    uint64_t position_size = 0;
    uint64_t normal_size = 0;
    uint64_t color_chain_size = 0;
    uint64_t color_ss_size = 0;
    uint64_t index_size = 0;
    if (!_file_size(position_path, &position_size) || !_file_size(normal_path, &normal_size) ||
        !_file_size(color_chain_path, &color_chain_size) ||
        !_file_size(color_ss_path, &color_ss_size) || !_file_size(index_path, &index_size))
    {
        return;
    }

    if (position_size == 0 || position_size % (3u * sizeof(float)) != 0 ||
        normal_size != position_size || index_size == 0 || index_size % sizeof(DvzIndex) != 0)
    {
        return;
    }

    uint64_t vertex_count64 = position_size / (3u * sizeof(float));
    uint64_t index_count64 = index_size / sizeof(DvzIndex);
    if (vertex_count64 > UINT32_MAX || index_count64 > UINT32_MAX)
        return;
    if (color_chain_size != vertex_count64 * sizeof(DvzColor) ||
        color_ss_size != vertex_count64 * sizeof(DvzColor))
    {
        return;
    }

    out->ribbon_vertex_count = (uint32_t)vertex_count64;
    out->ribbon_index_count = (uint32_t)index_count64;
    out->ribbon_positions = (float*)dvz_calloc((size_t)out->ribbon_vertex_count * 3u, sizeof(float));
    out->ribbon_normals = (float*)dvz_calloc((size_t)out->ribbon_vertex_count * 3u, sizeof(float));
    out->ribbon_colors_chain = (DvzColor*)dvz_calloc(out->ribbon_vertex_count, sizeof(DvzColor));
    out->ribbon_colors_ss = (DvzColor*)dvz_calloc(out->ribbon_vertex_count, sizeof(DvzColor));
    out->ribbon_indices = (DvzIndex*)dvz_calloc(out->ribbon_index_count, sizeof(DvzIndex));
    if (out->ribbon_positions == NULL || out->ribbon_normals == NULL ||
        out->ribbon_colors_chain == NULL || out->ribbon_colors_ss == NULL ||
        out->ribbon_indices == NULL)
    {
        _protein_bundle_destroy(out);
        return;
    }

    if (!_read_file_exact(position_path, out->ribbon_positions, position_size) ||
        !_read_file_exact(normal_path, out->ribbon_normals, normal_size) ||
        !_read_file_exact(color_chain_path, out->ribbon_colors_chain, color_chain_size) ||
        !_read_file_exact(color_ss_path, out->ribbon_colors_ss, color_ss_size) ||
        !_read_file_exact(index_path, out->ribbon_indices, index_size))
    {
        _protein_bundle_destroy(out);
        return;
    }

    out->has_ribbon = true;
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
    char color_element_path[1200] = {0};
    char color_chain_path[1200] = {0};
    char color_bfactor_path[1200] = {0};
    if (!_join_path(dir, "atom_position.f32", position_path, sizeof(position_path)) ||
        !_join_path(dir, "atom_radius_vdw.f32", radius_path, sizeof(radius_path)) ||
        !_join_path(dir, "atom_color_element.rgba8", color_element_path, sizeof(color_element_path)) ||
        !_join_path(dir, "atom_color_chain.rgba8", color_chain_path, sizeof(color_chain_path)) ||
        !_join_path(
            dir, "atom_color_bfactor.rgba8", color_bfactor_path, sizeof(color_bfactor_path)))
    {
        return false;
    }

    uint64_t position_size = 0;
    uint64_t radius_size = 0;
    uint64_t color_element_size = 0;
    uint64_t color_chain_size = 0;
    uint64_t color_bfactor_size = 0;
    if (!_file_size(position_path, &position_size) || !_file_size(radius_path, &radius_size) ||
        !_file_size(color_element_path, &color_element_size) ||
        !_file_size(color_chain_path, &color_chain_size) ||
        !_file_size(color_bfactor_path, &color_bfactor_size))
    {
        return false;
    }
    if (position_size == 0 || position_size % (3u * sizeof(float)) != 0)
        return false;

    uint64_t atom_count64 = position_size / (3u * sizeof(float));
    if (atom_count64 > UINT32_MAX)
        return false;
    if (radius_size != atom_count64 * sizeof(float) ||
        color_element_size != atom_count64 * sizeof(DvzColor) ||
        color_chain_size != atom_count64 * sizeof(DvzColor) ||
        color_bfactor_size != atom_count64 * sizeof(DvzColor))
    {
        return false;
    }

    out->atom_count = (uint32_t)atom_count64;
    out->positions = (float*)dvz_calloc((size_t)out->atom_count * 3u, sizeof(float));
    out->radii = (float*)dvz_calloc(out->atom_count, sizeof(float));
    out->atom_colors_element = (DvzColor*)dvz_calloc(out->atom_count, sizeof(DvzColor));
    out->atom_colors_chain = (DvzColor*)dvz_calloc(out->atom_count, sizeof(DvzColor));
    out->atom_colors_bfactor = (DvzColor*)dvz_calloc(out->atom_count, sizeof(DvzColor));
    if (out->positions == NULL || out->radii == NULL || out->atom_colors_element == NULL ||
        out->atom_colors_chain == NULL || out->atom_colors_bfactor == NULL)
    {
        _protein_bundle_destroy(out);
        return false;
    }
    if (!_read_file_exact(position_path, out->positions, position_size) ||
        !_read_file_exact(radius_path, out->radii, radius_size) ||
        !_read_file_exact(color_element_path, out->atom_colors_element, color_element_size) ||
        !_read_file_exact(color_chain_path, out->atom_colors_chain, color_chain_size) ||
        !_read_file_exact(color_bfactor_path, out->atom_colors_bfactor, color_bfactor_size))
    {
        _protein_bundle_destroy(out);
        return false;
    }

    _protein_bundle_load_ribbon(dir, out);
    return out->atom_count > 0;
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
 * Return the active atom color array.
 *
 * @param bundle protein bundle
 * @param mode atom color mode
 * @return atom color array
 */
static DvzColor* _atom_colors(const ProteinBundle* bundle, int mode)
{
    ANN(bundle);
    if (mode == PROTEIN_ATOM_COLOR_CHAIN)
        return bundle->atom_colors_chain;
    if (mode == PROTEIN_ATOM_COLOR_BFACTOR)
        return bundle->atom_colors_bfactor;
    return bundle->atom_colors_element;
}



/**
 * Return the active ribbon color array.
 *
 * @param bundle protein bundle
 * @param mode ribbon color mode
 * @return ribbon color array
 */
static DvzColor* _ribbon_colors(const ProteinBundle* bundle, int mode)
{
    ANN(bundle);
    if (mode == PROTEIN_RIBBON_COLOR_SS)
        return bundle->ribbon_colors_ss;
    return bundle->ribbon_colors_chain;
}



/**
 * Update sphere colors from the current color mode.
 *
 * @param state example state
 */
static void _apply_atom_color(ProteinExampleState* state)
{
    ANN(state);
    ANN(state->bundle);
    DvzColor* colors = _atom_colors(state->bundle, state->atom_color_mode);
    if (dvz_sphere_color(state->spheres, 0, state->bundle->atom_count, colors) != 0)
        dvz_fprintf(stderr, "dvz_sphere_color() failed\n");
}



/**
 * Update ribbon colors from the current color mode.
 *
 * @param state example state
 */
static void _apply_ribbon_color(ProteinExampleState* state)
{
    ANN(state);
    ANN(state->bundle);
    if (state->ribbon == NULL || !state->bundle->has_ribbon)
        return;
    DvzColor* colors = _ribbon_colors(state->bundle, state->ribbon_color_mode);
    dvz_visual_set_data(state->ribbon, "color", colors, state->bundle->ribbon_vertex_count);
}



/**
 * Update visual visibility from the current render mode.
 *
 * @param state example state
 */
static void _apply_render_mode(ProteinExampleState* state)
{
    ANN(state);
    ANN(state->spheres);

    if (state->render_mode == PROTEIN_RENDER_RIBBON && !state->bundle->has_ribbon)
        state->render_mode = PROTEIN_RENDER_SPHERES;
    dvz_visual_set_visible(state->spheres, state->render_mode == PROTEIN_RENDER_SPHERES);
    if (state->ribbon != NULL)
        dvz_visual_set_visible(state->ribbon, state->render_mode == PROTEIN_RENDER_RIBBON);
}



/**
 * Update sphere and ribbon materials from the current material controls.
 *
 * @param state example state
 */
static void _apply_material(ProteinExampleState* state)
{
    ANN(state);

    DvzMaterialDesc material = dvz_material_desc();
    material.light_direction[0] = 0.25f;
    material.light_direction[1] = 0.65f;
    material.light_direction[2] = 0.72f;
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

    if (state->spheres != NULL && dvz_visual_set_material(state->spheres, &material) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_material() failed for spheres\n");
    if (state->ribbon != NULL && dvz_visual_set_material(state->ribbon, &material) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_material() failed for ribbon\n");
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
 * Update the arcball spin animation from live controls.
 *
 * @param state example state
 */
static void _apply_spin(ProteinExampleState* state)
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

        const char* render_modes[] = {"Spheres", "Ribbon"};
        int render_mode_count = state->bundle->has_ribbon ? 2 : 1;
        if (igCombo_Str_arr("Rendering", &state->render_mode, render_modes, render_mode_count, 2))
            _apply_render_mode(state);

        if (state->render_mode == PROTEIN_RENDER_RIBBON && state->bundle->has_ribbon)
        {
            const char* color_modes[] = {"Chain", "Secondary structure"};
            if (igCombo_Str_arr("Coloring", &state->ribbon_color_mode, color_modes, 2, 2))
                _apply_ribbon_color(state);
        }
        else
        {
            const char* color_modes[] = {"Element", "Chain", "B-factor"};
            if (igCombo_Str_arr("Coloring", &state->atom_color_mode, color_modes, 3, 3))
                _apply_atom_color(state);
        }

        bool material_changed = false;
        material_changed |= dvz_gui_checkbox(gui, "Standard material", &state->standard_material);
        if (state->standard_material)
        {
            material_changed |=
                dvz_gui_slider_float(gui, "Roughness", &state->roughness, 0.02f, 1.0f);
            material_changed |=
                dvz_gui_slider_float(gui, "Specular", &state->specular, 0.0f, 1.5f);
            material_changed |= dvz_gui_slider_float(gui, "Rim", &state->rim_strength, 0.0f, 1.0f);
        }
        else
        {
            material_changed |=
                dvz_gui_slider_float(gui, "Ambient", &state->ambient, 0.0f, 1.0f);
            material_changed |=
                dvz_gui_slider_float(gui, "Diffuse", &state->diffuse, 0.0f, 1.5f);
            material_changed |=
                dvz_gui_slider_float(gui, "Specular", &state->specular, 0.0f, 1.5f);
            material_changed |=
                dvz_gui_slider_float(gui, "Shininess", &state->shininess, 1.0f, 160.0f);
        }
        if (material_changed)
            _apply_material(state);

        bool spin_changed = false;
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->spin_enabled);
        if (spin_changed)
            _apply_spin(state);

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
    dvz_fprintf(stderr, "  uv run --with rs-dssp tools/preprocess_protein.py 1UBQ --dssp\n");
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the protein arcball viewer example.
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
    DvzVisual* ribbon = NULL;

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
        dvz_sphere_color(spheres, 0, bundle.atom_count, bundle.atom_colors_element) != 0 ||
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

    if (bundle.has_ribbon)
    {
        ribbon = dvz_mesh(scene, 0);
        DvzSceneBuffer* index_buffer = dvz_scene_buffer(
            scene, &(DvzSceneBufferDesc){
                       .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                       .stride = sizeof(DvzIndex),
                   });
        if (ribbon == NULL || index_buffer == NULL ||
            !dvz_scene_buffer_set_data(
                index_buffer, bundle.ribbon_indices,
                (uint64_t)bundle.ribbon_index_count * sizeof(DvzIndex)) ||
            dvz_visual_set_data(
                ribbon, "position", bundle.ribbon_positions, bundle.ribbon_vertex_count) != 0 ||
            dvz_visual_set_data(ribbon, "normal", bundle.ribbon_normals, bundle.ribbon_vertex_count) !=
                0 ||
            dvz_visual_set_data(
                ribbon, "color", bundle.ribbon_colors_chain, bundle.ribbon_vertex_count) != 0 ||
            !dvz_visual_set_buffer(ribbon, "index", index_buffer) ||
            dvz_panel_add_visual(panel, ribbon, NULL) != 0)
        {
            dvz_fprintf(stderr, "ribbon visual setup failed\n");
            dvz_scene_destroy(scene);
            dvz_free(scaled_radii);
            _protein_bundle_destroy(&bundle);
            return 1;
        }

        dvz_visual_set_primitive_shading(
            ribbon,
            &(DvzPrimitiveShadingDesc){
                .light_direction = {0.25f, 0.65f, 0.72f},
                .ambient = 0.26f,
                .diffuse = 0.78f,
                .specular = 0.35f,
                .shininess = 48.0f,
            });
        dvz_visual_set_visible(ribbon, false);
    }

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
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "protein_arcball_viewer_glfw");
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
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        dvz_free(scaled_radii);
        _protein_bundle_destroy(&bundle);
        return 1;
    }

    ProteinExampleState state = {
        .panel = panel,
        .spheres = spheres,
        .ribbon = ribbon,
        .arcball = arcball,
        .spin = spin,
        .bundle = &bundle,
        .live_radii = scaled_radii,
        .render_mode = PROTEIN_RENDER_SPHERES,
        .atom_color_mode = PROTEIN_ATOM_COLOR_ELEMENT,
        .ribbon_color_mode = PROTEIN_RIBBON_COLOR_CHAIN,
        .standard_material = true,
        .ssao_enabled = true,
        .msaa_enabled = true,
        .msaa_alpha_to_coverage = true,
        .spin_enabled = false,
        .atom_scale = atom_scale,
        .ssao_radius = 0.496f,
        .ssao_strength = 1.458f,
        .ssao_bias = 0.012f,
        .ssao_power = 2.153f,
        .ssao_min_visibility = 0.582f,
        .ssao_samples = 32.0f,
        .msaa_samples = 8.0f,
        .ambient = 0.20f,
        .diffuse = 0.76f,
        .specular = 0.55f,
        .shininess = 80.0f,
        .roughness = 0.30f,
        .rim_strength = 0.12f,
        .ssao_blur = true,
    };
    _apply_material(&state);
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
