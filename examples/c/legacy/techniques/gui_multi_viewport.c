/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_multi_viewport - two independent docked Datoviz viewports inside Dear ImGui. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



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
    dvz_visual_set_data(source->visual, "diameter", source->sizes, 4);
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
    DvzPanel* panel = dvz_panel_full(figure);
    if (panel == NULL)
        return NULL;
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.07f + shift * 0.05f, 0.08f, 0.11f, 1.0f));

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return NULL;
    state->visual = visual;
    state->point_size = 28.0f;
    state->show_points = true;

    vec3 positions[4] = {
        {-0.55f + shift, -0.45f, 0.0f},
        { 0.45f + shift, -0.35f, 0.0f},
        {-0.30f + shift,  0.45f, 0.0f},
        { 0.55f + shift,  0.35f, 0.0f},
    };
    DvzColor colors[4] = {
        {245,  90,  80, 255},
        { 85, 205, 140, 255},
        { 80, 145, 245, 255},
        {235, 205,  80, 255},
    };
    update_source(state);
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 4},
        {.attr_name = "color", .data = colors, .item_count = 4},
        {.attr_name = "diameter", .data = state->sizes, .item_count = 4},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 3);
    if (rc != 0)
        return NULL;
    rc = dvz_panel_add_visual(panel, visual, NULL);
    if (rc != 0)
        return NULL;
    return panel;
}



/**
 * Build the dockable GUI windows.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void gui_callback(DvzGui* gui, DvzView* win, void* user_data)
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



int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* source_figures[2] = {
        dvz_figure(scene, 480, 360, 0),
        dvz_figure(scene, 480, 360, 0),
    };
    DvzFigure* host_figure = dvz_figure(scene, 1100, 760, 0);
    EXAMPLE_CHECK(
        source_figures[0] != NULL && source_figures[1] != NULL && host_figure != NULL,
        "dvz_figure() failed");

    MultiViewportState state = {0};
    DvzPanel* source_panels[2] = {
        setup_source(scene, source_figures[0], &state.sources[0], -0.10f),
        setup_source(scene, source_figures[1], &state.sources[1], 0.10f),
    };
    DvzPanel* host_panel = dvz_panel_full(host_figure);
    EXAMPLE_CHECK(
        source_panels[0] != NULL && source_panels[1] != NULL && host_panel != NULL,
        "panel setup failed");
    dvz_panel_set_background_color(host_panel, dvz_color_from_unit(0.06f, 0.07f, 0.09f, 1.0f));

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* host_win =
        dvz_view_glfw(app, host_figure, 1100, 760, "gui_multi_viewport");
    EXAMPLE_CHECK(host_win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(host_win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");

    DvzGuiViewportConfig viewport_config = dvz_gui_viewport_config();
    viewport_config.initial_width = 480;
    viewport_config.initial_height = 360;
    viewport_config.resize_step = 16;
    for (uint32_t i = 0; i < 2; i++)
    {
        state.sources[i].gui_viewport = dvz_gui_viewport(gui, source_figures[i], &viewport_config);
        EXAMPLE_CHECK(state.sources[i].gui_viewport != NULL, "dvz_gui_viewport() failed");
        DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
        EXAMPLE_CHECK(panzoom_controller != NULL, "dvz_panzoom() failed");

        int rc = dvz_panel_bind_controller(source_panels[i], panzoom_controller, DVZ_DIM_MASK_XY);
        EXAMPLE_CHECK(rc == 0, "dvz_panel_bind_controller() failed");
        dvz_panel_connect_input(
            source_panels[i], dvz_gui_viewport_input(state.sources[i].gui_viewport));
    }
    dvz_view_set_gui_callback(host_win, gui_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (state.sources[0].gui_viewport != NULL)
        dvz_gui_viewport_destroy(state.sources[0].gui_viewport);
    if (state.sources[1].gui_viewport != NULL)
        dvz_gui_viewport_destroy(state.sources[1].gui_viewport);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
