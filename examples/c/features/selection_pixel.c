/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* selection_pixel - retained pixel hover and click selection.
 *
 * Scenario: feature.selection_pixel
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Move the cursor over the pixel field to query the frontmost pixel item. Hover and selection are
 * rendered by the retained item-state API. Click a pixel to toggle selection; click the background
 * to clear it.
 *
 * Build:  just example-c features/selection_pixel
 * Run:    ./build/examples/c/features/selection_pixel --live
 * Smoke:  ./build/examples/c/features/selection_pixel --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"


DvzScenarioSpec dvz_example_selection_pixel_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define GRID_WIDTH  56u
#define GRID_HEIGHT 40u
#define PIXEL_COUNT (GRID_WIDTH * GRID_HEIGHT)
#define PIXEL_SIZE  12.0f
#define QUERY_HOVER_ID 11u
#define QUERY_CLICK_ID 12u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PixelSelectionState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzHover* hover;
    vec3* positions;
    DvzColor* colors;
    float* sizes;
    DvzQueryResult latest_hover_query;
    bool has_hover_query;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    bool pending_click;
    double click_x;
    double click_y;
} PixelSelectionState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp one value to the unit interval.
 *
 * @param x value
 * @return clamped value
 */
static float _clamp01(float x)
{
    return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x;
}


/**
 * Return one deterministic scalar sample for the pixel grid.
 *
 * @param u normalized grid x coordinate
 * @param v normalized grid y coordinate
 * @return normalized scalar value
 */
static float _sample_value(float u, float v)
{
    const float ridge = 0.5f + 0.5f * sinf(TAU * (1.7f * u + 0.35f * v));
    const float wave = 0.5f + 0.5f * cosf(TAU * (0.6f * u - 1.9f * v));
    const float dx = u - 0.34f;
    const float dy = v - 0.64f;
    const float blob = expf(-(dx * dx + 1.6f * dy * dy) / 0.012f);
    const float value = 0.08f + 0.36f * u + 0.18f * v + 0.16f * ridge + 0.10f * wave +
                        0.28f * blob;
    return _clamp01(value);
}


/**
 * Return a graphite-cyan ramp sample.
 *
 * @param t normalized scalar value
 * @return RGBA8 color
 */
static DvzColor _ramp(float t)
{
    t = _clamp01(t);
    const DvzColor c0 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    const DvzColor c1 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    const DvzColor c2 = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    const DvzColor a = t < 0.5f ? c0 : c1;
    const DvzColor b = t < 0.5f ? c1 : c2;
    const float s = t < 0.5f ? 2.0f * t : 2.0f * (t - 0.5f);
    return dvz_color_rgba(
        (uint8_t)((1.0f - s) * (float)a.r + s * (float)b.r + 0.5f),
        (uint8_t)((1.0f - s) * (float)a.g + s * (float)b.g + 0.5f),
        (uint8_t)((1.0f - s) * (float)a.b + s * (float)b.b + 0.5f), 245u);
}


/**
 * Fill the deterministic pixel grid.
 *
 * @param positions output pixel positions
 * @param colors output pixel colors
 * @param sizes output pixel sprite sizes in pixels
 */
static void _fill_pixels(
    vec3 positions[PIXEL_COUNT], DvzColor colors[PIXEL_COUNT], float sizes[PIXEL_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    const float step_x = 1.82f / (float)(GRID_WIDTH - 1u);
    const float step_y = 1.36f / (float)(GRID_HEIGHT - 1u);

    for (uint32_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < GRID_WIDTH; x++)
        {
            const uint32_t i = y * GRID_WIDTH + x;
            const float u = (float)x / (float)(GRID_WIDTH - 1u);
            const float v = (float)y / (float)(GRID_HEIGHT - 1u);
            const float value = _sample_value(u, v);

            positions[i][0] = -0.91f + step_x * (float)x;
            positions[i][1] = -0.68f + step_y * (float)y;
            positions[i][2] = 0.0f;
            colors[i] = _ramp(value);
            sizes[i] = PIXEL_SIZE;
        }
    }
}


/**
 * Free the retained state buffers.
 *
 * @param state pixel selection state
 */
static void _free_state(PixelSelectionState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->sizes);
    dvz_free(state->colors);
    dvz_free(state->positions);
    dvz_free(state);
}


/**
 * Toggle retained selection for one queried pixel.
 *
 * @param state pixel-selection example state
 * @param query pixel item query result
 */
static void _toggle_pixel_selection(PixelSelectionState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    if (
        query->status != DVZ_QUERY_STATUS_HIT || !query->hit ||
        query->visual_family != DVZ_SCENE_VISUAL_FAMILY_PIXEL ||
        query->resolved_target != DVZ_SCENE_TARGET_ITEM || query->resolved_id >= PIXEL_COUNT)
        return;

    if (dvz_selection_apply_query(state->selection, query) != 0)
        fprintf(stderr, "dvz_selection_apply_query() failed\n");
    fprintf(stdout, "toggle pixel id=%" PRIu64 "\n", query->resolved_id);
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record pointer position and click intent in panel coordinates.
 *
 * @param event portable pointer event
 * @param user_data pixel-selection example state
 */
static void _selection_pixel_pointer(const DvzScenarioPointerEvent* event, void* user_data)
{
    PixelSelectionState* state = (PixelSelectionState*)user_data;
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
        {
            dvz_selection_clear(state->selection);
            return;
        }
        state->click_x = state->cursor_x;
        state->click_y = state->cursor_y;
        state->pending_click = true;
    }
}


