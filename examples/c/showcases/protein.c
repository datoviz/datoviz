/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* protein - RCSB PDB protein structure rendered as clustered spheres.
 *
 * Scenario: protein_arcball_viewer
 * Style: showcase scientific, graphite_cyan, 1600x1200 capture target
 *
 * Data:    RCSB PDB structure data. The default cache target is 6M0J; the repository fallback is
 *          data/examples/proteins/1ubq/prepared, generated from RCSB PDB 1UBQ.
 * Source:  https://files.rcsb.org/download/{PDB_ID}.pdb
 * Terms:   RCSB PDB data usage policy applies.
 * Prepare: python tools/data/prepare_protein_arcball.py 1UBQ --regenerate
 *          python tools/preprocess_protein.py 6M0J
 * Build:   just example-c showcases/protein
 * Run:     ./build/examples/c/showcases/protein
 * Smoke:   ./build/examples/c/showcases/protein 60
 * Options: --spin, --debug, [bundle-path], [frame-count]
 * Debug:   DVZ_EXAMPLE_DEBUG=gui enables post-processing diagnostics
 *
 * The full interactive GUI workbench lives in examples/c/lab/protein_viewer.c.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define DEFAULT_PDB_ID             "6m0j"
#define DEFAULT_BUNDLE_PATH        "data/examples/proteins/1ubq/prepared"
#define ROTATION_SPEED_RAD_PER_SEC 0.18f
#define DEFAULT_ATOM_SCALE         0.4f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ProteinAtoms
{
    char path[1024];
    uint32_t count;
    float* positions;
    float* radii;
    DvzColor* colors;
} ProteinAtoms;


typedef struct ProteinState
{
    ProteinAtoms atoms;
    float* scaled_radii;
    DvzExampleVisualSpin sphere_spin;
    DvzExampleVisualSpin selection_spin;
    DvzExampleVisualSpin crosshair_spin;
} ProteinState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_protein_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Join a directory and child filename.
 *
 * @param dir parent directory
 * @param name child filename
 * @param out output path
 * @param out_size output path size
 * @return whether the result fits
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
 * Return the size of a file.
 *
 * @param path file path
 * @param out_size output size in bytes
 * @return whether the size was read
 */
static bool _file_size(const char* path, uint64_t* out_size)
{
    ANN(path);
    ANN(out_size);
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return false;
    if (fseeko(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return false;
    }
    off_t end = ftello(f);
    fclose(f);
    if (end < 0)
        return false;
    *out_size = (uint64_t)end;
    return true;
}



/**
 * Read an entire file into an existing buffer.
 *
 * @param path file path
 * @param dst destination buffer
 * @param size number of bytes to read
 * @return whether the exact byte count was read
 */
static bool _read_file_exact(const char* path, void* dst, uint64_t size)
{
    ANN(path);
    ANN(dst);
    if (size > SIZE_MAX)
        return false;
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return false;
    size_t read = fread(dst, 1, (size_t)size, f);
    bool ok = read == (size_t)size && ferror(f) == 0;
    fclose(f);
    return ok;
}



/**
 * Return a cache path for one PDB id.
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
 * Return whether a bundle directory contains the required atom data.
 *
 * @param dir prepared protein bundle directory
 * @return whether the bundle has required atom arrays
 */
static bool _bundle_exists(const char* dir)
{
    ANN(dir);
    char position_path[1200] = {0};
    uint64_t position_size = 0;
    return _join_path(dir, "atom_position.f32", position_path, sizeof(position_path)) &&
           _file_size(position_path, &position_size) && position_size > 0;
}



/**
 * Return the default bundle path.
 *
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit
 */
static bool _default_bundle_path(char* out, size_t out_size)
{
    ANN(out);
    if (_cache_bundle_path(DEFAULT_PDB_ID, out, out_size) && _bundle_exists(out))
        return true;
    int n = dvz_snprintf(out, out_size, "%s", DEFAULT_BUNDLE_PATH);
    return n > 0 && (size_t)n < out_size;
}



/**
 * Return the strict gallery palette color for one element-style atom color.
 *
 * @param src source atom color
 * @return palette color
 */
static DvzColor _showcase_atom_color(DvzColor src)
{
    if (src.r > 210 && src.g > 210 && src.b > 210)
        return example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    if (src.r > 180 && src.g < 130 && src.b < 130)
        return example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR);
    if (src.b > src.r + 30 && src.b > src.g + 10)
        return example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    if (src.g > src.r + 20 && src.g > src.b + 10)
        return example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    if (src.r > 170 && src.g > 120 && src.b < 120)
        return example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    return example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
}



