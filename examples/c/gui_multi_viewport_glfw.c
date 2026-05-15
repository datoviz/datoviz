/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_multi_viewport_glfw - two independent docked Datoviz viewports inside Dear ImGui. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



typedef struct SourceState
{
    DvzGuiViewport* gui_viewport;
    DvzVisual* visual;
    float sizes[4];
    float point_size;
    bool show_points;
} SourceState;



typedef struct MultiViewportState
{
    SourceState sources[2];
    bool show_demo;
} MultiViewportState;



/**
 * Upload point sizes controlled from the GUI.
 *
 * @param source source state
 */
static void update_source(SourceState* source)
{
    float size = source->show_points ? source->point_size : 0.0f;
    for (uint32_t i = 0; i < 4; i++)
        source->sizes[i] = size;
    dvz_visual_set_data(source->visual, "size", source->sizes, 4);
}



/**
 * Initialize one source panel and visual.
 *
 * @param scene owning scene
 * @param figure source figure
 * @param state source state
 * @param shift x shift applied to positions
 * @return created source panel, or NULL on failure
 */
static DvzPanel*
setup_source(DvzScene* scene, DvzFigure* figure, SourceState* state, float shift)
{
    DvzPanelDesc full = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, full);
    if (panel == NULL)
        return NULL;
    dvz_panel_set_background_color(panel, 0.07f + shift * 0.05f, 0.08f, 0.11f, 1.0f);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return NULL;
    state->visual = visual;
    state->point_size = 28.0f;
    state->show_points = true;

    float positions[4][3] = {
        {-0.55f + shift, -0.45f, 0.0f},
        { 0.45f + shift, -0.35f, 0.0f},
        {-0.30f + shift,  0.45f, 0.0f},
        { 0.55f + shift,  0.35f, 0.0f},
    };
    uint8_t colors[4][4] = {
        {245,  90,  80, 255},
        { 85, 205, 140, 255},
        { 80, 145, 245, 255},
        {235, 205,  80, 255},
    };
    update_source(state);
    dvz_visual_set_data(visual, "position", positions, 4);
    dvz_visual_set_data(visual, "color", colors, 4);
    dvz_panel_add_visual(panel, visual, NULL);
    return panel;
}



/**
 * Build the dockable GUI windows.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    MultiViewportState* state = (MultiViewportState*)user_data;

    (void)dvz_gui_viewport_window(state->sources[0].gui_viewport, "Datoviz viewport A", NULL, 0);
    (void)dvz_gui_viewport_window(state->sources[1].gui_viewport, "Datoviz viewport B", NULL, 0);

    if (dvz_gui_begin(gui, "Controls", NULL, 0))
    {
        for (uint32_t i = 0; i < 2; i++)
        {
            char label[64] = {0};
            snprintf(label, sizeof(label), "Viewport %c point size", 'A' + (int)i);
            bool changed = dvz_gui_slider_float(
                gui, label, &state->sources[i].point_size, 2.0f, 80.0f);
            snprintf(label, sizeof(label), "Viewport %c visible", 'A' + (int)i);
            changed |= dvz_gui_checkbox(gui, label, &state->sources[i].show_points);
            if (changed)
                update_source(&state->sources[i]);
        }
        (void)dvz_gui_checkbox(gui, "Show ImGui demo", &state->show_demo);
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);
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

    DvzFigure* source_figures[2] = {
        dvz_figure(scene, 480, 360, 0),
        dvz_figure(scene, 480, 360, 0),
    };
    DvzFigure* host_figure = dvz_figure(scene, 1100, 760, 0);
    if (source_figures[0] == NULL || source_figures[1] == NULL || host_figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    MultiViewportState state = {0};
    DvzPanel* source_panels[2] = {
        setup_source(scene, source_figures[0], &state.sources[0], -0.10f),
        setup_source(scene, source_figures[1], &state.sources[1], 0.10f),
    };
    DvzPanelDesc full = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* host_panel = dvz_panel(host_figure, full);
    if (source_panels[0] == NULL || source_panels[1] == NULL || host_panel == NULL)
    {
        fprintf(stderr, "panel setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(host_panel, 0.06f, 0.07f, 0.09f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* host_win =
        dvz_app_window_glfw(app, host_figure, 1100, 760, "gui_multi_viewport_glfw");
    if (host_win == NULL)
    {
        fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(host_win, &gui_config);
    if (gui == NULL)
    {
        fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzGuiViewportConfig viewport_config = dvz_gui_viewport_config();
    viewport_config.initial_width = 480;
    viewport_config.initial_height = 360;
    viewport_config.resize_step = 16;
    for (uint32_t i = 0; i < 2; i++)
    {
        state.sources[i].gui_viewport = dvz_gui_viewport(gui, source_figures[i], &viewport_config);
        if (state.sources[i].gui_viewport == NULL)
        {
            fprintf(stderr, "dvz_gui_viewport() failed\n");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return 1;
        }
        dvz_panel_set_panzoom(
            source_panels[i], dvz_gui_viewport_input(state.sources[i].gui_viewport), 0);
    }
    dvz_app_window_set_gui_callback(host_win, gui_callback, &state);

    dvz_app_run(app, frame_count(argc, argv));

    dvz_gui_viewport_destroy(state.sources[0].gui_viewport);
    dvz_gui_viewport_destroy(state.sources[1].gui_viewport);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
