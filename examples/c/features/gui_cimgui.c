/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_cimgui - This example shows raw cimgui widgets controlling a Datoviz visual.
 *
 * Scenario: features_gui_cimgui
 * Style: features, native GUI/app
 *
 * Build:  just example-c features/gui_cimgui
 * Run:    ./build/examples/c/features/gui_cimgui
 *
 * What to look for: four point positions and colors are uploaded once, while the GUI slider
 * rewrites the diameter_px attribute for all points. The raw cimgui window also displays a small
 * status table and can open the Dear ImGui demo. Move the diameter slider and compare the live
 * marker sizes with the unchanged positions and colors; this demonstrates how advanced users can
 * mix direct imgui calls with retained Datoviz data updates.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 4u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiCimguiState
{
    DvzVisual* point;
    float diameter_px;
    bool show_demo;
} GuiCimguiState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload a uniform point diameter_px.
 *
 * @param state cimgui example state
 */
static void _gui_cimgui_upload(GuiCimguiState* state)
{
    if (state == NULL || state->point == NULL)
        return;

    float diameters[POINT_COUNT] = {
        state->diameter_px,
        state->diameter_px,
        state->diameter_px,
        state->diameter_px,
    };
    if (dvz_visual_set_data(state->point, "diameter_px", diameters, POINT_COUNT) != 0)
        dvz_fprintf(stderr, "gui_cimgui: failed to upload point diameters\n");
}



/**
 * Build the raw cimgui panel.
 *
 * @param gui Datoviz GUI overlay
 * @param view app view
 * @param user_data cimgui example state
 */
static void _gui_cimgui_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)gui;
    (void)view;
    GuiCimguiState* state = (GuiCimguiState*)user_data;
    if (state == NULL)
        return;

    bool open = true;
    if (igBegin("Raw cimgui", &open, 0))
    {
        igText("Dear ImGui %s", igGetVersion());
        if (igBeginTable(
                "status", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInner,
                (ImVec2){0.0f, 0.0f}, 0.0f))
        {
            igTableNextRow(0, 0.0f);
            (void)igTableSetColumnIndex(0);
            igText("binding");
            (void)igTableSetColumnIndex(1);
            igText("datoviz/imgui.h");
            igTableNextRow(0, 0.0f);
            (void)igTableSetColumnIndex(0);
            igText("visual");
            (void)igTableSetColumnIndex(1);
            igText("retained point");
            igEndTable();
        }
        if (igSliderFloat("Diameter", &state->diameter_px, 8.0f, 90.0f, "%.1f", 0))
            _gui_cimgui_upload(state);
        (void)igCheckbox("ImGui demo", &state->show_demo);
    }
    igEnd();

    if (state->show_demo)
        igShowDemoWindow(&state->show_demo);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(
        figure != NULL && panel != NULL && point != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);

    GuiCimguiState state = {
        .point = point,
        .diameter_px = 44.0f,
    };
    vec3 positions[POINT_COUNT] = {
        {-0.60f, -0.30f, 0.0f},
        {-0.20f, +0.30f, 0.0f},
        {+0.20f, -0.30f, 0.0f},
        {+0.60f, +0.30f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
    };
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
    };
    EXAMPLE_CHECK(dvz_visual_set_data_many(point, updates, 2) == 0, "failed to upload point data");
    _gui_cimgui_upload(&state);
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, point, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* view = dvz_view_window(app, figure, WIDTH, HEIGHT, "gui_cimgui");
    EXAMPLE_CHECK(view != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzGui* gui = dvz_view_gui(view, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(view, _gui_cimgui_callback, &state);

    if (example_png_capture_requested(argc, argv))
    {
        DvzAppCaptureConfig capture = {0};
        EXAMPLE_CHECK(
            example_png_capture_config("feature_gui_cimgui", &capture),
            "failed to configure PNG capture");
        EXAMPLE_CHECK(
            example_run_with_capture(
                app, view, example_frame_count_any_or_default(argc, argv, 4), &capture),
            "PNG capture failed");
    }
    else
    {
        dvz_app_run(app, example_frame_count(argc, argv));
    }
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
