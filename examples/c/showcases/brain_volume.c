/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* brain_volume - This example renders a prepared Allen mouse brain volume with a visible slice.
 *
 * What to look for: the loader reads the prepared compressed NumPy RGBA volume, downsamples it,
 * derives scalar voxels for occlusion, and binds the same 3D sampled field to a composite volume and
 * a slice visual. Compare the translucent context volume with the sharper slice plane while using
 * the arcball camera.
 *
 * This workflow is useful for volume-data previews where a slice should stay spatially embedded in
 * the 3D anatomy. The prepared data-submodule file must exist, or it must be generated with the
 * preparation command.
 *
 * Scenario: brain_volume
 * Style: showcase, graphite_cyan, 1280x720 window target
 *
 * Build:   just example-c showcases/brain_volume
 * Run:     ./build/examples/c/showcases/brain_volume --live
 * Smoke:   ./build/examples/c/showcases/brain_volume --png
 * Data:    data/examples/allen_ibl/prepared/allen_mouse_brain_rgba.npy.gz
 * Prepare: python tools/data/prepare_brain_volume.py
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
#include "datoviz/fileio.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "example_tuner.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT

#define DEFAULT_DATA_PATH "data/examples/allen_ibl/prepared/allen_mouse_brain_rgba.npy.gz"
#define DEFAULT_AXIS          DVZ_VOLUME_AXIS_Y
#define DEFAULT_SLICE_POS     0.5f
#define DEFAULT_SLICE_OPACITY 1.0f
#define DEFAULT_VOLUME_OPACITY 0.85f
#define DEFAULT_VOLUME_STEPS  192.0f
#define DEFAULT_DOWNSAMPLE    2u
#define DEFAULT_OCCLUSION_THRESHOLD 0.000001f
#define DEFAULT_OCCLUSION_FADE 0.000001f
#define DEFAULT_OCCLUSION_HIDDEN_ALPHA 0.097f
#define MOUSE_BRAIN_WIDTH 320
#define MOUSE_BRAIN_HEIGHT 456
#define MOUSE_BRAIN_DEPTH 528
#define BRAIN_SCALAR_TISSUE_FLOOR 32u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct AllenMouseBrainVolume
{
    char* raw_data;
    uint8_t* downsampled_data;
    uint8_t* voxels;
    uint8_t* scalar_voxels;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t downsample;
} AllenMouseBrainVolume;


typedef struct BrainVolumeState
{
    AllenMouseBrainVolume volume_data;
    ExampleTuner tuner;
} BrainVolumeState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
 * Return the raw Allen texture-axis mapping into IBL scene axes.
 *
 * The raw texture is stored as DV, ML, AP. The IBL scene axes are ML, reversed AP, reversed DV.
 *
 * @param axis_order output texture-axis source order
 * @param axis_flip output per-texture-axis flip flags
 */
