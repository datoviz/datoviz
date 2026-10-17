/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_assertions.h"
#include "_scene.h"
#include "controller_internal.h"
#include "core/pan_anchor_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/geom.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Helpers and functions                                                                        */
/*************************************************************************************************/

/**
 * Exercise private depth completion and queue isolation through ordinary panel pointer dispatch.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
static int test_scene_navigation_pan_queue_and_completion(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    for (uint32_t scenario = 0; scenario < 3; scenario++)
    {
        DvzScene* scene = dvz_scene();
        ANN(scene);
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        const bool offset_panel = scenario == 1;
        const float scale = offset_panel ? 2.0f : 1.0f;
        const float width = offset_panel ? 400.0f : 800.0f;
        const float height = offset_panel ? 360.0f : 600.0f;
        DvzPanel* panel = offset_panel
                              ? dvz_panel(figure, &(DvzPanelDesc){0.25f, 0.1f, 0.5f, 0.6f})
                              : dvz_panel_full(figure);
        vec2 press_window = {offset_panel ? 295.0f : 400.0f,
                             offset_panel ? 120.0f : 300.0f};
        DvzCameraDesc camera = dvz_camera_desc();
        glm_vec3_copy((vec3){0, 0, 3}, camera.view.eye);
        glm_vec3_zero(camera.view.target);
        camera.projection.fov_y = 0.72f;
        camera.projection.near_clip = 0.01f;
        camera.projection.far_clip = 100;
        AT(dvz_panel_set_camera_desc(panel, &camera) == DVZ_OK);
        DvzController* controller = dvz_arcball(scene, NULL);
        DvzArcball* arcball = dvz_controller_arcball(controller);
        ANN(arcball);
        AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == DVZ_OK);
        DvzInputRouter* router = dvz_input_router();
        ANN(router);
        AT(dvz_panel_connect_input(panel, router) == DVZ_OK);
        DvzVisual* mesh = dvz_mesh(scene, 0);
        ANN(mesh);
        // Context meshes can supply navigation depth while remaining scientifically unpickable.
        AT(dvz_visual_set_query_capabilities(mesh, 0) == DVZ_OK);
        AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
        const float dolly[] = {0, 2.0f, 3.1f};
        const float surface_z[] = {0.5f, 0.5f, -0.5f};
        AT(dvz_arcball_zoom(arcball, expf(dolly[scenario])) == DVZ_OK);
        AT(dvz_panel_query_px(panel, 50, 60, NULL) == DVZ_OK);
        DvzInputEvent event = {.type = DVZ_INPUT_EVENT_POINTER};
        DvzPointerEvent* pointer = &event.content.pointer;
        pointer->type = DVZ_POINTER_EVENT_PRESS;
        pointer->button = DVZ_POINTER_BUTTON_RIGHT;
        glm_vec2_copy(press_window, pointer->pos);
        glm_vec2_copy((vec2){800 / scale, 600 / scale}, pointer->window_size);
        pointer->content_scale = scale;
        dvz_input_emit_event(router, &event);
        AT(!arcball->pan_depth_pending);
        AT(scene->pending_query_count == 1);
        // Hover emphasis may change query freshness after PRESS, before the actual drag.
        _scene_notify_query_input_changed(figure);
        pointer->type = DVZ_POINTER_EVENT_DRAG_START;
        glm_vec2_copy(press_window, pointer->content.d.press_pos);
        pointer->content.d.is_press_valid = true;
        // On the offset HiDPI panel, the threshold move crosses its right edge.
        pointer->pos[0] += 14;
        dvz_input_emit_event(router, &event);
        AT(arcball->pan_depth_pending);
        AT(scene->pending_query_count == 2);
        AT(scene->pending_queries[1].navigation_depth);
        AC(scene->pending_queries[1].x, offset_panel ? 390 : 400, 1e-6);
        AC(scene->pending_queries[1].y, offset_panel ? 180 : 300, 1e-6);
        DvzPendingQueryRequest pending = scene->pending_queries[1];
        AT(_scene_arcball_pan_anchor_valid(&pending));
        // Public latest-wins id zero cannot evict the independent private navigation purpose.
        AT(dvz_panel_query_px(panel, 70, 80, NULL) == DVZ_OK);
        AT(scene->pending_query_count == 2);
        AT(scene->pending_queries[0].navigation_depth);
        pointer->type = DVZ_POINTER_EVENT_DRAG;
        glm_vec2_copy(press_window, pointer->content.d.press_pos);
        glm_vec2_copy((vec2){40 / scale, -30 / scale}, pointer->content.d.shift);
        glm_vec2_add(press_window, pointer->content.d.shift, pointer->pos);
        pointer->content.d.is_press_valid = true;
        dvz_input_emit_event(router, &event);
        AT(_scene_arcball_pan_anchor_valid(&pending));
        AC(arcball->pan[0], 0, 1e-6f);
        AC(arcball->pan[1], 0, 1e-6f);
        if (scenario == 1)
        {
            pointer->type = DVZ_POINTER_EVENT_DRAG_STOP;
            dvz_input_emit_event(router, &event);
            AT(arcball->pan_depth_pending);
        }
        DvzMVP before = pending.navigation_mvp;
        vec4 eye = {0}, clip = {0};
        glm_mat4_mulv(before.view, (vec4){0, 0, surface_z[scenario], 1}, eye);
        glm_mat4_mulv(before.proj, eye, clip);
        // Synthetic completion exercises conversion of the actual GPU depth convention.
        DvzQueryResult result = {
            .status = DVZ_QUERY_STATUS_HIT, .hit = true, .has_depth = true,
            .depth = 0.5 * (clip[2] / clip[3] + 1),
        };
        _scene_arcball_pan_anchor_finish(&pending, &result);
        AT(!arcball->pan_depth_pending);
        AT(scene->query_result_count == 0);
        DvzMVP after = {0};
        _scene_panel_apply_mvp(panel, &after);
        vec4 after_eye = {0}, after_clip = {0};
        glm_mat4_mulv(after.view, (vec4){0, 0, surface_z[scenario], 1}, after_eye);
        glm_mat4_mulv(after.proj, after_eye, after_clip);
        AC((after_clip[0] / after_clip[3] - clip[0] / clip[3]) * width / 2, 40, 0.02f);
        AC((after_clip[1] / after_clip[3] - clip[1] / clip[3]) * height / 2, 30, 0.02f);
        if (scenario == 1)
        {
            AC(arcball->pan_center[0], arcball->pan[0], 1e-6f);
            AC(arcball->pan_center[1], arcball->pan[1], 1e-6f);
        }
        dvz_scene_destroy(scene);
        dvz_input_router_destroy(router);
    }
    return 0;
}



/**
 * Reject old gesture epochs, geometry revisions and destroyed controller identities.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
static int test_scene_navigation_pan_rejects_stale_completion(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    DvzController* controller = dvz_arcball(scene, NULL);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == DVZ_OK);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
    DvzPointerEvent press = {
        .type = DVZ_POINTER_EVENT_PRESS, .button = DVZ_POINTER_BUTTON_MIDDLE,
        .pos = {400, 300},
    };
    press.type = DVZ_POINTER_EVENT_PRESS;
    AT(dvz_arcball_pointer(arcball, &press));
    press.type = DVZ_POINTER_EVENT_DRAG_START;
    AT(dvz_arcball_pointer(arcball, &press));
    _scene_arcball_pan_anchor_begin(panel, controller, &press);
    AT(scene->pending_query_count == 1);
    DvzPendingQueryRequest old = scene->pending_queries[0];
    press.type = DVZ_POINTER_EVENT_PRESS;
    AT(dvz_arcball_pointer(arcball, &press));
    press.type = DVZ_POINTER_EVENT_DRAG_START;
    AT(dvz_arcball_pointer(arcball, &press));
    _scene_arcball_pan_anchor_begin(panel, controller, &press);
    AT(scene->pending_query_count == 1);
    DvzPendingQueryRequest current = scene->pending_queries[0];
    AT(current.navigation_pan_epoch != old.navigation_pan_epoch);
    _scene_arcball_pan_anchor_finish(&old, NULL);
    AT(arcball->pan_depth_pending);
    _scene_notify_query_input_changed(figure);
    AT(!_scene_arcball_pan_anchor_valid(&current));
    _scene_arcball_pan_anchor_finish(&current, NULL);
    AT(!arcball->pan_depth_pending);
    AC(arcball->pan[0], 0, 1e-6f);
    AC(arcball->pan[1], 0, 1e-6f);
    press.type = DVZ_POINTER_EVENT_PRESS;
    AT(dvz_arcball_pointer(arcball, &press));
    press.type = DVZ_POINTER_EVENT_DRAG_START;
    AT(dvz_arcball_pointer(arcball, &press));
    _scene_arcball_pan_anchor_begin(panel, controller, &press);
    current = scene->pending_queries[0];
    dvz_controller_destroy(controller);
    DvzController* replacement = dvz_arcball(scene, NULL);
    AT(dvz_panel_bind_controller(panel, replacement, DVZ_DIM_MASK_XYZ) == DVZ_OK);
    AT(!_scene_arcball_pan_anchor_valid(&current));
    _scene_arcball_pan_anchor_finish(&current, NULL);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Reclaim navigation requests after their panel, figure or controller binding disappears.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
static int test_scene_navigation_pan_reclaims_dead_source(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    for (uint32_t scenario = 0; scenario < 3; scenario++)
    {
        DvzScene* scene = dvz_scene();
        ANN(scene);
        DvzFigure* source_figure = dvz_figure(scene, 800, 600, 0);
        DvzFigure* surviving_figure = dvz_figure(scene, 200, 100, 0);
        DvzPanel* panel = dvz_panel_full(source_figure);
        DvzController* controller = dvz_arcball(scene, NULL);
        DvzArcball* arcball = dvz_controller_arcball(controller);
        ANN(arcball);
        AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == DVZ_OK);
        DvzVisual* mesh = dvz_mesh(scene, 0);
        AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
        DvzPointerEvent start = {
            .type = DVZ_POINTER_EVENT_DRAG_START, .button = DVZ_POINTER_BUTTON_RIGHT,
            .pos = {400, 300},
        };
        AT(dvz_arcball_pointer(arcball, &start));
        _scene_arcball_pan_anchor_begin(panel, controller, &start);
        AT(arcball->pan_depth_pending);
        AT(scene->pending_query_count == 1);
        if (scenario == 0)
            dvz_panel_destroy(panel);
        else if (scenario == 1)
        {
            DvzController* replacement = dvz_arcball(scene, NULL);
            AT(dvz_panel_bind_controller(panel, replacement, DVZ_DIM_MASK_XYZ) == DVZ_OK);
        }
        else
            dvz_figure_destroy(source_figure);
        // Processing another figure must free the dead queue slot and cancel its live payload.
        AT(dvz_figure_process_queries(surviving_figure, NULL, NULL) == 1);
        AT(scene->pending_query_count == 0);
        AT(scene->query_result_count == 0);
        AT(!arcball->pan_depth_pending);
        AT(!arcball->pan_plane_active);
        AC(arcball->pan[0], 0, 1e-6f);
        AC(arcball->pan[1], 0, 1e-6f);
        dvz_scene_destroy(scene);
    }
    return 0;
}



/**
 * Register private navigation-depth queue regressions.
 *
 * @param suite test suite
 * @return zero on success
 */
int test_scene_navigation_pan(TstSuite* suite)
{
    const char* tags = "scene";
    TST_MODULE(suite, "scene");
    TST_GROUP("navigation_pan");
    TST_CASE(test_scene_navigation_pan_queue_and_completion);
    TST_CASE(test_scene_navigation_pan_rejects_stale_completion);
    TST_CASE(test_scene_navigation_pan_reclaims_dead_source);
    return 0;
}