/**
 * Release loaded atom arrays.
 *
 * @param atoms loaded atoms
 */
static void _protein_atoms_destroy(ProteinAtoms* atoms)
{
    if (atoms == NULL)
        return;
    dvz_free(atoms->colors);
    dvz_free(atoms->radii);
    dvz_free(atoms->positions);
    dvz_memset(atoms, sizeof(ProteinAtoms), 0, sizeof(ProteinAtoms));
}



/**
 * Load prepared atom arrays and map colors to the showcase palette.
 *
 * @param dir prepared protein bundle directory
 * @param out output atoms
 * @return whether loading succeeded
 */
static bool _protein_atoms_load(const char* dir, ProteinAtoms* out)
{
    ANN(dir);
    ANN(out);
    dvz_memset(out, sizeof(ProteinAtoms), 0, sizeof(ProteinAtoms));
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

    uint64_t count64 = position_size / (3u * sizeof(float));
    if (count64 > UINT32_MAX || radius_size != count64 * sizeof(float) ||
        color_size != count64 * sizeof(DvzColor))
    {
        return false;
    }

    out->count = (uint32_t)count64;
    out->positions = (float*)dvz_calloc((size_t)out->count * 3u, sizeof(float));
    out->radii = (float*)dvz_calloc(out->count, sizeof(float));
    out->colors = (DvzColor*)dvz_calloc(out->count, sizeof(DvzColor));
    DvzColor* element_colors = (DvzColor*)dvz_calloc(out->count, sizeof(DvzColor));
    if (out->positions == NULL || out->radii == NULL || out->colors == NULL ||
        element_colors == NULL)
    {
        dvz_free(element_colors);
        _protein_atoms_destroy(out);
        return false;
    }

    bool ok = _read_file_exact(position_path, out->positions, position_size) &&
              _read_file_exact(radius_path, out->radii, radius_size) &&
              _read_file_exact(color_path, element_colors, color_size);
    if (!ok)
    {
        dvz_free(element_colors);
        _protein_atoms_destroy(out);
        return false;
    }

    for (uint32_t i = 0; i < out->count; i++)
    {
        out->colors[i] = _showcase_atom_color(element_colors[i]);
        out->colors[i].a = 255;
    }
    dvz_free(element_colors);
    return out->count > 0;
}



/**
 * Allocate scaled atom radii.
 *
 * @param atoms loaded atoms
 * @param scale radius scale
 * @return scaled radii or NULL on failure
 */
static float* _scaled_radii(const ProteinAtoms* atoms, float scale)
{
    ANN(atoms);
    float* out = (float*)dvz_calloc(atoms->count, sizeof(float));
    if (out == NULL)
        return NULL;
    for (uint32_t i = 0; i < atoms->count; i++)
        out[i] = atoms->radii[i] * scale;
    return out;
}



/**
 * Return a deterministic foreground atom for the gallery highlight.
 *
 * @param atoms loaded atoms
 * @return selected atom index
 */
static uint32_t _selected_atom(const ProteinAtoms* atoms)
{
    ANN(atoms);
    uint32_t best = 0;
    float best_score = -INFINITY;
    for (uint32_t i = 0; i < atoms->count; i++)
    {
        const float* p = &atoms->positions[3u * i];
        float score = p[0] + 0.42f * p[2] - 0.10f * fabsf(p[1]) + 0.12f * atoms->radii[i];
        if (score > best_score)
        {
            best_score = score;
            best = i;
        }
    }
    return best;
}



/**
 * Upload the selected atom halo and crosshair.
 *
 * @param selection selected atom sphere visual
 * @param crosshair crosshair segment visual
 * @param atoms loaded atoms
 * @param atom_index selected atom index
 * @param atom_scale atom scale factor
 * @return whether upload succeeded
 */
