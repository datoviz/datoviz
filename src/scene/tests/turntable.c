/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene turntable tests                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_controllers.h"
#include "_scene.h"
#include "controller_internal.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/turntable.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_turntable_create_default(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntable* turntable = _dvz_turntable(NULL);
    ANN(turntable);
    AC(turntable->pivot[0], 0.0f, 1e-5f);
    AC(turntable->pivot[1], 0.0f, 1e-5f);
    AC(turntable->pivot[2], 0.0f, 1e-5f);
    AC(turntable->eye[0], 0.0f, 1e-5f);
    AC(turntable->eye[1], 0.0f, 1e-5f);
    AC(turntable->eye[2], 3.0f, 1e-5f);
    AC(turntable->distance, 3.0f, 1e-5f);
    dvz_turntable_destroy(turntable);
    return 0;
}



int test_turntable_orbit_preserves_distance(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntable* turntable = _dvz_turntable(NULL);
    ANN(turntable);
    dvz_turntable_orbit(turntable, GLM_PI_2f, 0.0f);
    AC(turntable->distance, 3.0f, 1e-5f);
    AC(turntable->eye[0], -3.0f, 1e-5f);
    AC(turntable->eye[1], 0.0f, 1e-5f);
    AC(turntable->eye[2], 0.0f, 1e-4f);
    AT(turntable->pivot_marker_visible);
    dvz_turntable_destroy(turntable);
    return 0;
}



int test_turntable_z_up_orbit_uses_xy_plane(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntableDesc desc = dvz_turntable_desc();
    desc.initial_view.eye[0] = -3.0f;
    desc.initial_view.eye[1] = 0.0f;
    desc.initial_view.eye[2] = 0.0f;
    desc.initial_view.target[0] = 0.0f;
    desc.initial_view.target[1] = 0.0f;
    desc.initial_view.target[2] = 0.0f;
    desc.initial_view.up[0] = 0.0f;
    desc.initial_view.up[1] = 0.0f;
    desc.initial_view.up[2] = 1.0f;
    DvzTurntable* turntable = _dvz_turntable(&desc);
    ANN(turntable);
    AC(turntable->eye[0], -3.0f, 1e-5f);
    AC(turntable->eye[1], 0.0f, 1e-5f);
    AC(turntable->eye[2], 0.0f, 1e-5f);

    dvz_turntable_orbit(turntable, GLM_PI_2f, 0.0f);
    AC(turntable->distance, 3.0f, 1e-5f);
    AC(turntable->eye[0], 0.0f, 1e-4f);
    AC(turntable->eye[1], 3.0f, 1e-5f);
    AC(turntable->eye[2], 0.0f, 1e-5f);

    dvz_turntable_destroy(turntable);
    return 0;
}



int test_turntable_pivot_preserves_eye(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntable* turntable = _dvz_turntable(NULL);
    ANN(turntable);
    dvz_turntable_pivot(turntable, (vec3){1.0f, 0.0f, 3.0f});
    AC(turntable->eye[0], 0.0f, 1e-5f);
    AC(turntable->eye[1], 0.0f, 1e-5f);
    AC(turntable->eye[2], 3.0f, 1e-5f);
    AC(turntable->distance, 1.0f, 1e-5f);
    AC(turntable->yaw, 0.0f, 1e-5f);
    AC(turntable->pitch, 0.0f, 1e-5f);
    dvz_turntable_destroy(turntable);
    return 0;
}



int test_turntable_pan_moves_pivot_and_eye(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntable* turntable = _dvz_turntable(NULL);
    ANN(turntable);
    dvz_turntable_pan(turntable, 1.0f, 2.0f);
    AC(turntable->pivot[0], 1.0f, 1e-5f);
    AC(turntable->pivot[1], 2.0f, 1e-5f);
    AC(turntable->pivot[2], 0.0f, 1e-5f);
    AC(turntable->eye[0], 1.0f, 1e-5f);
    AC(turntable->eye[1], 2.0f, 1e-5f);
    AC(turntable->eye[2], 3.0f, 1e-5f);
    dvz_turntable_destroy(turntable);
    return 0;
}