static void _allen_mouse_brain_axis_mapping(uint32_t axis_order[3], bool axis_flip[3])
{
    ANN(axis_order);
    ANN(axis_flip);
    axis_order[0] = 2;  /* texture DV samples scene Z. */
    axis_order[1] = 0;  /* texture ML samples scene X. */
    axis_order[2] = 1;  /* texture AP samples scene Y. */
    axis_flip[0] = true;
    axis_flip[1] = false;
    axis_flip[2] = true;
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
 * Return the number of voxels in a loaded Allen mouse brain volume.
 *
 * @param volume volume metadata
 * @param out output voxel count
 * @return whether the count is valid
 */
static bool _allen_mouse_brain_voxel_count(const AllenMouseBrainVolume* volume, uint64_t* out)
{
    ANN(volume);
    ANN(out);
    if (volume->width == 0 || volume->height == 0 || volume->depth == 0)
        return false;

    uint64_t voxel_count = 0;
    if (_dvz_mul_u64_overflows(volume->width, volume->height, &voxel_count) ||
        _dvz_mul_u64_overflows(voxel_count, volume->depth, &voxel_count))
    {
        return false;
    }
    *out = voxel_count;
    return true;
}



/**
 * Derive an R8 scalar density volume from the source RGBA Allen mouse brain volume.
 *
 * @param volume volume metadata and storage
 * @return whether the scalar volume was created
 */
static bool _build_scalar_volume(AllenMouseBrainVolume* volume)
{
    ANN(volume);
    if (volume->voxels == NULL)
        return false;

    uint64_t voxel_count = 0;
    if (!_allen_mouse_brain_voxel_count(volume, &voxel_count) || voxel_count > SIZE_MAX)
        return false;

    uint8_t* scalar = (uint8_t*)dvz_malloc((DvzSize)voxel_count);
    if (scalar == NULL)
        return false;

    uint8_t min_signal = 255;
    uint8_t max_signal = 0;
    for (uint64_t i = 0; i < voxel_count; i++)
    {
        const uint8_t* rgba = volume->voxels + 4 * i;
        const uint32_t alpha = rgba[3];
        if (alpha == 0)
        {
            scalar[i] = 0;
            continue;
        }

        const uint32_t luma =
            (77u * rgba[0] + 150u * rgba[1] + 29u * rgba[2] + 128u) / 256u;
        scalar[i] = (uint8_t)luma;
        if (scalar[i] < min_signal)
            min_signal = scalar[i];
        if (scalar[i] > max_signal)
            max_signal = scalar[i];
    }

    if (max_signal > min_signal)
    {
        for (uint64_t i = 0; i < voxel_count; i++)
        {
            if (scalar[i] == 0)
                continue;
            uint32_t stretched =
                BRAIN_SCALAR_TISSUE_FLOOR +
                ((uint32_t)(scalar[i] - min_signal) * (255u - BRAIN_SCALAR_TISSUE_FLOOR) +
                 (uint32_t)(max_signal - min_signal) / 2u) /
                    (uint32_t)(max_signal - min_signal);
            if (stretched > 255u)
                stretched = 255u;
            scalar[i] = (uint8_t)stretched;
        }
    }

    dvz_free(volume->scalar_voxels);
    volume->scalar_voxels = scalar;
    return true;
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
    if (volume->scalar_voxels != NULL)
        dvz_free(volume->scalar_voxels);
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
 * Attach a scalar volume transfer function using the shared example palette.
 *
 * @param scene scene owning scale resources
 * @param visual volume visual
 * @param slice whether the visual is the opaque slice plane
 * @return whether transfer resources were attached
 */
static bool _attach_brain_transfer(DvzScene* scene, DvzVisual* visual, bool slice)
{
    ANN(scene);
    ANN(visual);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "brain density"});
    if (scale == NULL)
        return false;
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return false;

    DvzColor bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_FRAME_BG);
    DvzColor panel = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzColormapStop stops[] = {
        {.position = 0.00, .rgba = {bg.r, bg.g, bg.b, 255}},
        {.position = 0.10, .rgba = {panel.r, panel.g, panel.b, 255}},
        {.position = 0.26, .rgba = {24, 64, 82, 255}},
        {.position = 0.48, .rgba = {primary.r, primary.g, primary.b, 255}},
        {.position = 0.70, .rgba = {secondary.r, secondary.g, secondary.b, 255}},
        {.position = 0.90, .rgba = {secondary.r, secondary.g, secondary.b, 255}},
        {.position = 1.00, .rgba = {text.r, text.g, text.b, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, DVZ_ARRAY_COUNT(stops));
    dvz_scale_set_colormap(scale, colormap);

    if (slice)
    {
        DvzVolumeAlphaStop alpha[] = {
            {.position = 0.00, .alpha = 0.00f},
            {.position = 0.01, .alpha = 0.00f},
            {.position = 0.03, .alpha = 1.00f},
            {.position = 1.00, .alpha = 1.00f},
        };
        if (dvz_volume_set_alpha_stops(visual, alpha, DVZ_ARRAY_COUNT(alpha)) != 0)
            return false;
    }
    else
    {
        DvzVolumeAlphaStop alpha[] = {
            {.position = 0.00, .alpha = 0.00f},
            {.position = 0.12, .alpha = 0.00f},
            {.position = 0.24, .alpha = 0.06f},
            {.position = 0.42, .alpha = 0.22f},
            {.position = 0.62, .alpha = 0.46f},
            {.position = 0.84, .alpha = 0.74f},
            {.position = 1.00, .alpha = 0.90f},
        };
        if (dvz_volume_set_alpha_stops(visual, alpha, DVZ_ARRAY_COUNT(alpha)) != 0)
            return false;
    }
    return dvz_visual_set_scale(visual, "color", scale) == 0;
}



/**
 * Configure the fixed volume and slice composition.
 *
 * @param volume volume visual
 * @param slice slice visual
 */
static void _configure_volume_slice(DvzVisual* volume, DvzVisual* slice)
{
    ANN(volume);
    ANN(slice);

    (void)dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED);
    (void)dvz_visual_set_alpha_mode(slice, DVZ_ALPHA_BLENDED);

    (void)dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE);
    (void)dvz_volume_set_value_range(volume, 0.04, 1.0);
    (void)dvz_volume_set_opacity(volume, DEFAULT_VOLUME_OPACITY);
    (void)dvz_volume_set_sampling(volume, DVZ_VOLUME_SAMPLING_LINEAR);
    (void)dvz_volume_set_step_count(volume, (uint32_t)DEFAULT_VOLUME_STEPS);

    double clip_min[3] = {0.0, 0.0, 0.0};
    double clip_max[3] = {1.0, 1.0, 1.0};
    clip_max[(uint32_t)DEFAULT_AXIS] = DEFAULT_SLICE_POS;
    (void)dvz_volume_set_clipping_box(volume, clip_min, clip_max);

    (void)dvz_volume_set_render_mode(slice, DVZ_VOLUME_RENDER_SLICE);
    (void)dvz_volume_set_value_range(slice, 0.00, 1.0);
    (void)dvz_volume_set_opacity(slice, DEFAULT_SLICE_OPACITY);
    (void)dvz_volume_set_sampling(slice, DVZ_VOLUME_SAMPLING_LINEAR);
    (void)dvz_volume_set_slice_axis(slice, DEFAULT_AXIS);
    (void)dvz_volume_set_slice_position(slice, (double)DEFAULT_SLICE_POS);
}



