/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pick_marker - retained marker item picking and selection proof.
 *
 * Opens a GLFW window showing mixed marker shapes. Move the cursor over the panel to query the
 * frontmost marker item. The hovered marker grows through a partial retained update. Click a marker
 * to toggle persistent selection; click the background to clear it.
 *
 * Current marker queries prove item/frontmost picking. They use the marker family's query bounds;
 * exact SDF shape-discard picking is a later precision improvement.
 *
 * Build:  just build
 * Run:    ./build/examples/c/features/pick_marker
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1024
#define HEIGHT       900
#define GRID_COLS    9
#define GRID_ROWS    6
#define MARKER_COUNT (GRID_COLS * GRID_ROWS + 5)
#define BASE_SIZE    34.0f
#define HOVER_SIZE   50.0f
#define PI_F         3.14159265358979323846f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MarkerPickState MarkerPickState;

struct MarkerPickState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzVisual* visual;
    DvzSelection* selection;
    DvzColor base_colors[MARKER_COUNT];
    DvzColor colors[MARKER_COUNT];
    float diameters[MARKER_COUNT];
    bool selected[MARKER_COUNT];
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
 * Return a deterministic mixed marker shape.
 *
 * @param index marker index
 * @return marker shape enum value
 */
static uint32_t _marker_shape(uint32_t index)
{
    switch (index % 6u)
    {
    case 1:
        return DVZ_MARKER_SHAPE_SQUARE;
    case 2:
        return DVZ_MARKER_SHAPE_TRIANGLE;
    case 3:
        return DVZ_MARKER_SHAPE_DIAMOND;
    case 4:
        return DVZ_MARKER_SHAPE_CROSS;
    case 5:
        return DVZ_MARKER_SHAPE_RING;
    default:
        return DVZ_MARKER_SHAPE_DISC;
    }
}



/**
 * Update one retained marker diameter through a partial visual upload.
 *
 * @param state marker-pick example state
 * @param index marker index to update
 * @param diameter marker diameter in pixels
 */
static void _set_marker_diameter(MarkerPickState* state, uint32_t index, float diameter)
{
    if (state == NULL || index >= MARKER_COUNT)
        return;
    if (state->diameters[index] == diameter)
        return;

    state->diameters[index] = diameter;
    if (dvz_visual_set_data_range(state->visual, "diameter", &state->diameters[index], index, 1) != 0)
        fprintf(stderr, "dvz_visual_set_data_range(diameter, %u) failed\n", index);
}



/**
 * Apply the visible selected/unselected marker color for one item.
 *
 * @param state marker-pick example state
 * @param index marker index to update
 */
static void _set_marker_selection_color(MarkerPickState* state, uint32_t index)
{
    if (state == NULL || index >= MARKER_COUNT)
        return;

    state->colors[index] = state->selected[index] ? dvz_color_rgb(255, 235, 90)
                                                  : state->base_colors[index];
    if (dvz_visual_set_data_range(state->visual, "color", &state->colors[index], index, 1) != 0)
        fprintf(stderr, "dvz_visual_set_data_range(color, %u) failed\n", index);
}



/**
 * Clear the example-side visible selection colors.
 *
 * @param state marker-pick example state
 */
