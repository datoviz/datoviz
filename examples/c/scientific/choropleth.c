/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* choropleth - contiguous U.S. state population-density choropleth.
 *
 * Scenario: us_state_choropleth
 * Style: scientific, polygon-set, 1600x1200 capture target
 *
 * Data:    U.S. Census Bureau 2024 cartographic state boundaries and Vintage 2025 resident
 *          population estimates, prepared into flat C-readable polygon rings.
 * Source:  https://www2.census.gov/geo/tiger/GENZ2024/shp/cb_2024_us_state_20m.zip
 *          https://www2.census.gov/programs-surveys/popest/tables/2020-2025/state/totals/NST-EST2025-POP.xlsx
 * Terms:   U.S. Census Bureau public data; cite the Census Bureau as source.
 * Prepare: python tools/data/prepare_us_state_choropleth.py
 * Promote: python tools/data/prepare_us_state_choropleth.py --output data/examples/us_state_choropleth
 * Build:   cmake --build build --target choropleth
 * Run:     ./build/examples/c/scientific/choropleth
 * Smoke:   ./build/examples/c/scientific/choropleth 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_debug.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define CHOROPLETH_MAGIC       "DVZCHOR"
#define CHOROPLETH_VERSION     1u
#define REGION_NAME_LENGTH     64u
#define DEFAULT_DATA_BUNDLE    "data/examples/us_state_choropleth/prepared"
#define DEFAULT_CACHE_BUNDLE   ".cache/datoviz/examples/us_state_choropleth/prepared"
#define CHOROPLETH_BINARY_NAME "us_state_choropleth.bin"

static const DvzColor CHOROPLETH_RAMP[5] = {
    {26, 35, 46, 255},
    {33, 99, 126, 255},
    {50, 160, 147, 255},
    {237, 191, 94, 255},
    {221, 96, 73, 255},
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ChoroplethRegion
{
    uint32_t geoid;
    uint32_t ring_first;
    uint32_t ring_count;
    uint32_t point_count;
    double value;
    double population;
    double area_km2;
    double centroid_x;
    double centroid_y;
    char name[REGION_NAME_LENGTH + 1u];
    DvzColor color;
} ChoroplethRegion;


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
    ChoroplethRegion* regions;
    ChoroplethRing* rings;
    dvec2* points;
} ChoroplethBundle;



/*************************************************************************************************/
/*  Binary helpers                                                                               */
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
 * Read one unsigned 32-bit value from a binary file.
 *
 * @param fp opened file
 * @param out output value
 * @return whether the value was read
 */
static bool _read_u32(FILE* fp, uint32_t* out)
{
    ANN(fp);
    ANN(out);
    return fread(out, sizeof(uint32_t), 1, fp) == 1;
}



/**
 * Read one double value from a binary file.
 *
 * @param fp opened file
 * @param out output value
 * @return whether the value was read
 */
static bool _read_f64(FILE* fp, double* out)
{
    ANN(fp);
    ANN(out);
    return fread(out, sizeof(double), 1, fp) == 1;
}



/**
 * Read one fixed-width UTF-8 name field.
 *
 * @param fp opened file
 * @param out output buffer
 * @param out_size output buffer size
 * @return whether the field was read
 */
static bool _read_name(FILE* fp, char* out, size_t out_size)
{
    ANN(fp);
    ANN(out);
    if (out_size == 0)
        return false;
    char raw[REGION_NAME_LENGTH] = {0};
    if (fread(raw, 1, sizeof(raw), fp) != sizeof(raw))
        return false;
    dvz_memset(out, out_size, 0, out_size);
    size_t n = strnlen(raw, sizeof(raw));
    if (n >= out_size)
        n = out_size - 1;
    dvz_memcpy(out, out_size, raw, n);
    return true;
}



/**
 * Read one RGBA8 color.
 *
 * @param fp opened file
 * @param out output color
 * @return whether the color was read
 */
static bool _read_color(FILE* fp, DvzColor* out)
{
    ANN(fp);
    ANN(out);
    return fread(out, sizeof(DvzColor), 1, fp) == 1;
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
    dvz_free(bundle->points);
    dvz_free(bundle->rings);
    dvz_free(bundle->regions);
    dvz_memset(bundle, sizeof(ChoroplethBundle), 0, sizeof(ChoroplethBundle));
}



/**
 * Read and validate the binary bundle header.
 *
 * @param fp opened file
 * @param out output bundle metadata
 * @return whether the header is valid
 */
