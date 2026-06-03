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
 * Run:    ./build/examples/c/features/pick_hover
 * Smoke:  ./build/examples/c/features/pick_hover 1
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
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained point hover feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_pick_hover");

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

    DvzHover* hover = dvz_hover(
        scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    EXAMPLE_CHECK(hover != NULL, "dvz_hover() failed");
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = HOVER_SIZE / BASE_SIZE;
    EXAMPLE_CHECK(
        dvz_hover_set_visual_style(hover, &hover_style) == 0,
        "dvz_hover_set_visual_style() failed");

    HoverState state = {
        .scene = scene,
        .panel = panel,
        .hover = hover,
    };

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
    EXAMPLE_CHECK(dvz_visual_set_data_many(visual, updates, 3) == 0, "point data upload failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, visual, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "pick_hover");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    dvz_input_subscribe_pointer(router, _hover_pointer, &state);
    dvz_view_set_frame_callback(win, _hover_frame, &state);

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
