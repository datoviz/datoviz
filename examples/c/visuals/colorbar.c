/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* colorbar - static scalar image with a scene-generated colorbar.
 *
 * Build:  just example-c visuals/colorbar
 * Run:    ./build/examples/c/visuals/colorbar
 * Smoke:  ./build/examples/c/visuals/colorbar 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      800
#define HEIGHT     600
#define FIELD_SIZE 96
#define FIELD_MIN  10.0f
#define FIELD_MAX  20.0f
#define COLORMAP_COUNT 6u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ColorbarState
{
    DvzScale* scale;
    DvzColormap* colormaps[COLORMAP_COUNT];
    DvzAppWindow* win;
    int colormap_index;
    float range_min;
    float range_max;
} ColorbarState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a scalar image with a smooth bounded synthetic field.
 *
 * @param values output scalar field values
 */
static void _fill_field(float values[FIELD_SIZE * FIELD_SIZE])
{
    for (uint32_t y = 0; y < FIELD_SIZE; y++)
    {
        for (uint32_t x = 0; x < FIELD_SIZE; x++)
        {
            float fx = (float)x / (float)(FIELD_SIZE - 1);
            float fy = (float)y / (float)(FIELD_SIZE - 1);
            float ridge = 1.0f - 4.0f * (fx - 0.60f) * (fx - 0.60f);
            float basin = 1.0f - 5.0f * (fy - 0.35f) * (fy - 0.35f);
            float diagonal = 0.45f * fx + 0.25f * fy;
            float value = 0.20f + 0.30f * ridge + 0.28f * basin + diagonal;
            if (value < 0.0f)
                value = 0.0f;
            if (value > 1.0f)
                value = 1.0f;
            values[y * FIELD_SIZE + x] = FIELD_MIN + (FIELD_MAX - FIELD_MIN) * value;
        }
    }
}



/**
 * Apply the current GUI controls to the scene scale.
 *
 * @param state colorbar example state
 * @param update_colormap whether the selected colormap changed
 * @param update_range whether the visible scale range changed
 */
static void _apply_colorbar_controls(
    ColorbarState* state, bool update_colormap, bool update_range)
{
    if (state == NULL || state->scale == NULL)
        return;

    if (state->range_min > state->range_max - 0.01f)
        state->range_min = state->range_max - 0.01f;
    if (state->range_max < state->range_min + 0.01f)
        state->range_max = state->range_min + 0.01f;
    if (state->range_min < FIELD_MIN)
        state->range_min = FIELD_MIN;
    if (state->range_max > FIELD_MAX)
        state->range_max = FIELD_MAX;

    if (update_colormap)
    {
        uint32_t index = state->colormap_index >= 0 ? (uint32_t)state->colormap_index : 0;
        if (index >= COLORMAP_COUNT)
            index = 0;
        dvz_scale_set_colormap(state->scale, state->colormaps[index]);
    }
    if (update_range)
    {
        dvz_scale_set_view_range(state->scale, state->range_min, state->range_max);
    }
    if (state->win != NULL)
        dvz_app_window_request_frame(state->win);
}



/**
 * Render the colorbar example controls.
 *
 * @param gui GUI context
 * @param win app window
 * @param user_data colorbar example state
 */
static void _colorbar_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    ColorbarState* state = (ColorbarState*)user_data;
    if (state == NULL)
        return;

    bool colormap_changed = false;
    bool range_changed = false;
    if (dvz_gui_begin(gui, "Colorbar", NULL, 0))
    {
        const char* const colormap_names[COLORMAP_COUNT] = {
            "Viridis", "Magma", "Plasma", "Inferno", "Cividis", "Turbo",
        };
        colormap_changed = dvz_gui_combo(
            gui, "Colormap", &state->colormap_index, colormap_names, (int)COLORMAP_COUNT);
        range_changed = dvz_gui_range_float(
            gui, "Range", &state->range_min, &state->range_max, 0.05f, FIELD_MIN, FIELD_MAX,
            "%.2f");
    }
    dvz_gui_end(gui);

    if (colormap_changed || range_changed)
        _apply_colorbar_controls(state, colormap_changed, range_changed);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
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

    DvzPanel* panel = dvz_panel_full(figure);
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_full() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Intensity",
                   .format = {.precision = 2},
               });
    if (scale == NULL)
    {
        dvz_fprintf(stderr, "dvz_scale() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_scale_set_domain(scale, FIELD_MIN, FIELD_MAX);
    dvz_scale_set_view_range(scale, FIELD_MIN, FIELD_MAX);

    ColorbarState state = {
        .scale = scale,
        .colormap_index = 0,
        .range_min = FIELD_MIN,
        .range_max = FIELD_MAX,
    };
    const DvzBuiltinColormap builtins[COLORMAP_COUNT] = {
        DVZ_BUILTIN_COLORMAP_VIRIDIS, //
        DVZ_BUILTIN_COLORMAP_MAGMA,   //
        DVZ_BUILTIN_COLORMAP_PLASMA,  //
        DVZ_BUILTIN_COLORMAP_INFERNO, //
        DVZ_BUILTIN_COLORMAP_CIVIDIS, //
        DVZ_BUILTIN_COLORMAP_TURBO,   //
    };
    for (uint32_t i = 0; i < COLORMAP_COUNT; i++)
    {
        state.colormaps[i] = dvz_colormap_builtin(scene, builtins[i]);
        if (state.colormaps[i] == NULL)
        {
            dvz_fprintf(stderr, "dvz_colormap_builtin() failed\n");
            dvz_scene_destroy(scene);
            return 1;
        }
    }
    if (state.colormaps[state.colormap_index] == NULL)
    {
        dvz_fprintf(stderr, "dvz_colormap_builtin() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_scale_set_colormap(scale, state.colormaps[state.colormap_index]);

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
    {
        dvz_fprintf(stderr, "dvz_image() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[4][3] = {
        {-0.90f, -0.90f, 0.0f},
        {-0.90f, 0.90f, 0.0f},
        {0.90f, -0.90f, 0.0f},
        {0.90f, 0.90f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    if (dvz_visual_set_data(image, "position", positions, 4) != 0 ||
        dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0 ||
        dvz_visual_set_scale(image, "colormap", scale) != 0)
    {
        dvz_fprintf(stderr, "image visual setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_SIZE,
                   .height = FIELD_SIZE,
                   .depth = 1,
               });
    if (field == NULL)
    {
        dvz_fprintf(stderr, "dvz_sampled_field() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float values[FIELD_SIZE * FIELD_SIZE] = {0};
    _fill_field(values);
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       .data = values,
                       .bytes_per_row = FIELD_SIZE * sizeof(float),
                       .rows_per_image = FIELD_SIZE,
                   }))
    {
        dvz_fprintf(stderr, "dvz_sampled_field_set_data() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    if (!dvz_visual_set_field(image, "field", field) ||
        dvz_panel_add_visual(panel, image, NULL) != 0)
    {
        dvz_fprintf(stderr, "field binding failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Intensity",
        });
    if (colorbar == NULL)
    {
        dvz_fprintf(stderr, "dvz_colorbar() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_colorbar_set_format(colorbar, &(DvzFormatDesc){.precision = 2});

    dvz_panel_set_background_color(panel, 0.04f, 0.05f, 0.06f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "colorbar");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    state.win = win;

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _colorbar_gui, &state);

    dvz_app_run(app, example_frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