static void _clear_marker_selection_colors(MarkerPickState* state)
{
    if (state == NULL)
        return;
    for (uint32_t i = 0; i < MARKER_COUNT; i++)
    {
        if (!state->selected[i])
            continue;
        state->selected[i] = false;
        _set_marker_selection_color(state, i);
    }
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest pointer position and click intent in panel coordinates.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data marker-pick example state
 */
static void
_marker_pick_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    MarkerPickState* state = (MarkerPickState*)user_data;
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
 * Consume marker query results, update hover styling, and queue the next query.
 *
 * @param win view whose frame just completed
 * @param user_data marker-pick example state
 */
static void _marker_pick_frame(DvzView* win, void* user_data)
{
    (void)win;
    MarkerPickState* state = (MarkerPickState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    DvzQueryResult latest_query = {0};
    bool saw_query = false;
    uint32_t next_hovered = UINT32_MAX;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        saw_query = true;
        latest_query = query;
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MARKER &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < MARKER_COUNT)
        {
            next_hovered = (uint32_t)query.resolved_id;
        }
    }

    if (saw_query && next_hovered != state->hovered_index)
    {
        if (state->hovered_index < MARKER_COUNT)
            _set_marker_diameter(state, state->hovered_index, BASE_SIZE);
        if (next_hovered < MARKER_COUNT)
        {
            _set_marker_diameter(state, next_hovered, HOVER_SIZE);
            fprintf(stdout, "hover marker id=%u\n", next_hovered);
        }
        state->hovered_index = next_hovered;
    }

    if (state->click_pending && saw_query)
    {
        if (
            latest_query.status == DVZ_QUERY_STATUS_HIT && latest_query.hit &&
            latest_query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MARKER &&
            latest_query.resolved_target == DVZ_SCENE_TARGET_ITEM)
        {
            uint32_t index = (uint32_t)latest_query.resolved_id;
            if (dvz_selection_apply_query(state->selection, &latest_query) != 0)
                fprintf(stderr, "dvz_selection_apply_query() failed\n");
            if (index < MARKER_COUNT)
            {
                state->selected[index] = !state->selected[index];
                _set_marker_selection_color(state, index);
                fprintf(
                    stdout, "%s marker id=%u\n",
                    state->selected[index] ? "select" : "deselect", index);
            }
        }
        else
        {
            dvz_selection_clear(state->selection);
            _clear_marker_selection_colors(state);
        }
        state->click_pending = false;
    }

    if (state->cursor_valid)
    {
        if (dvz_panel_query(
                state->panel, state->cursor_x, state->cursor_y,
                &(DvzQueryRequest){
                    .request_id = 1,
                    .target = DVZ_SCENE_TARGET_ITEM,
                    .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
                }) != 0)
        {
            fprintf(stderr, "dvz_panel_query() failed\n");
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    dvz_panel_set_background_color(panel, 0.045f, 0.055f, 0.075f, 1.0f);

    DvzVisual* visual = dvz_marker(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_marker() failed");
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    style.edge_color = dvz_color_rgb(5, 8, 13);
    style.stroke_width = 2.0f;
    EXAMPLE_CHECK(dvz_marker_set_style(visual, &style) == 0, "dvz_marker_set_style() failed");

    DvzSelection* selection = dvz_selection(
        scene, &(DvzSelectionDesc){.mode = DVZ_SELECT_TOGGLE, .target = DVZ_SCENE_TARGET_ITEM});
    EXAMPLE_CHECK(selection != NULL, "dvz_selection() failed");

    MarkerPickState state = {
        .scene = scene,
        .panel = panel,
        .visual = visual,
        .selection = selection,
        .hovered_index = UINT32_MAX,
    };

    vec3 positions[MARKER_COUNT] = {0};
    float angles[MARKER_COUNT] = {0};
    uint32_t shapes[MARKER_COUNT] = {0};

    for (uint32_t row = 0; row < GRID_ROWS; row++)
    {
        for (uint32_t col = 0; col < GRID_COLS; col++)
        {
            uint32_t index = row * GRID_COLS + col;
            positions[index][0] = -0.88f + 1.76f * ((float)col / (float)(GRID_COLS - 1));
            positions[index][1] = -0.72f + 1.44f * ((float)row / (float)(GRID_ROWS - 1));
            positions[index][2] = 0.0f;
            state.base_colors[index] = dvz_color_rgb(
                (uint8_t)(60 + (170 * col) / (GRID_COLS - 1)),
                (uint8_t)(110 + (100 * row) / (GRID_ROWS - 1)),
                (uint8_t)(230 - (100 * col) / (GRID_COLS - 1)));
            state.diameters[index] = BASE_SIZE;
            angles[index] = ((float)(index % 12u) / 12.0f) * 2.0f * PI_F;
            shapes[index] = _marker_shape(index);
        }
    }

    for (uint32_t i = 0; i < 5; i++)
    {
        uint32_t index = GRID_COLS * GRID_ROWS + i;
        positions[index][0] = -0.05f + 0.025f * (float)i;
        positions[index][1] = -0.02f + 0.020f * (float)(i % 3u);
        positions[index][2] = 0.0f;
        state.base_colors[index] =
            dvz_color_rgb(255, (uint8_t)(205 - 22u * i), (uint8_t)(90 + 24u * i));
        state.diameters[index] = BASE_SIZE;
        angles[index] = 0.25f * PI_F * (float)i;
        shapes[index] = _marker_shape(index + 2u);
    }

    for (uint32_t i = 0; i < MARKER_COUNT; i++)
        state.colors[i] = state.base_colors[i];

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = MARKER_COUNT},
        {.attr_name = "color", .data = state.colors, .item_count = MARKER_COUNT},
        {.attr_name = "diameter", .data = state.diameters, .item_count = MARKER_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = MARKER_COUNT},
        {.attr_name = "shape", .data = shapes, .item_count = MARKER_COUNT},
    };
    EXAMPLE_CHECK(dvz_visual_set_data_many(visual, updates, 5) == 0, "marker data upload failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, visual, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "pick_marker");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    dvz_input_subscribe_pointer(router, _marker_pick_pointer, &state);
    dvz_view_set_frame_callback(win, _marker_pick_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
