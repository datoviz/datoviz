/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* dense_points_spatial_omics_glfw - napari Points-layer dense spatial-omics MVP.
 *
 * This Stage 1 demo intentionally uses the current v0.4 scene point visual: positions, RGBA8
 * colors, and one size value per point. Category, continuous, and density modes are emulated by
 * precomputed color buffers. A missing cache falls back to deterministic synthetic data.
 *
 * Build:  cmake --build build --target dense_points_spatial_omics_glfw
 * Run:    ./build/examples/napari/dense_points_spatial_omics_glfw
 * Smoke:  ./build/examples/napari/dense_points_spatial_omics_glfw --frames 2 --count 200000
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
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"
#include "datoviz/scene/panzoom.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH                   1180
#define HEIGHT                  800
#define SYNTHETIC_IMAGE_SIZE    1024
#define SYNTHETIC_CATEGORY_COUNT 16
#define DEFAULT_SYNTHETIC_COUNT 1000000u
#define MAX_PATH_LENGTH         1024



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
    double fps_elapsed;
};


struct DenseConfig
{
    const char* cache_path;
    uint32_t synthetic_count;
    uint32_t frames;
    bool no_cache;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
    DenseConfig cfg = {.synthetic_count = DEFAULT_SYNTHETIC_COUNT};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--cache") == 0 && i + 1 < argc)
        {
            cfg.cache_path = argv[++i];
        }
        else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc)
        {
            (void)_parse_u32(argv[++i], &cfg.synthetic_count);
        }
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
        {
            (void)_parse_u32(argv[++i], &cfg.frames);
        }
        else if (strcmp(argv[i], "--no-cache") == 0)
        {
            cfg.no_cache = true;
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
 * Return a deterministic integer hash.
 *
 * @param x input integer
 * @return hashed integer
 */
static uint32_t _hash_u32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}



/**
 * Convert a hashed integer to a float in [0, 1].
 *
 * @param x input integer
 * @return normalized pseudo-random value
 */
static float _unit_u32(uint32_t x)
{
    return (float)(_hash_u32(x) & 0xffffu) / 65535.0f;
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
 * Fill the default repo-local ignored cache path.
 *
 * @param out output path buffer
 * @param out_size output buffer size
 * @return whether the path was constructed
 */
static bool _default_cache_path(char* out, size_t out_size)
{
    if (out == NULL || out_size == 0)
        return false;
    (void)snprintf(out, out_size, ".cache/datoviz-napari-demos/spatial_points/synthetic");
    return true;
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
 * Write one categorical color.
 *
 * @param category category id
 * @param out output RGBA color
 */
static void _category_color(uint32_t category, uint8_t out[4])
{
    uint32_t h = _hash_u32(category + 17u);
    out[0] = (uint8_t)(55u + (h & 0x9fu));
    out[1] = (uint8_t)(70u + ((h >> 8) & 0x9fu));
    out[2] = (uint8_t)(85u + ((h >> 16) & 0x9fu));
    out[3] = category == 0 ? 170 : 220;
}



/**
 * Write one continuous color.
 *
 * @param value normalized scalar value
 * @param out output RGBA color
 */
static void _continuous_color(float value, uint8_t out[4])
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    float red = 1.7f * value;
    float green = 1.0f - (value > 0.55f ? value - 0.55f : 0.55f - value) * 1.7f;
    float blue = 1.8f * (1.0f - value);
    red = red > 1.0f ? 1.0f : red;
    green = green < 0.0f ? 0.0f : green;
    blue = blue > 1.0f ? 1.0f : blue;
    out[0] = (uint8_t)(255.0f * red + 0.5f);
    out[1] = (uint8_t)(255.0f * green + 0.5f);
    out[2] = (uint8_t)(255.0f * blue + 0.5f);
    out[3] = 215;
}



/**
 * Write one density-mode color.
 *
 * @param value normalized scalar value
 * @param out output RGBA color
 */
static void _density_color(float value, uint8_t out[4])
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    out[0] = 245;
    out[1] = 248;
    out[2] = 255;
    out[3] = (uint8_t)(16.0f + 30.0f * value + 0.5f);
}



/**
 * Generate synthetic image pixels.
 *
 * @param image output RGBA8 image
 * @param width image width
 * @param height image height
 */
