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
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_debug.h"
#include "example_gui_controls.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define DEFAULT_PDB_ID             "6m0j"
#define DEFAULT_BUNDLE_PATH        "data/examples/proteins/1ubq/prepared"
#define ROTATION_SPEED_RAD_PER_SEC 0.18f
#define DEFAULT_ATOM_SCALE         0.78f



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


typedef struct ProteinArgs
{
    const char* bundle_path;
    const char* frame_arg;
    bool spin;
} ProteinArgs;


typedef struct ProteinDiagnostics
{
    DvzPanel* panel;
    DvzExampleGuiEdlControls edl;
    DvzExampleGuiMsaaControls msaa;
    DvzExampleGuiSsaoControls ssao;
} ProteinDiagnostics;



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
 * Return the default bundle path.
 *
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit
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
 * Return whether text is a non-empty unsigned integer.
 *
 * @param text candidate text
 * @return true when text contains only digits
 */
static bool _is_uint_text(const char* text)
{
    if (text == NULL || text[0] == '\0')
        return false;
    for (size_t i = 0; text[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)text[i]))
            return false;
    }
    return true;
}



/**
 * Parse showcase arguments.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return parsed arguments
 */
static ProteinArgs _parse_args(int argc, char** argv)
{
    ProteinArgs args = {0};
    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if (strcmp(arg, "--spin") == 0)
        {
            args.spin = true;
        }
        else if (example_debug_arg(arg))
        {
            continue;
        }
        else if (args.bundle_path == NULL && args.frame_arg == NULL && _is_uint_text(arg))
        {
            args.frame_arg = arg;
        }
        else if (args.bundle_path == NULL)
        {
            args.bundle_path = arg;
        }
        else if (args.frame_arg == NULL)
        {
            args.frame_arg = arg;
        }
        else
        {
            dvz_fprintf(stderr, "warning: ignoring extra argument '%s'\n", arg);
        }
    }
    return args;
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
        {.attr_name = "stroke_width", .data = widths, .item_count = 6},
    };
    return dvz_visual_set_data_many(crosshair, crosshair_updates, 4) == 0;
}



/**
 * Snap an MSAA sample-count slider value to a supported sample count.
 *
 * @param value slider value
 * @return supported MSAA sample count
 */
static uint32_t _snap_msaa_samples(float value)
{
    if (value <= 2.0f)
        return 2;
    if (value <= 4.0f)
        return 4;
    if (value <= 8.0f)
        return 8;
    return 16;
}



/**
 * Apply the diagnostics EDL state to the protein panel.
 *
 * @param diagnostics diagnostics state
 */