static bool _read_header(FILE* fp, ChoroplethBundle* out)
{
    ANN(fp);
    ANN(out);

    char magic[8] = {0};
    if (fread(magic, 1, sizeof(magic), fp) != sizeof(magic))
        return false;
    if (strncmp(magic, CHOROPLETH_MAGIC, strlen(CHOROPLETH_MAGIC)) != 0)
        return false;

    uint32_t version = 0;
    uint32_t reserved = 0;
    if (!_read_u32(fp, &version) || version != CHOROPLETH_VERSION)
        return false;
    if (!_read_u32(fp, &out->region_count) || !_read_u32(fp, &out->ring_count) ||
        !_read_u32(fp, &out->point_count) || !_read_u32(fp, &reserved))
    {
        return false;
    }

    if (!_read_f64(fp, &out->xmin) || !_read_f64(fp, &out->xmax) ||
        !_read_f64(fp, &out->ymin) || !_read_f64(fp, &out->ymax) ||
        !_read_f64(fp, &out->value_min) || !_read_f64(fp, &out->value_max) ||
        !_read_f64(fp, &out->density_min) || !_read_f64(fp, &out->density_max))
    {
        return false;
    }

    return out->region_count > 0 && out->ring_count > 0 && out->point_count > 0 &&
           out->value_min < out->value_max && out->xmin < out->xmax && out->ymin < out->ymax;
}



/**
 * Load a prepared choropleth bundle from disk.
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
    if (!_join_path(dir, CHOROPLETH_BINARY_NAME, out->path, sizeof(out->path)))
        return false;

    FILE* fp = fopen(out->path, "rb");
    if (fp == NULL)
        return false;

    bool ok = _read_header(fp, out);
    if (!ok)
        goto cleanup;

    out->regions = (ChoroplethRegion*)dvz_calloc(out->region_count, sizeof(ChoroplethRegion));
    out->rings = (ChoroplethRing*)dvz_calloc(out->ring_count, sizeof(ChoroplethRing));
    out->points = (dvec2*)dvz_calloc(out->point_count, sizeof(dvec2));
    if (out->regions == NULL || out->rings == NULL || out->points == NULL)
    {
        ok = false;
        goto cleanup;
    }

    for (uint32_t i = 0; i < out->region_count; i++)
    {
        ChoroplethRegion* region = &out->regions[i];
        ok = _read_u32(fp, &region->geoid) && _read_u32(fp, &region->ring_first) &&
             _read_u32(fp, &region->ring_count) && _read_u32(fp, &region->point_count) &&
             _read_f64(fp, &region->value) && _read_f64(fp, &region->population) &&
             _read_f64(fp, &region->area_km2) && _read_f64(fp, &region->centroid_x) &&
             _read_f64(fp, &region->centroid_y) &&
             _read_name(fp, region->name, sizeof(region->name)) &&
             _read_color(fp, &region->color);
        if (!ok)
            goto cleanup;
    }

    for (uint32_t i = 0; i < out->ring_count; i++)
    {
        ChoroplethRing* ring = &out->rings[i];
        ok = _read_u32(fp, &ring->region_index) && _read_u32(fp, &ring->point_first) &&
             _read_u32(fp, &ring->point_count);
        if (!ok || ring->region_index >= out->region_count ||
            ring->point_first + ring->point_count > out->point_count || ring->point_count < 3)
        {
            ok = false;
            goto cleanup;
        }
    }

    for (uint32_t i = 0; i < out->point_count; i++)
    {
        ok = _read_f64(fp, &out->points[i][0]) && _read_f64(fp, &out->points[i][1]);
        if (!ok)
            goto cleanup;
    }

cleanup:
    fclose(fp);
    if (!ok)
        _choropleth_bundle_destroy(out);
    return ok;
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
    char binary_path[1200] = {0};
    if (_join_path(DEFAULT_DATA_BUNDLE, CHOROPLETH_BINARY_NAME, binary_path, sizeof(binary_path)) &&
        _file_readable(binary_path))
    {
        return dvz_snprintf(out, out_size, "%s", DEFAULT_DATA_BUNDLE) > 0;
    }
    if (_join_path(
            DEFAULT_CACHE_BUNDLE, CHOROPLETH_BINARY_NAME, binary_path, sizeof(binary_path)) &&
        _file_readable(binary_path))
    {
        return dvz_snprintf(out, out_size, "%s", DEFAULT_CACHE_BUNDLE) > 0;
    }
    return false;
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
 * Return the explicit bundle path argument, if any.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return bundle path or NULL
 */
