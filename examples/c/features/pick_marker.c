/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pick_marker - retained marker item picking and selection proof.
 *
 * Scenario: marker_picking
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Opens a GLFW window showing mixed marker shapes. Move the cursor over the panel to query the
 * frontmost marker item. Hover and selection are rendered by the retained item-state API. Click a
 * marker to toggle persistent selection; click the background to clear it.
 *
 * Current marker queries prove item/frontmost picking. They use the marker family's query bounds;
 * exact SDF shape-discard picking is a later precision improvement.
 *
 * Build:  just build
 * Run:    ./build/examples/c/features/pick_marker --live
 * Smoke:  ./build/examples/c/features/pick_marker --png
 */

#include <math.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_pick_marker_scenario(void);


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600
#define HEIGHT       1200
#define GRID_COLS    9
#define GRID_ROWS    6
#define MARKER_COUNT (GRID_COLS * GRID_ROWS + 5)
#define BASE_SIZE    34.0f
#define HOVER_SIZE   50.0f
#define QUERY_ID     1u
#define PI_F         3.14159265358979323846f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MarkerPickState MarkerPickState;

struct MarkerPickState
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
 * Return one deterministic marker color from the shared graphite-cyan palette.
 *
 * @param index marker index
 * @return marker color
 */
static DvzColor _marker_palette_color(uint32_t index)
{
    const ExampleStyleColorRole roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_TEXT,
    };
    DvzColor color = example_graphite_cyan_color(roles[index % DVZ_ARRAY_COUNT(roles)]);
    color.a = 245u;
    return color;
}



/**
 * Toggle retained selection for one queried marker.
 *
 * @param state marker-pick example state
 * @param query marker item query result
 */
static void _toggle_marker_selection(MarkerPickState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    if (
        query->status != DVZ_QUERY_STATUS_HIT || !query->hit ||
        query->visual_family != DVZ_SCENE_VISUAL_FAMILY_MARKER ||
        query->resolved_target != DVZ_SCENE_TARGET_ITEM || query->resolved_id >= MARKER_COUNT)
        return;

    if (dvz_selection_apply_query(state->selection, query) != 0)
        fprintf(stderr, "dvz_selection_apply_query() failed\n");
    fprintf(stdout, "toggle marker id=%" PRIu64 "\n", query->resolved_id);
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest pointer position and click intent in panel coordinates.
 *
 * @param event portable pointer event
 * @param user_data marker-pick example state
 */
static void _marker_pick_pointer(const DvzScenarioPointerEvent* event, void* user_data)
{
    MarkerPickState* state = (MarkerPickState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (
        event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_PRESS)
        return;

    state->cursor_valid = dvz_scenario_panel_pointer_position(
        state->panel, event, &state->cursor_x, &state->cursor_y);
    if (event->type == DVZ_SCENARIO_POINTER_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        if (!state->cursor_valid)
            return;
        if (state->has_hover_query)
            _toggle_marker_selection(state, &state->latest_hover_query);
        else
            dvz_selection_clear(state->selection);
    }
}



/**
 * Consume marker query results, update hover styling, and queue the next query.
 *
 * @param ctx scenario context
 * @param user_data marker-pick example state
 */
static void _marker_pick_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    MarkerPickState* state = (MarkerPickState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    bool saw_marker_query = false;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id != QUERY_ID)
            continue;

        saw_marker_query = true;
        if (dvz_hover_apply_query(state->hover, &query) != 0)
            fprintf(stderr, "dvz_hover_apply_query() failed\n");
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MARKER &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < MARKER_COUNT)
        {
            state->latest_hover_query = query;
            state->has_hover_query = true;
            fprintf(stdout, "hover marker id=%" PRIu64 "\n", query.resolved_id);
        }
        else
        {
            state->has_hover_query = false;
        }
    }
    if (saw_marker_query && !state->has_hover_query)
        dvz_hover_clear(state->hover);

    if (state->cursor_valid)
    {
        DvzQueryRequest request = dvz_query_request();
        request.request_id = QUERY_ID;
        request.target = DVZ_SCENE_TARGET_ITEM;
        request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

        if (dvz_scenario_panel_query(state->panel, state->cursor_x, state->cursor_y, &request) != 0)
        {
            fprintf(stderr, "dvz_scenario_panel_query() failed\n");
        }
    }
}



/**
 * Handle portable scenario events.
 *
 * @param ctx scenario context
 * @param event portable event
 * @param user scenario state
 */
static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    (void)ctx;
    if (event == NULL)
        return;
    if (event->kind == DVZ_SCENARIO_EVENT_POINTER)
        _marker_pick_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained marker picking feature scenario.
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

    MarkerPickState* state = (MarkerPickState*)calloc(1, sizeof(*state));
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

    DvzVisual* visual = dvz_marker(ctx->scene, 0);
    if (visual == NULL)
        return false;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
    DvzColor grid = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    style.edge_color = dvz_color_rgba(grid.r, grid.g, grid.b, 210);
    style.stroke_width = 2.0f;
    if (dvz_marker_set_style(visual, &style) != 0)
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

    vec3 positions[MARKER_COUNT] = {0};
    DvzColor colors[MARKER_COUNT] = {0};
    float diameters[MARKER_COUNT] = {0};
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
            colors[index] = _marker_palette_color(index);
            diameters[index] = BASE_SIZE;
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
        colors[index] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        diameters[index] = BASE_SIZE;
        angles[index] = 0.25f * PI_F * (float)i;
        shapes[index] = _marker_shape(index + 2u);
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = MARKER_COUNT},
        {.attr_name = "color", .data = colors, .item_count = MARKER_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = MARKER_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = MARKER_COUNT},
        {.attr_name = "shape", .data = shapes, .item_count = MARKER_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 5) != 0)
        return false;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Destroy the retained marker picking feature scenario state.
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
 * Return the retained marker picking scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_pick_marker_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_pick_marker",
        .title = "pick_marker",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_QUERY_READBACK |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _marker_pick_post_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained marker picking feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_pick_marker_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
