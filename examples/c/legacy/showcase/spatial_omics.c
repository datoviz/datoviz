/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* spatial_omics - napari Points-layer dense spatial-omics MVP.
 *
 * This Stage 1 demo intentionally uses the current v0.4 scene point visual: positions, RGBA8
 * colors, and one size value per point. Category, continuous, and density modes are emulated by
 * precomputed color buffers. A prepared binary cache is required at runtime.
 *
 * Build:  cmake --build build --target spatial_omics
 * Run:    ./build/examples/c/showcase/spatial_omics
 * Smoke:  ./build/examples/c/showcase/spatial_omics --frames 2
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"
#include "_time_utils.h"
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"
#include "datoviz/scene/panzoom.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH                   1180
#define HEIGHT                  800
#define MAX_PATH_LENGTH         1024
#define FPS_PRINT_INTERVAL      2.0
#define DENSE_DEFAULT_CACHE     "data/examples/c/showcase/spatial_points/synthetic"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DenseColorMode
{
    DENSE_COLOR_CATEGORY = 0,
    DENSE_COLOR_CONTINUOUS,
    DENSE_COLOR_DENSITY,
    DENSE_COLOR_COUNT,
} DenseColorMode;


typedef enum DenseDatasetChoice
{
    DENSE_DATASET_MERFISH = 0,
    DENSE_DATASET_MIBITOF,
    DENSE_DATASET_COUNT,
} DenseDatasetChoice;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DenseDataset DenseDataset;
typedef struct DenseState   DenseState;
typedef struct DenseConfig  DenseConfig;


struct DenseDataset
{
    uint32_t count;
    uint32_t image_width;
    uint32_t image_height;
    float* positions;
    uint8_t* colors[DENSE_COLOR_COUNT];
    uint8_t* image_rgba;
    bool from_cache;
};


struct DenseState
{
    DenseDataset dataset;
    DvzVisual* points;
    DvzVisual* background;
    uint8_t* upload_colors;
    float* upload_sizes;
    uint32_t upload_capacity;
    uint32_t rendered_count;
    int point_preset;
    int color_mode;
    float point_size;
    float opacity;
    bool lod_enabled;
    bool background_visible;
    bool dirty;
    uint64_t frame_count;
    uint64_t fps_frame_start;
    DvzClock fps_clock;
    int dataset_choice;
};


struct DenseConfig
{
    const char* cache_path;
    uint32_t frames;
    DenseDatasetChoice dataset_choice;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DenseDatasetChoice _dataset_choice_from_text(const char* text);



/**
 * Parse a positive unsigned integer from a command-line token.
 *
 * @param text input token
 * @param out output integer
 * @return whether parsing succeeded
 */
static bool _parse_u32(const char* text, uint32_t* out)
{
    if (text == NULL || out == NULL)
        return false;
    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0') || value > UINT32_MAX)
        return false;
    *out = (uint32_t)value;
    return true;
}



/**
 * Parse demo command-line options.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return parsed demo configuration
 */
static DenseConfig _parse_args(int argc, char** argv)
{
    DenseConfig cfg = {0};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--cache") == 0 && i + 1 < argc)
        {
            cfg.cache_path = argv[++i];
        }
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
        {
            (void)_parse_u32(argv[++i], &cfg.frames);
        }
        else if (strcmp(argv[i], "--dataset") == 0 && i + 1 < argc)
        {
            cfg.dataset_choice = _dataset_choice_from_text(argv[++i]);
        }
        else
        {
            uint32_t frames = 0;
            if (_parse_u32(argv[i], &frames))
                cfg.frames = frames;
            else
                cfg.cache_path = argv[i];
        }
    }
    return cfg;
}



/**
 * Return whether a file exists.
 *
 * @param path path to test
 * @return whether the file can be opened
 */
static bool _file_exists(const char* path)
{
    if (path == NULL)
        return false;
    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return false;
    fclose(file);
    return true;
}



/**
 * Join a directory and file name into an output path.
 *
 * @param dir directory path
 * @param name file name
 * @param out output path buffer
 * @param out_size output buffer size
 */
static void _join_path(const char* dir, const char* name, char* out, size_t out_size)
{
    ANN(out);
    if (dir == NULL || name == NULL || out_size == 0)
        return;
    size_t n = strlen(dir);
    const char* sep = n > 0 && dir[n - 1] == '/' ? "" : "/";
    (void)snprintf(out, out_size, "%s%s%s", dir, sep, name);
}



/**
 * Return the cache directory for a named demo dataset.
 *
 * @param choice dataset selector value
 * @param out output path buffer
 * @param out_size output buffer size
 * @return whether the path was constructed
 */
