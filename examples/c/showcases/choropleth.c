/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* choropleth - contiguous U.S. state population-density choropleth.
 *
 * Scenario: us_state_choropleth
 * Style: showcase scientific, polygon-set, 1600x1200 capture target
 *
 * Data:    U.S. Census Bureau 2024 cartographic state boundaries and Vintage 2025 resident
 *          population estimates, prepared into flat polygon-set arrays.
 * Source:  https://www2.census.gov/geo/tiger/GENZ2024/shp/cb_2024_us_state_20m.zip
 *          https://www2.census.gov/programs-surveys/popest/tables/2020-2025/state/totals/NST-EST2025-POP.xlsx
 * Terms:   U.S. Census Bureau public data; cite the Census Bureau as source.
 * Prepare: python tools/data/prepare_us_state_choropleth.py
 * Promote: python tools/data/prepare_us_state_choropleth.py --output data/examples/us_state_choropleth
 * Build:   just example-c showcases/choropleth
 * Run:     ./build/examples/c/showcases/choropleth --live
 * Smoke:   ./build/examples/c/showcases/choropleth --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define DEFAULT_DATA_BUNDLE  "data/examples/us_state_choropleth/prepared"
#define DEFAULT_CACHE_BUNDLE ".cache/datoviz/examples/us_state_choropleth/prepared"

#define METADATA_NAME    "metadata.tsv"
#define POINTS_NAME      "points_xy_f64.bin"
#define RINGS_NAME       "rings_u32.bin"
#define RING_FILL_NAME   "ring_fill_rgba8.bin"
#define RING_STROKE_NAME "ring_stroke_rgba8.bin"
#define RING_WIDTH_NAME  "ring_width_f32.bin"
#define RING_ID_NAME     "ring_id_u64.bin"

