/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* protein_viewer - full interactive protein workbench with GUI controls.
 *
 * Prepare: python tools/data/prepare_protein_arcball.py 1UBQ --regenerate
 * Build:   cmake --build build --target protein_viewer
 * Run:     ./build/examples/c/lab/protein_viewer
 * Smoke:   ./build/examples/c/lab/protein_viewer data/examples/proteins/1ubq/prepared 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <errno.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
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
#include "example_common.h"
#include "example_gui_controls.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define DEFAULT_PDB_ID "6m0j"
#define DEFAULT_BUNDLE_PATH "data/examples/proteins/1ubq/prepared"

#define ROTATION_SPEED_RAD_PER_SEC 0.22f
#define PROTEIN_RIBBON_DEFAULT_CROSS_SECTION_COUNT 24u

#define PROTEIN_RENDER_SPHERES 0
#define PROTEIN_RENDER_RIBBON  1

#define PROTEIN_ATOM_COLOR_ELEMENT 0
#define PROTEIN_ATOM_COLOR_CHAIN   1
#define PROTEIN_ATOM_COLOR_BFACTOR 2

#define PROTEIN_RIBBON_COLOR_CHAIN 0
#define PROTEIN_RIBBON_COLOR_SS    1

#define PROTEIN_PRESET_COUNT 4

static const char* PRESET_PDB_IDS[PROTEIN_PRESET_COUNT] = {"1CRN", "1UBQ", "4HHB", "6M0J"};


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
    uint32_t ribbon_cross_section_count;
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
    DvzSceneBuffer* ribbon_index_buffer;
    DvzArcball* arcball;
    DvzExampleVisualSpin sphere_spin;
    DvzExampleVisualSpin ribbon_spin;
    ProteinBundle* bundle;
    DvzIndex* ribbon_indices_upload;
    uint32_t ribbon_index_upload_count;
    int selected_molecule;
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
    float ssao_blur_radius;
    float msaa_samples;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float roughness;
    float rim_strength;
    bool ssao_blur;
} ProteinExampleState;



static void _apply_render_mode(ProteinExampleState* state);
static bool _cache_bundle_path(const char* pdb_id, char* out, size_t out_size);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the default bundle path.
 *
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit in the output buffer
 */
static bool _default_bundle_path(char* out, size_t out_size)
{
    ANN(out);
    if (_cache_bundle_path(DEFAULT_PDB_ID, out, out_size))
        return true;
    int n = dvz_snprintf(out, out_size, "%s", DEFAULT_BUNDLE_PATH);
    return n > 0 && (size_t)n < out_size;
}



/**
 * Return a bundle path for a given PDB id under the default cache directory.
 *
 * @param pdb_id PDB identifier
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fits
 */
static bool _cache_bundle_path(const char* pdb_id, char* out, size_t out_size)
{
    ANN(pdb_id);
    ANN(out);
    const char* home = getenv("HOME");
    if (home == NULL || home[0] == '\0')
        return false;

    char lower_id[16] = {0};
    size_t i = 0;
    for (; i < sizeof(lower_id) - 1 && pdb_id[i] != '\0'; ++i)
        lower_id[i] = (char)tolower((unsigned char)pdb_id[i]);
    lower_id[i] = '\0';
    if (lower_id[0] == '\0')
        return false;

    int n = dvz_snprintf(out, out_size, "%s/.cache/datoviz/proteins/%s", home, lower_id);
    return n > 0 && (size_t)n < out_size;
}



/**
 * Return the preset dropdown index for a loaded bundle path, or 0 if unknown.
 *
 * @param bundle_path current bundle path
 * @return preset index
 */
