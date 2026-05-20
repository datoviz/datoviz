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

int test_fly_create_default(TstContext* suite, const TstCase* item)
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



int test_fly_lookat_initialization(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.position[0] = 1.0f;
    desc.position[1] = 2.0f;
    desc.position[2] = 3.0f;
    desc.target[0] = 1.0f;
    desc.target[1] = 2.0f;
    desc.target[2] = 2.0f;

    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);

    vec3 pos = {0};
    vec3 target = {0};
    dvz_fly_get_position(fly, pos);
    dvz_fly_get_target(fly, target);
    AC(pos[0], 1.0f, 1e-5f);
    AC(pos[1], 2.0f, 1e-5f);
    AC(pos[2], 3.0f, 1e-5f);
    AC(target[0], 1.0f, 1e-5f);
    AC(target[1], 2.0f, 1e-5f);
    AC(target[2], 2.0f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_pitch_clamp_and_reset(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.use_angles = true;
    desc.position[0] = 0.0f;
    desc.position[1] = 0.0f;
    desc.position[2] = 0.0f;
    desc.pitch = GLM_PIf;

    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);
    AT(fly->pitch < GLM_PI_2f);
    AT(fly->pitch > 0.0f);

    dvz_fly_move_forward(fly, 1.0f);
    dvz_fly_rotate(fly, 0.5f, -1.0f);
    dvz_fly_reset(fly);

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[0], 0.0f, 1e-5f);
    AC(pos[1], 0.0f, 1e-5f);
    AC(pos[2], 0.0f, 1e-5f);
    AC(fly->yaw, fly->yaw_init, 1e-5f);
    AC(fly->pitch, fly->pitch_init, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_free_and_plane_movement(TstContext* suite, const TstCase* item)
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



int test_fly_set_mode(TstContext* suite, const TstCase* item)
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
    desc.mode = DVZ_FLY_MODE_PLANE;

    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);
    dvz_fly_move_forward(fly, 1.0f);
    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[1], 0.0f, 1e-5f);

    dvz_fly_reset(fly);
    dvz_fly_set_mode(fly, DVZ_FLY_MODE_FREE);
    dvz_fly_move_forward(fly, 1.0f);
    dvz_fly_get_position(fly, pos);
    AT(pos[1] > 0.5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_keyboard_arrows_update(TstContext* suite, const TstCase* item)
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



int test_fly_wasd_and_arrows_equivalent(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* wasd = dvz_fly(&desc);
    DvzFly* arrows = dvz_fly(&desc);
    ANN(wasd);
    ANN(arrows);

    DvzKeyboardEvent press_w = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_W};
    DvzKeyboardEvent press_up = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_UP};
    AT(dvz_fly_keyboard(wasd, &press_w));
    AT(dvz_fly_keyboard(arrows, &press_up));
    dvz_fly_update(wasd, 0.5);
    dvz_fly_update(arrows, 0.5);

    vec3 pos_wasd = {0};
    vec3 pos_arrows = {0};
    dvz_fly_get_position(wasd, pos_wasd);
    dvz_fly_get_position(arrows, pos_arrows);
    AC(pos_wasd[0], pos_arrows[0], 1e-5f);
    AC(pos_wasd[1], pos_arrows[1], 1e-5f);
    AC(pos_wasd[2], pos_arrows[2], 1e-5f);

    DvzKeyboardEvent release_w = {.type = DVZ_KEYBOARD_EVENT_RELEASE, .key = DVZ_KEY_W};
    DvzKeyboardEvent release_up = {.type = DVZ_KEYBOARD_EVENT_RELEASE, .key = DVZ_KEY_UP};
    AT(dvz_fly_keyboard(wasd, &release_w));
    AT(dvz_fly_keyboard(arrows, &release_up));
    dvz_fly_update(wasd, 0.5);
    dvz_fly_update(arrows, 0.5);
    dvz_fly_get_position(wasd, pos_wasd);
    dvz_fly_get_position(arrows, pos_arrows);
    AC(pos_wasd[2], pos_arrows[2], 1e-5f);

    dvz_fly_destroy(wasd);
    dvz_fly_destroy(arrows);
    return 0;
}