/**
 * Enable volume occlusion so the slice is hidden by the visible volume side.
 *
 * @param panel panel owning the visuals
 * @param volume volume occluder
 * @param slice volume-occluded slice
 */
static void _enable_slice_volume_occlusion(DvzPanel* panel, DvzVisual* volume, DvzVisual* slice)
{
    ANN(panel);
    ANN(volume);
    ANN(slice);

    DvzVolumeOcclusionDesc volume_occlusion = {DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
        .enabled = true,
        .alpha_threshold = DEFAULT_OCCLUSION_THRESHOLD,
        .fade_distance = DEFAULT_OCCLUSION_FADE,
        .occluded_alpha = DEFAULT_OCCLUSION_HIDDEN_ALPHA,
    };
    (void)dvz_panel_set_volume_occluder(panel, volume, &volume_occlusion);
    (void)dvz_visual_set_scene_occluder(volume, false);
    (void)dvz_visual_set_volume_occluded(slice, true);
    (void)dvz_visual_set_scene_occluded(slice, false);
    (void)dvz_panel_set_scene_occlusion(panel, NULL);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the brain volume showcase scenario.
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
    BrainVolumeState* state = (BrainVolumeState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->tuner = example_tuner("Allen brain view");
    if (out_user != NULL)
        *out_user = state;

    AllenMouseBrainVolume* volume_data = &state->volume_data;
    if (!_read_allen_mouse_brain(DEFAULT_DATA_PATH, volume_data))
    {
        dvz_fprintf(stderr, "failed to load %s\n", DEFAULT_DATA_PATH);
        goto cleanup;
    }
    if (!_downsample_allen_mouse_brain(volume_data, DEFAULT_DOWNSAMPLE))
    {
        dvz_fprintf(
            stderr, "failed to downsample Allen mouse brain volume by %u\n",
            DEFAULT_DOWNSAMPLE);
        goto cleanup;
    }
    _normalize_allen_alpha(volume_data);
    if (!_build_scalar_volume(volume_data))
    {
        dvz_fprintf(stderr, "failed to derive scalar Allen mouse brain volume\n");
        goto cleanup;
    }
    if (volume_data->downsample > 1)
    {
        dvz_fprintf(
            stderr, "using downsampled Allen mouse brain volume %ux%ux%u (factor %u)\n",
            volume_data->width, volume_data->height, volume_data->depth,
            volume_data->downsample);
    }

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    dvz_scene_set_capabilities(ctx->scene, &caps);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    example_tuner_figure(&state->tuner, ctx->figure);

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[0] = -1.55f;
    camera_desc.view.eye[1] = 1.58f;
    camera_desc.view.eye[2] = -2.45f;
    camera_desc.projection.fov_y = 0.72f;
    camera_desc.projection.near_clip = 0.01f;
    camera_desc.projection.far_clip = 100.0f;
    DvzResult camera_rc = dvz_panel_set_camera_desc(panel, &camera_desc);
    EXAMPLE_CHECK(camera_rc == 0, "dvz_panel_set_camera_desc() failed");
    DvzCamera* camera = dvz_panel_camera(panel);
    EXAMPLE_CHECK(camera != NULL, "dvz_panel_camera() failed");

    DvzSampledField* field = dvz_sampled_field(
        ctx->scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = volume_data->width,
                   .height = volume_data->height,
                   .depth = volume_data->depth,
               });
    EXAMPLE_CHECK(field != NULL, "dvz_sampled_field() failed");
    ok = dvz_sampled_field_set_data(
             field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                        .data = volume_data->scalar_voxels,
                        .bytes_per_row = volume_data->width,
                        .rows_per_image = volume_data->height,
                    }) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_sampled_field_set_data() failed");

    DvzVisual* volume_3d = dvz_volume(ctx->scene, 0);
    DvzVisual* volume_slice = dvz_volume(ctx->scene, 0);
    EXAMPLE_CHECK(volume_3d != NULL && volume_slice != NULL, "dvz_volume() failed");
    ok = dvz_visual_set_field(volume_3d, "field", field) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_visual_set_field(volume_3d) failed");

    ok = dvz_visual_set_field(volume_slice, "field", field) == DVZ_OK;
    EXAMPLE_CHECK(ok, "dvz_visual_set_field(volume_slice) failed");
    EXAMPLE_CHECK(
        _attach_brain_transfer(ctx->scene, volume_3d, false), "volume transfer setup failed");
    EXAMPLE_CHECK(
        _attach_brain_transfer(ctx->scene, volume_slice, true), "slice transfer setup failed");

    AllenMouseBrainVolume display_volume = *volume_data;
    display_volume.width = volume_data->height;
    display_volume.height = volume_data->depth;
    display_volume.depth = volume_data->width;
    double bounds_min[3] = {0};
    double bounds_max[3] = {0};
    _volume_aspect_bounds(&display_volume, bounds_min, bounds_max);
    int rc = dvz_volume_set_bounds(volume_3d, bounds_min, bounds_max);
    EXAMPLE_CHECK(rc == 0, "dvz_volume_set_bounds(volume_3d) failed");

    rc = dvz_volume_set_bounds(volume_slice, bounds_min, bounds_max);
    EXAMPLE_CHECK(rc == 0, "dvz_volume_set_bounds(volume_slice) failed");

    uint32_t axis_order[3] = {0};
    bool axis_flip[3] = {0};
    _allen_mouse_brain_axis_mapping(axis_order, axis_flip);
    rc = dvz_volume_set_axis_mapping(volume_3d, axis_order, axis_flip);
    EXAMPLE_CHECK(rc == 0, "dvz_volume_set_axis_mapping(volume_3d) failed");

    rc = dvz_volume_set_axis_mapping(volume_slice, axis_order, axis_flip);
    EXAMPLE_CHECK(rc == 0, "dvz_volume_set_axis_mapping(volume_slice) failed");

    DvzVisualAttachDesc volume_attach = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
        .z_layer = 0,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    DvzVisualAttachDesc slice_attach = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
        .z_layer = 1,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    rc = dvz_panel_add_visual(panel, volume_3d, &volume_attach);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(volume_3d) failed");

    rc = dvz_panel_add_visual(panel, volume_slice, &slice_attach);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(volume_slice) failed");

    example_graphite_cyan_set_panel_background(panel);

    _configure_volume_slice(volume_3d, volume_slice);
    _enable_slice_volume_occlusion(panel, volume_3d, volume_slice);

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");
    vec3 arcball_angles = {-0.42f, +0.26f, -0.16f};
    vec2 arcball_pan = {0.0f, 0.0f};
    dvz_arcball_set(arcball, arcball_angles);
    example_tuner_camera_ref(&state->tuner, "Camera", panel, camera, &camera_desc);
    example_tuner_arcball(
        &state->tuner, "Arcball", arcball, arcball_angles, 1.0f, arcball_pan);
    example_tuner_volume(&state->tuner, "3D volume", volume_3d, NULL);
    example_tuner_volume(&state->tuner, "Slice", volume_slice, NULL);

    ok = true;
cleanup:
    return ok;
}



static bool _scenario_native_view(DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)app;
    BrainVolumeState* state = (BrainVolumeState*)user;
    if (
        ctx == NULL || ctx->presentation != DVZ_RUNNER_PRESENT_GLFW || state == NULL ||
        view == NULL)
        return true;

    return example_tuner_attach(&state->tuner, view);
}



/**
 * Destroy the brain volume showcase scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    BrainVolumeState* state = (BrainVolumeState*)user;
    if (state == NULL)
        return;
    example_tuner_detach(&state->tuner);
    _allen_mouse_brain_destroy(&state->volume_data);
    dvz_free(state);
}



/**
 * Return the brain volume showcase scenario.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _brain_volume_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "brain_volume",
        .title = "Allen Mouse Brain",
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
 * Run the brain volume showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _brain_volume_scenario();
    if (example_cli_wants_live_gui(argc, argv))
        spec.native_view = _scenario_native_view;
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