static const DvzColor CHOROPLETH_RAMP[5] = {
    {26, 35, 46, 235},
    {33, 99, 126, 235},
    {50, 160, 147, 235},
    {237, 191, 94, 235},
    {221, 96, 73, 235},
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ChoroplethRing
{
    uint32_t region_index;
    uint32_t point_first;
    uint32_t point_count;
} ChoroplethRing;


typedef struct ChoroplethBundle
{
    char path[1024];
    uint32_t region_count;
    uint32_t ring_count;
    uint32_t point_count;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double value_min;
    double value_max;
    double density_min;
    double density_max;
    ChoroplethRing* rings;
    dvec2* points;
    DvzColor* fill;
    DvzColor* stroke;
    float* widths;
    uint64_t* ids;
} ChoroplethBundle;


typedef struct ChoroplethState
{
    ChoroplethBundle bundle;
} ChoroplethState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_us_state_choropleth_scenario(void);



/*************************************************************************************************/
/*  Bundle loading                                                                               */
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
 * Return whether a file can be opened for reading.
 *
 * @param path file path
 * @return true when the file exists and is readable
 */
static bool _file_readable(const char* path)
{
    ANN(path);
    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    fclose(fp);
    return true;
}



/**
 * Destroy a loaded choropleth bundle.
 *
 * @param bundle bundle to clear
 */
static void _choropleth_bundle_destroy(ChoroplethBundle* bundle)
{
    if (bundle == NULL)
        return;
    dvz_free(bundle->ids);
    dvz_free(bundle->widths);
    dvz_free(bundle->stroke);
    dvz_free(bundle->fill);
    dvz_free(bundle->points);
    dvz_free(bundle->rings);
    dvz_memset(bundle, sizeof(ChoroplethBundle), 0, sizeof(ChoroplethBundle));
}



/**
 * Read one typed binary array with an exact expected byte size.
 *
 * @param dir prepared bundle directory
 * @param name array file name
 * @param count expected item count
 * @param item_size expected item byte size
 * @param out output pointer
 * @return whether loading succeeded
 */
static bool _load_array(
    const char* dir, const char* name, uint32_t count, DvzSize item_size, void** out)
{
    ANN(dir);
    ANN(name);
    ANN(out);

    char path[1024] = {0};
    if (!_join_path(dir, name, path, sizeof(path)))
        return false;

    DvzSize size = 0;
    void* data = dvz_read_file(path, &size);
    const DvzSize expected = (DvzSize)count * item_size;
    if (data == NULL || size != expected)
    {
        dvz_free(data);
        return false;
    }

    *out = data;
    return true;
}



/**
 * Parse an unsigned integer metadata field.
 *
 * @param text field text
 * @param out output value
 * @return whether parsing succeeded
 */
static bool _parse_u32(const char* text, uint32_t* out)
{
    ANN(text);
    ANN(out);
    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0') || value > UINT32_MAX)
        return false;
    *out = (uint32_t)value;
    return true;
}



/**
 * Parse a floating-point metadata field.
 *
 * @param text field text
 * @param out output value
 * @return whether parsing succeeded
 */
static bool _parse_f64(const char* text, double* out)
{
    ANN(text);
    ANN(out);
    char* end = NULL;
    double value = strtod(text, &end);
    if (end == text || (end != NULL && *end != '\0'))
        return false;
    *out = value;
    return true;
}



/**
 * Apply one metadata key/value pair to a choropleth bundle.
 *
 * @param bundle target bundle
 * @param key metadata key
 * @param value metadata value
 * @return whether the key was known and the value valid
 */
static bool _metadata_apply(ChoroplethBundle* bundle, const char* key, const char* value)
{
    ANN(bundle);
    ANN(key);
    ANN(value);

    if (strcmp(key, "region_count") == 0)
        return _parse_u32(value, &bundle->region_count);
    if (strcmp(key, "ring_count") == 0)
        return _parse_u32(value, &bundle->ring_count);
    if (strcmp(key, "point_count") == 0)
        return _parse_u32(value, &bundle->point_count);
    if (strcmp(key, "xmin") == 0)
        return _parse_f64(value, &bundle->xmin);
    if (strcmp(key, "xmax") == 0)
        return _parse_f64(value, &bundle->xmax);
    if (strcmp(key, "ymin") == 0)
        return _parse_f64(value, &bundle->ymin);
    if (strcmp(key, "ymax") == 0)
        return _parse_f64(value, &bundle->ymax);
    if (strcmp(key, "value_min") == 0)
        return _parse_f64(value, &bundle->value_min);
    if (strcmp(key, "value_max") == 0)
        return _parse_f64(value, &bundle->value_max);
    if (strcmp(key, "density_min") == 0)
        return _parse_f64(value, &bundle->density_min);
    if (strcmp(key, "density_max") == 0)
        return _parse_f64(value, &bundle->density_max);
    return false;
}



/**
 * Load bundle metadata from a tiny key/value TSV sidecar.
 *
 * @param dir prepared bundle directory
 * @param out output bundle
 * @return whether loading succeeded
 */
static bool _load_metadata(const char* dir, ChoroplethBundle* out)
{
    ANN(dir);
    ANN(out);

    char path[1024] = {0};
    if (!_join_path(dir, METADATA_NAME, path, sizeof(path)))
        return false;

    FILE* fp = fopen(path, "r");
    if (fp == NULL)
        return false;

    bool ok = true;
    char line[256] = {0};
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char key[96] = {0};
        char value[128] = {0};
        if (sscanf(line, "%95[^\t]\t%127s", key, value) != 2)
        {
            ok = false;
            break;
        }
        if (!_metadata_apply(out, key, value))
        {
            ok = false;
            break;
        }
    }
    if (ferror(fp) != 0)
        ok = false;
    fclose(fp);

    return ok && out->region_count > 0 && out->ring_count > 0 && out->point_count > 0 &&
           out->xmin < out->xmax && out->ymin < out->ymax && out->value_min < out->value_max;
}



