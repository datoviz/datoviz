/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* allen_mouse_brain_slice_glfw - local Allen mouse brain RGBA slice and 3D volume.
 *
 * Build:   just example-c allen_mouse_brain_slice_glfw
 * Run:     ./build/examples/c/allen_mouse_brain_slice_glfw [frames] [--downsample=2]
 * Data:    data/volumes/allen_mouse_brain_rgba.npy.gz
 * Meshes:  data/allen_ibl_assets, prepared with tools/prepare_allen_ibl_assets.py
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_overflow.h"
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  900
#define HEIGHT 650

#define DEFAULT_DATA_PATH "data/volumes/allen_mouse_brain_rgba.npy.gz"
#define DEFAULT_VOLUME_FILE "allen_mouse_brain_rgba.npy.gz"
#define DEFAULT_IBL_ASSET_DIR "data/allen_ibl_assets"
#define DEFAULT_AXIS          DVZ_VOLUME_AXIS_Y
#define DEFAULT_SLICE_POS     0.5f
#define DEFAULT_SLICE_OPACITY 1.0f
#define DEFAULT_VOLUME_OPACITY 0.85f
#define DEFAULT_VOLUME_STEPS  192.0f
#define DEFAULT_ATLAS_ALPHA_SCALE 1.0f
#define DEFAULT_OCCLUSION_THRESHOLD 0.000001f
#define DEFAULT_OCCLUSION_FADE 0.000001f
#define DEFAULT_OCCLUSION_HIDDEN_ALPHA 0.097f
#define MAX_ATLAS_REGIONS 32
#define MOUSE_BRAIN_WIDTH 320
#define MOUSE_BRAIN_HEIGHT 456
#define MOUSE_BRAIN_DEPTH 528



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct AllenMouseBrainVolume
{
    char* raw_data;
    uint8_t* downsampled_data;
    uint8_t* voxels;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t downsample;
} AllenMouseBrainVolume;



typedef struct AllenIblAtlasRegion
{
    uint32_t id;
    char acronym[32];
    char name[96];
    uint32_t vertex_start;
    uint32_t vertex_count;
    uint32_t index_start;
    uint32_t index_count;
    float alpha;
    bool visible;
} AllenIblAtlasRegion;



typedef struct AllenIblAtlasMesh
{
    float (*pos)[3];
    float (*normal)[3];
    DvzColor* base_color;
    DvzColor* color;
    DvzIndex* idx;
    DvzIndex* draw_idx;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t draw_index_count;
    double volume_bounds_min[3];
    double volume_bounds_max[3];
    bool has_volume_bounds;
    AllenIblAtlasRegion regions[MAX_ATLAS_REGIONS];
    uint32_t region_count;
} AllenIblAtlasMesh;



typedef struct AllenMouseBrainState
{
    DvzPanel* panel;
    DvzVisual* slice_visual;
    DvzVisual* volume_visual;
    DvzVisual* atlas_mesh_visual;
    DvzSceneBuffer* atlas_index_buffer;
    AllenIblAtlasMesh* atlas_mesh;
    bool show_slice;
    bool show_volume;
    bool show_atlas_mesh;
    bool atlas_uploads_initialized;
    bool clip_volume_at_slice;
    bool keep_positive_side;
    bool linear_sampling;
    bool volume_occlusion_enabled;
    int render_mode;
    float slice_opacity;
    float volume_opacity;
    float volume_steps;
    float occlusion_threshold;
    float occlusion_fade;
    float occlusion_hidden_alpha;
    float atlas_alpha_scale;
    float atlas_ambient;
    float atlas_diffuse;
    float atlas_light_direction[3];
    float slice_position;
    DvzVolumeAxis axis;
} AllenMouseBrainState;



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
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        char* end = NULL;
        unsigned long value = strtoul(argv[i], &end, 10);
        if (end == argv[i] || (end != NULL && *end != '\0'))
            continue;
        if (value > UINT32_MAX)
            return UINT32_MAX;
        return (uint32_t)value;
    }
    return 0;
}



/**
 * Parse an optional bounded unsigned command-line option.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param name long option name without leading dashes
 * @param default_value fallback value when the option is absent
 * @param min_value minimum accepted value
 * @param max_value maximum accepted value
 * @return parsed option value
 */
static uint32_t _option_u32(
    int argc, char** argv, const char* name, uint32_t default_value, uint32_t min_value,
    uint32_t max_value)
{
    ANN(name);
    if (argc < 2 || argv == NULL)
        return default_value;

    char prefix[64] = {0};
    int prefix_len = dvz_snprintf(prefix, sizeof(prefix), "--%s=", name);
    if (prefix_len <= 0 || (size_t)prefix_len >= sizeof(prefix))
        return default_value;

    char flag[64] = {0};
    int flag_len = dvz_snprintf(flag, sizeof(flag), "--%s", name);
    if (flag_len <= 0 || (size_t)flag_len >= sizeof(flag))
        return default_value;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;

        const char* value_str = NULL;
        if (strncmp(argv[i], prefix, (size_t)prefix_len) == 0)
        {
            value_str = argv[i] + prefix_len;
        }
        else if (strcmp(argv[i], flag) == 0 && i + 1 < argc && argv[i + 1] != NULL)
        {
            value_str = argv[i + 1];
        }
        if (value_str == NULL)
            continue;

        char* end = NULL;
        unsigned long value = strtoul(value_str, &end, 10);
        if (end == value_str || (end != NULL && *end != '\0'))
            return default_value;
        if (value < min_value)
            return min_value;
        if (value > max_value)
            return max_value;
        return (uint32_t)value;
    }
    return default_value;
}



/**
 * Join a directory and a file name into a fixed-size output path buffer.
 *
 * @param dir directory path
 * @param basename file base name
 * @param out output buffer
 * @param out_size output buffer size in bytes
 * @return whether the output path fits
 */
static bool _join_path(
    const char* dir, const char* basename, char* out, size_t out_size)
{
    ANN(dir);
    ANN(basename);
    ANN(out);
    size_t dir_len = strlen(dir);
    if (dir_len == 0 || basename[0] == '\0')
        return false;
    const char* sep =
        (dir_len > 0 && dir[dir_len - 1] == '/') || (dir[dir_len - 1] == '\\') ? "" : "/";
    int written = dvz_snprintf(out, out_size, "%s%s%s", dir, sep, basename);
    return written > 0 && (size_t)written < out_size;
}



/**
 * Copy a plain path into the output buffer.
 *
 * @param path source path
 * @param out output buffer
 * @param out_size output buffer size
 * @return whether the path fits
 */
static bool _copy_path(const char* path, char* out, size_t out_size)
{
    ANN(path);
    ANN(out);
    if (path[0] == '\0')
        return false;
    int written = dvz_snprintf(out, out_size, "%s", path);
    return written > 0 && (size_t)written < out_size;
}


/**
 * Return the requested data file path.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param out output path buffer
 * @param out_size output buffer size
 * @return whether path extraction succeeded
 */
static bool _data_path(int argc, char** argv, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0)
        return false;
    out[0] = '\0';

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        if (strncmp(argv[i], "--data-file=", 11) == 0)
            return _copy_path(argv[i] + 11, out, out_size);

        if (strncmp(argv[i], "--data-dir=", 11) == 0)
        {
            return _join_path(argv[i] + 11, DEFAULT_VOLUME_FILE, out, out_size);
        }

        if (strcmp(argv[i], "--data-file") == 0 && i + 1 < argc && argv[i + 1] != NULL)
        {
            return _copy_path(argv[i + 1], out, out_size);
        }
        if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc && argv[i + 1] != NULL)
            return _join_path(argv[i + 1], DEFAULT_VOLUME_FILE, out, out_size);
    }

    if (dvz_snprintf(out, out_size, "%s", DEFAULT_DATA_PATH) <= 0 ||
        strlen(out) >= out_size)
        return false;
    return true;
}



