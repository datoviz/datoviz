/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_viewport - dockable Dear ImGui window containing a live Datoviz render target.
 *
 * Scenario: feature.gui_viewport
 * Style: features, native GUI/app
 *
 * Build:  just example-c features/gui_viewport
 * Run:    ./build/examples/c/features/gui_viewport
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define HOST_WIDTH    EXAMPLE_WINDOW_WIDTH
#define HOST_HEIGHT   EXAMPLE_WINDOW_HEIGHT
#define SOURCE_WIDTH  640u
#define SOURCE_HEIGHT 480u
#define POINT_COUNT   5u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiViewportState
{
    DvzGuiViewport* viewport;
    DvzVisual* point;
    float diameter_px;
    bool show_points;
    bool show_demo;
} GuiViewportState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload the point diameters controlled by the host GUI.
 *
 * @param state GUI viewport state
 * @return true on success
 */
static bool _gui_viewport_upload(GuiViewportState* state)
{
    if (state == NULL || state->point == NULL)
        return false;

    const float diameter_px = state->show_points ? state->diameter_px : 0.0f;
    float diameters[POINT_COUNT] = {diameter_px, diameter_px, diameter_px, diameter_px, diameter_px};
    return dvz_visual_set_data(state->point, "diameter_px", diameters, POINT_COUNT) == 0;
}



/**
 * Build the dockable GUI viewport and its controls.
 *
 * @param gui GUI overlay
 * @param view host GLFW view
 * @param user_data GUI viewport state
 */
static void _gui_viewport_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    GuiViewportState* state = (GuiViewportState*)user_data;
    if (state == NULL || state->viewport == NULL)
        return;

    (void)dvz_gui_viewport_window(state->viewport, "Datoviz viewport", NULL, 0);

    bool changed = false;
    if (dvz_gui_begin(gui, "Viewport controls", NULL, 0))
    {
        changed |= dvz_gui_slider_float(gui, "Diameter", &state->diameter_px, 4.0f, 80.0f);
        changed |= dvz_gui_checkbox(gui, "Show points", &state->show_points);
        (void)dvz_gui_checkbox(gui, "ImGui demo", &state->show_demo);
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);
    if (changed && !_gui_viewport_upload(state))
        dvz_fprintf(stderr, "gui_viewport: failed to upload point sizes\n");
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzGuiViewport* viewport = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* source_figure = dvz_figure(scene, SOURCE_WIDTH, SOURCE_HEIGHT, 0);
    DvzFigure* host_figure = dvz_figure(scene, HOST_WIDTH, HOST_HEIGHT, 0);
    EXAMPLE_CHECK(source_figure != NULL && host_figure != NULL, "dvz_figure() failed");

    DvzPanel* source_panel = dvz_panel_full(source_figure);
    DvzPanel* host_panel = dvz_panel_full(host_figure);
    EXAMPLE_CHECK(source_panel != NULL && host_panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(source_panel);
    dvz_panel_set_background_color(host_panel, dvz_color_from_unit(0.045f, 0.050f, 0.064f, 1.0f));

    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(point != NULL, "dvz_point() failed");
    GuiViewportState state = {
        .point = point,
        .diameter_px = 34.0f,
        .show_points = true,
    };
    vec3 positions[POINT_COUNT] = {
        {-0.68f, -0.45f, 0.0f}, {-0.28f, +0.28f, 0.0f}, {+0.00f, -0.05f, 0.0f},
        {+0.34f, +0.48f, 0.0f}, {+0.72f, -0.25f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
    };
    EXAMPLE_CHECK(_gui_viewport_upload(&state), "failed to upload point sizes");
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
    };
    EXAMPLE_CHECK(dvz_visual_set_data_many(point, updates, 2) == 0, "failed to upload point data");
    EXAMPLE_CHECK(
        dvz_panel_add_visual(source_panel, point, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* host_view = dvz_view_glfw(app, host_figure, HOST_WIDTH, HOST_HEIGHT, "gui_viewport");
    EXAMPLE_CHECK(host_view != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzGui* gui = dvz_view_gui(host_view, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");

    DvzGuiViewportConfig viewport_config = dvz_gui_viewport_config();
    viewport_config.viewport_flags = DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT;
    viewport = dvz_gui_viewport(gui, source_figure, &viewport_config);
    EXAMPLE_CHECK(viewport != NULL, "dvz_gui_viewport() failed");
    state.viewport = viewport;

    DvzController* panzoom = dvz_panzoom(scene, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "dvz_panzoom() failed");
    EXAMPLE_CHECK(
        dvz_panel_bind_controller(source_panel, panzoom, DVZ_DIM_MASK_XY) == 0,
        "dvz_panel_bind_controller() failed");
    EXAMPLE_CHECK(
        dvz_panel_connect_input(source_panel, dvz_gui_viewport_input(viewport)) == 0,
        "dvz_panel_connect_input() failed");

    dvz_view_set_gui_callback(host_view, _gui_viewport_callback, &state);
    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (viewport != NULL)
        dvz_gui_viewport_destroy(viewport);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
