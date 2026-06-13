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
#include "_controllers.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/fly.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create and bind one scene-owned fly controller for tests.
 *
 * @param panel the panel
 * @param router optional input router
 * @param desc optional fly descriptor
 * @param out_controller optional output controller handle
 * @return the fly payload, or NULL
 */
static DvzFly* _test_panel_bind_fly(
    DvzPanel* panel, DvzInputRouter* router, const DvzFlyDesc* desc,
    DvzController** out_controller)
{
    ANN(panel);
    ANN(panel->figure);
    ANN(panel->figure->scene);
    DvzController* controller = dvz_fly(panel->figure->scene, desc);
    if (out_controller != NULL)
        *out_controller = controller;
    if (controller == NULL)
        return NULL;
    DvzFly* fly = dvz_controller_fly(controller);
    if (fly == NULL || dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    if (router != NULL && dvz_panel_connect_input(panel, router) != 0)
        return NULL;
    return fly;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_fly_create_default(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFly* fly = _dvz_fly(NULL);
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

    DvzFly* fly = _dvz_fly(&desc);
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

    DvzFly* fly = _dvz_fly(&desc);
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

    DvzFly* free_fly = _dvz_fly(&desc);
    ANN(free_fly);
    dvz_fly_move_forward(free_fly, 1.0f);
    vec3 free_pos = {0};
    dvz_fly_get_position(free_fly, free_pos);
    AT(free_pos[1] > 0.5f);
    AT(free_pos[2] < -0.5f);

    desc.mode = DVZ_FLY_MODE_PLANE;
    DvzFly* plane_fly = _dvz_fly(&desc);
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

    DvzFly* fly = _dvz_fly(&desc);
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
    DvzFly* fly = _dvz_fly(&desc);
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
    DvzFly* wasd = _dvz_fly(&desc);
    DvzFly* arrows = _dvz_fly(&desc);
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
    DvzFly* fly = _dvz_fly(&desc);
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

    DvzFly* fly = _dvz_fly(NULL);
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



int test_fly_wheel_uses_calm_default_speed(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFly* fly = _dvz_fly(NULL);
    ANN(fly);

    DvzPointerEvent wheel = {
        .type = DVZ_POINTER_EVENT_WHEEL,
        .pos = {100.0f, 100.0f},
        .content.w.dir = {0.0f, 1.0f},
    };
    AT(dvz_fly_pointer(fly, &wheel));

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.96f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_router_keyboard_updates_key_state(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzInputRouter* router = dvz_input_router();
    ANN(router);

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = _test_panel_bind_fly(panel, router, &desc, NULL);
    ANN(fly);

    DvzKeyboardEvent press = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_W};
    dvz_input_emit_keyboard(router, &press);
    AT(_dvz_figure_fly_update(figure, 0.5));

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.8f, 1e-5f);

    DvzKeyboardEvent release = {.type = DVZ_KEYBOARD_EVENT_RELEASE, .key = DVZ_KEY_W};
    dvz_input_emit_keyboard(router, &release);
    AT(!_dvz_figure_fly_update(figure, 0.5));

    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}



int test_fly_ctrl_and_space_use_same_vertical_speed(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = _dvz_fly(&desc);
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
    AC(pos[1], 0.75f, 1e-5f);

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
    AC(pos[1], -0.75f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_right_drag_moves_vertical_plane(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.speed = 2.0f;
    DvzFly* fly = _dvz_fly(&desc);
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
    AC(pos[1], 0.5f, 1e-5f);
    AC(pos[2], 3.0f, 1e-5f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_fly_pivot_preserves_eye_and_orbits(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFly* fly = _dvz_fly(NULL);
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



int test_fly_pivot_marker_visual_tracks_visibility(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzFly* fly = _test_panel_bind_fly(panel, NULL, NULL, NULL);
    ANN(fly);
    AT(panel->fly_pivot_marker_visual == NULL);

    dvz_fly_pivot(fly, (vec3){1.0f, 0.0f, 3.0f});
    AT(_dvz_figure_fly_update(figure, 0.1));
    ANN(panel->fly_pivot_marker_visual);
    AT(panel->fly_pivot_marker_visual->visible);
    AT(panel->fly_pivot_marker_visual->type == DVZ_VISUAL_TYPE_POINT);
    int pos_attr = _attr_index(panel->fly_pivot_marker_visual, "position");
    AT(pos_attr >= 0);
    uint32_t pos_index = (uint32_t)pos_attr;
    AT(panel->fly_pivot_marker_visual->attrs[pos_index].item_count == 1);

    for (uint32_t i = 0; i < 12; i++)
        (void)_dvz_figure_fly_update(figure, 0.2);
    AT(!panel->fly_pivot_marker_visual->visible);

    dvz_scene_destroy(scene);
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

    AT(panel->fly == NULL);
    DvzFly* fly = _test_panel_bind_fly(panel, NULL, NULL, NULL);
    ANN(fly);
    AT(panel->fly == fly);
    AT(panel->camera != NULL);
    AT(fly->camera == NULL);

    dvz_scene_destroy(scene);
    return 0;
}



int test_fly_scene_controller_binding(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzController* controller = dvz_fly(scene, NULL);
    ANN(controller);
    AT(dvz_controller_type(controller) == DVZ_CONTROLLER_TYPE_FLY);
    DvzFly* fly = dvz_controller_fly(controller);
    ANN(fly);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);
    AT(dvz_panel_controller(panel, DVZ_DIM_X) == controller);
    AT(dvz_panel_controller(panel, DVZ_DIM_Y) == controller);
    AT(dvz_panel_controller(panel, DVZ_DIM_Z) == controller);
    AT(panel->fly == fly);
    AT(panel->camera != NULL);

    dvz_scene_destroy(scene);
    return 0;
}



int test_fly_controller_rejects_partial_dims(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzController* controller = dvz_fly(scene, NULL);
    ANN(controller);

    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_X) != 0);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) != 0);
    AT(dvz_panel_controller(panel, DVZ_DIM_X) == NULL);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    dvz_scene_destroy(scene);
    return 0;
}



int test_fly_controller_survives_panel_destroy(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzController* controller = dvz_fly(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    dvz_panel_destroy(panel);
    AT(dvz_controller_type(controller) == DVZ_CONTROLLER_TYPE_FLY);
    DvzFly* fly = dvz_controller_fly(controller);
    ANN(fly);

    dvz_scene_destroy(scene);
    return 0;
}



int test_shared_fly_updates_once_for_two_panels(TstContext* suite, const TstCase* item)
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
    DvzController* controller = dvz_fly(scene, &desc);
    ANN(controller);
    DvzFly* fly = dvz_controller_fly(controller);
    ANN(fly);
    AT(dvz_panel_bind_controller(left, controller, DVZ_DIM_MASK_XYZ) == 0);
    AT(dvz_panel_bind_controller(right, controller, DVZ_DIM_MASK_XYZ) == 0);

    DvzKeyboardEvent press = {.type = DVZ_KEYBOARD_EVENT_PRESS, .key = DVZ_KEY_W};
    AT(dvz_fly_keyboard(fly, &press));
    AT(_dvz_figure_fly_update(figure, 0.5));

    vec3 pos = {0};
    dvz_fly_get_position(fly, pos);
    AC(pos[2], 2.8f, 1e-5f);

    DvzMVP left_mvp = {0};
    DvzMVP right_mvp = {0};
    _scene_panel_apply_mvp(left, &left_mvp);
    _scene_panel_apply_mvp(right, &right_mvp);
    AC(left_mvp.view[3][2], -2.8f, 1e-4f);
    AC(right_mvp.view[3][2], -2.8f, 1e-4f);

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
    DvzFly* fly = _test_panel_bind_fly(panel, NULL, &desc, NULL);
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
    DvzFly* fly = _test_panel_bind_fly(panel, NULL, &desc, NULL);
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
    DvzFly* left_fly = _test_panel_bind_fly(left, NULL, &desc, NULL);
    DvzFly* right_fly = _test_panel_bind_fly(right, NULL, &desc, NULL);
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
    TST_CASE(test_fly_wheel_uses_calm_default_speed);
    TST_CASE(test_fly_router_keyboard_updates_key_state);
    TST_CASE(test_fly_ctrl_and_space_use_same_vertical_speed);
    TST_CASE(test_fly_right_drag_moves_vertical_plane);
    TST_CASE(test_fly_pivot_preserves_eye_and_orbits);
    TST_CASE(test_fly_pivot_marker_visual_tracks_visibility);
    TST_CASE(test_panel_fly_getter);
    TST_CASE(test_fly_scene_controller_binding);
    TST_CASE(test_fly_controller_rejects_partial_dims);
    TST_CASE(test_fly_controller_survives_panel_destroy);
    TST_CASE(test_shared_fly_updates_once_for_two_panels);
    TST_CASE(test_figure_fly_update_advances_panel_camera);
    TST_CASE(test_figure_fly_update_clamps_dt);
    TST_CASE(test_fly_state_is_panel_scoped);
    return 0;
}
