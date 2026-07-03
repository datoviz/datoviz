/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* selection_mesh_instances - retained instanced mesh hover and click selection.
 *
 * Scenario: feature.selection_mesh_instances
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Move the cursor over the cube field to query the frontmost mesh instance. Hover and selection
 * are rendered by the retained item-state API. Click a cube to toggle selection; click the
 * background to clear it. Drag to rotate the 3D scene with the arcball controller.
 *
 * Build:  just example-c features/selection_mesh_instances
 * Run:    ./build/examples/c/features/selection_mesh_instances --live
 * Smoke:  ./build/examples/c/features/selection_mesh_instances --png
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
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define GRID_X         6u
#define GRID_Y         4u
#define GRID_Z         2u
#define INSTANCE_COUNT (GRID_X * GRID_Y * GRID_Z)
#define QUERY_HOVER_ID 23u
#define QUERY_CLICK_ID 24u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_selection_mesh_instances_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MeshInstanceSelectionState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSelection* selection;
    DvzHover* hover;
    float (*transforms)[16];
    DvzQueryResult latest_hover_query;
    bool has_hover_query;
    bool cursor_valid;
} MeshInstanceSelectionState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill one column-major rotation/scale/translation transform.
 *
 * @param transform output mat4 storage
 * @param tx translation on X
 * @param ty translation on Y
 * @param tz translation on Z
 * @param scale uniform scale
 * @param angle_x rotation around X in radians
 * @param angle_z rotation around Z in radians
 * @param angle_y rotation around Y in radians
 */
static void _cube_transform(
    float transform[16],
    float tx,
    float ty,
    float tz,
    float scale,
    float angle_x,
    float angle_z,
    float angle_y)
{
    if (transform == NULL)
        return;

    const float cx = cosf(angle_x);
    const float sx = sinf(angle_x);
    const float cz = cosf(angle_z);
    const float sz = sinf(angle_z);
    const float cy = cosf(angle_y);
    const float sy = sinf(angle_y);

    transform[0] = scale * cz * cy;
    transform[1] = scale * sz * cy;
    transform[2] = scale * -sy;
    transform[3] = 0.0f;
    transform[4] = scale * (cz * sy * sx - sz * cx);
    transform[5] = scale * (sz * sy * sx + cz * cx);
    transform[6] = scale * cy * sx;
    transform[7] = 0.0f;
    transform[8] = scale * (cz * sy * cx + sz * sx);
    transform[9] = scale * (sz * sy * cx - cz * sx);
    transform[10] = scale * cy * cx;
    transform[11] = 0.0f;
    transform[12] = tx;
    transform[13] = ty;
    transform[14] = tz;
    transform[15] = 1.0f;
}


/**
 * Allocate and fill the cube instance transform grid.
 *
 * @return heap-allocated transform array, or NULL on failure
 */
static float (*_make_cube_transforms(void))[16]
{
    float (*transforms)[16] = (float(*)[16])dvz_calloc(INSTANCE_COUNT, sizeof(*transforms));
    if (transforms == NULL)
        return NULL;

    const float spacing = 0.50f;
    const float half_x = 0.5f * (float)(GRID_X - 1u);
    const float half_y = 0.5f * (float)(GRID_Y - 1u);
    const float half_z = 0.5f * (float)(GRID_Z - 1u);
    uint32_t idx = 0;
    for (uint32_t z = 0; z < GRID_Z; z++)
    {
        for (uint32_t y = 0; y < GRID_Y; y++)
        {
            for (uint32_t x = 0; x < GRID_X; x++)
            {
                const float fx = ((float)x - half_x) * spacing +
                                 0.035f * sinf(1.70f * (float)x + 0.90f * (float)y);
                const float fy = ((float)y - half_y) * spacing +
                                 0.030f * cosf(1.10f * (float)y + 0.80f * (float)z);
                const float fz = ((float)z - half_z) * spacing +
                                 0.045f * sinf(0.80f * (float)x - 1.15f * (float)y);
                const float wave =
                    0.5f + 0.5f * sinf(TAU * ((float)x / (float)GRID_X + (float)z * 0.11f));
                const float scale = 0.62f + 0.30f * wave;
                const float angle_x =
                    0.22f * sinf(0.95f * (float)x + 1.55f * (float)y + 0.70f * (float)z);
                const float angle_z = 0.10f * (float)x + 0.17f * (float)y;
                const float angle_y = 0.24f * (float)z - 0.06f * (float)y +
                                      0.14f * cosf(1.20f * (float)x + 0.50f * (float)y);
                _cube_transform(
                    transforms[idx++], fx, fy, fz, scale, angle_x, angle_z, angle_y);
            }
        }
    }

    return transforms;
}


/**
 * Free the retained state buffers.
 *
 * @param state mesh-instance selection state
 */
static void _free_state(MeshInstanceSelectionState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->transforms);
    dvz_free(state);
}


/**
 * Create a muted colored cube mesh for the selection field.
 *
 * @param scene scene owning the visual
 * @return mesh visual, or NULL on error
 */