static bool _upload_selection(
    DvzVisual* selection, DvzVisual* crosshair, const ProteinAtoms* atoms, uint32_t atom_index,
    float atom_scale)
{
    ANN(selection);
    ANN(crosshair);
    ANN(atoms);
    if (atom_index >= atoms->count)
        return false;

    const float* p = &atoms->positions[3u * atom_index];
    vec3 pos[1] = {{p[0], p[1], p[2]}};
    float radius[1] = {atoms->radii[atom_index] * atom_scale * 1.32f};
    DvzColor amber = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    DvzColor halo_color[1] = {dvz_color_rgba(amber.r, amber.g, amber.b, 255)};
    DvzVisualDataUpdate halo_updates[] = {
        {.attr_name = "position", .data = pos, .item_count = 1},
        {.attr_name = "color", .data = halo_color, .item_count = 1},
        {.attr_name = "radius", .data = radius, .item_count = 1},
    };
    if (dvz_visual_set_data_many(selection, halo_updates, 3) != 0)
        return false;

    const float r = radius[0] * 2.15f;
    const float gap = radius[0] * 1.18f;
    vec3 starts[6] = {
        {p[0] - r, p[1], p[2]},   {p[0] + gap, p[1], p[2]}, {p[0], p[1] - r, p[2]},
        {p[0], p[1] + gap, p[2]}, {p[0], p[1], p[2] - r},   {p[0], p[1], p[2] + gap},
    };
    vec3 ends[6] = {
        {p[0] - gap, p[1], p[2]}, {p[0] + r, p[1], p[2]},   {p[0], p[1] - gap, p[2]},
        {p[0], p[1] + r, p[2]},   {p[0], p[1], p[2] - gap}, {p[0], p[1], p[2] + r},
    };
    DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor colors[6] = {
        dvz_color_rgba(amber.r, amber.g, amber.b, 245),
        dvz_color_rgba(amber.r, amber.g, amber.b, 245),
        dvz_color_rgba(cyan.r, cyan.g, cyan.b, 220),
        dvz_color_rgba(cyan.r, cyan.g, cyan.b, 220),
        dvz_color_rgba(cyan.r, cyan.g, cyan.b, 180),
        dvz_color_rgba(cyan.r, cyan.g, cyan.b, 180),
    };
    float widths[6] = {2.8f, 2.8f, 2.4f, 2.4f, 1.8f, 1.8f};
    DvzVisualDataUpdate crosshair_updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = 6},
        {.attr_name = "position_end", .data = ends, .item_count = 6},
        {.attr_name = "color", .data = colors, .item_count = 6},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = 6},
    };
    return dvz_visual_set_data_many(crosshair, crosshair_updates, 4) == 0;
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
 * Initialize the protein gallery showcase.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    ProteinState* state = (ProteinState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    char bundle_path[1024] = {0};
    EXAMPLE_CHECK(
        _default_bundle_path(bundle_path, sizeof(bundle_path)),
        "HOME is not set; cannot resolve the default protein bundle path");

    if (!_protein_atoms_load(bundle_path, &state->atoms))
    {
        _print_prepare_hint(bundle_path);
        goto cleanup;
    }

    state->scaled_radii = _scaled_radii(&state->atoms, DEFAULT_ATOM_SCALE);
    EXAMPLE_CHECK(state->scaled_radii != NULL, "failed to allocate scaled radii");

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(ctx->figure != NULL && panel != NULL, "scene setup failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[0] = 0.18f;
    camera_desc.view.eye[1] = -0.08f;
    camera_desc.view.eye[2] = 2.95f;
    camera_desc.view.up[1] = 1.0f;
    camera_desc.projection.fov_y = 0.57f;
    camera_desc.projection.near_clip = 0.05f;
    camera_desc.projection.far_clip = 100.0f;
    EXAMPLE_CHECK(
        dvz_panel_set_camera(panel, &camera_desc) != NULL, "dvz_panel_set_camera() failed");

    DvzVisual* spheres = dvz_sphere(ctx->scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* selection = dvz_sphere(ctx->scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* crosshair = dvz_segment(ctx->scene, 0);
    EXAMPLE_CHECK(
        spheres != NULL && selection != NULL && crosshair != NULL, "visual creation failed");
    EXAMPLE_CHECK(
        dvz_sphere_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0,
        "dvz_sphere_mode() failed");
    EXAMPLE_CHECK(
        dvz_sphere_mode(selection, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) == 0,
        "dvz_sphere_mode(selection) failed");

    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_STANDARD;
    material.light_direction[0] = 0.25f;
    material.light_direction[1] = 0.65f;
    material.light_direction[2] = 0.72f;
    material.standard.roughness = 0.36f;
    material.standard.specular = 0.68f;
    material.standard.rim_strength = 0.12f;
    (void)dvz_visual_set_material(spheres, &material);
    (void)dvz_visual_set_material(selection, &material);

    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position",
         .data = state->atoms.positions,
         .item_count = state->atoms.count},
        {.attr_name = "color", .data = state->atoms.colors, .item_count = state->atoms.count},
        {.attr_name = "radius", .data = state->scaled_radii, .item_count = state->atoms.count},
    };
    EXAMPLE_CHECK(
        dvz_visual_set_data_many(spheres, sphere_updates, 3) == 0,
        "dvz_visual_set_data_many() failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, spheres, NULL) == 0, "add spheres failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, selection, NULL) == 0, "add selection failed");

    EXAMPLE_CHECK(
        dvz_segment_set_caps(crosshair, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) == 0,
        "dvz_segment_set_caps() failed");
    (void)dvz_visual_set_depth_test(crosshair, false);
    (void)dvz_visual_set_alpha_mode(crosshair, DVZ_ALPHA_BLENDED);
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, crosshair, NULL) == 0, "add crosshair failed");
    EXAMPLE_CHECK(
        _upload_selection(
            selection, crosshair, &state->atoms, _selected_atom(&state->atoms),
            DEFAULT_ATOM_SCALE),
        "selection upload failed");

