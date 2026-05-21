/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_viewport - dockable Dear ImGui window containing a live Datoviz render target.
 *
 * Build:  just example-c gui_viewport
 * Run:    ./build/examples/c/techniques/gui_viewport
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



typedef struct GuiViewportState
{
    DvzGuiViewport* gui_viewport;
    DvzVisual* visual;
    float sizes[5];
    float point_size;
    bool show_points;
    bool show_demo;
} GuiViewportState;



/**
 * Upload point sizes controlled from the GUI.
 *
 * @param state example state
 */
static void update_visual(GuiViewportState* state)
{
    float size = state->show_points ? state->point_size : 0.0f;
    for (uint32_t i = 0; i < 5; i++)
        state->sizes[i] = size;
    dvz_visual_set_data(state->visual, "diameter", state->sizes, 5);
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
    GuiViewportState* state = (GuiViewportState*)user_data;

    (void)dvz_gui_viewport_window(state->gui_viewport, "Datoviz viewport", NULL, 0);

    bool changed = false;
    if (dvz_gui_begin(gui, "Controls", NULL, 0))
    {
        changed |= dvz_gui_slider_float(gui, "Point size", &state->point_size, 2.0f, 80.0f);
        changed |= dvz_gui_checkbox(gui, "Show points", &state->show_points);
        (void)dvz_gui_checkbox(gui, "Show ImGui demo", &state->show_demo);
        if (dvz_gui_push_mono(gui))
        {
            dvz_gui_text(gui, "offscreen source -> ImGui::Image");
            dvz_gui_pop_font(gui);
        }
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);

    if (changed)
        update_visual(state);
}



int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* source_figure = dvz_figure(scene, 640, 480, 0);
    DvzFigure* host_figure = dvz_figure(scene, 1000, 700, 0);
    if (source_figure == NULL || host_figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* source_panel = dvz_panel_full(source_figure);
    DvzPanel* host_panel = dvz_panel_full(host_figure);
    if (source_panel == NULL || host_panel == NULL)
    {
        fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(source_panel, 0.08f, 0.09f, 0.12f, 1.0f);
    dvz_panel_set_background_color(host_panel, 0.06f, 0.07f, 0.09f, 1.0f);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
    {
        fprintf(stderr, "dvz_point() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[5][3] = {
        {-0.70f, -0.50f, 0.0f},
        { 0.70f, -0.45f, 0.0f},
        { 0.00f,  0.60f, 0.0f},
        {-0.35f,  0.05f, 0.0f},
        { 0.35f,  0.08f, 0.0f},
    };
    uint8_t colors[5][4] = {
        {245,  90,  80, 255},
        { 95, 210, 130, 255},
        { 80, 145, 245, 255},
        {245, 200,  80, 255},
        {210, 110, 240, 255},
    };
    GuiViewportState state = {
        .visual = visual,
        .point_size = 30.0f,
        .show_points = true,
    };
    update_visual(&state);
    dvz_point_data(visual, positions, colors, state.sizes, 5);
    dvz_panel_add_visual(source_panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* host_win =
        dvz_app_window_glfw(app, host_figure, 1000, 700, "gui_viewport");
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

    state.gui_viewport = dvz_gui_viewport(gui, source_figure, NULL);
    if (state.gui_viewport == NULL)
    {
        fprintf(stderr, "dvz_gui_viewport() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
    if (panzoom_controller == NULL ||
        dvz_panel_bind_controller(source_panel, panzoom_controller, DVZ_DIM_MASK_XY) != 0)
    {
        fprintf(stderr, "failed to create or bind panzoom controller\n");
        dvz_gui_viewport_destroy(state.gui_viewport);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_connect_input(source_panel, dvz_gui_viewport_input(state.gui_viewport));
    dvz_app_window_set_gui_callback(host_win, gui_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));

    dvz_gui_viewport_destroy(state.gui_viewport);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
