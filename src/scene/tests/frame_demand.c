/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene frame demand tests                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_scene.h"
#include "controller_internal.h"
#include "core/frame_demand_internal.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_figure_frame_demand_panzoom(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);

    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);
    DvzPointerEvent move = {
        .type = DVZ_POINTER_EVENT_MOVE,
        .pos = {400.0f, 300.0f},
        .window_size = {800.0f, 600.0f},
    };
    AT(!dvz_panzoom_pointer(panzoom, &move));
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .content.d = {
            .press_pos = {400.0f, 300.0f},
            .last_pos = {400.0f, 300.0f},
            .shift = {10.0f, 5.0f},
            .is_press_valid = true,
        },
        .pos = {410.0f, 305.0f},
        .window_size = {800.0f, 600.0f},
        .button = DVZ_POINTER_BUTTON_LEFT,
    };
    AT(dvz_panzoom_pointer(panzoom, &drag));
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_INTERACTION);

    DvzPointerEvent stop = drag;
    stop.type = DVZ_POINTER_EVENT_DRAG_STOP;
    AT(dvz_panzoom_pointer(panzoom, &stop));
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);

    dvz_scene_destroy(scene);
    return 0;
}



int test_figure_frame_demand_controller_families(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    ANN(arcball_controller);
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0);
    arcball->interacting = true;
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_INTERACTION);
    arcball->interacting = false;
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);

    DvzController* fly_controller = dvz_fly(scene, NULL);
    ANN(fly_controller);
    DvzFly* fly = dvz_controller_fly(fly_controller);
    ANN(fly);
    AT(dvz_panel_bind_controller(panel, fly_controller, DVZ_DIM_MASK_XYZ) == 0);
    fly->key_forward = true;
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_INTERACTION);
    fly->key_forward = false;
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);

    DvzController* turntable_controller = dvz_turntable(scene, NULL);
    ANN(turntable_controller);
    DvzTurntable* turntable = dvz_controller_turntable(turntable_controller);
    ANN(turntable);
    AT(dvz_panel_bind_controller(panel, turntable_controller, DVZ_DIM_MASK_XYZ) == 0);
    turntable->interacting = true;
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_INTERACTION);
    turntable->interacting = false;
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);

    dvz_scene_destroy(scene);
    return 0;
}



int test_figure_frame_demand_ignores_unbound_controller(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    ANN(panel);
    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    panzoom->interacting = true;

    AT(panel->panzoom == NULL);
    AT(_scene_figure_frame_demand(figure) == DVZ_FRAME_DEMAND_NONE);

    dvz_scene_destroy(scene);
    return 0;
}



int test_scene_frame_demand(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";
    TST_MODULE(suite, "scene");
    TST_GROUP("frame-demand");
    TST_CASE(test_figure_frame_demand_panzoom);
    TST_CASE(test_figure_frame_demand_controller_families);
    TST_CASE(test_figure_frame_demand_ignores_unbound_controller);
    return 0;
}
