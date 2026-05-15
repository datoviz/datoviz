/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_gui_glfw — interactive GLFW scene with a dockable Dear ImGui control panel.
 *
 * Build:  just example-c hello_gui_glfw
 * Run:    ./build/examples/c/hello_gui_glfw
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



typedef struct GuiState
{
    DvzVisual* visual;
    float base_positions[3][3];
    float positions[3][3];
    float sizes[3];
    float point_size;
    float x_offset;
    bool show_points;
    bool show_demo;
} GuiState;



/**
 * Upload the current GUI-controlled point attributes.
 *
 * @param state example state
 */
static void update_visual(GuiState* state)
{
    float size = state->show_points ? state->point_size : 0.0f;
    for (uint32_t i = 0; i < 3; i++)
    {
        state->positions[i][0] = state->base_positions[i][0] + state->x_offset;
        state->positions[i][1] = state->base_positions[i][1];
        state->positions[i][2] = state->base_positions[i][2];
        state->sizes[i] = size;
    }
    dvz_visual_set_data(state->visual, "position", state->positions, 3);
    dvz_visual_set_data(state->visual, "size", state->sizes, 3);
}



/**
 * Build the dockable ImGui controls for the example.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    GuiState* state = (GuiState*)user_data;
    bool changed = false;

    if (dvz_gui_begin(gui, "Datoviz controls", NULL, 0))
    {
        dvz_gui_text(gui, "Dock this window by dragging its title bar.");
        changed |= dvz_gui_slider_float(gui, "Point size", &state->point_size, 2.0f, 80.0f);
        changed |= dvz_gui_slider_float(gui, "X offset", &state->x_offset, -0.5f, 0.5f);
        changed |= dvz_gui_checkbox(gui, "Show points", &state->show_points);
        (void)dvz_gui_checkbox(gui, "Show ImGui demo", &state->show_demo);
        if (dvz_gui_push_mono(gui))
        {
            dvz_gui_text(gui, "mono: Cousine-Regular");
            dvz_gui_pop_font(gui);
        }
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);

    if (changed)
        update_visual(state);
}



/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;
    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, 900, 650, 0);
    if (figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    if (panel == NULL)
    {
        fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
    {
        fprintf(stderr, "dvz_point() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    GuiState gui_state = {
        .visual = visual,
        .base_positions = {
            {-0.5f, -0.5f, 0.0f},
            { 0.5f, -0.5f, 0.0f},
            { 0.0f,  0.5f, 0.0f},
        },
        .point_size = 24.0f,
        .show_points = true,
    };
    update_visual(&gui_state);

    uint8_t colors[3][4] = {
        {255,  64,  64, 255},
        { 64, 220, 130, 255},
        { 80, 150, 255, 255},
    };
    dvz_visual_set_data(visual, "color", colors, 3);
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.10f, 0.12f, 0.16f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, 900, 650, "hello_gui_glfw");
    if (win == NULL)
    {
        fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_panzoom(panel, dvz_app_window_input(win), 0);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, gui_callback, &gui_state);

    dvz_app_run(app, frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
