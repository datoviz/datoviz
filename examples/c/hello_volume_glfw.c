/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_volume_glfw - synthetic retained volume visual via scene/app/GLFW.
 *
 * Build:  just example-c hello_volume_glfw
 * Run:    ./build/examples/c/hello_volume_glfw
 * Smoke:  ./build/examples/c/hello_volume_glfw 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  900
#define HEIGHT 650
#define VOLUME_SIZE 64

#define ROTATION_SPEED_RAD_PER_SEC 0.45f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VolumeGlfwState VolumeGlfwState;

struct VolumeGlfwState
{
    DvzVisual* volume;
    DvzScale* transfer_scale;
    DvzColormap* transfer_map;
    int render_mode;
    bool transfer;
    bool clipping;
    bool clip_plane;
    bool plane_keep_positive;
    float opacity;
    float tf_threshold;
    float tf_window;
    float tf_ramp;
    float tf_alpha;
    float clip_min[3];
    float clip_max[3];
    float clip_plane_point[3];
    float clip_plane_normal[3];
    float step_count;
};



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
 * Return an unsigned absolute integer distance.
 *
 * @param a first coordinate
 * @param b second coordinate
 * @return absolute distance
 */
static uint32_t _abs_diff_u32(uint32_t a, uint32_t b)
{
    return a > b ? a - b : b - a;
}



/**
 * Return a compact quadratic blob intensity.
 *
 * @param x sample x coordinate
 * @param y sample y coordinate
 * @param z sample z coordinate
 * @param cx blob x center
 * @param cy blob y center
 * @param cz blob z center
 * @param radius blob radius
 * @param peak peak intensity
 * @return 8-bit intensity contribution
 */
static uint8_t _blob_value(
    uint32_t x, uint32_t y, uint32_t z, uint32_t cx, uint32_t cy, uint32_t cz,
    uint32_t radius, uint8_t peak)
{
    uint32_t dx = _abs_diff_u32(x, cx);
    uint32_t dy = _abs_diff_u32(y, cy);
    uint32_t dz = _abs_diff_u32(z, cz);
    uint32_t r2 = radius * radius;
    uint32_t d2 = dx * dx + dy * dy + dz * dz;
    if (d2 >= r2)
        return 0;
    uint32_t value = (uint32_t)peak * (r2 - d2) / r2;
    return value > 255 ? 255 : (uint8_t)value;
}



/**
 * Fill a synthetic microscopy-like scalar volume.
 *
 * @param data output volume storage
 * @param size cubic volume edge length
 */
static void _fill_volume(uint8_t* data, uint32_t size)
{
    ANN(data);
    for (uint32_t z = 0; z < size; z++)
    {
        for (uint32_t y = 0; y < size; y++)
        {
            for (uint32_t x = 0; x < size; x++)
            {
                uint8_t value = 0;
                uint8_t a = _blob_value(
                    x, y, z, size / 3, size / 2, size / 2, size / 4, 230);
                uint8_t b = _blob_value(
                    x, y, z, 2 * size / 3, size / 3, 2 * size / 3, size / 5, 255);
                uint8_t c = _blob_value(
                    x, y, z, size / 2, 2 * size / 3, size / 3, size / 6, 190);
                value = a > value ? a : value;
                value = b > value ? b : value;
                value = c > value ? c : value;
                data[(z * size + y) * size + x] = value;
            }
        }
    }
}



/**
 * Clamp a floating-point value to a closed interval.
 *
 * @param value input value
 * @param min_value interval lower bound
 * @param max_value interval upper bound
 * @return clamped value
 */
static float _clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}



/**
 * Rebuild the editable opacity transfer function.
 *
 * @param state example state
 */
static void _update_transfer_function(VolumeGlfwState* state)
{
    ANN(state);
    if (state->transfer_map == NULL)
        return;

    state->tf_threshold = _clamp_float(state->tf_threshold, 0.0f, 0.98f);
    state->tf_window = _clamp_float(state->tf_window, 0.02f, 1.0f);
    state->tf_ramp = _clamp_float(state->tf_ramp, 0.05f, 1.0f);
    state->tf_alpha = _clamp_float(state->tf_alpha, 0.0f, 1.0f);

    float low = state->tf_threshold;
    float high = _clamp_float(low + state->tf_window, low + 0.01f, 1.0f);
    float mid = low + (high - low) * state->tf_ramp;
    DvzColormapStop transfer_stops[5] = {
        {.position = 0.00, .rgba = {0, 0, 0, 255}},
        {.position = low, .rgba = {0, 0, 0, 255}},
        {.position = mid, .rgba = {30, 120, 220, 255}},
        {.position = high, .rgba = {230, 230, 245, 255}},
        {.position = 1.00, .rgba = {255, 190, 80, 255}},
    };
    dvz_colormap_set_stops(state->transfer_map, transfer_stops, 5);

    if (state->volume != NULL)
    {
        DvzVolumeAlphaStop alpha_stops[5] = {
            {.position = 0.00, .alpha = 0.0f},
            {.position = low, .alpha = 0.0f},
            {.position = mid, .alpha = 0.5f * state->tf_alpha},
            {.position = high, .alpha = state->tf_alpha},
            {.position = 1.00, .alpha = state->tf_alpha},
        };
        (void)dvz_volume_set_alpha_stops(state->volume, alpha_stops, 5);
    }
}



