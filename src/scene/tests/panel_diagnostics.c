/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_scene.h"
#include "core/figure_emit_internal.h"
#include "core/panel_diagnostics.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Check opt-in panel diagnostics and panel-local camera mutation reporting without GPU execution.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
static int test_scene_panel_diagnostics_tracks_panel_work(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 640, 360, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(
        figure, &(DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 0.5f, .height = 1.0f});
    DvzPanel* right = dvz_panel(
        figure, &(DvzPanelDesc){.x = 0.5f, .y = 0.0f, .width = 0.5f, .height = 1.0f});
    ANN(left);
    ANN(right);
    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[2] = 4.0f;
    AT(dvz_panel_set_camera_desc(right, &camera_desc) == DVZ_OK);
    DvzCamera* camera = dvz_panel_camera(right);
    ANN(camera);
    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(right, controller, DVZ_DIM_MASK_XYZ) == DVZ_OK);

    const float left_position[3] = {-0.25f, 0.0f, 0.0f};
    const float right_position[3] = {0.25f, 0.0f, 0.0f};
    const DvzColor point_color = {220, 140, 80, 255};
    const float point_size = 8.0f;
    DvzVisual* left_point = dvz_point(scene, 0);
    DvzVisual* right_point = dvz_point(scene, 0);
    ANN(left_point);
    ANN(right_point);
    AT(dvz_visual_set_data(left_point, "position", left_position, 1) == DVZ_OK);
    AT(dvz_visual_set_data(left_point, "color", &point_color, 1) == DVZ_OK);
    AT(dvz_visual_set_data(left_point, "size", &point_size, 1) == DVZ_OK);
    AT(dvz_visual_set_data(right_point, "position", right_position, 1) == DVZ_OK);
    AT(dvz_visual_set_data(right_point, "color", &point_color, 1) == DVZ_OK);
    AT(dvz_visual_set_data(right_point, "size", &point_size, 1) == DVZ_OK);
    AT(dvz_panel_add_visual(left, left_point, NULL) == DVZ_OK);
    AT(dvz_panel_add_visual(right, right_point, NULL) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);

    figure->panel_diagnostics_enabled = false;
    DvzDrp2CommandStream* stream =
        _scene_figure_emit_stream_ex(figure, &caps, &report, NULL);
    ANN(stream);
    AT(figure->panel_diagnostics_generation == 0);
    AT(figure->panel_diagnostics[0].generation == 0);
    dvz_drp2_stream_destroy(stream);

    const float updated_left_position[3] = {-0.15f, 0.1f, 0.0f};
    AT(dvz_visual_set_data(left_point, "position", updated_left_position, 1) == DVZ_OK);
    figure->panel_diagnostics_enabled = true;
    stream = _scene_figure_emit_stream_ex(figure, &caps, &report, NULL);
    ANN(stream);
    AT(figure->panel_diagnostics_generation == 1);
    AT(figure->panel_diagnostics[0].generation == 1);
    AT(figure->panel_diagnostics[1].generation == 1);
    AT(figure->panel_diagnostics[0].panel_planning_succeeded);
    AT(figure->panel_diagnostics[1].panel_planning_succeeded);
    AT(figure->panel_diagnostics[0].stream_emitted);
    AT(figure->panel_diagnostics[1].stream_emitted);
    AT(figure->panel_diagnostics[0].planned_node_count > 0);
    AT(figure->panel_diagnostics[1].planned_node_count > 0);
    AT(figure->panel_diagnostics[0].draw_count > 0);
    AT(figure->panel_diagnostics[1].draw_count > 0);
    AT(figure->panel_diagnostics[0].prelude_upload_count > 0);
    AT(figure->panel_diagnostics[1].prelude_upload_count > 0);
    AT((figure->panel_diagnostics[1].change_mask &
        DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW) == 0);
    dvz_drp2_stream_destroy(stream);

    uint64_t descriptor_view_revision = right->view3d_revision;
    uint64_t previous_mvp_signature = figure->panel_diagnostics[1].observed_mvp_signature;
    AT(dvz_arcball_set(arcball, (vec3){0.55f, -0.25f, 0.15f}) == DVZ_OK);
    AT(right->view3d_revision == descriptor_view_revision);
    stream = _scene_figure_emit_stream_ex(figure, &caps, &report, NULL);
    ANN(stream);
    AT(figure->panel_diagnostics_generation == 2);
    AT((figure->panel_diagnostics[1].change_mask &
        DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW) != 0);
    AT(figure->panel_diagnostics[1].observed_view_revision == descriptor_view_revision);
    AT(figure->panel_diagnostics[1].observed_mvp_signature != previous_mvp_signature);
    dvz_drp2_stream_destroy(stream);

    DvzCameraView view = {
        .eye = {0.0f, 0.0f, 6.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
    };
    const float updated_right_position[3] = {0.35f, 0.1f, 0.0f};
    AT(dvz_visual_set_data(right_point, "position", updated_right_position, 1) == DVZ_OK);
    AT(dvz_camera_set_view(camera, &view) == DVZ_OK);
    stream = _scene_figure_emit_stream_ex(figure, &caps, &report, NULL);
    ANN(stream);
    AT(figure->panel_diagnostics_generation == 3);
    AT((figure->panel_diagnostics[1].change_mask &
        DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW) != 0);
    AT((figure->panel_diagnostics[0].change_mask &
        DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW) == 0);
    AT((figure->panel_diagnostics[0].change_mask &
        DVZ_PANEL_DIAGNOSTIC_CHANGE_UNKNOWN_FRAME) != 0);
    AT(figure->panel_diagnostics[0].observed_view_revision == 0);
    AT(figure->panel_diagnostics[1].observed_view_revision == right->view3d_revision);
    dvz_drp2_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Register CPU-only panel diagnostic regressions.
 *
 * @param suite test suite
 * @return zero on success
 */
int test_scene_panel_diagnostics(TstSuite* suite)
{
    const char* tags = "scene";
    TST_MODULE(suite, "scene");
    TST_GROUP("panel_diagnostics");
    TST_CASE(test_scene_panel_diagnostics_tracks_panel_work);
    return 0;
}
