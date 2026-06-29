/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* selection_sphere - retained sphere hover and click selection.
 *
 * Scenario: feature.selection_sphere
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Move the cursor over the sphere cluster to query the frontmost sphere item. Hover and selection
 * are rendered by the retained item-state API. Click a sphere to toggle selection; click the
 * background to clear it. Drag to rotate the 3D scene with the arcball controller.
 *
 * Build:  just example-c features/selection_sphere
 * Run:    ./build/examples/c/features/selection_sphere --live
 * Smoke:  ./build/examples/c/features/selection_sphere --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SPHERE_COUNT 54u
#define QUERY_HOVER_ID 17u
#define QUERY_CLICK_ID 18u

static const float TAU = 6.28318530718f;

DvzScenarioSpec dvz_example_selection_sphere_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SphereSelectionState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzHover* hover;
    vec3* positions;
    DvzColor* colors;
    float* radii;
    DvzQueryResult latest_hover_query;
    bool has_hover_query;
    bool cursor_valid;
} SphereSelectionState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic compact sphere cluster.
 *
 * @param positions output sphere centers
 * @param radii output sphere radii
 * @param colors output sphere colors
 */
static void _fill_spheres(
    vec3 positions[SPHERE_COUNT], float radii[SPHERE_COUNT], DvzColor colors[SPHERE_COUNT])
{
    ANN(positions);
    ANN(radii);
    ANN(colors);

    const ExampleStyleColorRole palette[] = {
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_ERROR,
    };

    for (uint32_t i = 0; i < SPHERE_COUNT; i++)
    {
        const float t = (float)i / (float)(SPHERE_COUNT - 1u);
        const float angle = TAU * (0.132f * (float)i + 0.08f * sinf(9.0f * t));
        const float layer = 2.0f * t - 1.0f;
        const float ring = 0.34f + 0.58f * sqrtf(1.0f - 0.60f * layer * layer);
        const float wobble = 0.5f + 0.5f * sinf(19.0f * t + 0.35f);

        positions[i][0] = ring * cosf(angle);
        positions[i][1] = 0.72f * layer + 0.10f * sinf(3.0f * angle);
        positions[i][2] = ring * sinf(angle) + 0.24f * layer;
        radii[i] = 0.055f + 0.065f * (1.0f - fabsf(layer)) + 0.026f * wobble;
        colors[i] = example_graphite_cyan_color(palette[i % DVZ_ARRAY_COUNT(palette)]);
    }
}


/**
 * Free the retained state buffers.
 *
 * @param state sphere-selection example state
 */
static void _free_state(SphereSelectionState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->radii);
    dvz_free(state->colors);
    dvz_free(state->positions);
    dvz_free(state);
}


/**
 * Toggle retained selection for one queried sphere.
 *
 * @param state sphere-selection example state
 * @param query sphere item query result
 */