static DvzVisual* _selection_cube_mesh(DvzScene* scene)
{
    if (scene == NULL)
        return NULL;

    DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        dvz_color_rgb(76, 201, 240),
        dvz_color_rgb(38, 132, 167),
        dvz_color_rgb(128, 255, 219),
        dvz_color_rgb(42, 176, 142),
        dvz_color_rgb(132, 142, 239),
        dvz_color_rgb(176, 112, 221),
    };

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return NULL;

    DvzGeometry* cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = 0.32,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return NULL;

    int rc = dvz_mesh_set_geometry(visual, cube);
    dvz_geometry_destroy(cube);
    return rc == 0 ? visual : NULL;
}


/**
 * Toggle retained selection for one queried mesh instance.
 *
 * @param state mesh-instance selection state
 * @param query mesh item query result
 */
static void _toggle_mesh_selection(
    MeshInstanceSelectionState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    if (
        query->status != DVZ_QUERY_STATUS_HIT || !query->hit ||
        query->visual_family != DVZ_SCENE_VISUAL_FAMILY_MESH ||
        query->resolved_target != DVZ_SCENE_TARGET_ITEM || query->resolved_id >= INSTANCE_COUNT)
        return;

    if (dvz_selection_apply_query(state->selection, query) != 0)
        fprintf(stderr, "dvz_selection_apply_query() failed\n");
    fprintf(stdout, "toggle mesh instance id=%" PRIu64 "\n", query->resolved_id);
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Queue hover and click queries from panel-local pointer events.
 *
 * @param event portable pointer event
 * @param user_data mesh-instance selection state
 */
static void _selection_mesh_pointer(const DvzScenarioPointerEvent* event, void* user_data)
{
    MeshInstanceSelectionState* state = (MeshInstanceSelectionState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (
        event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_CLICK)
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
        if (event->type == DVZ_SCENARIO_POINTER_CLICK && event->button == DVZ_POINTER_BUTTON_LEFT)
            dvz_selection_clear(state->selection);
        return;
    }

    DvzQueryRequest request = dvz_query_request();
    request.target = DVZ_SCENE_TARGET_ITEM;
    request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;
    if (event->type == DVZ_SCENARIO_POINTER_MOVE)
    {
        request.request_id = QUERY_HOVER_ID;
        if (dvz_panel_query_px(state->panel, x, y, &request) != 0)
            fprintf(stderr, "dvz_panel_query_px() failed\n");
    }
    else if (event->type == DVZ_SCENARIO_POINTER_CLICK && event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        request.request_id = QUERY_CLICK_ID;
        if (dvz_panel_query_px(state->panel, x, y, &request) != 0)
            fprintf(stderr, "dvz_panel_query_px(click) failed\n");
    }
}


/**
 * Consume mesh query results and update hover styling.
 *
 * @param ctx scenario context
 * @param user_data mesh-instance selection state
 */
static void _selection_mesh_post_frame(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    MeshInstanceSelectionState* state = (MeshInstanceSelectionState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id == QUERY_CLICK_ID)
        {
            if (
                query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
                query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MESH &&
                query.resolved_target == DVZ_SCENE_TARGET_ITEM &&
                query.resolved_id < INSTANCE_COUNT)
            {
                _toggle_mesh_selection(state, &query);
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
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MESH &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < INSTANCE_COUNT)
        {
            bool same_item =
                state->latest_hover_query.visual_id == query.visual_id &&
                state->latest_hover_query.resolved_id == query.resolved_id;
            bool hover_changed = !state->has_hover_query || !same_item;
            if (hover_changed && dvz_hover_apply_query(state->hover, &query) != 0)
                fprintf(stderr, "dvz_hover_apply_query() failed\n");
            if (!same_item)
                fprintf(stdout, "hover mesh instance id=%" PRIu64 "\n", query.resolved_id);
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
        _selection_mesh_pointer(&event->content.pointer, user);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained mesh instance selection feature scenario.
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

    MeshInstanceSelectionState* state =
        (MeshInstanceSelectionState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->transforms = _make_cube_transforms();
    if (state->transforms == NULL)
        goto error;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (example_set_default_3d_camera(panel, 1.4f) == NULL)
        goto error;

    DvzVisual* visual = _selection_cube_mesh(ctx->scene);
    if (visual == NULL)
        goto error;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzMaterialDesc material = example_default_standard_material_desc();
    material.light_direction[0] = +0.32f;
    material.light_direction[1] = -0.50f;
    material.light_direction[2] = +0.80f;
    material.standard.roughness = 0.54f;
    material.standard.specular = 0.28f;
    material.standard.rim_strength = 0.08f;
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
    hover_style.scale = 1.24f;
    if (dvz_hover_set_visual_style(hover, &hover_style) != 0)
        goto error;

    if (dvz_visual_set_data(visual, "instance_transform", state->transforms, INSTANCE_COUNT) != 0)
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
 * Destroy the retained mesh instance selection feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((MeshInstanceSelectionState*)user);
}


/**
 * Return the retained mesh instance selection scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_selection_mesh_instances_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_selection_mesh_instances",
        .title = "Mesh Instance Selection",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_QUERY_READBACK | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .event = _scenario_event,
        .post_frame = _selection_mesh_post_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained mesh instance selection feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_selection_mesh_instances_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
