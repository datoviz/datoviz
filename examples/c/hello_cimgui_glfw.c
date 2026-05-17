/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_cimgui_glfw - interactive GLFW scene controlled with raw cimgui calls.
 *
 * Build:  just example-c hello_cimgui_glfw
 * Run:    ./build/examples/c/hello_cimgui_glfw
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"



typedef struct CimguiState
{
    DvzVisual* visual;
    float base_positions[4][3];
    float positions[4][3];
    float sizes[4];
    float point_size;
    float x_offset;
    bool show_points;
    bool show_demo;
    int clicks;
} CimguiState;



/**
 * Upload the current raw cimgui-controlled point attributes.
 *
 * @param state example state
 */
static void update_visual(CimguiState* state)
{
    float size = state->show_points ? state->point_size : 0.0f;
    for (uint32_t i = 0; i < 4; i++)
    {
        state->positions[i][0] = state->base_positions[i][0] + state->x_offset;
        state->positions[i][1] = state->base_positions[i][1];
        state->positions[i][2] = state->base_positions[i][2];
        state->sizes[i] = size;
    }
    dvz_visual_set_data(state->visual, "position", state->positions, 4);
    dvz_visual_set_data(state->visual, "diameter", state->sizes, 4);
}



/**
 * Build the dockable ImGui controls with the raw cimgui API.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)gui;
    (void)win;
    CimguiState* state = (CimguiState*)user_data;
    bool changed = false;

    if (igBegin("Raw cimgui controls", NULL, 0))
    {
        igTextUnformatted("This dockable window is built with raw ig* calls.", NULL);
        igSeparator();
        changed |= igSliderFloat("Point size", &state->point_size, 2.0f, 80.0f, "%.1f", 0);
        changed |= igSliderFloat("X offset", &state->x_offset, -0.5f, 0.5f, "%.2f", 0);
        changed |= igCheckbox("Show points", &state->show_points);
        (void)igCheckbox("Show ImGui demo", &state->show_demo);

        if (igButton("Count click", (ImVec2){0.0f, 0.0f}))
            state->clicks++;

        igSameLine(0.0f, 8.0f);
        char text[64] = {0};
        snprintf(text, sizeof(text), "clicks: %d", state->clicks);
        igTextUnformatted(text, NULL);
    }
    igEnd();

    if (state->show_demo)
        igShowDemoWindow(&state->show_demo);

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

    CimguiState state = {
        .visual = visual,
        .base_positions = {
            {-0.55f, -0.45f, 0.0f},
            { 0.45f, -0.35f, 0.0f},
            {-0.30f,  0.45f, 0.0f},
            { 0.45f,  0.35f, 0.0f},
        },
        .point_size = 26.0f,
        .show_points = true,
    };
    update_visual(&state);

    uint8_t colors[4][4] = {
        {255,  74,  74, 255},
        { 64, 210, 125, 255},
        { 74, 145, 255, 255},
        {250, 206,  80, 255},
    };
    dvz_visual_set_data(visual, "color", colors, 4);
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.10f, 0.12f, 0.16f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, 900, 650, "hello_cimgui_glfw");
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
    dvz_app_window_set_gui_callback(win, gui_callback, &state);

    dvz_app_run(app, frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
