/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pick_point - retained point item picking proof.
 *
 * Scenario: feature.pick_point
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Opens a GLFW window showing a deterministic point set. Move the cursor over the panel to query
 * the frontmost point item. Hover is rendered with retained item-state scaling. Left-click toggles
 * persistent selection; clicking the background clears it.
 *
 * Build:  just example-c features/pick_point
 * Run:    ./build/examples/c/features/pick_point --live
 * Smoke:  ./build/examples/c/features/pick_point --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define GRID_COLS   8u
#define GRID_ROWS   5u
#define POINT_COUNT (GRID_COLS * GRID_ROWS)
#define BASE_SIZE   34.0f
#define HOVER_SIZE  52.0f
#define QUERY_ID    1u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PointPickState PointPickState;

struct PointPickState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzHover* hover;
    DvzQueryResult latest_hover_query;
    bool has_hover_query;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one deterministic point color from the shared graphite-cyan palette.
 *
 * @param index point index
 * @return point color
 */
static DvzColor _point_color(uint32_t index)
{
    const ExampleStyleColorRole roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_WARNING,
    };
    DvzColor color = example_graphite_cyan_color(roles[index % DVZ_ARRAY_COUNT(roles)]);
    color.a = 245u;
    return color;
}



/**
 * Toggle retained selection for one queried point.
 *
 * @param state point-pick example state
 * @param query point item query result
 */
static void _toggle_point_selection(PointPickState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    if (
        query->status != DVZ_QUERY_STATUS_HIT || !query->hit ||
        query->visual_family != DVZ_SCENE_VISUAL_FAMILY_POINT ||
        query->resolved_target != DVZ_SCENE_TARGET_ITEM || query->resolved_id >= POINT_COUNT)
        return;

    if (dvz_selection_apply_query(state->selection, query) != 0)
        fprintf(stderr, "dvz_selection_apply_query() failed\n");
    fprintf(stdout, "toggle point id=%" PRIu64 "\n", query->resolved_id);
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record pointer position and dispatch click selection.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data point-pick example state
 */
static void _point_pick_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    PointPickState* state = (PointPickState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_PRESS)
        return;

    state->cursor_valid =
        example_panel_pointer_position(state->panel, event, &state->cursor_x, &state->cursor_y);
    if (event->type == DVZ_POINTER_EVENT_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        if (!state->cursor_valid)
            return;
        if (state->has_hover_query)
            _toggle_point_selection(state, &state->latest_hover_query);
        else
            dvz_selection_clear(state->selection);
    }
}



/**
 * Consume point query results, update hover styling, and queue the next query.
 *
 * @param win view whose frame just completed
 * @param user_data point-pick example state
 */
static void _point_pick_frame(DvzView* win, void* user_data)
{
    (void)win;
    PointPickState* state = (PointPickState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    bool saw_point_query = false;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id != QUERY_ID)
            continue;

        saw_point_query = true;
        if (dvz_hover_apply_query(state->hover, &query) != 0)
            fprintf(stderr, "dvz_hover_apply_query() failed\n");
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < POINT_COUNT)
        {
            state->latest_hover_query = query;
            state->has_hover_query = true;
            fprintf(stdout, "hover point id=%" PRIu64 "\n", query.resolved_id);
        }
        else
        {
            state->has_hover_query = false;
        }
    }
    if (saw_point_query && !state->has_hover_query)
        dvz_hover_clear(state->hover);

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
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained point picking feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    PointPickState* state = (PointPickState*)calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_point(ctx->scene, 0);
    if (visual == NULL)
        return false;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;

    DvzSelection* selection = dvz_selection(
        ctx->scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    if (selection == NULL)
        return false;
    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    selection_style.selected.tint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    selection_style.selected.tint_mix = 1.0f;
    selection_style.unselected.visual_flags = DVZ_ITEM_STATE_VISUAL_NONE;
    if (dvz_selection_set_visual_style(selection, &selection_style) != 0)
        return false;

    DvzHover* hover = dvz_hover(
        ctx->scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    if (hover == NULL)
        return false;
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = HOVER_SIZE / BASE_SIZE;
    if (dvz_hover_set_visual_style(hover, &hover_style) != 0)
        return false;

    state->scene = ctx->scene;
    state->panel = panel;
    state->selection = selection;
    state->hover = hover;

    vec3 positions[POINT_COUNT] = {0};
    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    for (uint32_t row = 0; row < GRID_ROWS; row++)
    {
        for (uint32_t col = 0; col < GRID_COLS; col++)
        {
            const uint32_t index = row * GRID_COLS + col;
            positions[index][0] = -0.82f + 1.64f * ((float)col / (float)(GRID_COLS - 1u));
            positions[index][1] = +0.62f - 1.24f * ((float)row / (float)(GRID_ROWS - 1u));
            positions[index][2] = 0.0f;
            colors[index] = _point_color(index);
            diameters[index] = BASE_SIZE;
        }
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Attach native GLFW callbacks for the retained point picking feature scenario.
 *
 * @param ctx scenario context
 * @param app native app
 * @param view native view
 * @param user scenario state
 * @return true on success
 */
static bool _scenario_native_view(
    DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)ctx;
    (void)app;
    PointPickState* state = (PointPickState*)user;
    if (state == NULL || view == NULL)
        return false;

    DvzInputRouter* router = dvz_view_input(view);
    if (router == NULL)
        return true;

    dvz_input_subscribe_pointer(router, _point_pick_pointer, state);
    dvz_view_set_frame_callback(view, _point_pick_frame, state);
    return true;
}



/**
 * Destroy the retained point picking feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    free(user);
}



/**
 * Return the retained point picking scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _pick_point_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_pick_point",
        .title = "pick_point",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .native_view = _scenario_native_view,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained point picking feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _pick_point_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
