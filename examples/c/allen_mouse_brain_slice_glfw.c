/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* allen_mouse_brain_slice_glfw - local Allen mouse brain RGBA volume slice.
 *
 * Build:   just example-c allen_mouse_brain_slice_glfw
 * Run:     ./build/examples/c/allen_mouse_brain_slice_glfw
 * Data:    data/volumes/allen_mouse_brain_rgba.npy.gz
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
#define DEFAULT_AXIS          DVZ_VOLUME_AXIS_Z
#define DEFAULT_SLICE_POS     0.5f
#define DEFAULT_OPACITY       1.0f
#define MOUSE_BRAIN_WIDTH 320
#define MOUSE_BRAIN_HEIGHT 456
#define MOUSE_BRAIN_DEPTH 528



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct AllenMouseBrainVolume
{
    char* raw_data;
    uint8_t* voxels;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} AllenMouseBrainVolume;



typedef struct AllenMouseBrainState
{
    DvzVisual* volume;
    bool linear_sampling;
    float opacity;
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
    return true;
}



/**
 * Apply retained volume controls.
 *
 * @param state example state
 */
static void _apply_volume_controls(AllenMouseBrainState* state)
{
    ANN(state);
    if (state->volume == NULL)
        return;
    (void)dvz_volume_set_render_mode(state->volume, DVZ_VOLUME_RENDER_SLICE);
    (void)dvz_volume_set_opacity(state->volume, state->opacity);
    (void)dvz_volume_set_sampling(
        state->volume,
        state->linear_sampling ? DVZ_VOLUME_SAMPLING_LINEAR : DVZ_VOLUME_SAMPLING_NEAREST);
    (void)dvz_volume_set_slice_axis(state->volume, state->axis);
    (void)dvz_volume_set_slice_position(state->volume, (double)state->slice_position);
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
    if (dvz_gui_begin(gui, "Allen Mouse Brain", NULL, 0))
    {
        if (dvz_gui_button(gui, "Axis X"))
        {
            state->axis = DVZ_VOLUME_AXIS_X;
            changed = true;
        }
        if (dvz_gui_button(gui, "Axis Y"))
        {
            state->axis = DVZ_VOLUME_AXIS_Y;
            changed = true;
        }
        if (dvz_gui_button(gui, "Axis Z"))
        {
            state->axis = DVZ_VOLUME_AXIS_Z;
            changed = true;
        }
        changed |= dvz_gui_checkbox(gui, "Linear sampling", &state->linear_sampling);
        changed |= dvz_gui_slider_float(gui, "Slice position", &state->slice_position, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Opacity", &state->opacity, 0.0f, 1.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            state->linear_sampling = true;
            state->opacity = DEFAULT_OPACITY;
            state->slice_position = DEFAULT_SLICE_POS;
            state->axis = DEFAULT_AXIS;
            changed = true;
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _apply_volume_controls(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = _frame_count(argc, argv);

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

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        dvz_free(volume_data.raw_data);
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_free(volume_data.raw_data);
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
        dvz_free(volume_data.raw_data);
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
        dvz_free(volume_data.raw_data);
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
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* volume = dvz_volume(scene, 0);
    if (volume == NULL)
    {
        dvz_fprintf(stderr, "dvz_volume() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (!dvz_visual_set_field(volume, "field", field))
    {
        dvz_fprintf(stderr, "dvz_visual_set_field() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) != 0)
    {
        dvz_fprintf(stderr, "dvz_visual_set_alpha_mode() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_panel_add_visual(panel, volume, NULL) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.025f, 0.035f, 0.045f, 1.0f);

    AllenMouseBrainState state = {
        .volume = volume,
        .linear_sampling = true,
        .opacity = DEFAULT_OPACITY,
        .slice_position = DEFAULT_SLICE_POS,
        .axis = DEFAULT_AXIS,
    };
    _apply_volume_controls(&state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_free(volume_data.raw_data);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "allen_mouse_brain_slice_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_free(volume_data.raw_data);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_free(volume_data.raw_data);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _allen_mouse_brain_gui, &state);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    dvz_app_run(app, frame_count);

    dvz_free(volume_data.raw_data);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
