/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_controls - curated Datoviz GUI controls mutating one retained point visual.
 *
 * Scenario: feature.gui_controls
 * Style: features, native GUI/app
 *
 * Build:  just example-c features/gui_controls
 * Run:    ./build/examples/c/features/gui_controls
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

#define WIDTH       1000u
#define HEIGHT      700u
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiControlsState
{
    DvzVisual* point;
    float diameter;
    float color[4];
    bool visible;
} GuiControlsState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload the point colors and sizes controlled by the GUI.
 *
 * @param state GUI controls example state
 * @return true on success
 */
static bool _gui_controls_upload(GuiControlsState* state)
{
    if (state == NULL || state->point == NULL)
        return false;

    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        colors[i].r = (uint8_t)(255.0f * state->color[0]);
        colors[i].g = (uint8_t)(255.0f * state->color[1]);
        colors[i].b = (uint8_t)(255.0f * state->color[2]);
        colors[i].a = (uint8_t)(255.0f * state->color[3]);
        diameters[i] = state->diameter;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(state->point, updates, 2) == 0;
}



/**
 * Build the Datoviz GUI controls for one retained visual.
 *
 * @param gui GUI overlay
 * @param view app view
 * @param user_data GUI controls example state
 */
static void _gui_controls_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    GuiControlsState* state = (GuiControlsState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool visible_changed = false;
    if (dvz_gui_begin(gui, "Point controls", NULL, 0))
    {
        changed |= dvz_gui_slider_float(gui, "Diameter", &state->diameter, 8.0f, 96.0f);
        changed |= dvz_gui_color_edit4(gui, "Color", state->color, 0);
        visible_changed |= dvz_gui_checkbox(gui, "Visible", &state->visible);
    }
    dvz_gui_end(gui);

    if (changed && !_gui_controls_upload(state))
        dvz_fprintf(stderr, "gui_controls: failed to upload visual data\n");
    if (visible_changed)
        dvz_visual_set_visible(state->point, state->visible);
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

    vec3 positions[POINT_COUNT] = {
        {-0.70f, -0.35f, 0.0f}, {-0.35f, +0.20f, 0.0f}, {+0.00f, -0.10f, 0.0f},
        {+0.35f, +0.35f, 0.0f}, {+0.70f, -0.20f, 0.0f},
    };
    GuiControlsState state = {
        .point = point,
        .diameter = 42.0f,
        .color = {0.28f, 0.78f, 1.00f, 1.00f},
        .visible = true,
    };
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(point, updates, 1);
    EXAMPLE_CHECK(rc == 0 && _gui_controls_upload(&state), "failed to upload point data");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    EXAMPLE_CHECK(dvz_point_set_style(point, &style) == 0, "dvz_point_set_style() failed");
    EXAMPLE_CHECK(
        dvz_visual_set_depth_test(point, false) == 0, "dvz_visual_set_depth_test() failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, point, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* view = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "gui_controls");
    EXAMPLE_CHECK(view != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(view, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(view, _gui_controls_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