static void _protein_apply_edl(ProteinDiagnostics* diagnostics)
{
    ANN(diagnostics);
    ANN(diagnostics->panel);
    if (!diagnostics->edl.enabled)
    {
        (void)dvz_panel_set_edl(diagnostics->panel, NULL);
        return;
    }

    DvzEdlDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc),
        .radius = diagnostics->edl.radius,
        .strength = diagnostics->edl.strength,
        .depth_scale = diagnostics->edl.depth_scale,
    };
    if (!dvz_panel_set_edl(diagnostics->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_edl() failed\n");
}



/**
 * Apply the diagnostics MSAA state to the protein panel.
 *
 * @param diagnostics diagnostics state
 */
static void _protein_apply_msaa(ProteinDiagnostics* diagnostics)
{
    ANN(diagnostics);
    ANN(diagnostics->panel);
    if (!diagnostics->msaa.enabled)
    {
        (void)dvz_panel_set_msaa(diagnostics->panel, NULL);
        return;
    }

    uint32_t sample_count = _snap_msaa_samples(diagnostics->msaa.samples);
    diagnostics->msaa.samples = (float)sample_count;
    DvzMsaaDesc desc = dvz_msaa_desc();
    desc.sample_count = sample_count;
    desc.alpha_to_coverage = diagnostics->msaa.alpha_to_coverage;
    if (!dvz_panel_set_msaa(diagnostics->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_msaa() failed\n");
}



/**
 * Apply the diagnostics SSAO state to the protein panel.
 *
 * @param diagnostics diagnostics state
 */
static void _protein_apply_ssao(ProteinDiagnostics* diagnostics)
{
    ANN(diagnostics);
    ANN(diagnostics->panel);
    if (!diagnostics->ssao.enabled)
    {
        (void)dvz_panel_set_ssao(diagnostics->panel, NULL);
        return;
    }

    DvzSsaoDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSsaoDesc),
        .radius = diagnostics->ssao.radius,
        .strength = diagnostics->ssao.strength,
        .bias = diagnostics->ssao.bias,
        .power = diagnostics->ssao.power,
        .min_visibility = diagnostics->ssao.min_visibility,
        .blur_radius = diagnostics->ssao.blur_radius,
        .blur_depth_sigma = diagnostics->ssao.blur_depth_sigma,
        .blur_normal_sigma = diagnostics->ssao.blur_normal_sigma,
        .sample_count = (uint32_t)(diagnostics->ssao.samples + 0.5f),
        .blur_enabled = diagnostics->ssao.blur,
        .debug_view = diagnostics->ssao.debug_view,
    };
    if (!dvz_panel_set_ssao(diagnostics->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_ssao() failed\n");
}



/**
 * Return initial diagnostics controls matching the showcase defaults.
 *
 * @param panel protein panel
 * @param msaa initial MSAA descriptor
 * @param ssao initial SSAO descriptor
 * @return initialized diagnostics state
 */
static ProteinDiagnostics
_protein_diagnostics(DvzPanel* panel, const DvzMsaaDesc* msaa, const DvzSsaoDesc* ssao)
{
    ANN(panel);
    ANN(msaa);
    ANN(ssao);
    return (ProteinDiagnostics){
        .panel = panel,
        .edl =
            {
                .enabled = false,
                .radius = 2.0f,
                .strength = 55.0f,
                .depth_scale = 1.0f,
            },
        .msaa =
            {
                .enabled = msaa->enabled,
                .alpha_to_coverage = msaa->alpha_to_coverage,
                .samples = (float)msaa->sample_count,
                .min_samples = 2.0f,
                .max_samples = 16.0f,
            },
        .ssao =
            {
                .enabled = true,
                .blur = ssao->blur_enabled,
                .debug_view = ssao->debug_view,
                .show_blur_sigmas = true,
                .show_debug_view = true,
                .radius = ssao->radius,
                .strength = ssao->strength,
                .bias = ssao->bias,
                .power = ssao->power,
                .min_visibility = ssao->min_visibility,
                .samples = (float)ssao->sample_count,
                .min_samples = 4.0f,
                .max_samples = 32.0f,
                .blur_radius = ssao->blur_radius,
                .blur_radius_max = 16.0f,
                .blur_depth_sigma = ssao->blur_depth_sigma,
                .blur_normal_sigma = ssao->blur_normal_sigma,
            },
    };
}



/**
 * Draw the optional protein diagnostics GUI.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data ProteinDiagnostics pointer
 */
static void _protein_diagnostics_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    ANN(gui);
    (void)win;
    ProteinDiagnostics* diagnostics = (ProteinDiagnostics*)user_data;
    if (diagnostics == NULL)
        return;

    if (dvz_gui_begin(gui, "Protein diagnostics", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Post-processing");
        if (dvz_example_gui_edl(gui, &diagnostics->edl))
            _protein_apply_edl(diagnostics);
        dvz_gui_separator_text(gui, "Antialiasing");
        if (dvz_example_gui_msaa(gui, &diagnostics->msaa))
            _protein_apply_msaa(diagnostics);
        dvz_gui_separator_text(gui, "SSAO");
        if (dvz_example_gui_ssao(gui, &diagnostics->ssao))
            _protein_apply_ssao(diagnostics);
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
 * Run the protein gallery showcase.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int status = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    float* scaled_radii = NULL;
    ExampleDebug debug = {0};
    ProteinDiagnostics diagnostics = {0};
    DvzExampleVisualSpin sphere_spin = {0};
    DvzExampleVisualSpin selection_spin = {0};
    DvzExampleVisualSpin crosshair_spin = {0};
    ProteinAtoms atoms = {0};
    char default_path[1024] = {0};
    ProteinArgs args = _parse_args(argc, argv);
    const char* bundle_path = args.bundle_path;
    if (bundle_path == NULL)
    {
        if (!_default_bundle_path(default_path, sizeof(default_path)))
        {
            dvz_fprintf(stderr, "HOME is not set; pass a protein bundle path explicitly\n");
            return 1;
        }
        bundle_path = default_path;
    }

    if (!_protein_atoms_load(bundle_path, &atoms))
    {
        _print_prepare_hint(bundle_path);
        goto cleanup;
    }

    scaled_radii = _scaled_radii(&atoms, DEFAULT_ATOM_SCALE);
    EXAMPLE_CHECK(scaled_radii != NULL, "failed to allocate scaled radii");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "scene setup failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.18f;
    camera_desc.eye[1] = -0.08f;
    camera_desc.eye[2] = 2.95f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.57f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    EXAMPLE_CHECK(
        dvz_panel_set_camera(panel, &camera_desc) != NULL, "dvz_panel_set_camera() failed");

    DvzVisual* spheres = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* selection = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    DvzVisual* crosshair = dvz_segment(scene, 0);
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
        {.attr_name = "position", .data = atoms.positions, .item_count = atoms.count},
        {.attr_name = "color", .data = atoms.colors, .item_count = atoms.count},
        {.attr_name = "radius", .data = scaled_radii, .item_count = atoms.count},
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
            selection, crosshair, &atoms, _selected_atom(&atoms), DEFAULT_ATOM_SCALE),
        "selection upload failed");

    DvzMsaaDesc msaa_desc = dvz_msaa_desc();
    msaa_desc.sample_count = 16;
    msaa_desc.alpha_to_coverage = true;
    (void)dvz_panel_set_msaa(panel, &msaa_desc);

    DvzSsaoDesc ssao_desc = {DVZ_STRUCT_INIT_FIELDS(DvzSsaoDesc),
        .radius = 0.72f,
        .strength = 1.82f,
        .bias = 0.007f,
        .power = 2.45f,
        .min_visibility = 0.42f,
        .blur_radius = 10.0f,
        .blur_depth_sigma = 0.65f,
        .blur_normal_sigma = 0.35f,
        .sample_count = 32,
        .blur_enabled = true,
    };
    (void)dvz_panel_set_ssao(panel, &ssao_desc);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "protein");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_view_bind_controller() failed");
    dvz_arcball_initial(arcball, (vec3){+0.790430f, -0.651732f, +0.810104f});

    EXAMPLE_CHECK(
        example_debug_setup(&debug, win, argc, argv, "protein"),
        "example_debug_setup() failed");
    example_debug_arcball(&debug, "protein", arcball);
    example_debug_camera(&debug, "protein", &camera_desc);
    if (example_debug_gui_requested())
    {
        diagnostics = _protein_diagnostics(panel, &msaa_desc, &ssao_desc);
        DvzGui* gui = dvz_view_gui(win, NULL);
        EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
        dvz_view_set_gui_callback(win, _protein_diagnostics_gui, &diagnostics);
        dvz_fprintf(stderr, "example debug: protein diagnostics GUI enabled\n");
    }

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    EXAMPLE_CHECK(
        example_visual_spin(
            scene, spheres, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &sphere_spin),
        "example_visual_spin(spheres) failed");
    EXAMPLE_CHECK(
        example_visual_spin(
            scene, selection, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &selection_spin),
        "example_visual_spin(selection) failed");
    EXAMPLE_CHECK(
        example_visual_spin(
            scene, crosshair, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
            arcball_controller, &crosshair_spin),
        "example_visual_spin(crosshair) failed");
    if (args.spin)
    {
        example_visual_spin_start(&sphere_spin, 0.0);
        example_visual_spin_start(&selection_spin, 0.0);
        example_visual_spin_start(&crosshair_spin, 0.0);
    }
    else
    {
        example_visual_spin_stop(&sphere_spin);
        example_visual_spin_stop(&selection_spin);
        example_visual_spin_stop(&crosshair_spin);
    }

    dvz_fprintf(stderr, "loaded %" PRIu32 " atoms from %s\n", atoms.count, atoms.path);
    dvz_app_run(app, example_frame_count_from_text(args.frame_arg));
    status = 0;

cleanup:
    example_debug_uninstall(&debug);
    if (app != NULL)
        dvz_app_destroy(app);
    example_visual_spin_destroy(&crosshair_spin);
    example_visual_spin_destroy(&selection_spin);
    example_visual_spin_destroy(&sphere_spin);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(scaled_radii);
    _protein_atoms_destroy(&atoms);
    return status;
}