/**
 * Validate loaded ring spans against the point and region arrays.
 *
 * @param bundle loaded bundle
 * @return whether all ring spans are valid
 */
static bool _validate_rings(const ChoroplethBundle* bundle)
{
    ANN(bundle);
    ANN(bundle->rings);
    for (uint32_t i = 0; i < bundle->ring_count; i++)
    {
        const ChoroplethRing* ring = &bundle->rings[i];
        if (ring->region_index >= bundle->region_count || ring->point_count < 3)
            return false;
        if (ring->point_first > bundle->point_count ||
            ring->point_count > bundle->point_count - ring->point_first)
        {
            return false;
        }
    }
    return true;
}



/**
 * Load a prepared choropleth typed-array bundle from disk.
 *
 * @param dir prepared bundle directory
 * @param out loaded bundle
 * @return whether loading succeeded
 */
static bool _choropleth_bundle_load(const char* dir, ChoroplethBundle* out)
{
    ANN(dir);
    ANN(out);

    dvz_memset(out, sizeof(ChoroplethBundle), 0, sizeof(ChoroplethBundle));
    if (dvz_snprintf(out->path, sizeof(out->path), "%s", dir) <= 0)
        return false;
    if (!_load_metadata(dir, out))
        goto fail;
    if (!_load_array(
            dir, RINGS_NAME, out->ring_count, sizeof(ChoroplethRing), (void**)&out->rings))
    {
        goto fail;
    }
    if (!_load_array(dir, POINTS_NAME, out->point_count, sizeof(dvec2), (void**)&out->points))
        goto fail;
    if (!_load_array(dir, RING_FILL_NAME, out->ring_count, sizeof(DvzColor), (void**)&out->fill))
        goto fail;
    if (!_load_array(
            dir, RING_STROKE_NAME, out->ring_count, sizeof(DvzColor), (void**)&out->stroke))
    {
        goto fail;
    }
    if (!_load_array(dir, RING_WIDTH_NAME, out->ring_count, sizeof(float), (void**)&out->widths))
        goto fail;
    if (!_load_array(dir, RING_ID_NAME, out->ring_count, sizeof(uint64_t), (void**)&out->ids))
        goto fail;
    if (!_validate_rings(out))
        goto fail;
    return true;

fail:
    _choropleth_bundle_destroy(out);
    return false;
}



/**
 * Return a default prepared bundle path.
 *
 * @param out output path
 * @param out_size output path size
 * @return whether a readable default bundle was found
 */
static bool _default_bundle_path(char* out, size_t out_size)
{
    ANN(out);
    char metadata_path[1200] = {0};
    if (_join_path(DEFAULT_DATA_BUNDLE, METADATA_NAME, metadata_path, sizeof(metadata_path)) &&
        _file_readable(metadata_path))
    {
        return dvz_snprintf(out, out_size, "%s", DEFAULT_DATA_BUNDLE) > 0;
    }
    if (_join_path(DEFAULT_CACHE_BUNDLE, METADATA_NAME, metadata_path, sizeof(metadata_path)) &&
        _file_readable(metadata_path))
    {
        return dvz_snprintf(out, out_size, "%s", DEFAULT_CACHE_BUNDLE) > 0;
    }
    return false;
}



/**
 * Print instructions when no prepared bundle is available.
 */
static void _print_missing_data(void)
{
    dvz_fprintf(stderr, "choropleth: missing prepared bundle\n");
    dvz_fprintf(stderr, "  python tools/data/prepare_us_state_choropleth.py\n");
}



/*************************************************************************************************/
/*  Scene helpers                                                                                */
/*************************************************************************************************/

/**
 * Configure the panel for the map view.
 *
 * @param panel target panel
 * @param bundle loaded map bundle
 * @return whether setup succeeded
 */