int test_fly_shift_changes_speed(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 1.0f;
    desc.fast_multiplier = 4.0f;
    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);

    DvzKeyboardEvent press_shift = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_LEFT_SHIFT};
    DvzKeyboardEvent press_w = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_W};
    AT(dvz_fly_keyboard(fly, &press_shift));
    AT(dvz_fly_keyboard(fly, &press_w));
    dvz_fly_update(fly, 0.5);

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 1.0f, 1e-5f);

    DvzKeyboardEvent release_shift = {
        .type = DVZ_KEYBOARD_EVENT_RELEASE,
        .key = DVZ_KEY_LEFT_SHIFT,
    };
    AT(dvz_fly_keyboard(fly, &release_shift));
    dvz_fly_update(fly, 0.5);
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 0.5f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_left_drag_updates_view(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFly* fly = dvz_fly(NULL);
    ANN(fly);
    dvz_fly_resize(fly, 800.0f, 600.0f);

    vec3 before = {0};
    vec3 after = {0};
    dvz_fly_get_target(fly, before);

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_LEFT,
        .pos = {200.0f, 100.0f},
        .content.d.last_pos = {100.0f, 200.0f},
    };
    AT(dvz_fly_pointer(fly, &drag));
    dvz_fly_get_target(fly, after);
    AT(after[0] > before[0]);
    AT(after[1] > before[1]);
    AT(after[2] > before[2]);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_ctrl_and_space_use_same_vertical_speed(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);

    DvzKeyboardEvent press_space = {
        .type = DVZ_KEYBOARD_EVENT_PRESS,
        .key = DVZ_KEY_SPACE,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &press_space));
    dvz_fly_update(fly, 0.5);

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[1], 0.5f, 1e-5f);

    DvzKeyboardEvent release_space = {
        .type = DVZ_KEYBOARD_EVENT_RELEASE,
        .key = DVZ_KEY_SPACE,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &release_space));
    dvz_fly_reset(fly);

    DvzKeyboardEvent press_ctrl = {
        .type = DVZ_KEYBOARD_EVENT_PRESS,
        .key = DVZ_KEY_LEFT_CONTROL,
        .mods = 0,
    };
    AT(dvz_fly_keyboard(fly, &press_ctrl));
    dvz_fly_update(fly, 0.5);

    dvz_fly_get_position(fly, pos);
    AC(pos[1], -0.5f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_right_drag_moves_vertical_plane(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = dvz_fly(&desc);
    ANN(fly);
    dvz_fly_resize(fly, 800.0f, 600.0f);

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_RIGHT,
        .pos = {200.0f, 100.0f},
        .content.d.last_pos = {100.0f, 200.0f},
    };
    AT(dvz_fly_pointer(fly, &drag));

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[0], 0.5f, 1e-5f);
    AC(pos[1], 0.3333333f, 1e-5f);
    AC(pos[2], 3.0f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_pivot_preserves_eye_and_orbits(TstContext* suite, const TstCase* item)
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



int test_panel_fly_getter(TstContext* suite, const TstCase* item)
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



int test_figure_fly_update_advances_panel_camera(TstContext* suite, const TstCase* item)
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
    AC(pos[2], 2.8f, 1e-5f);

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    AC(mvp.view[3][2], -2.8f, 1e-4f);

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



int test_figure_fly_update_clamps_dt(TstContext* suite, const TstCase* item)
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
    desc.speed = 10.0f;
    DvzFly* fly = dvz_panel_set_fly(panel, NULL, &desc);
    ANN(fly);

    DvzKeyboardEvent press = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_W};
    AT(dvz_fly_keyboard(fly, &press));
    AT(_dvz_figure_fly_update(figure, 10.0));

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.0f, 1e-5f);

    dvz_scene_destroy(scene);
    return 0;
}



int test_fly_state_is_panel_scoped(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    ANN(left);
    ANN(right);

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* left_fly = dvz_panel_set_fly(left, NULL, &desc);
    DvzFly* right_fly = dvz_panel_set_fly(right, NULL, &desc);
    ANN(left_fly);
    ANN(right_fly);

    DvzKeyboardEvent press = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_W};
    AT(dvz_fly_keyboard(left_fly, &press));
    AT(_dvz_figure_fly_update(figure, 0.5));

    vec3 left_pos = {0};
    vec3 right_pos = {0};
    dvz_fly_get_position(left_fly, left_pos);
    dvz_fly_get_position(right_fly, right_pos);
    AC(left_pos[2], 2.8f, 1e-5f);
    AC(right_pos[2], 3.0f, 1e-5f);

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
    TST_MODULE(suite, "scene");
    TST_GROUP("fly");
    TST_CASE(test_fly_create_default);
    TST_CASE(test_fly_lookat_initialization);
    TST_CASE(test_fly_pitch_clamp_and_reset);
    TST_CASE(test_fly_free_and_plane_movement);
    TST_CASE(test_fly_set_mode);
    TST_CASE(test_fly_keyboard_arrows_update);
    TST_CASE(test_fly_wasd_and_arrows_equivalent);
    TST_CASE(test_fly_shift_changes_speed);
    TST_CASE(test_fly_left_drag_updates_view);
    TST_CASE(test_fly_ctrl_and_space_use_same_vertical_speed);
    TST_CASE(test_fly_right_drag_moves_vertical_plane);
    TST_CASE(test_fly_pivot_preserves_eye_and_orbits);
    TST_CASE(test_panel_fly_getter);
    TST_CASE(test_figure_fly_update_advances_panel_camera);
    TST_CASE(test_figure_fly_update_clamps_dt);
    TST_CASE(test_fly_state_is_panel_scoped);
    return 0;
}