/**
 * Consume pixel query results, update hover styling, and queue the next query.
 *
 * @param ctx scenario context
 * @param user_data pixel-selection example state
 */
static void _selection_pixel_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    PixelSelectionState* state = (PixelSelectionState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    bool saw_pixel_query = false;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id == QUERY_CLICK_ID)
        {
            if (
                query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
                query.visual_family == DVZ_SCENE_VISUAL_FAMILY_PIXEL &&
                query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < PIXEL_COUNT)
            {
                _toggle_pixel_selection(state, &query);
            }
            else
            {
                dvz_selection_clear(state->selection);
            }
            continue;
        }
        if (query.request_id != QUERY_HOVER_ID)
            continue;

        saw_pixel_query = true;
        if (dvz_hover_apply_query(state->hover, &query) != 0)
            fprintf(stderr, "dvz_hover_apply_query() failed\n");
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_PIXEL &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < PIXEL_COUNT)
        {
            state->latest_hover_query = query;
            state->has_hover_query = true;
            fprintf(stdout, "hover pixel id=%" PRIu64 "\n", query.resolved_id);
        }
        else
        {
            state->has_hover_query = false;
        }
    }
    if (saw_pixel_query && !state->has_hover_query)
        dvz_hover_clear(state->hover);

    if (state->cursor_valid)
    {
        DvzQueryRequest request = dvz_query_request();
        request.request_id = QUERY_HOVER_ID;
        request.target = DVZ_SCENE_TARGET_ITEM;
        request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

        if (dvz_scenario_panel_query(state->panel, state->cursor_x, state->cursor_y, &request) != 0)
            fprintf(stderr, "dvz_scenario_panel_query() failed\n");
    }
    if (state->pending_click)
    {
        DvzQueryRequest request = dvz_query_request();
        request.request_id = QUERY_CLICK_ID;
        request.target = DVZ_SCENE_TARGET_ITEM;
        request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

        if (dvz_scenario_panel_query(state->panel, state->click_x, state->click_y, &request) != 0)
            fprintf(stderr, "dvz_scenario_panel_query(click) failed\n");
        state->pending_click = false;
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
        _selection_pixel_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained pixel selection feature scenario.
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

    PixelSelectionState* state = (PixelSelectionState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->positions = (vec3*)dvz_calloc(PIXEL_COUNT, sizeof(*state->positions));
    state->colors = (DvzColor*)dvz_calloc(PIXEL_COUNT, sizeof(*state->colors));
    state->sizes = (float*)dvz_calloc(PIXEL_COUNT, sizeof(*state->sizes));
    if (state->positions == NULL || state->colors == NULL || state->sizes == NULL)
        goto error;
    _fill_pixels(state->positions, state->colors, state->sizes);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_pixel(ctx->scene, 0);
    if (visual == NULL)
        goto error;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzSelection* selection = dvz_selection(
        ctx->scene,
        &(DvzSelectionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzSelectionDesc),
            .mode = DVZ_SELECT_TOGGLE,
            .target = DVZ_SCENE_TARGET_ITEM,
        });
    if (selection == NULL)
        goto error;
    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    selection_style.selected.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    selection_style.selected.tint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    selection_style.selected.tint_mix = 1.0f;
    selection_style.unselected.visual_flags = DVZ_ITEM_STATE_VISUAL_NONE;
    if (dvz_selection_set_visual_style(selection, &selection_style) != 0)
        goto error;

    DvzHover* hover = dvz_hover(
        ctx->scene,
        &(DvzHoverDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzHoverDesc),
            .target = DVZ_SCENE_TARGET_ITEM,
            .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
        });
    if (hover == NULL)
        goto error;
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    hover_style.scale = 1.7f;
    if (dvz_hover_set_visual_style(hover, &hover_style) != 0)
        goto error;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = PIXEL_COUNT},
        {.attr_name = "color", .data = state->colors, .item_count = PIXEL_COUNT},
        {.attr_name = "pixel_size", .data = state->sizes, .item_count = PIXEL_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        goto error;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        goto error;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        goto error;

    state->scene = ctx->scene;
    state->panel = panel;
    state->selection = selection;
    state->hover = hover;
    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}


/**
 * Destroy the retained pixel selection feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((PixelSelectionState*)user);
}


/**
 * Return the retained pixel selection scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_selection_pixel_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_selection_pixel",
        .title = "selection_pixel",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_PIXEL_VISUAL | DVZ_SCENARIO_REQ_QUERY_READBACK |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _selection_pixel_post_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Run the retained pixel selection feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_selection_pixel_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