/**
 * Read a prepared Allen/IBL asset NPY payload.
 *
 * @param data_dir asset directory
 * @param basename array filename
 * @param size output payload size in bytes
 * @return payload pointer owned by the caller, or NULL on failure
 */
static char* _read_ibl_asset_npy(const char* data_dir, const char* basename, DvzSize* size)
{
    ANN(data_dir);
    ANN(basename);
    ANN(size);

    char path[1024] = {0};
    if (!_join_path(data_dir, basename, path, sizeof(path)))
        return NULL;
    return dvz_read_npy(path, size);
}



/**
 * Parse six JSON numbers following a metadata key.
 *
 * @param text metadata JSON text
 * @param size metadata JSON byte size
 * @param key JSON key to find
 * @param values output values
 * @param value_count expected number of output values
 * @return whether all values were parsed
 */
static bool _parse_json_number_array(
    const char* text, DvzSize size, const char* key, double* values, uint32_t value_count)
{
    ANN(text);
    ANN(key);
    ANN(values);

    const char* end = text + size;
    const char* p = strstr(text, key);
    if (p == NULL || p >= end)
        return false;
    p += strlen(key);

    uint32_t count = 0;
    while (p < end && count < value_count)
    {
        if (*p == '-' || *p == '+' || *p == '.' || isdigit((uint8_t)(*p)))
        {
            char* next = NULL;
            values[count] = strtod(p, &next);
            if (next == NULL || next == p || next > end)
                return false;
            count++;
            p = next;
            continue;
        }
        p++;
    }
    return count == value_count;
}


/**
 * Parse one unsigned JSON value in an object slice.
 *
 * @param begin object slice start
 * @param end object slice end
 * @param key JSON key to find
 * @param out output value
 * @return whether parsing succeeded
 */
static bool _parse_json_u32_in_object(
    const char* begin, const char* end, const char* key, uint32_t* out)
{
    ANN(begin);
    ANN(end);
    ANN(key);
    ANN(out);

    const char* p = strstr(begin, key);
    if (p == NULL || p >= end)
        return false;
    p = strchr(p, ':');
    if (p == NULL || p >= end)
        return false;
    p++;
    while (p < end && isspace((uint8_t)(*p)))
        p++;
    char* next = NULL;
    unsigned long value = strtoul(p, &next, 10);
    if (next == NULL || next == p || next > end || value > UINT32_MAX)
        return false;
    *out = (uint32_t)value;
    return true;
}



/**
 * Parse one quoted JSON string value in an object slice.
 *
 * @param begin object slice start
 * @param end object slice end
 * @param key JSON key to find
 * @param out output string
 * @param out_size output string capacity
 * @return whether parsing succeeded
 */
static bool _parse_json_string_in_object(
    const char* begin, const char* end, const char* key, char* out, size_t out_size)
{
    ANN(begin);
    ANN(end);
    ANN(key);
    ANN(out);
    if (out_size == 0)
        return false;

    const char* p = strstr(begin, key);
    if (p == NULL || p >= end)
        return false;
    p = strchr(p, ':');
    if (p == NULL || p >= end)
        return false;
    p++;
    while (p < end && isspace((uint8_t)(*p)))
        p++;
    if (p >= end || *p != '"')
        return false;
    p++;
    const char* q = strchr(p, '"');
    if (q == NULL || q <= p || q > end)
        return false;
    size_t len = (size_t)(q - p);
    if (len >= out_size)
        len = out_size - 1;
    dvz_memcpy(out, out_size, p, len);
    out[len] = '\0';
    return true;
}



/**
 * Load prepared atlas region metadata from the JSON sidecar.
 *
 * @param data_dir asset directory
 * @param atlas atlas mesh state receiving region metadata
 */
static void _load_ibl_region_metadata(const char* data_dir, AllenIblAtlasMesh* atlas)
{
    ANN(data_dir);
    ANN(atlas);

    char path[1024] = {0};
    if (!_join_path(data_dir, "metadata.json", path, sizeof(path)))
        return;

    DvzSize size = 0;
    char* text = (char*)dvz_read_file(path, &size);
    if (text == NULL)
        return;
    if (size == 0)
    {
        dvz_free(text);
        return;
    }

    const char* end = text + size;
    const char* regions = strstr(text, "\"regions\"");
    if (regions == NULL || regions >= end)
        goto cleanup;
    const char* p = strchr(regions, '[');
    if (p == NULL || p >= end)
        goto cleanup;

    atlas->region_count = 0;
    while (p < end && atlas->region_count < MAX_ATLAS_REGIONS)
    {
        p = strchr(p, '{');
        if (p == NULL || p >= end)
            break;
        const char* object_end = strchr(p, '}');
        if (object_end == NULL || object_end >= end)
            break;

        AllenIblAtlasRegion region = {0};
        if (_parse_json_u32_in_object(p, object_end, "\"id\"", &region.id) &&
            _parse_json_u32_in_object(
                p, object_end, "\"vertex_start\"", &region.vertex_start) &&
            _parse_json_u32_in_object(
                p, object_end, "\"vertex_count\"", &region.vertex_count) &&
            _parse_json_u32_in_object(
                p, object_end, "\"index_start\"", &region.index_start) &&
            _parse_json_u32_in_object(
                p, object_end, "\"index_count\"", &region.index_count) &&
            _parse_json_string_in_object(
                p, object_end, "\"acronym\"", region.acronym, sizeof(region.acronym)) &&
            _parse_json_string_in_object(
                p, object_end, "\"name\"", region.name, sizeof(region.name)) &&
            region.vertex_start <= atlas->vertex_count &&
            region.vertex_count <= atlas->vertex_count - region.vertex_start &&
            region.index_start <= atlas->index_count &&
            region.index_count <= atlas->index_count - region.index_start)
        {
            region.visible = true;
            region.alpha = 1.0f;
            atlas->regions[atlas->region_count++] = region;
        }
        p = object_end + 1;
    }

cleanup:
    dvz_free(text);
}



/**
 * Load scene-space volume bounds from the prepared Allen/IBL metadata file.
 *
 * @param data_dir asset directory
 * @param atlas atlas mesh state receiving the bounds
 */
static void _load_ibl_volume_bounds(const char* data_dir, AllenIblAtlasMesh* atlas)
{
    ANN(data_dir);
    ANN(atlas);

    char path[1024] = {0};
    if (!_join_path(data_dir, "metadata.json", path, sizeof(path)))
        return;

    DvzSize size = 0;
    char* text = (char*)dvz_read_file(path, &size);
    if (text == NULL)
        return;
    if (size == 0)
    {
        dvz_free(text);
        return;
    }

    double values[6] = {0};
    if (_parse_json_number_array(text, size, "volume_bounds_scene", values, 6))
    {
        atlas->volume_bounds_min[0] = values[0];
        atlas->volume_bounds_min[1] = values[1];
        atlas->volume_bounds_min[2] = values[2];
        atlas->volume_bounds_max[0] = values[3];
        atlas->volume_bounds_max[1] = values[4];
        atlas->volume_bounds_max[2] = values[5];
        atlas->has_volume_bounds = true;
    }
    dvz_free(text);
}



/**
 * Load the prepared combined Allen/IBL atlas mesh bundle when present.
 *
 * @param data_dir asset directory
 * @param atlas output atlas mesh storage
 * @return whether loading succeeded
 */