static bool _dataset_cache_path(DenseDatasetChoice choice, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0)
        return false;

    (void)choice;
    (void)snprintf(out, out_size, "%s", DENSE_DEFAULT_CACHE);
    return true;
}



/**
 * Return the dataset selector for a cache directory or dataset token.
 *
 * @param text command-line token
 * @return matching dataset selector
 */
static DenseDatasetChoice _dataset_choice_from_text(const char* text)
{
    if (text == NULL)
        return DENSE_DATASET_MERFISH;
    if (strcmp(text, "merfish") == 0 || strstr(text, "/merfish") != NULL)
        return DENSE_DATASET_MERFISH;
    if (strcmp(text, "mibitof") == 0 || strstr(text, "/mibitof") != NULL)
        return DENSE_DATASET_MIBITOF;
    return DENSE_DATASET_MERFISH;
}



/**
 * Read a JSON integer field from a small metadata file.
 *
 * @param json metadata bytes
 * @param key field name
 * @param out output integer
 * @return whether the field was found and parsed
 */
static bool _json_u32(const char* json, const char* key, uint32_t* out)
{
    if (json == NULL || key == NULL || out == NULL)
        return false;

    char pattern[96] = {0};
    (void)snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (p == NULL)
        return false;
    p = strchr(p, ':');
    if (p == NULL)
        return false;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    char* end = NULL;
    unsigned long value = strtoul(p, &end, 10);
    if (end == p || value > UINT32_MAX)
        return false;
    *out = (uint32_t)value;
    return true;
}



/**
 * Read a binary payload with an exact byte size.
 *
 * @param path binary file path
 * @param expected_size expected byte size
 * @return owned byte payload, or NULL
 */
static void* _read_exact(const char* path, uint64_t expected_size)
{
    if (!_file_exists(path))
        return NULL;
    DvzSize size = 0;
    void* data = dvz_read_file(path, &size);
    if (data == NULL)
        return NULL;
    if ((uint64_t)size != expected_size)
    {
        dvz_fprintf(
            stderr, "dense_points: unexpected size for %s (%" PRIu64 " != %" PRIu64 ")\n",
            path, (uint64_t)size, expected_size);
        dvz_free(data);
        return NULL;
    }
    return data;
}



/**
 * Parse metadata and load a Datoviz-ready binary cache directory.
 *
 * @param dir cache directory
 * @param dataset output dataset
 * @return whether loading succeeded
 */
static bool _load_cache(const char* dir, DenseDataset* dataset)
{
    if (dir == NULL || dataset == NULL)
        return false;

    char path[MAX_PATH_LENGTH] = {0};
    _join_path(dir, "metadata.json", path, sizeof(path));
    if (!_file_exists(path))
        return false;

    DvzSize metadata_size = 0;
    char* metadata = (char*)dvz_read_file(path, &metadata_size);
    if (metadata == NULL)
        return false;

    char* json = (char*)dvz_calloc((DvzSize)metadata_size + 1, 1);
    if (json == NULL)
    {
        dvz_free(metadata);
        return false;
    }
    dvz_memcpy(json, (size_t)metadata_size + 1, metadata, (size_t)metadata_size);
    dvz_free(metadata);

    uint32_t count = 0;
    uint32_t image_width = 0;
    uint32_t image_height = 0;
    bool ok = _json_u32(json, "point_count", &count) &&
              _json_u32(json, "image_width", &image_width) &&
              _json_u32(json, "image_height", &image_height);
    dvz_free(json);
    if (!ok || count == 0 || image_width == 0 || image_height == 0)
        return false;

    if ((uint64_t)image_width > UINT64_MAX / image_height ||
        (uint64_t)image_width * image_height > UINT64_MAX / (4u * sizeof(uint8_t)))
    {
        return false;
    }

    uint64_t position_bytes = (uint64_t)count * 3u * sizeof(float);
    uint64_t color_bytes = (uint64_t)count * 4u * sizeof(uint8_t);
    uint64_t image_bytes = (uint64_t)image_width * image_height * 4u * sizeof(uint8_t);

    _join_path(dir, "positions_f32.bin", path, sizeof(path));
    float* positions = (float*)_read_exact(path, position_bytes);
    _join_path(dir, "colors_category_rgba8.bin", path, sizeof(path));
    uint8_t* category = (uint8_t*)_read_exact(path, color_bytes);
    _join_path(dir, "colors_continuous_rgba8.bin", path, sizeof(path));
    uint8_t* continuous = (uint8_t*)_read_exact(path, color_bytes);
    _join_path(dir, "colors_density_rgba8.bin", path, sizeof(path));
    uint8_t* density = (uint8_t*)_read_exact(path, color_bytes);
    _join_path(dir, "image_rgba8.bin", path, sizeof(path));
    uint8_t* image = (uint8_t*)_read_exact(path, image_bytes);

    if (positions == NULL || category == NULL || continuous == NULL || density == NULL)
    {
        dvz_free(positions);
        dvz_free(category);
        dvz_free(continuous);
        dvz_free(density);
        dvz_free(image);
        return false;
    }

    *dataset = (DenseDataset){
        .count = count,
        .image_width = image_width,
        .image_height = image_height,
        .positions = positions,
        .colors = {category, continuous, density},
        .image_rgba = image,
        .from_cache = true,
    };
    return true;
}