int test_panel_turntable_getter(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    AT(panel->turntable == NULL);
    DvzController* controller = dvz_turntable(scene, NULL);
    ANN(controller);
    DvzTurntable* turntable = dvz_controller_turntable(controller);
    ANN(turntable);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);
    AT(panel->turntable == turntable);
    AT(panel->camera != NULL);
    AT(turntable->camera == NULL);

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    AC(mvp.view[3][2], -3.0f, 1e-4f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_turntable_pitch_and_distance_clamps(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntableDesc desc = dvz_turntable_desc();
    desc.min_pitch = -0.25f;
    desc.max_pitch = +0.25f;
    desc.min_distance = 2.0f;
    desc.max_distance = 4.0f;
    desc.controller_flags |= DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE;
    DvzTurntable* turntable = _dvz_turntable(&desc);
    ANN(turntable);

    dvz_turntable_orbit(turntable, 0.0f, 10.0f);
    AC(turntable->pitch, 0.25f, 1e-5f);
    dvz_turntable_orbit(turntable, 0.0f, -10.0f);
    AC(turntable->pitch, -0.25f, 1e-5f);

    dvz_turntable_dolly(turntable, -10.0f);
    AC(turntable->distance, 2.0f, 1e-5f);
    dvz_turntable_dolly(turntable, +10.0f);
    AC(turntable->distance, 4.0f, 1e-5f);

    dvz_turntable_destroy(turntable);
    return 0;
}



int test_turntable_double_click_resets(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntable* turntable = _dvz_turntable(NULL);
    ANN(turntable);
    dvz_turntable_orbit(turntable, 0.5f, 0.2f);
    dvz_turntable_dolly(turntable, -1.0f);

    DvzPointerEvent ev = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    AT(dvz_turntable_pointer(turntable, &ev));
    AC(turntable->distance, 3.0f, 1e-5f);
    AC(turntable->yaw, -GLM_PI_2f, 1e-5f);
    AC(turntable->pitch, 0.0f, 1e-5f);

    dvz_turntable_destroy(turntable);
    return 0;
}



int test_turntable_scene_binding_uses_panel_input(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    ANN(panel);

    DvzController* controller = dvz_turntable(scene, NULL);
    ANN(controller);
    DvzTurntable* turntable = dvz_controller_turntable(controller);
    ANN(turntable);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);
    AT(panel->camera != NULL);
    AT(turntable->camera == NULL);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(panel, router) == 0);

    DvzInputEvent outside = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_WHEEL,
                .content.w.dir = {0.0f, 1.0f},
                .pos = {600.0f, 200.0f},
            },
    };
    dvz_input_emit_event(router, &outside);
    AC(turntable->distance, 3.0f, 1e-5f);

    DvzInputEvent inside = outside;
    inside.content.pointer.pos[0] = 200.0f;
    dvz_input_emit_event(router, &inside);
    AT(turntable->distance < 3.0f);
    AT(turntable->camera == NULL);

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    AT(mvp.view[3][2] < -2.0f);

    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}



/*************************************************************************************************/
/*  Test suite                                                                                   */
/*************************************************************************************************/

int test_scene_turntable(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene,turntable";
    TST_MODULE(suite, "scene");
    TST_GROUP("turntable");
    TST_CASE(test_turntable_create_default);
    TST_CASE(test_turntable_orbit_preserves_distance);
    TST_CASE(test_turntable_z_up_orbit_uses_xy_plane);
    TST_CASE(test_turntable_pivot_preserves_eye);
    TST_CASE(test_turntable_pan_moves_pivot_and_eye);
    TST_CASE(test_panel_turntable_getter);
    TST_CASE(test_turntable_pitch_and_distance_clamps);
    TST_CASE(test_turntable_double_click_resets);
    TST_CASE(test_turntable_scene_binding_uses_panel_input);
    return 0;
}