/**
 * Apply retained volume controls after GUI changes.
 *
 * @param state example state
 */
static void _apply_volume_controls(VolumeGlfwState* state)
{
    ANN(state);
    if (state->volume == NULL)
        return;

    DvzVolumeRenderMode mode = DVZ_VOLUME_RENDER_COMPOSITE;
    if (state->render_mode == DVZ_VOLUME_RENDER_SLICE)
        mode = DVZ_VOLUME_RENDER_SLICE;
    else if (state->render_mode == DVZ_VOLUME_RENDER_MIP)
        mode = DVZ_VOLUME_RENDER_MIP;
    (void)dvz_volume_set_render_mode(state->volume, mode);
    (void)dvz_volume_set_opacity(state->volume, state->opacity);
    uint32_t step_count = (uint32_t)(state->step_count + 0.5f);
    if (step_count < 1)
        step_count = 1;
    (void)dvz_volume_set_step_count(state->volume, step_count);
    (void)dvz_visual_set_scale(
        state->volume, "colormap", state->transfer ? state->transfer_scale : NULL);

    if (!state->clipping && !state->clip_plane)
    {
        (void)dvz_volume_clear_clipping(state->volume);
    }
    if (state->clipping)
    {
        double clip_min[3] = {
            state->clip_min[0],
            state->clip_min[1],
            state->clip_min[2],
        };
        double clip_max[3] = {
            state->clip_max[0],
            state->clip_max[1],
            state->clip_max[2],
        };
        (void)dvz_volume_set_clipping_box(state->volume, clip_min, clip_max);
    }
    if (state->clip_plane)
    {
        double point[3] = {
            state->clip_plane_point[0],
            state->clip_plane_point[1],
            state->clip_plane_point[2],
        };
        double normal[3] = {
            state->clip_plane_normal[0],
            state->clip_plane_normal[1],
            state->clip_plane_normal[2],
        };
        (void)dvz_volume_set_clipping_plane(
            state->volume, point, normal, state->plane_keep_positive);
    }
    else
    {
        (void)dvz_volume_clear_clipping_plane(state->volume);
    }
}



/**
 * Build the live volume controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data volume GLFW example state
 */