static bool _load_ibl_atlas_mesh(const char* data_dir, AllenIblAtlasMesh* atlas)
{
    ANN(data_dir);
    ANN(atlas);

    DvzSize pos_size = 0;
    DvzSize normal_size = 0;
    DvzSize color_size = 0;
    DvzSize idx_size = 0;
    char* pos = _read_ibl_asset_npy(data_dir, "allen_ibl_mesh_pos.npy", &pos_size);
    char* normal = _read_ibl_asset_npy(data_dir, "allen_ibl_mesh_normal.npy", &normal_size);
    char* color = _read_ibl_asset_npy(data_dir, "allen_ibl_mesh_color.npy", &color_size);
    char* idx = _read_ibl_asset_npy(data_dir, "allen_ibl_mesh_idx.npy", &idx_size);

    if (pos == NULL || normal == NULL || color == NULL || idx == NULL)
    {
        dvz_free(idx);
        dvz_free(color);
        dvz_free(normal);
        dvz_free(pos);
        return false;
    }

    if (pos_size == 0 || pos_size % (3 * sizeof(float)) != 0 ||
        normal_size % (3 * sizeof(float)) != 0 || color_size % sizeof(DvzColor) != 0 ||
        idx_size == 0 || idx_size % sizeof(DvzIndex) != 0)
    {
        dvz_fprintf(stderr, "invalid Allen/IBL atlas asset payload sizes\n");
        goto error;
    }

    DvzSize vertex_count = pos_size / (3 * sizeof(float));
    DvzSize index_count = idx_size / sizeof(DvzIndex);
    if (vertex_count == 0 || vertex_count > UINT32_MAX ||
        normal_size / (3 * sizeof(float)) != vertex_count ||
        color_size / sizeof(DvzColor) != vertex_count || index_count == 0 ||
        index_count > UINT32_MAX || index_count % 3 != 0)
    {
        dvz_fprintf(stderr, "inconsistent Allen/IBL atlas asset sizes\n");
        goto error;
    }

    DvzIndex* indices = (DvzIndex*)idx;
    for (DvzSize i = 0; i < index_count; i++)
    {
        if (indices[i] >= vertex_count)
        {
            dvz_fprintf(stderr, "Allen/IBL atlas mesh index out of range\n");
            goto error;
        }
    }

    atlas->pos = (float(*)[3])pos;
    atlas->normal = (float(*)[3])normal;
    atlas->color = (DvzColor*)color;
    atlas->idx = (DvzIndex*)idx;
    atlas->vertex_count = (uint32_t)vertex_count;
    atlas->index_count = (uint32_t)index_count;
    atlas->draw_index_count = (uint32_t)index_count;

    atlas->base_color = (DvzColor*)dvz_calloc(color_size, 1);
    if (atlas->base_color == NULL)
        goto error;
    dvz_memcpy(atlas->base_color, color_size, atlas->color, color_size);
    atlas->draw_idx = (DvzIndex*)dvz_calloc(idx_size, 1);
    if (atlas->draw_idx == NULL)
        goto error;
    dvz_memcpy(atlas->draw_idx, idx_size, atlas->idx, idx_size);
    for (uint32_t i = 0; i < (uint32_t)vertex_count; i++)
    {
        atlas->base_color[i][3] = 255;
        atlas->color[i][3] = 255;
    }

    _load_ibl_volume_bounds(data_dir, atlas);
    _load_ibl_region_metadata(data_dir, atlas);

    dvz_fprintf(
        stderr, "loaded Allen/IBL atlas mesh: %u vertices, %u triangles, %u regions\n",
        atlas->vertex_count, atlas->index_count / 3, atlas->region_count);
    return true;

error:
    dvz_free(idx);
    dvz_free(color);
    dvz_free(normal);
    dvz_free(pos);
    dvz_free(atlas->draw_idx);
    dvz_free(atlas->base_color);
    dvz_memset(atlas, sizeof(AllenIblAtlasMesh), 0, sizeof(AllenIblAtlasMesh));
    return false;
}



/**
 * Free prepared Allen/IBL atlas mesh storage.
 *
 * @param atlas atlas mesh storage
 */
static void _ibl_atlas_mesh_destroy(AllenIblAtlasMesh* atlas)
{
    if (atlas == NULL)
        return;
    dvz_free(atlas->idx);
    dvz_free(atlas->draw_idx);
    dvz_free(atlas->color);
    dvz_free(atlas->base_color);
    dvz_free(atlas->normal);
    dvz_free(atlas->pos);
    dvz_memset(atlas, sizeof(AllenIblAtlasMesh), 0, sizeof(AllenIblAtlasMesh));
}



/**
 * Skip ASCII spaces.
 *
 * @param p current pointer
 * @param end one-past-end pointer
 * @return first non-space pointer
 */
static const char* _skip_spaces(const char* p, const char* end)
{
    ANN(p);
    ANN(end);
    while (p < end && isspace((uint8_t)(*p)))
        p++;
    return p;
}



/**
 * Parse a NumPy 64-bit little-endian u16.
 *
 * @param p pointer to input bytes
 * @return parsed uint16 value
 */
static uint16_t _u16_le(const uint8_t* p)
{
    ANN(p);
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8u);
}



/**
 * Parse a NumPy 32-bit little-endian u32.
 *
 * @param p pointer to input bytes
 * @return parsed uint32 value
 */
static uint32_t _u32_le(const uint8_t* p)
{
    ANN(p);
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
}



/**
 * Extract a quoted header value after a known key.
 *
 * @param header header string
 * @param header_end one-past-end pointer
 * @param key key name to locate
 * @param out output string
 * @param out_size output size
 * @return whether parsing succeeded
 */
static bool _parse_key_quoted_value(
    const char* header, const char* header_end, const char* key, char* out, size_t out_size)
{
    ANN(header);
    ANN(header_end);
    ANN(key);
    ANN(out);
    if (out_size == 0)
        return false;

    const char* key_pos = strstr(header, key);
    if (key_pos == NULL || key_pos >= header_end)
        return false;

    const char* colon = strchr(key_pos, ':');
    if (colon == NULL || colon >= header_end)
        return false;

    colon = _skip_spaces(colon + 1, header_end);
    char quote = *colon;
    if (quote != '\'' && quote != '\"')
        return false;
    colon++;

    const char* closing = strchr(colon, quote);
    if (closing == NULL || closing <= colon || closing > header_end)
        return false;

    size_t value_len = (size_t)(closing - colon);
    if (value_len >= out_size)
        return false;
    dvz_memcpy(out, out_size, colon, value_len);
    out[value_len] = '\0';
    return true;
}



/**
 * Parse the NPY shape tuple.
 *
 * @param header start of header text
 * @param header_end one-past-end pointer
 * @param dims output shape dimensions
 * @param max_dims max number of dimensions
 * @param out_count actual number of parsed dimensions
 * @return whether shape parsing succeeded
 */
static bool _parse_shape(
    const char* header, const char* header_end, uint32_t dims[], uint32_t max_dims,
    uint32_t* out_count)
{
    ANN(header);
    ANN(header_end);
    ANN(dims);
    ANN(out_count);

    const char* shape_key = strstr(header, "'shape'");
    if (shape_key == NULL || shape_key >= header_end)
        shape_key = strstr(header, "\"shape\"");
    if (shape_key == NULL || shape_key >= header_end)
        return false;

    const char* open = strchr(shape_key, '(');
    if (open == NULL || open >= header_end)
        return false;
    const char* close = strchr(open, ')');
    if (close == NULL || close <= open || close > header_end)
        return false;

    uint32_t count = 0;
    const char* p = open + 1;
    while (p < close && count < max_dims)
    {
        p = _skip_spaces(p, close);
        if (p >= close)
            break;
        if (!isdigit((uint8_t)(*p))
            || *p == ',')
        {
            p++;
            continue;
        }
        char* next = NULL;
        unsigned long value = strtoul(p, &next, 10);
        if (next == NULL || next == p)
            return false;
        dims[count++] = (uint32_t)value;
        p = next;
        if (p < close)
            p++;
    }
    if (count == 0)
        return false;
    *out_count = count;
    return true;
}