/**
 * Load a selected prepared dataset.
 *
 * @param choice dataset selector
 * @param dataset output dataset
 * @return whether a dataset was loaded
 */
static bool _load_selected_dataset(DenseDatasetChoice choice, DenseDataset* dataset)
{
    if (dataset == NULL)
        return false;

    char path[MAX_PATH_LENGTH] = {0};
    if (_dataset_cache_path(choice, path, sizeof(path)) && _load_cache(path, dataset))
    {
        dvz_fprintf(stdout, "dense_points: loaded cache %s (%u points)\n", path, dataset->count);
        return true;
    }

    dvz_fprintf(stderr, "dense_points: missing prepared cache %s\n", path);
    return false;
}



/**
 * Release all dataset-owned arrays.
 *
 * @param dataset dataset to destroy
 */
static void _dataset_destroy(DenseDataset* dataset)
{
    if (dataset == NULL)
        return;
    dvz_free(dataset->positions);
    for (uint32_t i = 0; i < DENSE_COLOR_COUNT; i++)
        dvz_free(dataset->colors[i]);
    dvz_free(dataset->image_rgba);
    dvz_memset(dataset, sizeof(DenseDataset), 0, sizeof(DenseDataset));
}



/**
 * Return the selected unclamped point-count preset.
 *
 * @param state demo state
 * @return requested point count
 */
static uint32_t _requested_count(const DenseState* state)
{
    ANN(state);
    switch (state->point_preset)
    {
    case 1:
        return 1000000u;
    case 2:
        return 5000000u;
    case 3:
        return 10000000u;
    default:
        return state->dataset.count;
    }
}



/**
 * Return the active point count after clamping and LOD.
 *
 * @param state demo state
 * @return rendered point count
 */
static uint32_t _active_count(const DenseState* state)
{
    ANN(state);
    uint32_t count = _requested_count(state);
    if (count > state->dataset.count)
        count = state->dataset.count;
    if (state->lod_enabled && count > 250000u)
        count = 250000u + (count - 250000u) / 4u;
    if (count == 0)
        count = state->dataset.count;
    return count;
}



/**
 * Ensure upload scratch arrays can hold a point count.
 *
 * @param state demo state
 * @param count point count
 * @return whether the arrays are ready
 */
static bool _ensure_upload(DenseState* state, uint32_t count)
{
    ANN(state);
    if (count <= state->upload_capacity)
        return true;
    uint64_t color_count = (uint64_t)count * 4u;
    uint64_t size_bytes = (uint64_t)count * sizeof(float);
    if (color_count > (uint64_t)SIZE_MAX || size_bytes > (uint64_t)SIZE_MAX)
        return false;
    uint8_t* colors = (uint8_t*)dvz_realloc(state->upload_colors, color_count);
    if (colors == NULL)
        return false;

    float* sizes = (float*)dvz_realloc(state->upload_sizes, (DvzSize)size_bytes);
    if (sizes == NULL)
    {
        state->upload_colors = colors;
        return false;
    }
    state->upload_colors = colors;
    state->upload_sizes = sizes;
    state->upload_capacity = count;
    return true;
}



/**
 * Upload the current point attributes to the retained visual.
 *
 * @param state demo state
 * @return whether upload succeeded
 */
