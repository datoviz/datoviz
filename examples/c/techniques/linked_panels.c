/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* linked_panels — live linked-panel panzoom smoke example.
 *
 * Opens a GLFW window with three panels. The two top panels share panzoom state: pan or zoom
 * either one and the other follows on the next frame. The bottom panel stays independent.
 *
 * Build:  just example-c linked_panels
 * Run:    ./build/examples/c/techniques/linked_panels
 * Smoke:  ./build/examples/c/techniques/linked_panels 300
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1000
#define HEIGHT      800
#define PANEL_COUNT 3
#define LINK_COUNT  2
#define POINT_COLS  14
#define POINT_ROWS  9
#define POINT_COUNT (POINT_COLS * POINT_ROWS)
#define LINK_EPSILON 1e-5f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LinkedPanelsState LinkedPanelsState;

struct LinkedPanelsState
{
    DvzPanzoom* panzooms[PANEL_COUNT];
    float last_pan[PANEL_COUNT][2];
    float last_zoom[PANEL_COUNT][2];
    uint32_t frame;
    bool initialized;
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
    if (argc < 2 || argv == NULL || argv[1] == NULL)
        return 0;

    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/**
 * Return whether one panzoom differs from the last frame snapshot.
 *
 * @param state linked-panels example state
 * @param index panzoom index
 * @return whether pan or zoom changed
 */
static bool _panzoom_changed(const LinkedPanelsState* state, uint32_t index)
{
    if (state == NULL || index >= PANEL_COUNT || state->panzooms[index] == NULL)
        return false;

    const DvzPanzoom* pz = state->panzooms[index];
    return fabsf(pz->pan[0] - state->last_pan[index][0]) > LINK_EPSILON ||
           fabsf(pz->pan[1] - state->last_pan[index][1]) > LINK_EPSILON ||
           fabsf(pz->zoom[0] - state->last_zoom[index][0]) > LINK_EPSILON ||
           fabsf(pz->zoom[1] - state->last_zoom[index][1]) > LINK_EPSILON;
}



/**
 * Snapshot all panzoom states for change detection on the next frame.
 *
 * @param state linked-panels example state
 */
static void _snapshot_panzooms(LinkedPanelsState* state)
{
    if (state == NULL)
        return;
    for (uint32_t i = 0; i < PANEL_COUNT; i++)
    {
        DvzPanzoom* pz = state->panzooms[i];
        if (pz == NULL)
            continue;
        state->last_pan[i][0] = pz->pan[0];
        state->last_pan[i][1] = pz->pan[1];
        state->last_zoom[i][0] = pz->zoom[0];
        state->last_zoom[i][1] = pz->zoom[1];
    }
}



/**
 * Copy navigation state from one panzoom to another while preserving the target viewport.
 *
 * @param source source panzoom
 * @param target target panzoom
 */
static void _copy_panzoom_state(const DvzPanzoom* source, DvzPanzoom* target)
{
    if (source == NULL || target == NULL)
        return;

    target->pan[0] = source->pan[0];
    target->pan[1] = source->pan[1];
    target->pan_center[0] = source->pan_center[0];
    target->pan_center[1] = source->pan_center[1];
    target->zoom[0] = source->zoom[0];
    target->zoom[1] = source->zoom[1];
    target->zoom_center[0] = source->zoom_center[0];
    target->zoom_center[1] = source->zoom_center[1];
    target->pan_lock[0] = source->pan_lock[0];
    target->pan_lock[1] = source->pan_lock[1];
    target->zoom_lock[0] = source->zoom_lock[0];
    target->zoom_lock[1] = source->zoom_lock[1];
    target->pan_locked[0] = source->pan_locked[0];
    target->pan_locked[1] = source->pan_locked[1];
    target->zoom_locked[0] = source->zoom_locked[0];
    target->zoom_locked[1] = source->zoom_locked[1];
}



/**
 * Add a point grid visual to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param variant visual color/shape variant
 * @return true on success, false on error
 */
static bool _add_point_grid(DvzScene* scene, DvzPanel* panel, uint32_t variant)
{
    if (scene == NULL || panel == NULL)
        return false;

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;

    float positions[POINT_COUNT][3] = {0};
    uint8_t colors[POINT_COUNT][4] = {0};
    float sizes[POINT_COUNT] = {0};
    for (uint32_t row = 0; row < POINT_ROWS; row++)
    {
        for (uint32_t col = 0; col < POINT_COLS; col++)
        {
            uint32_t index = row * POINT_COLS + col;
            float x = -0.90f + 1.80f * ((float)col / (float)(POINT_COLS - 1));
            float y = -0.80f + 1.60f * ((float)row / (float)(POINT_ROWS - 1));
            float wave = 0.08f * sinf(0.65f * (float)col + 0.9f * (float)variant);
            positions[index][0] = x;
            positions[index][1] = y + wave;
            positions[index][2] = 0.0f;

            colors[index][0] = (uint8_t)(80 + 45 * variant + (85 * col) / (POINT_COLS - 1));
            colors[index][1] = (uint8_t)(80 + (120 * row) / (POINT_ROWS - 1));
            colors[index][2] = (uint8_t)(215 - 45 * variant);
            colors[index][3] = 255;
            sizes[index] = 16.0f + 6.0f * (float)((row + col + variant) % 3);
        }
    }

    if (dvz_visual_set_data(visual, "position", positions, POINT_COUNT) != 0 ||
        dvz_visual_set_data(visual, "color", colors, POINT_COUNT) != 0 ||
        dvz_visual_set_data(visual, "diameter", sizes, POINT_COUNT) != 0)
    {
        return false;
    }
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Propagate panzoom changes across the linked top panels after each frame.
 *
 * @param win app-window whose frame just completed
 * @param user_data linked-panels example state
 */
static void _linked_panels_frame(DvzAppWindow* win, void* user_data)
{
    (void)win;
    LinkedPanelsState* state = (LinkedPanelsState*)user_data;
    if (state == NULL)
        return;

    if (!state->initialized)
    {
        _snapshot_panzooms(state);
        state->initialized = true;
        state->frame++;
        return;
    }

    uint32_t source = UINT32_MAX;
    for (uint32_t i = 0; i < PANEL_COUNT; i++)
    {
        if (_panzoom_changed(state, i))
        {
            source = i;
            break;
        }
    }

    if (source < LINK_COUNT)
    {
        for (uint32_t i = 0; i < LINK_COUNT; i++)
        {
            if (i != source)
                _copy_panzoom_state(state->panzooms[source], state->panzooms[i]);
        }
        DvzPanzoom* pz = state->panzooms[source];
        fprintf(
            stdout,
            "linked panzoom frame=%u source=top-%u pan=(%.3f, %.3f) zoom=(%.3f, %.3f)\n",
            state->frame, source + 1, pz->pan[0], pz->pan[1], pz->zoom[0], pz->zoom[1]);
    }
    else if (source < PANEL_COUNT)
    {
        DvzPanzoom* pz = state->panzooms[source];
        fprintf(
            stdout,
            "independent panzoom frame=%u source=bottom pan=(%.3f, %.3f) zoom=(%.3f, %.3f)\n",
            state->frame, pz->pan[0], pz->pan[1], pz->zoom[0], pz->zoom[1]);
    }

    _snapshot_panzooms(state);
    state->frame++;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panels[PANEL_COUNT] = {
        dvz_panel(figure, (DvzPanelDesc){.x = 0.00f, .y = 0.00f, .width = 0.50f, .height = 0.50f}),
        dvz_panel(figure, (DvzPanelDesc){.x = 0.50f, .y = 0.00f, .width = 0.50f, .height = 0.50f}),
        dvz_panel(figure, (DvzPanelDesc){.x = 0.00f, .y = 0.50f, .width = 1.00f, .height = 0.50f}),
    };
    for (uint32_t i = 0; i < PANEL_COUNT; i++)
    {
        if (panels[i] == NULL)
        {
            fprintf(stderr, "dvz_panel() failed\n");
            dvz_scene_destroy(scene);
            return 1;
        }
    }

    dvz_panel_set_background_color(panels[0], 0.045f, 0.060f, 0.075f, 1.0f);
    dvz_panel_set_background_color(panels[1], 0.070f, 0.055f, 0.050f, 1.0f);
    dvz_panel_set_background_color(panels[2], 0.050f, 0.060f, 0.050f, 1.0f);

    if (!_add_point_grid(scene, panels[0], 0) || !_add_point_grid(scene, panels[1], 1) ||
        !_add_point_grid(scene, panels[2], 2))
    {
        fprintf(stderr, "visual setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "linked_panels");
    if (win == NULL)
    {
        fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzInputRouter* router = dvz_app_window_input(win);
    if (router == NULL)
    {
        fprintf(stderr, "dvz_app_window_input() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    LinkedPanelsState state = {0};
    for (uint32_t i = 0; i < PANEL_COUNT; i++)
    {
        DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
        state.panzooms[i] = dvz_controller_panzoom(panzoom_controller);
        if (state.panzooms[i] == NULL ||
            dvz_panel_bind_controller(panels[i], panzoom_controller, DVZ_DIM_MASK_XY) != 0)
        {
            fprintf(stderr, "failed to create or bind panzoom controller\n");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return 1;
        }
        dvz_panel_connect_input(panels[i], router);
    }

    dvz_app_window_set_frame_callback(win, _linked_panels_frame, &state);
    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