static bool _configure_panel(DvzPanel* panel, const ChoroplethBundle* bundle)
{
    ANN(panel);
    ANN(bundle);

    example_graphite_cyan_set_panel_background(panel);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.035f, .right = 0.105f, .bottom = 0.045f,
                                        .top = 0.095f});
    if (!ok)
        return false;

    DvzPanelDomainFit fit = dvz_panel_domain_fit();
    fit.aspect = DVZ_PANEL_DOMAIN_ASPECT_EQUAL;
    fit.x = (DvzDataDomain){.min = bundle->xmin, .max = bundle->xmax};
    fit.y = (DvzDataDomain){.min = bundle->ymin, .max = bundle->ymax};
    fit.padding = 0.035;
    return dvz_panel_set_domain_fit(panel, &fit) == 0;
}



/**
 * Add screen-space text.
 *
 * @param panel target panel
 * @param text string to draw
 * @param x screen X in logical pixels
 * @param y screen Y in logical pixels
 * @param size text size in pixels
 * @param role graphite-cyan color role
 * @return whether setup succeeded
 */
static bool _add_screen_text(
    DvzPanel* panel, const char* text, float x, float y, float size, ExampleStyleColorRole role)
{
    ANN(panel);
    ANN(text);

    DvzText* label = dvz_text(panel, 0);
    if (label == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(role);
    DvzTextStyle style = dvz_text_style();
    style.size_px = size;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = color.a;
    if (dvz_text_set_style(label, &style) != 0)
        return false;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = x;
    placement.position[1] = y;
    placement.position[2] = 0.0;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    dvz_text_set_placement(label, &placement);
    dvz_text_set_string(label, text);
    return true;
}



/**
 * Create the choropleth color scale.
 *
 * @param scene owning scene
 * @param bundle loaded map bundle
 * @return created scale or NULL
 */
static DvzScale* _add_scale(DvzScene* scene, const ChoroplethBundle* bundle)
{
    ANN(scene);
    ANN(bundle);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "log10 people/km2",
                   .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                              .precision = 2,
                              .trim_trailing_zeros = true},
               });
    if (scale == NULL)
        return NULL;
    dvz_scale_set_domain(scale, bundle->value_min, bundle->value_max);
    dvz_scale_set_view_range(scale, bundle->value_min, bundle->value_max);

    DvzColormap* colormap =
        dvz_colormap_custom(scene, "us_state_density", CHOROPLETH_RAMP, DVZ_ARRAY_COUNT(CHOROPLETH_RAMP));
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add the filled state polygons and strokes.
 *
 * @param scene owning scene
 * @param panel target panel
 * @param bundle loaded map bundle
 * @return whether setup succeeded
 */
static bool _add_choropleth_polygons(
    DvzScene* scene, DvzPanel* panel, const ChoroplethBundle* bundle)
{
    ANN(scene);
    ANN(panel);
    ANN(bundle);

    DvzPolygonSet* set = dvz_polygon_set(scene, 0);
    if (set == NULL)
        return false;

    bool ok = true;
    for (uint32_t i = 0; i < bundle->ring_count; i++)
    {
        const ChoroplethRing* ring = &bundle->rings[i];
        const dvec2* xy = (const dvec2*)&bundle->points[ring->point_first];
        const uint32_t index = dvz_polygon_set_add(
            set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                   .outer = {.xy = xy, .count = ring->point_count}});
        if (index == UINT32_MAX || index != i)
        {
            ok = false;
            break;
        }
    }

    if (ok && dvz_polygon_set_region_ids(set, 0, bundle->ring_count, bundle->ids) != 0)
        ok = false;
    if (ok && dvz_polygon_set_region_fill_colors(set, 0, bundle->ring_count, bundle->fill) != 0)
        ok = false;
    if (ok && dvz_polygon_set_region_stroke_colors(set, 0, bundle->ring_count, bundle->stroke) != 0)
        ok = false;
    if (ok && dvz_polygon_set_region_stroke_widths(set, 0, bundle->ring_count, bundle->widths) != 0)
        ok = false;
    if (ok && dvz_polygon_set_stroke_join(set, DVZ_PATH_JOIN_ROUND, 3.0f) != 0)
        ok = false;

    if (ok)
    {
        DvzComposite* composite = dvz_polygon_set_composite(set, 0);
        ok = composite != NULL &&
             dvz_panel_add_composite(
                 panel, composite,
                 &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
                                        .z_layer = 0,
                                        .coord_space = DVZ_COORD_DATA}) == 0;
    }

    return ok;
}