static bool _upload_points(DenseState* state)
{
    ANN(state);
    uint32_t count = _active_count(state);
    if (!_ensure_upload(state, count))
        return false;

    const uint8_t* base = state->dataset.colors[state->color_mode];
    if (base == NULL)
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        uint64_t p = 4ull * i;
        state->upload_colors[p + 0] = base[p + 0];
        state->upload_colors[p + 1] = base[p + 1];
        state->upload_colors[p + 2] = base[p + 2];
        float alpha = (float)base[p + 3] * state->opacity;
        if (alpha > 255.0f)
            alpha = 255.0f;
        state->upload_colors[p + 3] = (uint8_t)(alpha + 0.5f);
        state->upload_sizes[i] = state->point_size;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->dataset.positions, .item_count = count},
        {.attr_name = "color", .data = state->upload_colors, .item_count = count},
        {.attr_name = "diameter", .data = state->upload_sizes, .item_count = count},
    };
    int rc = dvz_visual_set_data_many(state->points, updates, 3);
    if (rc != 0)
        return false;
    state->rendered_count = count;
    state->dirty = false;
    return true;
}



/**
 * Create and initialize the optional background image visual.
 *
 * @param scene owning scene
 * @param dataset loaded dataset
 * @return initialized image visual, or NULL
 */