/**
 * Parse a NumPy header for an expected unsigned RGBA8 C-order 3D array.
 *
 * @param data decompressed NPY content
 * @param size decompressed size in bytes
 * @param out output parsed volume metadata
 * @return whether the header matches expectations
 */
static bool _parse_allen_mouse_brain_npy(
    char* data, DvzSize size, AllenMouseBrainVolume* out)
{
    ANN(data);
    ANN(out);
    if (size < 10)
        return false;

    const uint8_t* bytes = (const uint8_t*)data;
    if (memcmp(bytes, "\x93NUMPY", 6) != 0)
        return false;

    uint8_t major = bytes[6];
    uint8_t minor = bytes[7];
    (void)minor;
    uint64_t header_len = 0;
    if (major == 1)
    {
        if (size < 12)
            return false;
        header_len = _u16_le(bytes + 8);
    }
    else if (major == 2 || major == 3)
    {
        if (size < 14)
            return false;
        header_len = _u32_le(bytes + 8);
    }
    else
        return false;

    size_t header_start = 10;
    size_t header_end = header_start + (size_t)header_len;
    if (header_end > size || header_end <= header_start)
        return false;
    const char* header = (const char*)bytes + header_start;
    const char* header_limit = header + header_len;

    char descr[8] = {0};
    if (!_parse_key_quoted_value(header, header_limit, "'descr'", descr, sizeof(descr)) &&
        !_parse_key_quoted_value(header, header_limit, "\"descr\"", descr, sizeof(descr)))
        return false;
    if (strcmp(descr, "|u1") != 0)
        return false;

    const char* fortran_key = strstr(header, "'fortran_order'");
    if (fortran_key == NULL || fortran_key >= header_limit)
        fortran_key = strstr(header, "\"fortran_order\"");
    if (fortran_key == NULL || fortran_key >= header_limit)
        return false;
    const char* fortran_colon = strchr(fortran_key, ':');
    if (fortran_colon == NULL || fortran_colon >= header_limit)
        return false;
    const char* fortran_value = _skip_spaces(fortran_colon + 1, header_limit);
    if (strncmp(fortran_value, "False", 5) != 0)
        return false;

    uint32_t dims[5] = {0};
    uint32_t dims_count = 0;
    if (!_parse_shape(header, header_limit, dims, 5, &dims_count))
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
    if (dims_count == 3)
    {
        if (dims[0] != MOUSE_BRAIN_WIDTH || dims[1] != MOUSE_BRAIN_HEIGHT ||
            dims[2] != MOUSE_BRAIN_DEPTH)
            return false;
        width = MOUSE_BRAIN_WIDTH;
        height = MOUSE_BRAIN_HEIGHT;
        depth = MOUSE_BRAIN_DEPTH;
    }
    else if (dims_count == 4)
    {
        if (dims[3] != 4 || dims[0] != MOUSE_BRAIN_DEPTH || dims[1] != MOUSE_BRAIN_HEIGHT ||
            dims[2] != MOUSE_BRAIN_WIDTH)
            return false;
        width = MOUSE_BRAIN_WIDTH;
        height = MOUSE_BRAIN_HEIGHT;
        depth = MOUSE_BRAIN_DEPTH;
    }
    else
        return false;

    uint64_t payload_size = 0;
    if (_dvz_mul_u64_overflows((uint64_t)width, (uint64_t)height, &payload_size) ||
        _dvz_mul_u64_overflows(payload_size, depth, &payload_size) ||
        _dvz_mul_u64_overflows(payload_size, 4, &payload_size))
        return false;

    if (size < header_end || (DvzSize)(size - header_end) != payload_size)
        return false;

    out->raw_data = data;
    out->voxels = (uint8_t*)(data + header_end);
    out->width = width;
    out->height = height;
    out->depth = depth;
    return true;
}



/**
 * Read and validate the Allen mouse brain RGBA dataset.
 *
 * @param path file path
 * @param out output metadata and owned raw buffer
 * @return whether the dataset was read and validated
 */
static bool _read_allen_mouse_brain(const char* path, AllenMouseBrainVolume* out)
{
    ANN(path);
    ANN(out);
    if (out->raw_data != NULL)
        return false;

    DvzSize raw_size = 0;
    char* raw = dvz_read_gz(path, &raw_size);
    if (raw == NULL || raw_size == 0)
        return false;

    AllenMouseBrainVolume parsed = {0};
    if (!_parse_allen_mouse_brain_npy(raw, raw_size, &parsed))
    {
        dvz_free(raw);
        dvz_fprintf(stderr, "invalid Allen mouse brain .npy payload in %s\n", path);
        return false;
    }

    out->raw_data = parsed.raw_data;
    out->voxels = parsed.voxels;
    out->width = parsed.width;
    out->height = parsed.height;
    out->depth = parsed.depth;
    out->downsample = 1;
    return true;
}



/**
 * Return the ceiling of an unsigned integer division.
 *
 * @param value dividend
 * @param divisor divisor
 * @return ceil(value / divisor), or zero when divisor is zero
 */
static uint32_t _ceil_div_u32(uint32_t value, uint32_t divisor)
{
    if (divisor == 0)
        return 0;
    return value / divisor + (value % divisor != 0 ? 1u : 0u);
}



/**
 * Return the larger of two unsigned 32-bit integers.
 *
 * @param a first value
 * @param b second value
 * @return max(a, b)
 */
static uint32_t _max_u32(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}



/**
 * Downsample an RGBA volume by selecting the strongest voxel in every block.
 *
 * @param volume volume metadata and storage
 * @param factor integer downsample factor
 * @return whether downsampling succeeded
 */
static bool _downsample_allen_mouse_brain(AllenMouseBrainVolume* volume, uint32_t factor)
{
    ANN(volume);
    if (factor <= 1)
        return true;
    if (volume->voxels == NULL || volume->width == 0 || volume->height == 0 || volume->depth == 0)
        return false;

    uint32_t out_width = _ceil_div_u32(volume->width, factor);
    uint32_t out_height = _ceil_div_u32(volume->height, factor);
    uint32_t out_depth = _ceil_div_u32(volume->depth, factor);
    if (out_width == 0 || out_height == 0 || out_depth == 0)
        return false;

    uint64_t voxel_count = 0;
    uint64_t byte_count = 0;
    if (_dvz_mul_u64_overflows(out_width, out_height, &voxel_count) ||
        _dvz_mul_u64_overflows(voxel_count, out_depth, &voxel_count) ||
        _dvz_mul_u64_overflows(voxel_count, 4, &byte_count))
    {
        return false;
    }

    uint8_t* dst = (uint8_t*)dvz_calloc(byte_count, 1);
    if (dst == NULL)
        return false;

    const uint8_t* src = volume->voxels;
    uint32_t src_width = volume->width;
    uint32_t src_height = volume->height;
    uint32_t src_depth = volume->depth;
    for (uint32_t z = 0; z < out_depth; z++)
    {
        uint32_t z0 = z * factor;
        uint32_t z1 = _max_u32(z0 + 1, z0 + factor);
        if (z1 > src_depth)
            z1 = src_depth;
        for (uint32_t y = 0; y < out_height; y++)
        {
            uint32_t y0 = y * factor;
            uint32_t y1 = _max_u32(y0 + 1, y0 + factor);
            if (y1 > src_height)
                y1 = src_height;
            for (uint32_t x = 0; x < out_width; x++)
            {
                uint32_t x0 = x * factor;
                uint32_t x1 = _max_u32(x0 + 1, x0 + factor);
                if (x1 > src_width)
                    x1 = src_width;

                const uint8_t* best = NULL;
                uint32_t best_score = 0;
                for (uint32_t zz = z0; zz < z1; zz++)
                {
                    for (uint32_t yy = y0; yy < y1; yy++)
                    {
                        for (uint32_t xx = x0; xx < x1; xx++)
                        {
                            uint64_t src_index =
                                (((uint64_t)zz * src_height + yy) * src_width + xx) * 4u;
                            const uint8_t* candidate = src + src_index;
                            uint32_t luminance = (uint32_t)candidate[0] + candidate[1] +
                                                 candidate[2];
                            uint32_t score = (uint32_t)candidate[3] * 1024u + luminance;
                            if (best == NULL || score > best_score)
                            {
                                best = candidate;
                                best_score = score;
                            }
                        }
                    }
                }

                uint64_t dst_index = (((uint64_t)z * out_height + y) * out_width + x) * 4u;
                if (best != NULL)
                    dvz_memcpy(dst + dst_index, 4, best, 4);
            }
        }
    }

    volume->downsampled_data = dst;
    volume->voxels = dst;
    volume->width = out_width;
    volume->height = out_height;
    volume->depth = out_depth;
    volume->downsample = factor;
    return true;
}



