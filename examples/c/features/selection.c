/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* selection - retained point selection proof.
 *
 * Scenario: feature.selection
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Opens a GLFW window showing a deterministic point set. Left-click a point to toggle retained
 * item selection; click the background to clear it. Selected points use a retained tint style.
 *
 * Build:  just example-c features/selection
 * Run:    ./build/examples/c/features/selection
 * Smoke:  ./build/examples/c/features/selection 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 9u
#define POINT_SIZE  42.0f
#define QUERY_ID    1u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SelectionState SelectionState;

struct SelectionState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzQueryResult latest_query;
    bool has_query;
    bool cursor_valid;
    bool click_pending;
    double cursor_x;
    double cursor_y;
};



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record pointer position and click intent.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data selection example state
 */
static void _selection_pointer(
    DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    SelectionState* state = (SelectionState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_PRESS)
        return;

    state->cursor_valid = true;
    state->cursor_x = event->pos[0];
    state->cursor_y = event->pos[1];
    if (event->type == DVZ_POINTER_EVENT_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
        state->click_pending = true;
}



/**
 * Consume point query results and apply pending click selection.
 *
 * @param win view whose frame just completed
 * @param user_data selection example state
 */
static void _selection_frame(DvzView* win, void* user_data)
{
    (void)win;
    SelectionState* state = (SelectionState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id != QUERY_ID)
            continue;

        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < POINT_COUNT)
        {
            state->latest_query = query;
            state->has_query = true;
        }
        else
        {
            state->has_query = false;
        }
    }

    if (state->click_pending)
    {
        if (state->has_query)
        {
            if (dvz_selection_apply_query(state->selection, &state->latest_query) != 0)
                fprintf(stderr, "dvz_selection_apply_query() failed\n");
            fprintf(stdout, "toggle point id=%" PRIu64 "\n", state->latest_query.resolved_id);
        }
        else
        {
            dvz_selection_clear(state->selection);
            fprintf(stdout, "clear selection\n");
        }
        state->click_pending = false;
    }

    if (state->cursor_valid)
    {
        DvzQueryRequest request = dvz_query_request();
        request.request_id = QUERY_ID;
        request.target = DVZ_SCENE_TARGET_ITEM;
        request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

        if (dvz_panel_query(state->panel, state->cursor_x, state->cursor_y, &request) != 0)
            fprintf(stderr, "dvz_panel_query() failed\n");
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained point selection feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_selection");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    EXAMPLE_CHECK(dvz_point_set_style(visual, &style) == 0, "dvz_point_set_style() failed");
    EXAMPLE_CHECK(dvz_visual_set_depth_test(visual, false) == 0, "dvz_visual_set_depth_test() failed");

    DvzSelection* selection = dvz_selection(
        scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    EXAMPLE_CHECK(selection != NULL, "dvz_selection() failed");
    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    selection_style.selected.tint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    selection_style.selected.tint_mix = 1.0f;
    selection_style.unselected.visual_flags = DVZ_ITEM_STATE_VISUAL_NONE;
    EXAMPLE_CHECK(
        dvz_selection_set_visual_style(selection, &selection_style) == 0,
        "dvz_selection_set_visual_style() failed");

    SelectionState state = {
        .scene = scene,
        .panel = panel,
        .selection = selection,
    };

    const vec3 positions[POINT_COUNT] = {
        {-0.56f, +0.42f, 0.0f}, {+0.00f, +0.42f, 0.0f}, {+0.56f, +0.42f, 0.0f},
        {-0.56f, +0.00f, 0.0f}, {+0.00f, +0.00f, 0.0f}, {+0.56f, +0.00f, 0.0f},
        {-0.56f, -0.42f, 0.0f}, {+0.00f, -0.42f, 0.0f}, {+0.56f, -0.42f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
    };
    const float diameters[POINT_COUNT] = {
        POINT_SIZE, POINT_SIZE, POINT_SIZE, POINT_SIZE, POINT_SIZE,
        POINT_SIZE, POINT_SIZE, POINT_SIZE, POINT_SIZE,
    };

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    EXAMPLE_CHECK(dvz_visual_set_data_many(visual, updates, 3) == 0, "point data upload failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, visual, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "selection");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    dvz_input_subscribe_pointer(router, _selection_pointer, &state);
    dvz_view_set_frame_callback(win, _selection_frame, &state);

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