static void _volume_glfw_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    VolumeGlfwState* state = (VolumeGlfwState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    if (dvz_gui_begin(gui, "Volume", NULL, 0))
    {
        if (dvz_gui_button(gui, "Slice"))
        {
            state->render_mode = DVZ_VOLUME_RENDER_SLICE;
            changed = true;
        }
        if (dvz_gui_button(gui, "MIP"))
        {
            state->render_mode = DVZ_VOLUME_RENDER_MIP;
            changed = true;
        }
        if (dvz_gui_button(gui, "Composite"))
        {
            state->render_mode = DVZ_VOLUME_RENDER_COMPOSITE;
            changed = true;
        }
        changed |= dvz_gui_checkbox(gui, "Transfer", &state->transfer);
        changed |= dvz_gui_slider_float(gui, "Opacity", &state->opacity, 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Steps", &state->step_count, 4.0f, 256.0f);
        changed |= dvz_gui_slider_float(gui, "TF threshold", &state->tf_threshold, 0.0f, 0.98f);
        changed |= dvz_gui_slider_float(gui, "TF window", &state->tf_window, 0.02f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "TF ramp", &state->tf_ramp, 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "TF alpha", &state->tf_alpha, 0.0f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Clip", &state->clipping);
        changed |= dvz_gui_slider_float(gui, "Clip X min", &state->clip_min[0], 0.0f, 0.95f);
        changed |= dvz_gui_slider_float(gui, "Clip X max", &state->clip_max[0], 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Clip Y min", &state->clip_min[1], 0.0f, 0.95f);
        changed |= dvz_gui_slider_float(gui, "Clip Y max", &state->clip_max[1], 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Clip Z min", &state->clip_min[2], 0.0f, 0.95f);
        changed |= dvz_gui_slider_float(gui, "Clip Z max", &state->clip_max[2], 0.05f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Clip plane", &state->clip_plane);
        changed |= dvz_gui_checkbox(gui, "Keep positive side", &state->plane_keep_positive);
        changed |= dvz_gui_slider_float(
            gui, "Plane X", &state->clip_plane_point[0], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(
            gui, "Plane Y", &state->clip_plane_point[1], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(
            gui, "Plane Z", &state->clip_plane_point[2], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(
            gui, "Plane NX", &state->clip_plane_normal[0], -1.0f, 1.0f);
        changed |= dvz_gui_slider_float(
            gui, "Plane NY", &state->clip_plane_normal[1], -1.0f, 1.0f);
        changed |= dvz_gui_slider_float(
            gui, "Plane NZ", &state->clip_plane_normal[2], -1.0f, 1.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            state->render_mode = DVZ_VOLUME_RENDER_COMPOSITE;
            state->transfer = true;
            state->clipping = false;
            state->clip_plane = false;
            state->plane_keep_positive = true;
            state->opacity = 1.0f;
            state->step_count = 128.0f;
            state->tf_threshold = 0.25f;
            state->tf_window = 0.55f;
            state->tf_ramp = 0.70f;
            state->tf_alpha = 1.0f;
            state->clip_min[0] = 0.0f;
            state->clip_min[1] = 0.0f;
            state->clip_min[2] = 0.0f;
            state->clip_max[0] = 1.0f;
            state->clip_max[1] = 1.0f;
            state->clip_max[2] = 1.0f;
            state->clip_plane_point[0] = 0.5f;
            state->clip_plane_point[1] = 0.5f;
            state->clip_plane_point[2] = 0.5f;
            state->clip_plane_normal[0] = 1.0f;
            state->clip_plane_normal[1] = 1.0f;
            state->clip_plane_normal[2] = 0.0f;
            changed = true;
        }
    }
    dvz_gui_end(gui);

    for (uint32_t i = 0; i < 3; i++)
    {
        if (state->clip_min[i] > state->clip_max[i])
        {
            float tmp = state->clip_min[i];
            state->clip_min[i] = state->clip_max[i];
            state->clip_max[i] = tmp;
            changed = true;
        }
    }
    if (changed)
    {
        _update_transfer_function(state);
        _apply_volume_controls(state);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = _frame_count(argc, argv);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    uint64_t bytes = (uint64_t)VOLUME_SIZE * VOLUME_SIZE * VOLUME_SIZE;
    uint8_t* data = (uint8_t*)dvz_malloc(bytes);
    if (data == NULL)
    {
        dvz_fprintf(stderr, "volume allocation failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    _fill_volume(data, VOLUME_SIZE);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = VOLUME_SIZE,
                   .height = VOLUME_SIZE,
                   .depth = VOLUME_SIZE,
               });
    if (field == NULL)
    {
        dvz_fprintf(stderr, "dvz_sampled_field() failed\n");
        dvz_free(data);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = data,
                       .bytes_per_row = VOLUME_SIZE,
                       .rows_per_image = VOLUME_SIZE,
                   }))
    {
        dvz_fprintf(stderr, "dvz_sampled_field_set_data() failed\n");
        dvz_free(data);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_free(data);

    DvzVisual* volume = dvz_volume(scene, 0);
    if (volume == NULL)
    {
        dvz_fprintf(stderr, "dvz_volume() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    if (!dvz_visual_set_field(volume, "field", field))
    {
        dvz_fprintf(stderr, "dvz_visual_set_field() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_visual_set_alpha_mode(volume, DVZ_ALPHA_BLENDED) != 0)
    {
        dvz_fprintf(stderr, "dvz_visual_set_alpha_mode() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzScale* transfer_scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    if (transfer_scale == NULL)
    {
        dvz_fprintf(stderr, "dvz_scale() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_scale_set_domain(transfer_scale, 0.0, 1.0);
    DvzColormap* transfer_map = dvz_colormap(scene, NULL);
    if (transfer_map == NULL)
    {
        dvz_fprintf(stderr, "dvz_colormap() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_scale_set_colormap(transfer_scale, transfer_map);
    (void)dvz_volume_set_value_range(volume, 0.0, 1.0);
    if (dvz_panel_add_visual(panel, volume, NULL) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.025f, 0.035f, 0.045f, 1.0f);

    VolumeGlfwState state = {
        .volume = volume,
        .transfer_scale = transfer_scale,
        .transfer_map = transfer_map,
        .render_mode = DVZ_VOLUME_RENDER_COMPOSITE,
        .transfer = true,
        .plane_keep_positive = true,
        .opacity = 1.0f,
        .step_count = 128.0f,
        .tf_threshold = 0.25f,
        .tf_window = 0.55f,
        .tf_ramp = 0.70f,
        .tf_alpha = 1.0f,
        .clip_min = {0.0f, 0.0f, 0.0f},
        .clip_max = {1.0f, 1.0f, 1.0f},
        .clip_plane_point = {0.5f, 0.5f, 0.5f},
        .clip_plane_normal = {1.0f, 1.0f, 0.0f},
    };
    _update_transfer_function(&state);
    _apply_volume_controls(&state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "hello_volume_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.55f, 0.0f, +0.30f});

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _volume_glfw_gui, &state);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_anim_start(spin, 0.0);

    dvz_app_run(app, frame_count);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