/**
 * Swizzle the loaded Allen volume from raw storage axes to IBL scene axes.
 *
 * @param volume volume metadata and storage
 * @return whether swizzling succeeded
 */
static bool _swizzle_allen_mouse_brain_to_ibl_axes(AllenMouseBrainVolume* volume)
{
    ANN(volume);
    if (volume->voxels == NULL || volume->width == 0 || volume->height == 0 || volume->depth == 0)
        return false;

    uint32_t src_width = volume->width;
    uint32_t src_height = volume->height;
    uint32_t src_depth = volume->depth;
    uint32_t out_width = src_height;
    uint32_t out_height = src_depth;
    uint32_t out_depth = src_width;

    uint64_t voxel_count = 0;
    uint64_t byte_count = 0;
    if (_dvz_mul_u64_overflows(out_width, out_height, &voxel_count) ||
        _dvz_mul_u64_overflows(voxel_count, out_depth, &voxel_count) ||
        _dvz_mul_u64_overflows(voxel_count, 4, &byte_count))
    {
        return false;
    }

    uint8_t* dst = (uint8_t*)dvz_calloc(byte_count, 1);
    if (dst == NULL)
        return false;

    const uint8_t* src = volume->voxels;
    for (uint32_t ap = 0; ap < src_depth; ap++)
    {
        for (uint32_t ml = 0; ml < src_height; ml++)
        {
            for (uint32_t dv = 0; dv < src_width; dv++)
            {
                uint64_t src_index = (((uint64_t)ap * src_height + ml) * src_width + dv) * 4u;
                uint32_t ibl_dv = out_depth - 1u - dv;
                uint32_t ibl_ap = out_height - 1u - ap;
                uint64_t dst_index =
                    (((uint64_t)ibl_dv * out_height + ibl_ap) * out_width + ml) * 4u;
                dvz_memcpy(dst + dst_index, 4, src + src_index, 4);
            }
        }
    }

    uint8_t* previous_downsampled = volume->downsampled_data;
    volume->downsampled_data = dst;
    volume->voxels = dst;
    volume->width = out_width;
    volume->height = out_height;
    volume->depth = out_depth;
    if (previous_downsampled != NULL)
        dvz_free(previous_downsampled);
    return true;
}



/**
 * Normalize the Allen RGBA alpha channel for display transfer.
 *
 * @param volume volume metadata and storage
 */
static void _normalize_allen_alpha(AllenMouseBrainVolume* volume)
{
    ANN(volume);
    if (volume->voxels == NULL)
        return;

    uint64_t voxel_count = 0;
    if (_dvz_mul_u64_overflows(volume->width, volume->height, &voxel_count) ||
        _dvz_mul_u64_overflows(voxel_count, volume->depth, &voxel_count))
    {
        return;
    }

    uint8_t max_alpha = 0;
    for (uint64_t i = 0; i < voxel_count; i++)
    {
        uint8_t alpha = volume->voxels[4 * i + 3];
        if (alpha > max_alpha)
            max_alpha = alpha;
    }
    if (max_alpha == 0 || max_alpha == 255)
        return;

    for (uint64_t i = 0; i < voxel_count; i++)
    {
        uint8_t alpha = volume->voxels[4 * i + 3];
        if (alpha == 0)
            continue;
        uint32_t scaled = ((uint32_t)alpha * 255u + (uint32_t)max_alpha / 2u) / max_alpha;
        volume->voxels[4 * i + 3] = scaled > 255u ? 255u : (uint8_t)scaled;
    }
}



/**
 * Free owned Allen mouse brain volume storage.
 *
 * @param volume volume metadata and storage
 */
static void _allen_mouse_brain_destroy(AllenMouseBrainVolume* volume)
{
    if (volume == NULL)
        return;
    if (volume->downsampled_data != NULL)
        dvz_free(volume->downsampled_data);
    if (volume->raw_data != NULL)
        dvz_free(volume->raw_data);
    dvz_memset(volume, sizeof(AllenMouseBrainVolume), 0, sizeof(AllenMouseBrainVolume));
}



/**
 * Return centered object-space bounds preserving volume voxel aspect.
 *
 * @param volume volume metadata
 * @param bounds_min output minimum coordinate
 * @param bounds_max output maximum coordinate
 */
static void _volume_aspect_bounds(
    const AllenMouseBrainVolume* volume, double bounds_min[3], double bounds_max[3])
{
    ANN(volume);
    ANN(bounds_min);
    ANN(bounds_max);
    uint32_t max_dim = volume->width;
    if (volume->height > max_dim)
        max_dim = volume->height;
    if (volume->depth > max_dim)
        max_dim = volume->depth;
    if (max_dim == 0)
        max_dim = 1;

    double sx = (double)volume->width / (double)max_dim;
    double sy = (double)volume->height / (double)max_dim;
    double sz = (double)volume->depth / (double)max_dim;
    bounds_min[0] = -sx;
    bounds_min[1] = -sy;
    bounds_min[2] = -sz;
    bounds_max[0] = +sx;
    bounds_max[1] = +sy;
    bounds_max[2] = +sz;
}



/**
 * Apply the transparency technique required by the currently visible visual set.
 *
 * @param state example state
 */
static void _apply_transparency_modes(AllenMouseBrainState* state)
{
    ANN(state);
    if (state->slice_visual == NULL || state->volume_visual == NULL)
        return;

    (void)dvz_visual_set_alpha_mode(state->volume_visual, DVZ_ALPHA_BLENDED);
    (void)dvz_visual_set_alpha_mode(state->slice_visual, DVZ_ALPHA_BLENDED);
    if (state->atlas_mesh_visual != NULL)
    {
        bool atlas_opaque = state->show_atlas_mesh && state->atlas_mesh != NULL;
        if (atlas_opaque && state->atlas_mesh->region_count > 0)
        {
            for (uint32_t r = 0; r < state->atlas_mesh->region_count; r++)
            {
                const AllenIblAtlasRegion* region = &state->atlas_mesh->regions[r];
                float alpha = region->visible ? region->alpha * state->atlas_alpha_scale : 0.0f;
                if (alpha < 0.999f)
                {
                    atlas_opaque = false;
                    break;
                }
            }
        }
        else if (state->atlas_alpha_scale < 0.999f)
            atlas_opaque = false;
        (void)dvz_visual_set_alpha_mode(
            state->atlas_mesh_visual, atlas_opaque ? DVZ_ALPHA_OPAQUE : DVZ_ALPHA_WBOIT);
    }
}


