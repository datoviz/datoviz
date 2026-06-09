/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* selection_mesh_instances - retained instanced mesh hover and click selection.
 *
 * Scenario: feature.selection_mesh_instances
 * Style: features, graphite_cyan, 1600x1200 capture target
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

#define WIDTH          1600u
#define HEIGHT         1200u
#define GRID_X         7u
#define GRID_Y         5u
#define GRID_Z         3u
#define INSTANCE_COUNT (GRID_X * GRID_Y * GRID_Z)
#define QUERY_ID       23u

static const float TAU = 6.28318530718f;



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
    double cursor_x;
    double cursor_y;
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
 * @param angle_z rotation around Z in radians
 * @param angle_y rotation around Y in radians
 */
static void _cube_transform(
    float transform[16], float tx, float ty, float tz, float scale, float angle_z, float angle_y)
{
    if (transform == NULL)
        return;

    const float cz = cosf(angle_z);
    const float sz = sinf(angle_z);
    const float cy = cosf(angle_y);
    const float sy = sinf(angle_y);

    transform[0] = scale * cz * cy;
    transform[1] = scale * sz * cy;
    transform[2] = scale * -sy;
    transform[3] = 0.0f;
    transform[4] = scale * -sz;
    transform[5] = scale * cz;
    transform[6] = 0.0f;
    transform[7] = 0.0f;
    transform[8] = scale * cz * sy;
    transform[9] = scale * sz * sy;
    transform[10] = scale * cy;
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

    const float spacing = 0.42f;
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
                const float fx = ((float)x - half_x) * spacing;
                const float fy = ((float)y - half_y) * spacing;
                const float fz = ((float)z - half_z) * spacing;
                const float wave =
                    0.5f + 0.5f * sinf(TAU * ((float)x / (float)GRID_X + (float)z * 0.11f));
                const float scale = 0.62f + 0.30f * wave;
                const float angle_z = 0.10f * (float)x + 0.17f * (float)y;
                const float angle_y = 0.24f * (float)z - 0.06f * (float)y;
                _cube_transform(transforms[idx++], fx, fy, fz, scale, angle_z, angle_y);
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
 * Record pointer position and click intent in panel coordinates.
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
        event->type != DVZ_SCENARIO_POINTER_MOVE && event->type != DVZ_SCENARIO_POINTER_PRESS)
        return;

    state->cursor_valid = dvz_scenario_panel_pointer_position(
        state->panel, event, &state->cursor_x, &state->cursor_y);
    if (event->type == DVZ_SCENARIO_POINTER_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        if (!state->cursor_valid)
            return;
        if (state->has_hover_query)
            _toggle_mesh_selection(state, &state->latest_hover_query);
        else
            dvz_selection_clear(state->selection);
    }
}


/**
 * Consume mesh query results, update hover styling, and queue the next query.
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
    bool saw_mesh_query = false;
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.request_id != QUERY_ID)
            continue;

        saw_mesh_query = true;
        if (dvz_hover_apply_query(state->hover, &query) != 0)
            fprintf(stderr, "dvz_hover_apply_query() failed\n");
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.visual_family == DVZ_SCENE_VISUAL_FAMILY_MESH &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < INSTANCE_COUNT)
        {
            state->latest_hover_query = query;
            state->has_hover_query = true;
            fprintf(stdout, "hover mesh instance id=%" PRIu64 "\n", query.resolved_id);
        }
        else
        {
            state->has_hover_query = false;
        }
    }
    if (saw_mesh_query && !state->has_hover_query)
        dvz_hover_clear(state->hover);

    if (state->cursor_valid)
    {
        DvzQueryRequest request = dvz_query_request();
        request.request_id = QUERY_ID;
        request.target = DVZ_SCENE_TARGET_ITEM;
        request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

        if (dvz_scenario_panel_query(state->panel, state->cursor_x, state->cursor_y, &request) != 0)
            fprintf(stderr, "dvz_scenario_panel_query() failed\n");
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

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.10f;
    camera_desc.eye[1] = -4.30f;
    camera_desc.eye[2] = 2.40f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.62f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
        goto error;

    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(ctx->scene, 0.32, face_roles, NULL);
    if (visual == NULL)
        goto error;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.24f;
    material.light_direction[1] = -0.48f;
    material.light_direction[2] = 0.84f;
    material.phong.ambient = 0.28f;
    material.phong.diffuse = 0.78f;
    material.phong.specular = 0.34f;
    material.phong.shininess = 42.0f;
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

    DvzMsaaDesc msaa_desc = dvz_msaa_desc();
    msaa_desc.sample_count = 8;
    msaa_desc.alpha_to_coverage = true;
    (void)dvz_panel_set_msaa(panel, &msaa_desc);

    DvzController* arcball_controller = dvz_arcball(ctx->scene, NULL);
    if (arcball_controller == NULL)
        goto error;
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    if (arcball == NULL)
        goto error;
    if (dvz_scenario_bind_controller(ctx, panel, arcball_controller, DVZ_DIM_MASK_XYZ) != 0)
        goto error;
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.20f, +0.26f});

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
static DvzScenarioSpec _selection_mesh_instances_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_selection_mesh_instances",
        .title = "selection_mesh_instances",
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
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _selection_mesh_instances_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