static int _pdb_preset_index(const char* bundle_path)
{
    ANN(bundle_path);
    for (int i = 0; i < PROTEIN_PRESET_COUNT; ++i)
    {
        char preset_path[1024] = {0};
        if (!_cache_bundle_path(PRESET_PDB_IDS[i], preset_path, sizeof(preset_path)))
            continue;
        if (strcmp(bundle_path, preset_path) == 0)
            return i;
    }
    return 0;
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
 * Infer the ribbon cross-section vertex count from the first indexed segment.
 *
 * @param indices ribbon index array
 * @param index_count number of indices
 * @param vertex_count number of ribbon vertices
 * @return inferred cross-section count, or zero when it cannot be inferred
 */
static uint32_t _infer_ribbon_cross_section_count(
    const DvzIndex* indices, uint32_t index_count, uint32_t vertex_count)
{
    if (indices == NULL || index_count < 6 || vertex_count < 3)
        return 0;

    uint32_t max_guess = vertex_count / 2u;
    for (uint32_t group = 0; group + 5u < index_count; group += 6u)
    {
        uint32_t i = group / 6u;
        if (i == 0 || i > max_guess)
            continue;
        if (indices[group + 1u] == 0 && indices[group] == i)
            return i + 1u;
    }
    if (vertex_count % PROTEIN_RIBBON_DEFAULT_CROSS_SECTION_COUNT == 0)
        return PROTEIN_RIBBON_DEFAULT_CROSS_SECTION_COUNT;
    return 0;
}



/**
 * Prepare a padded index upload so stale GPU index tails only draw degenerate triangles.
 *
 * @param state example state
 * @param bundle source protein bundle
 * @return whether a padded upload buffer is available
 */
static bool _prepare_ribbon_indices_upload(
    ProteinExampleState* state, const ProteinBundle* bundle)
{
    ANN(state);
    ANN(bundle);
    if (!bundle->has_ribbon || bundle->ribbon_indices == NULL || bundle->ribbon_index_count == 0)
        return false;

    uint32_t upload_count = state->ribbon_index_upload_count;
    if (upload_count < bundle->ribbon_index_count)
        upload_count = bundle->ribbon_index_count;
    if (upload_count == 0)
        return false;

    if (state->ribbon_indices_upload == NULL ||
        state->ribbon_index_upload_count < upload_count)
    {
        DvzIndex* upload = (DvzIndex*)dvz_calloc(upload_count, sizeof(DvzIndex));
        if (upload == NULL)
            return false;
        dvz_free(state->ribbon_indices_upload);
        state->ribbon_indices_upload = upload;
        state->ribbon_index_upload_count = upload_count;
    }

    dvz_memset(
        state->ribbon_indices_upload, upload_count * sizeof(DvzIndex), 0,
        upload_count * sizeof(DvzIndex));
    dvz_memcpy(
        state->ribbon_indices_upload, bundle->ribbon_index_count * sizeof(DvzIndex),
        bundle->ribbon_indices, bundle->ribbon_index_count * sizeof(DvzIndex));
    return true;
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
    out->ribbon_cross_section_count = _infer_ribbon_cross_section_count(
        out->ribbon_indices, out->ribbon_index_count, out->ribbon_vertex_count);
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
    if (dvz_visual_set_data(state->spheres, "radius", state->live_radii, state->bundle->atom_count) != 0)
        dvz_fprintf(stderr, "sphere radius update failed\n");
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
    if (dvz_visual_set_data(state->spheres, "color", colors, state->bundle->atom_count) != 0)
        dvz_fprintf(stderr, "sphere color update failed\n");
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
 * Reload the active bundle and refresh visual data.
 *
 * @param state example state
 * @param bundle_path path to the protein bundle directory
 * @return true on success
 */
static bool _reload_bundle(ProteinExampleState* state, const char* bundle_path)
{
    ANN(state);
    ANN(bundle_path);

    ProteinBundle next = {0};
    if (!_protein_bundle_load(bundle_path, &next))
    {
        dvz_fprintf(stderr, "failed to load bundle '%s'\n", bundle_path);
        return false;
    }

    float* next_radii = _scaled_radii(&next, state->atom_scale);
    if (next_radii == NULL)
    {
        _protein_bundle_destroy(&next);
        dvz_fprintf(stderr, "failed to allocate scaled radii for '%s'\n", bundle_path);
        return false;
    }

    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = next.positions, .item_count = next.atom_count},
        {.attr_name = "color",
         .data = _atom_colors(&next, state->atom_color_mode),
         .item_count = next.atom_count},
        {.attr_name = "radius", .data = next_radii, .item_count = next.atom_count},
    };
    int rc = dvz_visual_set_data_many(state->spheres, sphere_updates, 3);
    if (rc != 0)
    {
        _protein_bundle_destroy(&next);
        dvz_free(next_radii);
        return false;
    }

    if (state->ribbon != NULL)
    {
        if (next.has_ribbon)
        {
            if (state->ribbon_index_buffer == NULL)
            {
                _protein_bundle_destroy(&next);
                dvz_free(next_radii);
                return false;
            }
            if (!_prepare_ribbon_indices_upload(state, &next))
            {
                _protein_bundle_destroy(&next);
                dvz_free(next_radii);
                return false;
            }
            bool ok = dvz_scene_buffer_set_data(
                state->ribbon_index_buffer, state->ribbon_indices_upload,
                (uint64_t)state->ribbon_index_upload_count * sizeof(DvzIndex));
            DvzVisualDataUpdate ribbon_updates[] = {
                {.attr_name = "position",
                 .data = next.ribbon_positions,
                 .item_count = next.ribbon_vertex_count},
                {.attr_name = "color",
                 .data = _ribbon_colors(&next, state->ribbon_color_mode),
                 .item_count = next.ribbon_vertex_count},
                {.attr_name = "normal",
                 .data = next.ribbon_normals,
                 .item_count = next.ribbon_vertex_count},
            };
            rc = ok ? dvz_visual_set_data_many(state->ribbon, ribbon_updates, 3) : -1;
            if (rc != 0)
            {
                _protein_bundle_destroy(&next);
                dvz_free(next_radii);
                return false;
            }
        }
        else
        {
            dvz_visual_set_visible(state->ribbon, false);
        }
    }

    _protein_bundle_destroy(state->bundle);
    dvz_free(state->live_radii);
    *state->bundle = next;
    state->live_radii = next_radii;
    _apply_atom_scale(state);
    if (state->ribbon != NULL && state->bundle->has_ribbon)
        _apply_ribbon_color(state);
    _apply_render_mode(state);
    return true;
}