/**
 * Apply retained atlas mesh visibility without mutating its buffers.
 *
 * @param state example state
 */
static void _apply_atlas_mesh_visibility(AllenMouseBrainState* state)
{
    ANN(state);
    if (state->atlas_mesh_visual == NULL)
        return;

    dvz_visual_set_visible(state->atlas_mesh_visual, state->show_atlas_mesh);
    _apply_transparency_modes(state);
}



/**
 * Upload retained atlas mesh controls.
 *
 * @param state example state
 */
static void _apply_atlas_mesh_controls(AllenMouseBrainState* state)
{
    ANN(state);
    if (state->atlas_mesh_visual == NULL || state->atlas_mesh == NULL)
        return;

    _apply_atlas_mesh_visibility(state);
    if (!state->show_atlas_mesh && state->atlas_uploads_initialized)
        return;

    for (uint32_t i = 0; i < state->atlas_mesh->vertex_count; i++)
    {
        uint32_t alpha = 255;
        alpha = (uint32_t)((float)alpha * state->atlas_alpha_scale + 0.5f);
        state->atlas_mesh->color[i][3] = alpha > 255u ? 255u : (uint8_t)alpha;
    }
    for (uint32_t r = 0; r < state->atlas_mesh->region_count; r++)
    {
        const AllenIblAtlasRegion* region = &state->atlas_mesh->regions[r];
        uint32_t end = region->vertex_start + region->vertex_count;
        if (end > state->atlas_mesh->vertex_count)
            end = state->atlas_mesh->vertex_count;
        float region_alpha = region->visible ? region->alpha * state->atlas_alpha_scale : 0.0f;
        uint32_t alpha = (uint32_t)(region_alpha * 255.0f + 0.5f);
        if (alpha > 255u)
            alpha = 255u;
        for (uint32_t i = region->vertex_start; i < end; i++)
            state->atlas_mesh->color[i][3] = (uint8_t)alpha;
    }

    if (dvz_visual_set_data(
            state->atlas_mesh_visual, "color", state->atlas_mesh->color,
            state->atlas_mesh->vertex_count) != 0)
    {
        dvz_fprintf(stderr, "failed to update Allen/IBL atlas mesh color\n");
    }

    if (dvz_visual_set_primitive_shading(
            state->atlas_mesh_visual,
            &(DvzPrimitiveShadingDesc){
                .light_direction = {
                    state->atlas_light_direction[0],
                    state->atlas_light_direction[1],
                    state->atlas_light_direction[2],
                },
                .ambient = state->atlas_ambient,
                .diffuse = state->atlas_diffuse,
            }) != 0)
    {
        dvz_fprintf(stderr, "failed to update Allen/IBL atlas mesh material\n");
    }
    state->atlas_uploads_initialized = true;
}



/**
 * Apply retained volume controls.
 *
 * @param state example state
 */
static void _apply_volume_controls(AllenMouseBrainState* state)
{
    ANN(state);
    if (state->slice_visual == NULL || state->volume_visual == NULL)
        return;

    DvzVolumeSamplingMode sampling =
        state->linear_sampling ? DVZ_VOLUME_SAMPLING_LINEAR : DVZ_VOLUME_SAMPLING_NEAREST;

    dvz_visual_set_visible(state->slice_visual, state->show_slice);
    (void)dvz_volume_set_render_mode(state->slice_visual, DVZ_VOLUME_RENDER_SLICE);
    (void)dvz_volume_set_opacity(
        state->slice_visual, state->show_slice ? state->slice_opacity : 0.0f);
    (void)dvz_volume_set_sampling(
        state->slice_visual, sampling);
    (void)dvz_volume_set_slice_axis(state->slice_visual, state->axis);
    (void)dvz_volume_set_slice_position(state->slice_visual, (double)state->slice_position);

    DvzVolumeRenderMode mode = DVZ_VOLUME_RENDER_MIP;
    if (state->render_mode == DVZ_VOLUME_RENDER_COMPOSITE)
        mode = DVZ_VOLUME_RENDER_COMPOSITE;
    uint32_t step_count = (uint32_t)(state->volume_steps + 0.5f);
    if (step_count < 1)
        step_count = 1;
    (void)dvz_volume_set_render_mode(state->volume_visual, mode);
    (void)dvz_volume_set_opacity(
        state->volume_visual, state->show_volume ? state->volume_opacity : 0.0f);
    (void)dvz_volume_set_sampling(state->volume_visual, sampling);
    (void)dvz_volume_set_step_count(state->volume_visual, step_count);

    if (state->clip_volume_at_slice)
    {
        double clip_min[3] = {0.0, 0.0, 0.0};
        double clip_max[3] = {1.0, 1.0, 1.0};
        uint32_t axis = (uint32_t)state->axis;
        if (axis > 2)
            axis = 2;
        if (state->keep_positive_side)
            clip_min[axis] = (double)state->slice_position;
        else
            clip_max[axis] = (double)state->slice_position;
        (void)dvz_volume_set_clipping_box(state->volume_visual, clip_min, clip_max);
    }
    else
    {
        (void)dvz_volume_clear_clipping(state->volume_visual);
    }
}


/**
 * Apply retained screen-space scene occlusion controls.
 *
 * @param state example state
 */
static void _apply_volume_occlusion_controls(AllenMouseBrainState* state)
{
    ANN(state);
    if (state->panel == NULL || state->slice_visual == NULL)
        return;

    DvzVolumeOcclusionDesc volume_occlusion = {
        .enabled = state->volume_occlusion_enabled,
        .alpha_threshold = state->occlusion_threshold,
        .fade_distance = state->occlusion_fade,
        .occluded_alpha = state->occlusion_hidden_alpha,
    };
    DvzSceneOcclusionDesc scene_occlusion = {
        .enabled = state->volume_occlusion_enabled,
        .depth_bias = 0.0005f,
        .soft_edge = state->occlusion_fade,
        .hidden_alpha = state->occlusion_hidden_alpha,
    };

    if (state->volume_visual != NULL)
    {
        (void)dvz_panel_set_volume_occluder(
            state->panel, state->volume_occlusion_enabled ? state->volume_visual : NULL,
            state->volume_occlusion_enabled ? &volume_occlusion : NULL);
        (void)dvz_visual_set_scene_occluder(
            state->volume_visual, state->volume_occlusion_enabled);
    }
    if (state->atlas_mesh_visual != NULL)
    {
        (void)dvz_visual_set_scene_occluder(
            state->atlas_mesh_visual, state->volume_occlusion_enabled);
    }
    (void)dvz_visual_set_volume_occluded(state->slice_visual, false);
    (void)dvz_visual_set_scene_occluded(
        state->slice_visual, state->volume_occlusion_enabled);
    (void)dvz_panel_set_scene_occlusion(
        state->panel, state->volume_occlusion_enabled ? &scene_occlusion : NULL);
}



/**
 * Build the retained controls GUI.
 *
 * @param gui GUI overlay
 * @param win application window
 * @param user_data callback state
 */
