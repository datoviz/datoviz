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
#include "../_scene.h"
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

    AT(dvz_panel_turntable(panel) == NULL);
    DvzTurntable* turntable = dvz_panel_set_turntable(panel, NULL, NULL);
    ANN(turntable);
    AT(dvz_panel_turntable(panel) == turntable);
    AT(panel->camera != NULL);
    AT(turntable->camera == panel->camera);

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    AC(mvp.view[3][2], -3.0f, 1e-4f);

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
    TST_CASE(test_turntable_pivot_preserves_eye);
    TST_CASE(test_turntable_pan_moves_pivot_and_eye);
    TST_CASE(test_panel_turntable_getter);
    return 0;
}