static void _generate_image(uint8_t* image, uint32_t width, uint32_t height)
{
    ANN(image);
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            float nx = (float)x / (float)(width - 1u) - 0.5f;
            float ny = (float)y / (float)(height - 1u) - 0.5f;
            float vignette = 1.0f - 1.65f * ((nx < 0 ? -nx : nx) + (ny < 0 ? -ny : ny));
            if (vignette < 0.0f)
                vignette = 0.0f;
            uint32_t h = _hash_u32(x * 1973u + y * 9277u);
            float texture = (float)(h & 63u) / 63.0f;
            float base = 34.0f + 150.0f * vignette + 22.0f * texture;
            uint64_t p = 4ull * ((uint64_t)y * width + x);
            image[p + 0] = (uint8_t)(base * 0.82f);
            image[p + 1] = (uint8_t)(base * 0.92f);
            image[p + 2] = (uint8_t)(base * 1.08f > 255.0f ? 255.0f : base * 1.08f);
            image[p + 3] = 255;
        }
    }
}



/**
 * Generate a deterministic synthetic dense spatial-points dataset.
 *
 * @param count point count
 * @param dataset output dataset
 * @return whether generation succeeded
 */
static bool _generate_synthetic(uint32_t count, DenseDataset* dataset)
{
    if (count == 0 || dataset == NULL)
        return false;

    uint64_t position_count = (uint64_t)count * 3u;
    uint64_t color_count = (uint64_t)count * 4u;
    uint64_t image_count = (uint64_t)SYNTHETIC_IMAGE_SIZE * SYNTHETIC_IMAGE_SIZE * 4u;
    if (position_count > SIZE_MAX / sizeof(float) || color_count > SIZE_MAX ||
        image_count > SIZE_MAX)
    {
        return false;
    }

    float* positions = (float*)dvz_calloc(position_count, sizeof(float));
    uint8_t* category = (uint8_t*)dvz_calloc(color_count, sizeof(uint8_t));
    uint8_t* continuous = (uint8_t*)dvz_calloc(color_count, sizeof(uint8_t));
    uint8_t* density = (uint8_t*)dvz_calloc(color_count, sizeof(uint8_t));
    uint8_t* image = (uint8_t*)dvz_calloc(image_count, sizeof(uint8_t));
    if (positions == NULL || category == NULL || continuous == NULL || density == NULL ||
        image == NULL)
    {
        dvz_free(positions);
        dvz_free(category);
        dvz_free(continuous);
        dvz_free(density);
        dvz_free(image);
        return false;
    }

    uint32_t cluster_count = count / 4096u;
    if (cluster_count < 64u)
        cluster_count = 64u;
    if (cluster_count > 512u)
        cluster_count = 512u;

    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t cluster = _hash_u32(i + 42u) % cluster_count;
        float cx = -0.82f + 1.64f * _unit_u32(cluster * 19u + 7u);
        float cy = -0.82f + 1.64f * _unit_u32(cluster * 23u + 11u);
        float radius = 0.018f + 0.055f * (float)(cluster % 17u) / 16.0f;
        float jx = _unit_u32(i * 3u + 11u) - 0.5f;
        float jy = _unit_u32(i * 5u + 23u) - 0.5f;
        float x = cx + jx * radius;
        float y = cy + jy * radius;
        if (x < -0.92f)
            x = -0.92f;
        if (x > +0.92f)
            x = +0.92f;
        if (y < -0.92f)
            y = -0.92f;
        if (y > +0.92f)
            y = +0.92f;

        positions[3ull * i + 0] = x;
        positions[3ull * i + 1] = y;
        positions[3ull * i + 2] = 0.0f;

        uint8_t* cat = &category[4ull * i];
        uint8_t* cont = &continuous[4ull * i];
        uint8_t* dens = &density[4ull * i];
        uint32_t cat_id = _hash_u32(cluster + 101u) % SYNTHETIC_CATEGORY_COUNT;
        float value = _unit_u32(i + 101u);
        _category_color(cat_id, cat);
        _continuous_color(value, cont);
        _density_color(value, dens);
    }
    _generate_image(image, SYNTHETIC_IMAGE_SIZE, SYNTHETIC_IMAGE_SIZE);

    *dataset = (DenseDataset){
        .count = count,
        .image_width = SYNTHETIC_IMAGE_SIZE,
        .image_height = SYNTHETIC_IMAGE_SIZE,
        .positions = positions,
        .colors = {category, continuous, density},
        .image_rgba = image,
    };
    return true;
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
    float* sizes = (float*)dvz_realloc(state->upload_sizes, (DvzSize)size_bytes);
    if (colors == NULL || sizes == NULL)
        return false;
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

    if (dvz_visual_set_data(state->points, "position", state->dataset.positions, count) != 0 ||
        dvz_visual_set_data(state->points, "color", state->upload_colors, count) != 0 ||
        dvz_visual_set_data(state->points, "size", state->upload_sizes, count) != 0)
    {
        return false;
    }
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
    float positions[4][3] = {
        {-0.92f, -0.92f, 0.0f},
        {-0.92f, +0.92f, 0.0f},
        {+0.92f, -0.92f, 0.0f},
        {+0.92f, +0.92f, 0.0f},
    };
    float texcoords[4][2] = {
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
 * Build the GUI controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data demo state
 */
static void _gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    DenseState* state = (DenseState*)user_data;
    bool changed = false;

    static const char* presets[] = {"real/base", "1M", "5M", "10M"};
    static const char* modes[] = {"category", "continuous", "density"};

    if (dvz_gui_begin(gui, "Dense spatial omics", NULL, 0))
    {
        igText("points: %u / %u", state->rendered_count, state->dataset.count);
        igText("source: %s", state->dataset.from_cache ? "prepared cache" : "synthetic fallback");
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
    }
}



