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

#include <math.h>

#include "_assertions.h"
#include "controller_internal.h"
#include "datoviz/controller.h"
#include "datoviz/math/_cglm.h"
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



int test_controller_panzoom_keep_aspect_drag(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoomDesc desc = dvz_panzoom_desc();
    desc.width = 1600.0f;
    desc.height = 1200.0f;
    desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzPanzoom* panzoom = dvz_panzoom_create(&desc);
    ANN(panzoom);

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_RIGHT,
        .pos = {600.0f, 300.0f},
        .content.d.press_pos = {400.0f, 300.0f},
        .content.d.last_pos = {400.0f, 300.0f},
        .content.d.shift = {200.0f, 0.0f},
        .content.d.is_press_valid = true,
    };
    AT(dvz_panzoom_pointer(panzoom, &drag));
    AT(fabsf(panzoom->zoom[0] - panzoom->zoom[1]) < 0.0001f);

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



static int _assert_mat4_close(mat4 actual, mat4 expected, float eps)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            AC(actual[i][j], expected[i][j], eps);
        }
    }
    return 0;
}



int test_controller_camera_orthographic_bounds(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzCamera* camera = dvz_camera_create(NULL);
    ANN(camera);
    dvz_camera_resize(camera, 640.0f, 480.0f);

    DvzMVP mvp = {0};
    mat4 expected = GLM_MAT4_IDENTITY_INIT;

    AT(dvz_camera_set_orthographic_bounds(camera, -2.0f, 4.0f, -1.0f, 3.0f, 0.1f, 100.0f) == 0);
    dvz_camera_mvp(camera, &mvp);
    glm_ortho(-2.0f, 4.0f, -1.0f, 3.0f, 0.1f, 100.0f, expected);
    AT(_assert_mat4_close(mvp.proj, expected, 1e-6f) == 0);

    float left = 0.0f, right = 0.0f, bottom = 0.0f, top = 0.0f, near = 0.0f, far = 0.0f;
    AT(dvz_camera_get_orthographic_bounds(
           camera, &left, &right, &bottom, &top, &near, &far) == 0);
    AC(left, -2.0f, 1e-6f);
    AC(right, 4.0f, 1e-6f);
    AC(bottom, -1.0f, 1e-6f);
    AC(top, 3.0f, 1e-6f);
    AC(near, 0.1f, 1e-6f);
    AC(far, 100.0f, 1e-6f);

    AT(dvz_camera_set_orthographic_bounds(camera, 4.0f, -2.0f, 3.0f, -1.0f, 0.1f, 100.0f) == 0);
    dvz_camera_resize(camera, 1200.0f, 300.0f);
    dvz_camera_mvp(camera, &mvp);
    glm_ortho(4.0f, -2.0f, 3.0f, -1.0f, 0.1f, 100.0f, expected);
    AT(_assert_mat4_close(mvp.proj, expected, 1e-6f) == 0);

    AT(dvz_camera_set_orthographic_bounds(camera, 1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f) == -1);
    AT(dvz_camera_set_orthographic_bounds(camera, -1.0f, 1.0f, 1.0f, 1.0f, 0.1f, 100.0f) == -1);
    AT(dvz_camera_set_orthographic_bounds(camera, -1.0f, 1.0f, -1.0f, 1.0f, 5.0f, 5.0f) == -1);

    dvz_camera_set_orthographic(camera, 2.0f, 0.1f, 100.0f);
    AT(dvz_camera_get_orthographic_bounds(camera, NULL, NULL, NULL, NULL, NULL, NULL) == -1);
    dvz_camera_mvp(camera, &mvp);
    glm_ortho(-4.0f, 4.0f, -1.0f, 1.0f, 0.1f, 100.0f, expected);
    AT(_assert_mat4_close(mvp.proj, expected, 1e-6f) == 0);

    AT(dvz_camera_set_orthographic_bounds(camera, -2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f) == 0);
    dvz_camera_set_perspective(camera, GLM_PI_4f, 0.1f, 100.0f);
    AT(dvz_camera_get_orthographic_bounds(camera, NULL, NULL, NULL, NULL, NULL, NULL) == -1);

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



int test_controller_fly_z_up_lookat_drag(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.mode = DVZ_FLY_MODE_PLANE;
    desc.initial_view.eye[0] = -0.36f;
    desc.initial_view.eye[1] = -3.80f;
    desc.initial_view.eye[2] = +1.34f;
    desc.initial_view.target[0] = 0.00f;
    desc.initial_view.target[1] = 0.00f;
    desc.initial_view.target[2] = 0.22f;
    desc.initial_view.up[0] = 0.0f;
    desc.initial_view.up[1] = 0.0f;
    desc.initial_view.up[2] = 1.0f;

    DvzFly* fly = dvz_fly_create(&desc);
    ANN(fly);

    vec3 position = {0};
    vec3 target = {0};
    dvz_fly_get_position(fly, position);
    dvz_fly_get_target(fly, target);

    vec3 expected = {0}, actual = {0};
    glm_vec3_sub(desc.initial_view.target, desc.initial_view.eye, expected);
    glm_vec3_sub(target, position, actual);
    glm_vec3_normalize(expected);
    glm_vec3_normalize(actual);
    AT(glm_vec3_dot(expected, actual) > 0.999f);

    const float z_before = actual[2];
    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_LEFT,
        .pos = {500.0f, 200.0f},
        .content.d.last_pos = {100.0f, 200.0f},
        .content.d.is_press_valid = true,
    };
    AT(dvz_fly_pointer(fly, &drag));
    dvz_fly_get_position(fly, position);
    dvz_fly_get_target(fly, target);
    glm_vec3_sub(target, position, actual);
    glm_vec3_normalize(actual);
    AT(fabsf(actual[2] - z_before) < 0.0001f);

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
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panzoom_create(&panzoom) == NULL);
    panzoom = dvz_panzoom_desc();
    panzoom.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_panzoom_create(&panzoom) == NULL);

    DvzArcballDesc arcball = dvz_arcball_desc();
    arcball.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_arcball_create(&arcball) == NULL);
    arcball = dvz_arcball_desc();
    arcball.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_arcball_create(&arcball) == NULL);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_camera_create(&camera) == NULL);
    camera = dvz_camera_desc();
    camera.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_camera_create(&camera) == NULL);

    DvzFlyDesc fly = dvz_fly_desc();
    fly.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_fly_create(&fly) == NULL);
    fly = dvz_fly_desc();
    fly.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_fly_create(&fly) == NULL);

    DvzTurntableDesc turntable = dvz_turntable_desc();
    turntable.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_turntable_create(&turntable) == NULL);
    turntable = dvz_turntable_desc();
    turntable.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_turntable_create(&turntable) == NULL);

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
    TST_CASE(test_controller_panzoom_keep_aspect_drag);
    TST_CASE(test_controller_arcball_create);
    TST_CASE(test_controller_camera_create);
    TST_CASE(test_controller_camera_orthographic_bounds);
    TST_CASE(test_controller_fly_create);
    TST_CASE(test_controller_fly_z_up_lookat_drag);
    TST_CASE(test_controller_turntable_create);
    TST_CASE(test_controller_desc_abi_rejects_invalid_structs);
    return 0;
}