#ifndef DVZ_EXAMPLE_NO_MAIN
    DvzSsaoDesc ssao_desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzSsaoDesc),
        .radius = 0.72f,
        .strength = 1.82f,
        .bias = 0.007f,
        .power = 2.45f,
        .min_visibility = 0.42f,
        .blur_radius = 5.0f,
        .blur_depth_sigma = 0.65f,
        .blur_normal_sigma = 0.35f,
        .sample_count = 16,
        .blur_enabled = true,
    };
    (void)dvz_panel_set_ssao(panel, &ssao_desc);
#endif

    DvzController* arcball_controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");
    dvz_arcball_initial(arcball, (vec3){+0.790430f, -0.651732f, +0.810104f});

    EXAMPLE_CHECK(
        example_visual_spin(
            ctx->scene, spheres, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &state->sphere_spin),
        "example_visual_spin(spheres) failed");
    EXAMPLE_CHECK(
        example_visual_spin(
            ctx->scene, selection, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &state->selection_spin),
        "example_visual_spin(selection) failed");
    EXAMPLE_CHECK(
        example_visual_spin(
            ctx->scene, crosshair, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &state->crosshair_spin),
        "example_visual_spin(crosshair) failed");
    example_visual_spin_stop(&state->sphere_spin);
    example_visual_spin_stop(&state->selection_spin);
    example_visual_spin_stop(&state->crosshair_spin);

    dvz_fprintf(
        stderr, "loaded %" PRIu32 " atoms from %s\n", state->atoms.count, state->atoms.path);
    ok = true;
cleanup:
    return ok;
}



/**
 * Destroy the protein gallery showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ProteinState* state = (ProteinState*)user;
    if (state == NULL)
        return;

    example_visual_spin_destroy(&state->crosshair_spin);
    example_visual_spin_destroy(&state->selection_spin);
    example_visual_spin_destroy(&state->sphere_spin);
    dvz_free(state->scaled_radii);
    _protein_atoms_destroy(&state->atoms);
    dvz_free(state);
}



/**
 * Return the protein gallery showcase scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_protein_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "protein_arcball_viewer",
        .title = "protein",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/**
 * Run the protein gallery showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_protein_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