/**
 * Print periodic FPS and rendered point-count status.
 *
 * @param win app window
 * @param user_data demo state
 */
static void _frame_callback(DvzAppWindow* win, void* user_data)
{
    (void)win;
    DenseState* state = (DenseState*)user_data;
    state->frame_count++;
    state->fps_elapsed += 1.0 / 60.0;
    if (state->frame_count % 120u == 0)
    {
        dvz_fprintf(
            stdout, "dense_points: frame=%" PRIu64 " rendered=%u mode=%d lod=%d\n",
            state->frame_count, state->rendered_count, state->color_mode,
            state->lod_enabled ? 1 : 0);
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
    DenseConfig cfg = _parse_args(argc, argv);
    DenseDataset dataset = {0};

    char default_cache[MAX_PATH_LENGTH] = {0};
    const char* cache_path = cfg.cache_path;
    if (cache_path == NULL && _default_cache_path(default_cache, sizeof(default_cache)))
        cache_path = default_cache;

    if (!cfg.no_cache && cache_path != NULL && _load_cache(cache_path, &dataset))
    {
        dvz_fprintf(stdout, "dense_points: loaded cache %s (%u points)\n", cache_path, dataset.count);
    }
    else
    {
        if (!_generate_synthetic(cfg.synthetic_count, &dataset))
        {
            dvz_fprintf(stderr, "dense_points: synthetic data generation failed\n");
            return 1;
        }
        dvz_fprintf(stdout, "dense_points: generated synthetic fallback (%u points)\n", dataset.count);
    }

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        _dataset_destroy(&dataset);
        dvz_fprintf(stderr, "dense_points: dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f}) :
                                       NULL;
    DvzVisual* points = panel != NULL ? dvz_point(scene, 0) : NULL;
    if (figure == NULL || panel == NULL || points == NULL)
    {
        dvz_fprintf(stderr, "dense_points: scene setup failed\n");
        dvz_scene_destroy(scene);
        _dataset_destroy(&dataset);
        return 1;
    }

    dvz_panel_set_background_color(panel, 0.035f, 0.040f, 0.052f, 1.0f);

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
    if (dvz_panel_add_visual(
            panel, points,
            &(DvzVisualAttachDesc){
                .z_layer = 1,
                .controller_mode = DVZ_CONTROLLER_APPLY,
            }) != 0)
    {
        dvz_fprintf(stderr, "dense_points: point visual attach failed\n");
        dvz_scene_destroy(scene);
        _dataset_destroy(&dataset);
        return 1;
    }

    DenseState state = {
        .dataset = dataset,
        .points = points,
        .background = background,
        .point_size = 2.0f,
        .opacity = 0.72f,
        .background_visible = true,
        .dirty = true,
    };
    if (!_upload_points(&state))
    {
        dvz_fprintf(stderr, "dense_points: initial upload failed\n");
        dvz_scene_destroy(scene);
        _state_destroy(&state);
        return 1;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dense_points: dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        _state_destroy(&state);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "Dense spatial omics points");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dense_points: GLFW window creation failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _state_destroy(&state);
        return 1;
    }

    dvz_panel_set_panzoom(panel, dvz_app_window_input(win), DVZ_PANZOOM_FLAGS_KEEP_ASPECT);
    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dense_points: GUI creation failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _state_destroy(&state);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _gui_callback, &state);
    dvz_app_window_set_frame_callback(win, _frame_callback, &state);

    dvz_app_run(app, cfg.frames);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _state_destroy(&state);
    return 0;
}