/**
 * Reload one of the preset molecules.
 *
 * @param state example state
 * @param index preset molecule index
 * @return true on success
 */
static bool _reload_preset_bundle(ProteinExampleState* state, int index)
{
    ANN(state);
    if (index < 0 || index >= PROTEIN_PRESET_COUNT)
        return false;
    char bundle_path[1024] = {0};
    if (!_cache_bundle_path(PRESET_PDB_IDS[index], bundle_path, sizeof(bundle_path)))
        return false;
    return _reload_bundle(state, bundle_path);
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
    if (state->render_mode == PROTEIN_RENDER_RIBBON && state->bundle->has_ribbon)
    {
        state->ribbon_color_mode = PROTEIN_RIBBON_COLOR_SS;
        _apply_ribbon_color(state);
    }
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
    if (state->ssao_blur_radius < 1.0f)
        state->ssao_blur_radius = 1.0f;
    if (state->ssao_blur_radius > 16.0f)
        state->ssao_blur_radius = 16.0f;

    (void)dvz_panel_set_ssao(
        state->panel,
        &(DvzSsaoDesc){DVZ_STRUCT_INIT_FIELDS(DvzSsaoDesc),
            .radius = state->ssao_radius,
            .strength = state->ssao_strength,
            .bias = state->ssao_bias,
            .power = state->ssao_power,
            .min_visibility = state->ssao_min_visibility,
            .blur_radius = state->ssao_blur_radius,
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
    if (state->msaa_samples > 16.0f)
        state->msaa_samples = 16.0f;

    uint32_t sample_count = (uint32_t)(state->msaa_samples + 0.5f);
    if (sample_count <= 2)
        sample_count = 2;
    else if (sample_count <= 4)
        sample_count = 4;
    else if (sample_count <= 8)
        sample_count = 8;
    else
        sample_count = 16;
    state->msaa_samples = (float)sample_count;

    DvzMsaaDesc desc = dvz_msaa_desc();
    desc.sample_count = sample_count;
    desc.alpha_to_coverage = state->msaa_alpha_to_coverage;
    if (!dvz_panel_set_msaa(state->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_msaa() failed\n");
}



/**
 * Update the protein spin animation from live controls.
 *
 * @param state example state
 */
static void _apply_spin(ProteinExampleState* state)
{
    ANN(state);
    if (state->sphere_spin.animation == NULL)
        return;
    if (state->spin_enabled)
    {
        dvz_anim_start(state->sphere_spin.animation, 0.0);
        dvz_anim_start(state->ribbon_spin.animation, 0.0);
    }
    else
    {
        dvz_anim_stop(state->sphere_spin.animation);
        dvz_anim_stop(state->ribbon_spin.animation);
    }
}



/**
 * Build the live protein controls.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _protein_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    ANN(gui);
    ANN(win);
    ProteinExampleState* state = (ProteinExampleState*)user_data;
    ANN(state);

    if (dvz_gui_begin(gui, "Protein", NULL, 0))
    {
        int selected_molecule = state->selected_molecule;
        if (igCombo_Str_arr(
                "Molecule", &selected_molecule, PRESET_PDB_IDS, PROTEIN_PRESET_COUNT, PROTEIN_PRESET_COUNT))
        {
            if (_reload_preset_bundle(state, selected_molecule))
            {
                state->selected_molecule = selected_molecule;
                dvz_arcball_reset(state->arcball);
            }
            else
            {
                dvz_fprintf(stderr, "failed to load %s\n", PRESET_PDB_IDS[selected_molecule]);
            }
        }

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
        bool material_changed = example_gui_material(gui, &material);
        state->standard_material = material.standard_material;
        state->ambient = material.ambient;
        state->diffuse = material.diffuse;
        state->specular = material.specular;
        state->shininess = material.shininess;
        state->roughness = material.roughness;
        state->rim_strength = material.rim_strength;
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

        dvz_gui_separator_text(gui, "MSAA");
        DvzExampleGuiMsaaControls msaa = {
            .enabled = state->msaa_enabled,
            .alpha_to_coverage = state->msaa_alpha_to_coverage,
            .samples = state->msaa_samples,
            .min_samples = 2.0f,
            .max_samples = 16.0f,
        };
        bool msaa_changed = example_gui_msaa(gui, &msaa);
        state->msaa_enabled = msaa.enabled;
        state->msaa_alpha_to_coverage = msaa.alpha_to_coverage;
        state->msaa_samples = msaa.samples;
        if (msaa_changed)
            _apply_msaa(state);

        dvz_gui_separator_text(gui, "SSAO");
        DvzExampleGuiSsaoControls ssao = {
            .enabled = state->ssao_enabled,
            .blur = state->ssao_blur,
            .radius = state->ssao_radius,
            .strength = state->ssao_strength,
            .bias = state->ssao_bias,
            .power = state->ssao_power,
            .min_visibility = state->ssao_min_visibility,
            .samples = state->ssao_samples,
            .min_samples = 4.0f,
            .max_samples = 32.0f,
            .blur_radius = state->ssao_blur_radius,
            .blur_radius_max = 16.0f,
        };
        bool ssao_changed = example_gui_ssao(gui, &ssao);
        state->ssao_enabled = ssao.enabled;
        state->ssao_blur = ssao.blur;
        state->ssao_radius = ssao.radius;
        state->ssao_strength = ssao.strength;
        state->ssao_bias = ssao.bias;
        state->ssao_power = ssao.power;
        state->ssao_min_visibility = ssao.min_visibility;
        state->ssao_samples = ssao.samples;
        state->ssao_blur_radius = ssao.blur_radius;
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
    dvz_fprintf(stderr, "  tools/preprocess_protein.py 6M0J\n");
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
    int status = 1;
    bool state_initialized = false;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    float* scaled_radii = NULL;
    ProteinExampleState state = {0};
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
        goto cleanup;
    }

    float atom_scale = 0.594f;
    scaled_radii = _scaled_radii(&bundle, atom_scale);
    EXAMPLE_CHECK(scaled_radii != NULL, "failed to allocate scaled radii");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    EXAMPLE_CHECK(figure != NULL && panel != NULL && spheres != NULL, "scene setup failed");
    DvzVisual* ribbon = NULL;
    DvzSceneBuffer* ribbon_index_buffer = NULL;
    DvzExampleVisualSpin sphere_spin = {0};
    DvzExampleVisualSpin ribbon_spin = {0};

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[2] = 3.35f;
    camera_desc.view.up[1] = 1.0f;
    camera_desc.projection.fov_y = 0.68f;
    camera_desc.projection.near_clip = 0.05f;
    camera_desc.projection.far_clip = 100.0f;
    DvzResult camera_rc = dvz_panel_set_camera_desc(panel, &camera_desc);
    EXAMPLE_CHECK(camera_rc == 0, "dvz_panel_set_camera_desc() failed");

    int rc = dvz_sphere_set_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    EXAMPLE_CHECK(rc == 0, "dvz_sphere_set_mode() failed");

    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = bundle.positions, .item_count = bundle.atom_count},
        {.attr_name = "color", .data = bundle.atom_colors_element, .item_count = bundle.atom_count},
        {.attr_name = "radius", .data = scaled_radii, .item_count = bundle.atom_count},
    };
    rc = dvz_visual_set_data_many(spheres, sphere_updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for spheres");

    rc = dvz_panel_add_visual(panel, spheres, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(spheres) failed");

    DvzMaterialDesc sphere_material = dvz_phong_material_desc();
    sphere_material.light_direction[0] = 0.25f;
    sphere_material.light_direction[1] = 0.65f;
    sphere_material.light_direction[2] = 0.72f;
    sphere_material.phong.ambient = 0.20f;
    sphere_material.phong.diffuse = 0.76f;
    sphere_material.phong.specular = 0.55f;
    sphere_material.phong.shininess = 80.0f;
    dvz_visual_set_material(spheres, &sphere_material);

    if (bundle.has_ribbon)
    {
        ribbon = dvz_mesh(scene, 0);
        ribbon_index_buffer = dvz_scene_buffer(
            scene, &(DvzSceneBufferDesc){
                       .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                       .stride = sizeof(DvzIndex),
                   });
        EXAMPLE_CHECK(ribbon != NULL, "dvz_mesh() failed for ribbon");
        EXAMPLE_CHECK(ribbon_index_buffer != NULL, "dvz_scene_buffer() failed for ribbon");

        bool ok = dvz_scene_buffer_set_data(
            ribbon_index_buffer, bundle.ribbon_indices,
            (uint64_t)bundle.ribbon_index_count * sizeof(DvzIndex));
        EXAMPLE_CHECK(ok, "dvz_scene_buffer_set_data() failed for ribbon");

        DvzVisualDataUpdate ribbon_updates[] = {
            {.attr_name = "position",
             .data = bundle.ribbon_positions,
             .item_count = bundle.ribbon_vertex_count},
            {.attr_name = "color",
             .data = bundle.ribbon_colors_chain,
             .item_count = bundle.ribbon_vertex_count},
            {.attr_name = "normal",
             .data = bundle.ribbon_normals,
             .item_count = bundle.ribbon_vertex_count},
        };
        rc = dvz_visual_set_data_many(ribbon, ribbon_updates, 3);
        EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for ribbon");

        ok = dvz_visual_set_buffer(ribbon, "index", ribbon_index_buffer);
        EXAMPLE_CHECK(ok, "dvz_visual_set_buffer() failed for ribbon");

        rc = dvz_panel_add_visual(panel, ribbon, NULL);
        EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(ribbon) failed");

        DvzMaterialDesc ribbon_material = dvz_phong_material_desc();
        ribbon_material.light_direction[0] = 0.25f;
        ribbon_material.light_direction[1] = 0.65f;
        ribbon_material.light_direction[2] = 0.72f;
        ribbon_material.phong.ambient = 0.26f;
        ribbon_material.phong.diffuse = 0.78f;
        ribbon_material.phong.specular = 0.35f;
        ribbon_material.phong.shininess = 48.0f;
        dvz_visual_set_material(ribbon, &ribbon_material);
        dvz_visual_set_visible(ribbon, false);
    }

    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.030f, 0.034f, 0.044f, 1.0f));
    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win =
        dvz_view_window(app, figure, WIDTH, HEIGHT, "protein_viewer");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_view_bind_controller() failed");
    dvz_arcball_initial(arcball, (vec3){+0.70f, 0.0f, +0.30f});
    dvz_scene_set_clock_mode(scene, DVZ_SCENE_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    rotation_desc.axis[1] = 1.0f;
    rotation_desc.speed_rad_per_sec = 1.0f;
    sphere_spin.rotation = dvz_track_rotation(&rotation_desc);
    EXAMPLE_CHECK(sphere_spin.rotation != NULL, "dvz_track_rotation(spheres) failed");
    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = sphere_spin.rotation;
    sphere_spin.animation = dvz_anim_visual_transform(scene, spheres, &transform_desc);
    EXAMPLE_CHECK(sphere_spin.animation != NULL, "dvz_anim_visual_transform(spheres) failed");
    dvz_anim_set_interaction_policy(
        sphere_spin.animation, arcball_controller, DVZ_ANIM_INTERACTION_PAUSE, 0.0);
    dvz_anim_set_speed(sphere_spin.animation, ROTATION_SPEED_RAD_PER_SEC);
    if (ribbon != NULL)
    {
        ribbon_spin.rotation = dvz_track_rotation(&rotation_desc);
        EXAMPLE_CHECK(ribbon_spin.rotation != NULL, "dvz_track_rotation(ribbon) failed");
        transform_desc = dvz_transform_motion_desc();
        transform_desc.rotation = ribbon_spin.rotation;
        ribbon_spin.animation = dvz_anim_visual_transform(scene, ribbon, &transform_desc);
        EXAMPLE_CHECK(
            ribbon_spin.animation != NULL, "dvz_anim_visual_transform(ribbon) failed");
        dvz_anim_set_interaction_policy(
            ribbon_spin.animation, arcball_controller, DVZ_ANIM_INTERACTION_PAUSE, 0.0);
        dvz_anim_set_speed(ribbon_spin.animation, ROTATION_SPEED_RAD_PER_SEC);
    }

    state = (ProteinExampleState){
        .panel = panel,
        .spheres = spheres,
        .ribbon = ribbon,
        .ribbon_index_buffer = ribbon_index_buffer,
        .arcball = arcball,
        .sphere_spin = sphere_spin,
        .ribbon_spin = ribbon_spin,
        .bundle = &bundle,
        .live_radii = scaled_radii,
        .ribbon_index_upload_count = bundle.ribbon_index_count,
        .selected_molecule = _pdb_preset_index(bundle.path),
        .render_mode = PROTEIN_RENDER_RIBBON,
        .atom_color_mode = PROTEIN_ATOM_COLOR_ELEMENT,
        .ribbon_color_mode = PROTEIN_RIBBON_COLOR_SS,
        .standard_material = true,
        .ssao_enabled = true,
        .msaa_enabled = true,
        .msaa_alpha_to_coverage = true,
        .spin_enabled = false,
        .atom_scale = atom_scale,
        .ssao_radius = 0.609f,
        .ssao_strength = 1.557f,
        .ssao_bias = 0.008f,
        .ssao_power = 2.261f,
        .ssao_min_visibility = 0.476f,
        .ssao_samples = 32.0f,
        .ssao_blur_radius = 11.259f,
        .msaa_samples = 16.0f,
        .ambient = 0.20f,
        .diffuse = 0.76f,
        .specular = 0.572f,
        .shininess = 80.0f,
        .roughness = 0.409f,
        .rim_strength = 0.024f,
        .ssao_blur = true,
    };
    state_initialized = true;
    _apply_render_mode(&state);
    _apply_material(&state);
    _apply_msaa(&state);
    _apply_ssao(&state);

    DvzGui* gui = dvz_view_gui(win, NULL);
    if (gui == NULL)
        dvz_fprintf(stderr, "warning: failed to attach GUI overlay\n");
    else
        dvz_view_set_gui_callback(win, _protein_gui, &state);

    dvz_fprintf(
        stderr, "loaded %" PRIu32 " atoms from %s\n", bundle.atom_count, bundle.path);
    dvz_app_run(app, example_frame_count_from_text(frame_arg));

    status = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_track_destroy(state.ribbon_spin.rotation);
    dvz_track_destroy(state.sphere_spin.rotation);
    if (!state_initialized)
    {
        dvz_track_destroy(ribbon_spin.rotation);
        dvz_track_destroy(sphere_spin.rotation);
    }
    if (scene != NULL)
        dvz_scene_destroy(scene);
    if (state_initialized)
    {
        dvz_free(state.ribbon_indices_upload);
        dvz_free(state.live_radii);
    }
    else
    {
        dvz_free(scaled_radii);
    }
    _protein_bundle_destroy(&bundle);
    return status;
}
