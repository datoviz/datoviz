/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pick_hover - retained point hover proof.
 *
 * Scenario: feature.pick_hover
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Opens a GLFW window showing a deterministic point set. Move the cursor over the panel to query
 * the frontmost point item. The hover state is rendered by retained item-state scaling; no
 * selection state is kept.
 *
 * Build:  just example-c features/pick_hover
 * Run:    ./build/examples/c/features/pick_hover --live
 * Smoke:  ./build/examples/c/features/pick_hover --png
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
#define POINT_COUNT 12u
#define BASE_SIZE   36.0f
#define HOVER_SIZE  58.0f
#define QUERY_ID    1u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct HoverState HoverState;

struct HoverState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzHover* hover;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
};



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest pointer position in panel coordinates.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data hover example state
 */
static void _hover_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    HoverState* state = (HoverState*)user_data;
    if (state == NULL || event == NULL || event->type != DVZ_POINTER_EVENT_MOVE)
        return;

    state->cursor_valid =
        example_panel_pointer_position(state->panel, event, &state->cursor_x, &state->cursor_y);
}



/**
 * Consume hover query results and queue the next query.
 *
 * @param win view whose frame just completed
 * @param user_data hover example state
 */
static void _hover_frame(DvzView* win, void* user_data)
{
    (void)win;
    HoverState* state = (HoverState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    bool saw_hover_query = false;
    bool has_hover_query = false;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id != QUERY_ID)
            continue;

        saw_hover_query = true;
        if (dvz_hover_apply_query(state->hover, &query) != 0)
            fprintf(stderr, "dvz_hover_apply_query() failed\n");
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_POINT &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < POINT_COUNT)
        {
            has_hover_query = true;
            fprintf(stdout, "hover point id=%" PRIu64 "\n", query.resolved_id);
        }
    }
    if (saw_hover_query && !has_hover_query)
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
 * Initialize the retained point hover feature scenario.
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

    HoverState* state = (HoverState*)calloc(1, sizeof(*state));
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
    state->hover = hover;

    const vec3 positions[POINT_COUNT] = {
        {-0.72f, -0.38f, 0.0f}, {-0.48f, -0.18f, 0.0f}, {-0.24f, -0.34f, 0.0f},
        {+0.00f, -0.10f, 0.0f}, {+0.24f, -0.34f, 0.0f}, {+0.48f, -0.18f, 0.0f},
        {+0.72f, -0.38f, 0.0f}, {-0.54f, +0.34f, 0.0f}, {-0.27f, +0.16f, 0.0f},
        {+0.00f, +0.42f, 0.0f}, {+0.27f, +0.16f, 0.0f}, {+0.54f, +0.34f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {0};
    float diameters[POINT_COUNT] = {0};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        colors[i] = example_graphite_cyan_color(
            i % 3u == 0 ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
                        : i % 3u == 1 ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                                      : EXAMPLE_STYLE_COLOR_TEXT);
        diameters[i] = BASE_SIZE;
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
 * Attach native GLFW callbacks for the retained point hover feature scenario.
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
    HoverState* state = (HoverState*)user;
    if (state == NULL || view == NULL)
        return false;

    DvzInputRouter* router = dvz_view_input(view);
    if (router == NULL)
        return true;

    dvz_input_subscribe_pointer(router, _hover_pointer, state);
    dvz_view_set_frame_callback(view, _hover_frame, state);
    return true;
}



/**
 * Destroy the retained point hover feature scenario state.
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
 * Return the retained point hover scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _pick_hover_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_pick_hover",
        .title = "pick_hover",
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
 * Run the retained point hover feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _pick_hover_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
