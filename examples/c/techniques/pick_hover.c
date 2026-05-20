/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pick_hover — interactive point-picking stress test.
 *
 * Opens a GLFW window showing a point grid.
 * Move the cursor over the panel to pick one point per frame.
 * The hovered point grows via dvz_visual_set_data_range(), exercising partial retained updates.
 * Click a point to toggle retained selection; selected points stay bright while others dim.
 * Left-drag to pan, right-drag or scroll to zoom, double-click to reset.
 *
 * Build:  just build
 * Run:    ./build/examples/c/techniques/pick_hover
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1024
#define HEIGHT      1024
#define GRID_COLS   20
#define GRID_ROWS   15
#define POINT_COUNT (GRID_COLS * GRID_ROWS)
#define BASE_SIZE   40.0f
#define HOVER_SIZE  60.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct HoverPickState HoverPickState;

struct HoverPickState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzVisual* visual;
    DvzSelection* selection;
    float sizes[POINT_COUNT];
    uint32_t hovered_index;
    bool cursor_valid;
    bool click_pending;
    double cursor_x;
    double cursor_y;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Update one retained point size through a partial visual upload.
 *
 * @param state hover-pick example state
 * @param index point index to update
 * @param size point size value to write
 */
static void _set_point_size(HoverPickState* state, uint32_t index, float size)
{
    if (state == NULL || index >= POINT_COUNT)
        return;
    if (state->sizes[index] == size)
        return;

    state->sizes[index] = size;
    if (dvz_visual_set_data_range(state->visual, "diameter", &state->sizes[index], index, 1) != 0)
        fprintf(stderr, "dvz_visual_set_data_range(diameter, %u) failed\n", index);
}


/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest cursor position and click intent in panel coordinates.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data hover-pick example state
 */
static void
_hover_pick_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    HoverPickState* state = (HoverPickState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_CLICK)
        return;

    state->cursor_valid = true;
    state->cursor_x = event->pos[0];
    state->cursor_y = event->pos[1];
    if (event->type == DVZ_POINTER_EVENT_CLICK && event->button == DVZ_POINTER_BUTTON_LEFT)
        state->click_pending = true;
}


/**
 * Consume pick results, update hover styling, and queue the next frame-local pick request.
 *
 * @param win app-window whose frame just completed
 * @param user_data hover-pick example state
 */
static void _hover_pick_frame(DvzAppWindow* win, void* user_data)
{
    (void)win;
    HoverPickState* state = (HoverPickState*)user_data;
    if (state == NULL)
        return;

    DvzPickResult pick = {0};
    bool saw_pick = false;
    DvzPickResult latest_pick = {0};
    uint32_t next_hovered = UINT32_MAX;
    while (dvz_scene_poll_pick(state->scene, &pick))
    {
        saw_pick = true;
        latest_pick = pick;
        if (pick.hit && pick.resolved_target == DVZ_SCENE_TARGET_ITEM &&
            pick.resolved_id < POINT_COUNT)
        {
            next_hovered = (uint32_t)pick.resolved_id;
        }
    }

    if (saw_pick && next_hovered != state->hovered_index)
    {
        if (state->hovered_index < POINT_COUNT)
            _set_point_size(state, state->hovered_index, BASE_SIZE);
        if (next_hovered < POINT_COUNT)
            _set_point_size(state, next_hovered, HOVER_SIZE);
        state->hovered_index = next_hovered;
    }

    if (state->click_pending && saw_pick)
    {
        if (latest_pick.hit && latest_pick.resolved_target == DVZ_SCENE_TARGET_ITEM)
        {
            if (dvz_selection_apply_pick(state->selection, &latest_pick) != 0)
                fprintf(stderr, "dvz_selection_apply_pick() failed\n");
        }
        else
        {
            dvz_selection_clear(state->selection);
        }
        state->click_pending = false;
    }

    if (state->cursor_valid)
    {
        if (dvz_panel_pick(
                state->panel, state->cursor_x, state->cursor_y,
                &(DvzPickRequest){
                    .request_id = 1,
                    .target = DVZ_SCENE_TARGET_ITEM,
                    .hit_policy = DVZ_PICK_HIT_FRONTMOST,
                }) != 0)
        {
            fprintf(stderr, "dvz_panel_pick() failed\n");
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(void)
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

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
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

    DvzSelection* selection = dvz_selection(
        scene, &(DvzSelectionDesc){.mode = DVZ_SELECT_TOGGLE, .target = DVZ_SCENE_TARGET_ITEM});
    if (selection == NULL)
    {
        fprintf(stderr, "dvz_selection() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    HoverPickState state = {
        .scene = scene,
        .panel = panel,
        .visual = visual,
        .selection = selection,
        .hovered_index = UINT32_MAX,
    };

    float positions[POINT_COUNT][3] = {0};
    uint8_t colors[POINT_COUNT][4] = {0};
    for (uint32_t row = 0; row < GRID_ROWS; row++)
    {
        for (uint32_t col = 0; col < GRID_COLS; col++)
        {
            uint32_t index = row * GRID_COLS + col;
            positions[index][0] = -0.95f + 1.90f * ((float)col / (float)(GRID_COLS - 1));
            positions[index][1] = -0.90f + 1.80f * ((float)row / (float)(GRID_ROWS - 1));
            positions[index][2] = 0.0f;

            colors[index][0] = (uint8_t)(40 + (215 * col) / (GRID_COLS - 1));
            colors[index][1] = (uint8_t)(70 + (160 * row) / (GRID_ROWS - 1));
            colors[index][2] = (uint8_t)(220 - (120 * row) / (GRID_ROWS - 1));
            colors[index][3] = 255;
            state.sizes[index] = BASE_SIZE;
        }
    }

    if (dvz_visual_set_data(visual, "position", positions, POINT_COUNT) != 0 ||
        dvz_visual_set_data(visual, "color", colors, POINT_COUNT) != 0 ||
        dvz_visual_set_data(visual, "diameter", state.sizes, POINT_COUNT) != 0)
    {
        fprintf(stderr, "dvz_visual_set_data() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_visual_set_pick_capabilities(visual, DVZ_PICK_CAPABILITY_ITEM);

    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_background_color(panel, 0.05f, 0.07f, 0.10f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "pick_hover");
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

    dvz_panel_set_panzoom(panel, router, 0);
    dvz_input_subscribe_pointer(router, _hover_pick_pointer, &state);
    dvz_app_window_set_frame_callback(win, _hover_pick_frame, &state);

    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
