/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene fly camera tests                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_assertions.h"
#include "../_scene.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/fly.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_fly_create_default(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzFly* fly = dvz_fly(NULL);
    ANN(fly);
    vec3 pos = {0};
    vec3 target = {0};
    vec3 up = {0};
    dvz_fly_get_position(fly, pos);
    dvz_fly_get_target(fly, target);
    dvz_fly_get_up(fly, up);
    AC(pos[0], 0.0f, 1e-5f);
    AC(pos[1], 0.0f, 1e-5f);
    AC(pos[2], 3.0f, 1e-5f);
    AC(target[0], 0.0f, 1e-5f);
    AC(target[1], 0.0f, 1e-5f);
    AC(target[2], 2.0f, 1e-5f);
    AC(up[0], 0.0f, 1e-5f);
    AC(up[1], 1.0f, 1e-5f);
    AC(up[2], 0.0f, 1e-5f);
    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_free_and_plane_movement(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.use_angles = true;
    desc.position[0] = 0.0f;
    desc.position[1] = 0.0f;
    desc.position[2] = 0.0f;
    desc.yaw = -GLM_PI_2f;
    desc.pitch = GLM_PI_4f;

    DvzFly* free_fly = dvz_fly(&desc);
    ANN(free_fly);
    dvz_fly_move_forward(free_fly, 1.0f);
    vec3 free_pos = {0};
    dvz_fly_get_position(free_fly, free_pos);
    AT(free_pos[1] > 0.5f);
    AT(free_pos[2] < -0.5f);

    desc.mode = DVZ_FLY_MODE_PLANE;
    DvzFly* plane_fly = dvz_fly(&desc);
    ANN(plane_fly);
    dvz_fly_move_forward(plane_fly, 1.0f);
    vec3 plane_pos = {0};
    dvz_fly_get_position(plane_fly, plane_pos);
    AC(plane_pos[1], 0.0f, 1e-5f);
    AC(plane_pos[2], -1.0f, 1e-5f);

    dvz_fly_destroy(free_fly);
    dvz_fly_destroy(plane_fly);
    return 0;
}



int test_fly_keyboard_arrows_update(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);

    DvzKeyboardEvent press = {
        .type = DVZ_KEYBOARD_EVENT_PRESS,
        .key = DVZ_KEY_UP,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &press));
    dvz_fly_update(fly, 0.5);

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.0f, 1e-5f);

    DvzKeyboardEvent release = {
        .type = DVZ_KEYBOARD_EVENT_RELEASE,
        .key = DVZ_KEY_UP,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &release));
    dvz_fly_update(fly, 0.5);
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.0f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_pivot_preserves_eye_and_orbits(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzFly* fly = dvz_fly(NULL);
    ANN(fly);
    dvz_fly_pivot(fly, (vec3){1.0f, 0.0f, 3.0f});
    AT(dvz_fly_has_pivot(fly));
    AC(fly->pivot_distance, 1.0f, 1e-5f);
    AT(fly->pivot_marker_visible);

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[0], 0.0f, 1e-5f);
    AC(pos[1], 0.0f, 1e-5f);
    AC(pos[2], 3.0f, 1e-5f);

    AT(dvz_fly_orbit(fly, GLM_PI_2f, 0.0f));
    dvz_fly_get_position(fly, pos);
    AC(pos[0], 1.0f, 1e-5f);
    AC(pos[1], 0.0f, 1e-5f);
    AC(pos[2], 2.0f, 1e-5f);

    dvz_fly_clear_pivot(fly);
    AT(!dvz_fly_has_pivot(fly));
    AT(!fly->pivot_marker_visible);
    dvz_fly_destroy(fly);
    return 0;
}



int test_panel_fly_getter(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    AT(dvz_panel_fly(panel) == NULL);
    DvzFly* fly = dvz_panel_set_fly(panel, NULL, NULL);
    ANN(fly);
    AT(dvz_panel_fly(panel) == fly);
    AT(panel->camera != NULL);
    AT(fly->camera == panel->camera);

    dvz_scene_destroy(scene);
    return 0;
}



int test_figure_fly_update_advances_panel_camera(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = dvz_panel_set_fly(panel, NULL, &desc);
    ANN(fly);

    DvzKeyboardEvent press = {
        .type = DVZ_KEYBOARD_EVENT_PRESS,
        .key = DVZ_KEY_W,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &press));
    AT(_dvz_figure_fly_update(figure, 0.5));

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.0f, 1e-5f);

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    AC(mvp.view[3][2], -2.0f, 1e-4f);

    DvzKeyboardEvent release = {
        .type = DVZ_KEYBOARD_EVENT_RELEASE,
        .key = DVZ_KEY_W,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &release));
    AT(!_dvz_figure_fly_update(figure, 0.5));

    dvz_scene_destroy(scene);
    return 0;
}



/*************************************************************************************************/
/*  Test suite                                                                                   */
/*************************************************************************************************/

int test_scene_fly(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene,fly";
    TEST_SIMPLE(test_fly_create_default);
    TEST_SIMPLE(test_fly_free_and_plane_movement);
    TEST_SIMPLE(test_fly_keyboard_arrows_update);
    TEST_SIMPLE(test_fly_pivot_preserves_eye_and_orbits);
    TEST_SIMPLE(test_panel_fly_getter);
    TEST_SIMPLE(test_figure_fly_update_advances_panel_camera);
    return 0;
}