static void _toggle_sphere_selection(SphereSelectionState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    if (
        query->status != DVZ_QUERY_STATUS_HIT || !query->hit ||
        query->visual_family != DVZ_SCENE_VISUAL_FAMILY_SPHERE ||
        query->resolved_target != DVZ_SCENE_TARGET_ITEM || query->resolved_id >= SPHERE_COUNT)
        return;

    if (dvz_selection_apply_query(state->selection, query) != 0)
        fprintf(stderr, "dvz_selection_apply_query() failed\n");
    fprintf(stdout, "toggle sphere id=%" PRIu64 "\n", query->resolved_id);
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Queue hover and click queries from panel-local pointer events.
 *
 * @param event portable pointer event
 * @param user_data sphere-selection example state
 */
static void _selection_sphere_pointer(const DvzScenarioPointerEvent* event, void* user_data)
{
    SphereSelectionState* state = (SphereSelectionState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (
        event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_PRESS)
        return;

    double x = 0.0;
    double y = 0.0;
    double panel_pos[2] = {0};
    state->cursor_valid = dvz_panel_transform_point(
        state->panel, DVZ_PANEL_COORD_FIGURE_PX, DVZ_PANEL_COORD_PANEL_PX,
        (const double[2]){event->x, event->y}, panel_pos);
    x = panel_pos[0];
    y = panel_pos[1];
    if (!state->cursor_valid)
    {
        if (state->has_hover_query)
        {
            dvz_hover_clear(state->hover);
            state->has_hover_query = false;
        }
        if (event->type == DVZ_SCENARIO_POINTER_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
            dvz_selection_clear(state->selection);
        return;
    }

    DvzQueryRequest request = dvz_query_request();
    request.target = DVZ_SCENE_TARGET_ITEM;
    request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;
    if (event->type == DVZ_SCENARIO_POINTER_MOVE)
    {
        request.request_id = QUERY_HOVER_ID;
        if (dvz_panel_query(state->panel, x, y, &request) != 0)
            fprintf(stderr, "dvz_panel_query() failed\n");
    }
    else if (event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        request.request_id = QUERY_CLICK_ID;
        if (dvz_panel_query(state->panel, x, y, &request) != 0)
            fprintf(stderr, "dvz_panel_query(click) failed\n");
    }
}


/**
 * Consume sphere query results and update hover styling.
 *
 * @param ctx scenario context
 * @param user_data sphere-selection example state
 */
static void _selection_sphere_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    SphereSelectionState* state = (SphereSelectionState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id == QUERY_CLICK_ID)
        {
            if (
                query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
                query.visual_family == DVZ_SCENE_VISUAL_FAMILY_SPHERE &&
                query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < SPHERE_COUNT)
            {
                _toggle_sphere_selection(state, &query);
            }
            else
            {
                dvz_selection_clear(state->selection);
            }
            continue;
        }
        if (query.request_id != QUERY_HOVER_ID)
            continue;
        if (!state->cursor_valid)
            continue;

        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_SPHERE &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < SPHERE_COUNT)
        {
            bool same_item =
                state->latest_hover_query.visual_id == query.visual_id &&
                state->latest_hover_query.resolved_id == query.resolved_id;
            bool hover_changed = !state->has_hover_query || !same_item;
            if (hover_changed && dvz_hover_apply_query(state->hover, &query) != 0)
                fprintf(stderr, "dvz_hover_apply_query() failed\n");
            if (!same_item)
                fprintf(stdout, "hover sphere id=%" PRIu64 "\n", query.resolved_id);
            state->has_hover_query = true;
            state->latest_hover_query = query;
        }
        else
        {
            if (state->has_hover_query)
                dvz_hover_clear(state->hover);
            state->has_hover_query = false;
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
        _selection_sphere_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained sphere selection feature scenario.
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

    SphereSelectionState* state = (SphereSelectionState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->positions = (vec3*)dvz_calloc(SPHERE_COUNT, sizeof(*state->positions));
    state->colors = (DvzColor*)dvz_calloc(SPHERE_COUNT, sizeof(*state->colors));
    state->radii = (float*)dvz_calloc(SPHERE_COUNT, sizeof(*state->radii));
    if (state->positions == NULL || state->colors == NULL || state->radii == NULL)
        goto error;
    _fill_spheres(state->positions, state->radii, state->colors);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (example_set_default_3d_camera(panel, 1.0f) == NULL)
        goto error;

    DvzVisual* visual = dvz_sphere(ctx->scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (visual == NULL)
        goto error;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);
    if (dvz_sphere_mode(visual, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        goto error;

    DvzMaterialDesc material = example_default_standard_material_desc();
    if (dvz_visual_set_material(visual, &material) != 0)
        goto error;

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
    hover_style.scale = 1.35f;
    if (dvz_hover_set_visual_style(hover, &hover_style) != 0)
        goto error;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = state->radii, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = state->colors, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        goto error;

    DvzController* arcball_controller = dvz_arcball(ctx->scene, NULL);
    if (arcball_controller == NULL)
        goto error;
    if (dvz_scenario_bind_controller(ctx, panel, arcball_controller, DVZ_DIM_MASK_XYZ) != 0)
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
 * Destroy the retained sphere selection feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((SphereSelectionState*)user);
}


/**
 * Return the retained sphere selection scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_selection_sphere_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_selection_sphere",
        .title = "selection_sphere",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_QUERY_READBACK | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _selection_sphere_post_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
/**
 * Run the retained sphere selection feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_selection_sphere_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
