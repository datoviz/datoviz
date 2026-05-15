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
    DvzArcball* arcball;
    DvzVisual* volume;
    bool mip;
    bool clipping;
    float opacity;
    float clip_min[3];
    float clip_max[3];
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
 * Apply retained volume controls after GUI changes.
 *
 * @param state example state
 */
static void _apply_volume_controls(VolumeGlfwState* state)
{
    ANN(state);
    if (state->volume == NULL)
        return;

    (void)dvz_volume_set_render_mode(
        state->volume, state->mip ? DVZ_VOLUME_RENDER_MIP : DVZ_VOLUME_RENDER_SLICE);
    (void)dvz_volume_set_opacity(state->volume, state->opacity);
    uint32_t step_count = (uint32_t)(state->step_count + 0.5f);
    if (step_count < 1)
        step_count = 1;
    (void)dvz_volume_set_step_count(state->volume, step_count);

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
    else
    {
        (void)dvz_volume_clear_clipping(state->volume);
    }
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Advance the volume orientation from the scene clock.
 *
 * @param animation timer animation
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time since the previous step
 * @param user_data volume GLFW example state
 */
static void _volume_glfw_timer(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)t;
    VolumeGlfwState* state = (VolumeGlfwState*)user_data;
    if (state == NULL || state->arcball == NULL)
        return;

    if (!dvz_arcball_is_interacting(state->arcball))
        dvz_arcball_rotate_axis(
            state->arcball, ROTATION_SPEED_RAD_PER_SEC * (float)dt,
            (vec3){0.0f, 1.0f, 0.0f});
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
        changed |= dvz_gui_checkbox(gui, "MIP", &state->mip);
        changed |= dvz_gui_slider_float(gui, "Opacity", &state->opacity, 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Steps", &state->step_count, 4.0f, 192.0f);
        changed |= dvz_gui_checkbox(gui, "Clip", &state->clipping);
        changed |= dvz_gui_slider_float(gui, "Clip X min", &state->clip_min[0], 0.0f, 0.95f);
        changed |= dvz_gui_slider_float(gui, "Clip X max", &state->clip_max[0], 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Clip Y min", &state->clip_min[1], 0.0f, 0.95f);
        changed |= dvz_gui_slider_float(gui, "Clip Y max", &state->clip_max[1], 0.05f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Clip Z min", &state->clip_min[2], 0.0f, 0.95f);
        changed |= dvz_gui_slider_float(gui, "Clip Z max", &state->clip_max[2], 0.05f, 1.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            state->mip = true;
            state->clipping = false;
            state->opacity = 1.0f;
            state->step_count = 64.0f;
            state->clip_min[0] = 0.0f;
            state->clip_min[1] = 0.0f;
            state->clip_min[2] = 0.0f;
            state->clip_max[0] = 1.0f;
            state->clip_max[1] = 1.0f;
            state->clip_max[2] = 1.0f;
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
        _apply_volume_controls(state);
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
    if (dvz_panel_add_visual(panel, volume, NULL) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.025f, 0.035f, 0.045f, 1.0f);

    VolumeGlfwState state = {
        .volume = volume,
        .mip = true,
        .opacity = 1.0f,
        .step_count = 128.0f,
        .clip_min = {0.0f, 0.0f, 0.0f},
        .clip_max = {1.0f, 1.0f, 1.0f},
    };
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
    state.arcball = arcball;

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

    DvzAnimation* spin = dvz_anim_timer(scene, 0.0, _volume_glfw_timer, &state);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_timer() failed\n");
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