static const char* _bundle_arg(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL || argv[i][0] == '-')
            continue;
        if (_is_uint_text(argv[i]))
            continue;
        return argv[i];
    }
    return NULL;
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
                                        .top = 0.075f});
    if (!ok)
        return false;

    const double w = bundle->xmax - bundle->xmin;
    const double h = bundle->ymax - bundle->ymin;
    const double pad = 0.035 * (w > h ? w : h);
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, bundle->xmin - pad, bundle->xmax + pad);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, bundle->ymin - pad, bundle->ymax + pad);
    return rc == 0;
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

    DvzColor* fill = (DvzColor*)dvz_calloc(bundle->ring_count, sizeof(DvzColor));
    DvzColor* stroke = (DvzColor*)dvz_calloc(bundle->ring_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(bundle->ring_count, sizeof(float));
    uint64_t* ids = (uint64_t*)dvz_calloc(bundle->ring_count, sizeof(uint64_t));
    if (fill == NULL || stroke == NULL || widths == NULL || ids == NULL)
    {
        dvz_free(ids);
        dvz_free(widths);
        dvz_free(stroke);
        dvz_free(fill);
        return false;
    }

    bool ok = true;
    const DvzColor stroke_color = {14, 24, 31, 230};
    for (uint32_t i = 0; i < bundle->ring_count; i++)
    {
        const ChoroplethRing* ring = &bundle->rings[i];
        const ChoroplethRegion* region = &bundle->regions[ring->region_index];
        const uint32_t index = dvz_polygon_set_add(
            set, &(DvzPolygonDesc){DVZ_STRUCT_INIT_FIELDS(DvzPolygonDesc),
                   .outer = {.xy = &bundle->points[ring->point_first],
                             .count = ring->point_count}});
        if (index == UINT32_MAX || index != i)
        {
            ok = false;
            break;
        }
        fill[i] = region->color;
        stroke[i] = stroke_color;
        widths[i] = 1.35f;
        ids[i] = region->geoid;
    }

    if (ok && dvz_polygon_set_region_ids(set, 0, bundle->ring_count, ids) != 0)
        ok = false;
    if (ok && dvz_polygon_set_region_fill_colors(set, 0, bundle->ring_count, fill) != 0)
        ok = false;
    if (ok && dvz_polygon_set_region_stroke_colors(set, 0, bundle->ring_count, stroke) != 0)
        ok = false;
    if (ok && dvz_polygon_set_region_stroke_widths(set, 0, bundle->ring_count, widths) != 0)
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
                                        .z_layer = 0}) == 0;
    }

    dvz_free(ids);
    dvz_free(widths);
    dvz_free(stroke);
    dvz_free(fill);
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
            panel, "Contiguous U.S. state population density", 32.0f, 34.0f, 27.0f,
            EXAMPLE_STYLE_COLOR_TEXT))
    {
        return false;
    }
    if (!_add_screen_text(
            panel, "Census 2024 boundaries + Vintage 2025 population estimates", 32.0f, 66.0f,
            15.0f, EXAMPLE_STYLE_COLOR_MINOR_TICK))
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



/**
 * Print instructions when no prepared bundle is available.
 */
static void _print_missing_data(void)
{
    dvz_fprintf(stderr, "choropleth: missing prepared bundle\n");
    dvz_fprintf(stderr, "  python tools/data/prepare_us_state_choropleth.py\n");
    dvz_fprintf(stderr, "or pass a prepared bundle directory explicitly\n");
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    ExampleDebug debug = {0};
    ChoroplethBundle bundle = {0};

    char default_bundle[1024] = {0};
    const char* bundle_path = _bundle_arg(argc, argv);
    if (bundle_path == NULL)
    {
        if (!_default_bundle_path(default_bundle, sizeof(default_bundle)))
        {
            _print_missing_data();
            goto cleanup;
        }
        bundle_path = default_bundle;
    }
    EXAMPLE_CHECK(
        _choropleth_bundle_load(bundle_path, &bundle), "failed to load choropleth bundle");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    EXAMPLE_CHECK(_configure_panel(panel, &bundle), "panel configuration failed");

    DvzScale* scale = _add_scale(scene, &bundle);
    EXAMPLE_CHECK(scale != NULL, "scale setup failed");
    EXAMPLE_CHECK(
        _add_choropleth_polygons(scene, panel, &bundle), "polygon-set setup failed");
    EXAMPLE_CHECK(_add_annotations(panel, scale), "annotation setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "choropleth");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    EXAMPLE_CHECK(
        example_debug_setup(&debug, win, argc, argv, "choropleth"),
        "example_debug_setup() failed");
    example_debug_panzoom(&debug, "choropleth", panzoom);

    dvz_fprintf(
        stderr, "choropleth: %u regions, %u rings, %u points from %s\n", bundle.region_count,
        bundle.ring_count, bundle.point_count, bundle.path);

    dvz_app_run(app, example_frame_count_any(argc, argv));
    ret = 0;

cleanup:
    example_debug_uninstall(&debug);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    _choropleth_bundle_destroy(&bundle);
    return ret;
}