static void _allen_mouse_brain_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    AllenMouseBrainState* state = (AllenMouseBrainState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool atlas_changed = false;
    bool atlas_visibility_changed = false;
    bool occlusion_changed = false;
    if (dvz_gui_begin(gui, "Allen Mouse Brain", NULL, 0))
    {
        dvz_gui_text(gui, "Slice");
        changed |= dvz_gui_checkbox(gui, "Show slice", &state->show_slice);
        changed |= dvz_gui_slider_float(gui, "Slice position", &state->slice_position, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Slice opacity", &state->slice_opacity, 0.0f, 1.0f);
        if (dvz_gui_button(gui, "Slice axis ML"))
        {
            state->axis = DVZ_VOLUME_AXIS_X;
            changed = true;
        }
        if (dvz_gui_button(gui, "Slice axis AP"))
        {
            state->axis = DVZ_VOLUME_AXIS_Y;
            changed = true;
        }
        if (dvz_gui_button(gui, "Slice axis DV"))
        {
            state->axis = DVZ_VOLUME_AXIS_Z;
            changed = true;
        }

        dvz_gui_text(gui, "Volume");
        changed |= dvz_gui_checkbox(gui, "Show full volume", &state->show_volume);
        changed |= dvz_gui_checkbox(gui, "Clip volume at slice", &state->clip_volume_at_slice);
        changed |= dvz_gui_checkbox(gui, "Keep positive side", &state->keep_positive_side);
        changed |= dvz_gui_checkbox(gui, "Linear sampling", &state->linear_sampling);
        const char* render_modes[] = {"MIP volume", "Composite volume"};
        int render_mode = state->render_mode == DVZ_VOLUME_RENDER_COMPOSITE ? 1 : 0;
        if (dvz_gui_combo(gui, "Render mode", &render_mode, render_modes, 2))
        {
            state->render_mode =
                render_mode == 1 ? DVZ_VOLUME_RENDER_COMPOSITE : DVZ_VOLUME_RENDER_MIP;
            changed = true;
        }
        changed |= dvz_gui_slider_float(gui, "Volume opacity", &state->volume_opacity, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Volume steps", &state->volume_steps, 8.0f, 512.0f);

        dvz_gui_text(gui, "Volume occlusion");
        occlusion_changed |= dvz_gui_checkbox(
            gui, "Enable occlusion", &state->volume_occlusion_enabled);
        occlusion_changed |= dvz_gui_slider_float_format(
            gui, "Volume front threshold", &state->occlusion_threshold, 0.000001f, 0.30f,
            "%.6f");
        occlusion_changed |= dvz_gui_slider_float_format(
            gui, "Soft edge", &state->occlusion_fade, 0.000001f, 0.25f, "%.6f");
        occlusion_changed |= dvz_gui_slider_float(
            gui, "Hidden slice visibility", &state->occlusion_hidden_alpha, 0.0f, 0.60f);
        if (dvz_gui_button(gui, "Reset occlusion"))
        {
            state->volume_occlusion_enabled = true;
            state->occlusion_threshold = DEFAULT_OCCLUSION_THRESHOLD;
            state->occlusion_fade = DEFAULT_OCCLUSION_FADE;
            state->occlusion_hidden_alpha = DEFAULT_OCCLUSION_HIDDEN_ALPHA;
            occlusion_changed = true;
        }

        if (state->atlas_mesh_visual != NULL)
        {
            dvz_gui_text(gui, "Atlas mesh");
            atlas_visibility_changed |=
                dvz_gui_checkbox(gui, "Show atlas mesh", &state->show_atlas_mesh);
            atlas_changed |=
                dvz_gui_slider_float(
                    gui, "Atlas alpha scale", &state->atlas_alpha_scale, 0.20f, 1.50f);
            atlas_changed |=
                dvz_gui_slider_float(gui, "Atlas ambient", &state->atlas_ambient, 0.0f, 1.0f);
            atlas_changed |=
                dvz_gui_slider_float(gui, "Atlas diffuse", &state->atlas_diffuse, 0.0f, 1.5f);
            if (state->atlas_mesh != NULL && state->atlas_mesh->region_count > 0)
            {
                dvz_gui_text(gui, "Atlas regions");
                if (dvz_gui_button(gui, "Show all regions"))
                {
                    for (uint32_t r = 0; r < state->atlas_mesh->region_count; r++)
                        state->atlas_mesh->regions[r].visible = true;
                    atlas_changed = true;
                }
                if (dvz_gui_button(gui, "Hide all regions"))
                {
                    for (uint32_t r = 0; r < state->atlas_mesh->region_count; r++)
                        state->atlas_mesh->regions[r].visible = false;
                    atlas_changed = true;
                }
                for (uint32_t r = 0; r < state->atlas_mesh->region_count; r++)
                {
                    AllenIblAtlasRegion* region = &state->atlas_mesh->regions[r];
                    char label[160] = {0};
                    dvz_snprintf(
                        label, sizeof(label), "%s - %s", region->acronym, region->name);
                    atlas_changed |= dvz_gui_checkbox(gui, label, &region->visible);
                    dvz_snprintf(
                        label, sizeof(label), "%s alpha", region->acronym);
                    atlas_changed |= dvz_gui_slider_float(gui, label, &region->alpha, 0.0f, 1.0f);
                }
            }
        }
        if (dvz_gui_button(gui, "Reset"))
        {
            state->show_slice = true;
            state->show_volume = true;
            state->show_atlas_mesh = false;
            state->clip_volume_at_slice = true;
            state->keep_positive_side = false;
            state->linear_sampling = true;
            state->volume_occlusion_enabled = true;
            state->render_mode = DVZ_VOLUME_RENDER_MIP;
            state->slice_opacity = DEFAULT_SLICE_OPACITY;
            state->volume_opacity = DEFAULT_VOLUME_OPACITY;
            state->volume_steps = DEFAULT_VOLUME_STEPS;
            state->occlusion_threshold = DEFAULT_OCCLUSION_THRESHOLD;
            state->occlusion_fade = DEFAULT_OCCLUSION_FADE;
            state->occlusion_hidden_alpha = DEFAULT_OCCLUSION_HIDDEN_ALPHA;
            state->atlas_alpha_scale = DEFAULT_ATLAS_ALPHA_SCALE;
            state->atlas_ambient = 0.22f;
            state->atlas_diffuse = 0.95f;
            state->slice_position = DEFAULT_SLICE_POS;
            state->axis = DEFAULT_AXIS;
            if (state->atlas_mesh != NULL)
            {
                for (uint32_t r = 0; r < state->atlas_mesh->region_count; r++)
                {
                    state->atlas_mesh->regions[r].visible = true;
                    state->atlas_mesh->regions[r].alpha = 1.0f;
                }
            }
            changed = true;
            atlas_changed = true;
            occlusion_changed = true;
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _apply_volume_controls(state);
    if (occlusion_changed)
        _apply_volume_occlusion_controls(state);
    if (atlas_visibility_changed)
        _apply_atlas_mesh_visibility(state);
    if (atlas_changed)
        _apply_atlas_mesh_controls(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = _frame_count(argc, argv);
    uint32_t downsample = _option_u32(argc, argv, "downsample", 1, 1, 8);

    char data_path[1024] = {0};
    if (!_data_path(argc, argv, data_path, sizeof(data_path)))
    {
        dvz_fprintf(stderr, "failed to resolve data path\n");
        return 1;
    }

    AllenMouseBrainVolume volume_data = {0};
    if (!_read_allen_mouse_brain(data_path, &volume_data))
    {
        dvz_fprintf(
            stderr,
            "failed to load %s\n"
            "prepare the file with the repository data folder or pass --data-file=<path>\n",
            data_path);
        return 1;
    }
    if (!_downsample_allen_mouse_brain(&volume_data, downsample))
    {
        dvz_fprintf(stderr, "failed to downsample Allen mouse brain volume by %u\n", downsample);
        _allen_mouse_brain_destroy(&volume_data);
        return 1;
    }
    if (!_swizzle_allen_mouse_brain_to_ibl_axes(&volume_data))
    {
        dvz_fprintf(stderr, "failed to swizzle Allen mouse brain volume into IBL axes\n");
        _allen_mouse_brain_destroy(&volume_data);
        return 1;
    }
    _normalize_allen_alpha(&volume_data);
    if (volume_data.downsample > 1)
    {
        dvz_fprintf(
            stderr, "using downsampled Allen mouse brain volume %ux%ux%u (factor %u)\n",
            volume_data.width, volume_data.height, volume_data.depth, volume_data.downsample);
    }

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        return 1;
    }
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    dvz_scene_set_capabilities(scene, &caps);

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 2.2f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.01f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = volume_data.width,
                   .height = volume_data.height,
                   .depth = volume_data.depth,
               });
    if (field == NULL)
    {
        dvz_fprintf(stderr, "dvz_sampled_field() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = volume_data.voxels,
                       .bytes_per_row = volume_data.width * 4,
                       .rows_per_image = volume_data.height,
                   }))
    {
        dvz_fprintf(stderr, "dvz_sampled_field_set_data() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* volume_3d = dvz_volume(scene, 0);
    DvzVisual* volume_slice = dvz_volume(scene, 0);
    if (volume_3d == NULL || volume_slice == NULL)
    {
        dvz_fprintf(stderr, "dvz_volume() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (!dvz_visual_set_field(volume_3d, "field", field) ||
        !dvz_visual_set_field(volume_slice, "field", field))
    {
        dvz_fprintf(stderr, "dvz_visual_set_field() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_visual_set_alpha_mode(volume_3d, DVZ_ALPHA_WBOIT) != 0 ||
        dvz_visual_set_alpha_mode(volume_slice, DVZ_ALPHA_WBOIT) != 0)
    {
        dvz_fprintf(stderr, "dvz_visual_set_alpha_mode() failed\n");
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    AllenIblAtlasMesh atlas_mesh = {0};
    bool atlas_loaded = _load_ibl_atlas_mesh(DEFAULT_IBL_ASSET_DIR, &atlas_mesh);
    if (!atlas_loaded)
    {
        dvz_fprintf(
            stderr,
            "Allen/IBL atlas mesh assets not found in %s; continuing with volume only\n"
            "prepare them with: python tools/prepare_allen_ibl_assets.py\n",
            DEFAULT_IBL_ASSET_DIR);
    }

    double bounds_min[3] = {0};
    double bounds_max[3] = {0};
    _volume_aspect_bounds(&volume_data, bounds_min, bounds_max);
    if (atlas_mesh.has_volume_bounds)
    {
        bounds_min[0] = atlas_mesh.volume_bounds_min[0];
        bounds_min[1] = atlas_mesh.volume_bounds_min[1];
        bounds_min[2] = atlas_mesh.volume_bounds_min[2];
        bounds_max[0] = atlas_mesh.volume_bounds_max[0];
        bounds_max[1] = atlas_mesh.volume_bounds_max[1];
        bounds_max[2] = atlas_mesh.volume_bounds_max[2];
    }
    if (dvz_volume_set_bounds(volume_3d, bounds_min, bounds_max) != 0 ||
        dvz_volume_set_bounds(volume_slice, bounds_min, bounds_max) != 0)
    {
        dvz_fprintf(stderr, "dvz_volume_set_bounds() failed\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* atlas_mesh_visual = NULL;
    DvzSceneBuffer* atlas_index_buffer = NULL;
    if (atlas_loaded)
    {
        atlas_mesh_visual = dvz_mesh(scene, 0);
        atlas_index_buffer = dvz_scene_buffer(
            scene, &(DvzSceneBufferDesc){
                       .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                       .stride = sizeof(DvzIndex),
                   });
        if (atlas_mesh_visual == NULL || atlas_index_buffer == NULL ||
            !dvz_scene_buffer_set_data(
                atlas_index_buffer, atlas_mesh.idx, atlas_mesh.index_count * sizeof(DvzIndex)) ||
            dvz_visual_set_data(
                atlas_mesh_visual, "position", atlas_mesh.pos, atlas_mesh.vertex_count) != 0 ||
            dvz_visual_set_data(
                atlas_mesh_visual, "normal", atlas_mesh.normal, atlas_mesh.vertex_count) != 0 ||
            !dvz_visual_set_buffer(atlas_mesh_visual, "index", atlas_index_buffer) ||
            dvz_visual_set_alpha_mode(atlas_mesh_visual, DVZ_ALPHA_WBOIT) != 0 ||
            dvz_visual_set_depth_test(atlas_mesh_visual, true) != 0)
        {
            dvz_fprintf(stderr, "Allen/IBL atlas mesh visual setup failed\n");
            _ibl_atlas_mesh_destroy(&atlas_mesh);
            _allen_mouse_brain_destroy(&volume_data);
            dvz_scene_destroy(scene);
            return 1;
        }
    }

    DvzVisualAttachDesc volume_attach = {
        .z_layer = 0,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    DvzVisualAttachDesc slice_attach = {
        .z_layer = 1,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    DvzVisualAttachDesc atlas_attach = {
        .z_layer = 2,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    if (dvz_panel_add_visual(panel, volume_3d, &volume_attach) != 0 ||
        dvz_panel_add_visual(panel, volume_slice, &slice_attach) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (atlas_mesh_visual != NULL && dvz_panel_add_visual(panel, atlas_mesh_visual, &atlas_attach) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed for Allen/IBL atlas mesh\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.025f, 0.035f, 0.045f, 1.0f);

    AllenMouseBrainState state = {
        .panel = panel,
        .slice_visual = volume_slice,
        .volume_visual = volume_3d,
        .atlas_mesh_visual = atlas_mesh_visual,
        .atlas_index_buffer = atlas_index_buffer,
        .atlas_mesh = atlas_loaded ? &atlas_mesh : NULL,
        .show_slice = true,
        .show_volume = true,
        .show_atlas_mesh = false,
        .clip_volume_at_slice = true,
        .keep_positive_side = false,
        .linear_sampling = true,
        .volume_occlusion_enabled = true,
        .render_mode = DVZ_VOLUME_RENDER_COMPOSITE,
        .slice_opacity = DEFAULT_SLICE_OPACITY,
        .volume_opacity = DEFAULT_VOLUME_OPACITY,
        .volume_steps = DEFAULT_VOLUME_STEPS,
        .occlusion_threshold = DEFAULT_OCCLUSION_THRESHOLD,
        .occlusion_fade = DEFAULT_OCCLUSION_FADE,
        .occlusion_hidden_alpha = DEFAULT_OCCLUSION_HIDDEN_ALPHA,
        .atlas_alpha_scale = DEFAULT_ATLAS_ALPHA_SCALE,
        .atlas_ambient = 0.22f,
        .atlas_diffuse = 0.95f,
        .atlas_light_direction = {0.20f, 0.70f, 0.45f},
        .slice_position = DEFAULT_SLICE_POS,
        .axis = DEFAULT_AXIS,
    };
    _apply_volume_controls(&state);
    _apply_volume_occlusion_controls(&state);
    _apply_transparency_modes(&state);
    _apply_atlas_mesh_controls(&state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "allen_mouse_brain_slice_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        _ibl_atlas_mesh_destroy(&atlas_mesh);
        _allen_mouse_brain_destroy(&volume_data);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _allen_mouse_brain_gui, &state);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    dvz_app_run(app, frame_count);

    _ibl_atlas_mesh_destroy(&atlas_mesh);
    _allen_mouse_brain_destroy(&volume_data);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
