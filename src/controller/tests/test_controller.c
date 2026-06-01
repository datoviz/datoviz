/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing controller                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "datoviz/controller.h"
#include "test_controller.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_controller_panzoom_create(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoomDesc desc = dvz_panzoom_desc();
    AT(desc.struct_size == DVZ_STRUCT_SIZE(DvzPanzoomDesc));
    desc.width = 640.0f;
    desc.height = 480.0f;

    DvzPanzoom* panzoom = dvz_panzoom_create(&desc);
    ANN(panzoom);
    AT(panzoom->viewport_size[0] == 640.0f);
    AT(panzoom->viewport_size[1] == 480.0f);
    AT(panzoom->zoom[0] == 1.0f);
    AT(panzoom->zoom[1] == 1.0f);

    dvz_panzoom_destroy(panzoom);
    return 0;
}



int test_controller_arcball_create(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcballDesc desc = dvz_arcball_desc();
    AT(desc.struct_size == DVZ_STRUCT_SIZE(DvzArcballDesc));
    desc.width = 640.0f;
    desc.height = 480.0f;

    DvzArcball* arcball = dvz_arcball_create(&desc);
    ANN(arcball);
    AT(arcball->viewport_size[0] == 640.0f);
    AT(arcball->viewport_size[1] == 480.0f);
    AT(arcball->zoom == 1.0f);

    dvz_arcball_destroy(arcball);
    return 0;
}



int test_controller_camera_create(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzCameraDesc desc = dvz_camera_desc();
    AT(desc.struct_size == DVZ_STRUCT_SIZE(DvzCameraDesc));
    DvzCamera* camera = dvz_camera_create(&desc);
    ANN(camera);

    DvzMVP mvp = {0};
    dvz_camera_resize(camera, 640.0f, 480.0f);
    dvz_camera_mvp(camera, &mvp);
    AT(mvp.view[3][2] != 0.0f);
    AT(mvp.proj[0][0] != 0.0f);

    dvz_camera_destroy(camera);
    return 0;
}



int test_controller_fly_create(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    AT(desc.struct_size == DVZ_STRUCT_SIZE(DvzFlyDesc));
    DvzFly* fly = dvz_fly_create(&desc);
    ANN(fly);

    vec3 position = {0};
    vec3 target = {0};
    vec3 up = {0};
    dvz_fly_get_position(fly, position);
    dvz_fly_get_target(fly, target);
    dvz_fly_get_up(fly, up);

    AT(position[2] == 3.0f);
    AT(target[2] == 2.0f);
    AT(up[1] == 1.0f);

    dvz_fly_destroy(fly);
    return 0;
}



int test_controller_turntable_create(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzTurntableDesc desc = dvz_turntable_desc();
    AT(desc.struct_size == DVZ_STRUCT_SIZE(DvzTurntableDesc));
    DvzTurntable* turntable = dvz_turntable_create(&desc);
    ANN(turntable);
    AT(turntable->distance > 0.0f);
    AT(turntable->up[1] == 1.0f);

    dvz_turntable_destroy(turntable);
    return 0;
}



int test_controller_desc_abi_rejects_invalid_structs(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoomDesc panzoom = dvz_panzoom_desc();
    panzoom.struct_size = 0;
    AT(dvz_panzoom_create(&panzoom) == NULL);
    panzoom = dvz_panzoom_desc();
    panzoom.flags = 1;
    AT(dvz_panzoom_create(&panzoom) == NULL);

    DvzArcballDesc arcball = dvz_arcball_desc();
    arcball.struct_size = 0;
    AT(dvz_arcball_create(&arcball) == NULL);
    arcball = dvz_arcball_desc();
    arcball.flags = 1;
    AT(dvz_arcball_create(&arcball) == NULL);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.struct_size = 0;
    AT(dvz_camera_create(&camera) == NULL);
    camera = dvz_camera_desc();
    camera.flags = 1;
    AT(dvz_camera_create(&camera) == NULL);

    DvzFlyDesc fly = dvz_fly_desc();
    fly.struct_size = 0;
    AT(dvz_fly_create(&fly) == NULL);
    fly = dvz_fly_desc();
    fly.flags = 1;
    AT(dvz_fly_create(&fly) == NULL);

    DvzTurntableDesc turntable = dvz_turntable_desc();
    turntable.struct_size = 0;
    AT(dvz_turntable_create(&turntable) == NULL);
    turntable = dvz_turntable_desc();
    turntable.flags = 1;
    AT(dvz_turntable_create(&turntable) == NULL);

    DvzOrbitCameraDesc orbit = dvz_orbit_camera_desc();
    orbit.struct_size = 0;
    AT(dvz_orbit_camera_create(&orbit) == NULL);
    orbit = dvz_orbit_camera_desc();
    orbit.flags = 1;
    AT(dvz_orbit_camera_create(&orbit) == NULL);
    return 0;
}



/**
 * Register the controller module tests.
 *
 * @param suite test suite
 * @return zero on success
 */
int test_controller(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "controller";
    TST_MODULE(suite, tags);
    TST_CASE(test_controller_panzoom_create);
    TST_CASE(test_controller_arcball_create);
    TST_CASE(test_controller_camera_create);
    TST_CASE(test_controller_fly_create);
    TST_CASE(test_controller_turntable_create);
    TST_CASE(test_controller_desc_abi_rejects_invalid_structs);
    return 0;
}