static DvzVisual* _background_visual(DvzScene* scene, const DenseDataset* dataset)
{
    if (scene == NULL || dataset == NULL || dataset->image_rgba == NULL)
        return NULL;

    DvzVisual* visual = dvz_image(scene, 0);
    if (visual == NULL)
        return NULL;
    vec3 positions[4] = {
        {-0.92f, -0.92f, 0.0f},
        {-0.92f, +0.92f, 0.0f},
        {+0.92f, -0.92f, 0.0f},
        {+0.92f, +0.92f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    if (dvz_visual_set_data(visual, "position", positions, 4) != 0 ||
        dvz_visual_set_data(visual, "texcoords", texcoords, 4) != 0 ||
        dvz_visual_set_texture(
            visual, dataset->image_rgba, dataset->image_width, dataset->image_height) != 0)
    {
        return NULL;
    }
    return visual;
}



/**
 * Upload the current dataset image to the background visual.
 *
 * @param state demo state
 * @return whether upload succeeded or no background exists
 */
static bool _upload_background(DenseState* state)
{
    ANN(state);
    if (state->background == NULL || state->dataset.image_rgba == NULL)
        return true;
    return dvz_visual_set_texture(
               state->background, state->dataset.image_rgba, state->dataset.image_width,
               state->dataset.image_height) == 0;
}



/**
 * Switch the retained point visual to another named dataset.
 *
 * @param state demo state
 * @param choice selected dataset
 * @return whether the switch succeeded
 */
static bool _switch_dataset(DenseState* state, DenseDatasetChoice choice)
{
    ANN(state);
    DenseDataset next = {0};
    if (!_load_selected_dataset(choice, &next))
        return false;

    _dataset_destroy(&state->dataset);
    state->dataset = next;
    state->dataset_choice = (int)choice;
    state->point_preset = 0;
    state->dirty = true;
    if (!_upload_background(state))
        return false;
    return _upload_points(state);
}



/**
 * Build the GUI controls.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data demo state
 */
static void _gui_callback(DvzGui* gui, DvzView* win, void* user_data)
{
    DenseState* state = (DenseState*)user_data;
    bool changed = false;

    static const char* datasets[] = {"MERFISH", "MIBI-TOF"};
    static const char* presets[] = {"real/base", "1M", "5M", "10M"};
    static const char* modes[] = {"category", "continuous", "density"};

    if (dvz_gui_begin(gui, "Dense spatial omics", NULL, 0))
    {
        igText("points: %u / %u", state->rendered_count, state->dataset.count);
        igText("source: prepared cache");
        int next_dataset = state->dataset_choice;
        if (igCombo_Str_arr("Dataset", &next_dataset, datasets, DENSE_DATASET_COUNT, 3) &&
            next_dataset != state->dataset_choice)
        {
            if (!_switch_dataset(state, (DenseDatasetChoice)next_dataset))
                dvz_fprintf(stderr, "dense_points: dataset switch failed\n");
            dvz_view_request_frame(win);
        }
        changed |= igCombo_Str_arr("Point count", &state->point_preset, presets, 4, 4);
        changed |= igCombo_Str_arr("Color mode", &state->color_mode, modes, 3, 3);
        changed |= dvz_gui_slider_float(gui, "Point size", &state->point_size, 1.0f, 12.0f);
        changed |= dvz_gui_slider_float(gui, "Opacity", &state->opacity, 0.02f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "LOD", &state->lod_enabled);
        if (state->background != NULL)
        {
            bool bg_changed =
                dvz_gui_checkbox(gui, "Background image", &state->background_visible);
            if (bg_changed)
                dvz_visual_set_visible(state->background, state->background_visible);
        }
        igSeparator();
        igTextWrapped(
            "Stage 1 uses precomputed colors and prefix/subsample LOD on the current v0.4 point "
            "visual.");
    }
    dvz_gui_end(gui);

    if (changed)
    {
        state->dirty = true;
        if (!_upload_points(state))
            dvz_fprintf(stderr, "dense_points: point upload failed\n");
        dvz_view_request_frame(win);
    }
}



/**
 * Print periodic FPS and rendered point-count status.
 *
 * @param win view
 * @param user_data demo state
 */
static void _frame_callback(DvzView* win, void* user_data)
{
    (void)win;
    DenseState* state = (DenseState*)user_data;
    state->frame_count++;
    double elapsed = dvz_clock_interval(&state->fps_clock);
    if (elapsed >= FPS_PRINT_INTERVAL)
    {
        uint64_t frames = state->frame_count - state->fps_frame_start;
        double fps = elapsed > 0.0 ? (double)frames / elapsed : 0.0;
        dvz_fprintf(
            stdout, "dense_points: fps=%.1f frames=%" PRIu64 " rendered=%u mode=%d lod=%d\n",
            fps, state->frame_count, state->rendered_count, state->color_mode,
            state->lod_enabled ? 1 : 0);
        state->fps_frame_start = state->frame_count;
        dvz_clock_tick(&state->fps_clock);
    }
}



/**
 * Release demo scratch arrays.
 *
 * @param state demo state
 */
static void _state_destroy(DenseState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->upload_colors);
    dvz_free(state->upload_sizes);
    _dataset_destroy(&state->dataset);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

/**
 * Run the dense spatial-omics point demo.
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
    DenseState state = {0};
    DenseConfig cfg = _parse_args(argc, argv);
    DenseDataset dataset = {0};

    if (cfg.cache_path != NULL && !_load_cache(cfg.cache_path, &dataset))
    {
        dvz_fprintf(stderr, "dense_points: failed to load prepared cache %s\n", cfg.cache_path);
        return 1;
    }
    else if (cfg.cache_path != NULL)
    {
        dvz_fprintf(
            stdout, "dense_points: loaded cache %s (%u points)\n", cfg.cache_path, dataset.count);
    }
    else if (!_load_selected_dataset(cfg.dataset_choice, &dataset))
    {
        dvz_fprintf(
            stderr,
            "dense_points: prepare data first, for example with "
            "python tools/data/prepare_napari_synthetic_spatial.py\n");
        goto cleanup;
    }

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dense_points: dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* points = panel != NULL ? dvz_point(scene, 0) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL && points != NULL, "dense_points: scene setup failed");

    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.035f, 0.040f, 0.052f, 1.0f));

    DvzVisual* background = _background_visual(scene, &dataset);
    if (background != NULL)
    {
        (void)dvz_panel_add_visual(
            panel, background,
            &(DvzVisualAttachDesc){
                .z_layer = 0,
                .controller_mode = DVZ_CONTROLLER_APPLY,
            });
    }
    dvz_visual_set_alpha_mode(points, DVZ_ALPHA_BLENDED);
    int rc = dvz_panel_add_visual(
        panel, points,
        &(DvzVisualAttachDesc){
            .z_layer = 1,
            .controller_mode = DVZ_CONTROLLER_APPLY,
        });
    EXAMPLE_CHECK(rc == 0, "dense_points: point visual attach failed");

    state = (DenseState){
        .dataset = dataset,
        .points = points,
        .background = background,
        .point_size = 2.0f,
        .opacity = 0.72f,
        .background_visible = true,
        .dirty = true,
        .fps_clock = dvz_clock(),
        .dataset_choice = (int)cfg.dataset_choice,
    };
    state_initialized = true;
    dvz_clock_tick(&state.fps_clock);
    EXAMPLE_CHECK(_upload_points(&state), "dense_points: initial upload failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dense_points: dvz_app() failed (no GPU or display?)");

    DvzView* win =
        dvz_view_glfw(app, figure, WIDTH, HEIGHT, "Dense spatial omics points");
    EXAMPLE_CHECK(win != NULL, "dense_points: GLFW window creation failed");

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, &panzoom_desc);
    EXAMPLE_CHECK(panzoom != NULL, "dense_points: panzoom setup failed");
    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dense_points: GUI creation failed");
    dvz_view_set_gui_callback(win, _gui_callback, &state);
    dvz_view_set_frame_callback(win, _frame_callback, &state);

    dvz_app_run(app, cfg.frames);

    status = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    if (state_initialized)
        _state_destroy(&state);
    else
        _dataset_destroy(&dataset);
    return status;
}