/**
 * Add title, source note, and scalar colorbar.
 *
 * @param panel target panel
 * @param scale color scale
 * @return whether setup succeeded
 */
static bool _add_annotations(DvzPanel* panel, DvzScale* scale)
{
    ANN(panel);
    ANN(scale);

    if (!_add_screen_text(
            panel, "Contiguous U.S. state population density", 32.0f, 38.0f, 34.0f,
            EXAMPLE_STYLE_COLOR_TEXT))
    {
        return false;
    }
    if (!_add_screen_text(
            panel, "Census 2024 boundaries + Vintage 2025 population estimates", 32.0f, 76.0f,
            20.0f, EXAMPLE_STYLE_COLOR_TEXT))
    {
        return false;
    }

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "log10 people/km2",
            .reserve_px = 120.0f,
            .ramp_width_px = 28.0f,
            .plot_gap_px = 14.0f,
            .tick_length_px = 6.0f,
            .label_gap_px = 7.0f,
        });
    if (colorbar == NULL)
        return false;
    dvz_colorbar_set_format(
        colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                      .precision = 2,
                      .trim_trailing_zeros = true});
    return true;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the choropleth scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    *out_user = NULL;

    ChoroplethState* state = (ChoroplethState*)dvz_calloc(1, sizeof(ChoroplethState));
    if (state == NULL)
        return false;

    char bundle_path[1024] = {0};
    if (!_default_bundle_path(bundle_path, sizeof(bundle_path)))
    {
        _print_missing_data();
        dvz_free(state);
        return false;
    }
    if (!_choropleth_bundle_load(bundle_path, &state->bundle))
    {
        dvz_fprintf(stderr, "choropleth: failed to load prepared bundle %s\n", bundle_path);
        dvz_free(state);
        return false;
    }

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto fail;

    DvzPanel* panel = dvz_panel(ctx->figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (panel == NULL)
        goto fail;
    if (!_configure_panel(panel, &state->bundle))
        goto fail;

    DvzScale* scale = _add_scale(ctx->scene, &state->bundle);
    if (scale == NULL)
        goto fail;
    if (!_add_choropleth_polygons(ctx->scene, panel, &state->bundle))
        goto fail;
    if (!_add_annotations(panel, scale))
        goto fail;

    DvzPanzoomDesc panzoom = dvz_panzoom_desc();
    panzoom.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    if (dvz_scenario_panzoom(ctx, panel, &panzoom, DVZ_DIM_MASK_XY) == NULL)
        goto fail;

    dvz_fprintf(
        stderr, "choropleth: %u regions, %u rings, %u points from %s\n",
        state->bundle.region_count, state->bundle.ring_count, state->bundle.point_count,
        state->bundle.path);

    *out_user = state;
    return true;

fail:
    _choropleth_bundle_destroy(&state->bundle);
    dvz_free(state);
    return false;
}



/**
 * Destroy choropleth scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ChoroplethState* state = (ChoroplethState*)user;
    if (state == NULL)
        return;
    _choropleth_bundle_destroy(&state->bundle);
    dvz_free(state);
}



/**
 * Return the choropleth scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_us_state_choropleth_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "us_state_choropleth",
        .title = "us_state_choropleth",
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
 * Run the U.S. state choropleth through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_us_state_choropleth_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
