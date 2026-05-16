/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene panzoom and arcball tests                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>

#include "_assertions.h"
#include "../_scene.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/arcball.h"
#include "datoviz/scene/panzoom.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_panzoom_create_reset(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);
    ANN(pz);
    AT(pz->zoom[0] == 1.0f);
    AT(pz->zoom[1] == 1.0f);
    AT(pz->pan[0] == 0.0f);
    AT(pz->pan[1] == 0.0f);

    /* Modify state then reset. */
    dvz_panzoom_pan(pz, (vec2){0.5f, -0.3f});
    dvz_panzoom_zoom(pz, (vec2){2.0f, 2.0f});
    dvz_panzoom_reset(pz);

    AT(pz->pan[0] == 0.0f);
    AT(pz->pan[1] == 0.0f);
    AT(pz->zoom[0] == 1.0f);
    AT(pz->zoom[1] == 1.0f);

    dvz_panzoom_destroy(pz);
    return 0;
}


int test_panzoom_pan_shift(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);

    /* Shift by half the viewport width → pan[0] should move by 1.0 NDC unit (at zoom=1). */
    dvz_panzoom_pan_shift(pz, (vec2){400.0f, 0.0f}, (vec2){0, 0});
    /* shift[0] = 2 * 400 / 800 = 1.0; pan[0] = pan_center[0] + 1.0 / zoom[0] = 1.0 */
    AT(fabsf(pz->pan[0] - 1.0f) < 1e-5f);
    AT(fabsf(pz->pan[1]) < 1e-5f);

    dvz_panzoom_destroy(pz);
    return 0;
}


int test_panzoom_zoom_wheel(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 800.0f, 0);

    /* Positive wheel delta should zoom in regardless of platform. */
    dvz_panzoom_zoom_wheel(pz, (vec2){0.0f, 1.0f}, (vec2){400.0f, 400.0f});
    AT(pz->zoom[0] > 1.0f);
    AT(pz->zoom[1] > 1.0f);

    DvzPanzoom* pz2 = dvz_panzoom(800.0f, 800.0f, 0);
    dvz_panzoom_zoom_wheel(pz2, (vec2){0.0f, -1.0f}, (vec2){400.0f, 400.0f});
    AT(pz2->zoom[0] < 1.0f);
    AT(pz2->zoom[1] < 1.0f);

    dvz_panzoom_destroy(pz);
    dvz_panzoom_destroy(pz2);
    return 0;
}


int test_panzoom_viewport_filters_pointer_events(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* left = dvz_panzoom(400.0f, 400.0f, 0);
    DvzPanzoom* right = dvz_panzoom(400.0f, 400.0f, 0);
    ANN(left);
    ANN(right);
    dvz_panzoom_viewport(left, 0.0f, 0.0f, 400.0f, 400.0f);
    dvz_panzoom_viewport(right, 400.0f, 0.0f, 400.0f, 400.0f);

    DvzPointerEvent wheel = {
        .type = DVZ_POINTER_EVENT_WHEEL,
        .content.w.dir = {0.0f, 1.0f},
        .pos = {600.0f, 200.0f},
    };
    AT(!dvz_panzoom_pointer(left, &wheel));
    AT(dvz_panzoom_pointer(right, &wheel));
    AT(left->zoom[0] == 1.0f);
    AT(right->zoom[0] > 1.0f);

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_LEFT,
        .content.d.press_pos = {600.0f, 200.0f},
        .content.d.last_pos = {700.0f, 200.0f},
        .content.d.shift = {100.0f, 0.0f},
        .content.d.is_press_valid = true,
        .pos = {700.0f, 200.0f},
    };
    AT(!dvz_panzoom_pointer(left, &drag));
    AT(dvz_panzoom_pointer(right, &drag));
    AT(fabsf(left->pan[0]) < 1e-5f);
    AT(fabsf(right->pan[0]) > 1e-5f);

    dvz_panzoom_destroy(left);
    dvz_panzoom_destroy(right);
    return 0;
}


int test_panzoom_double_click_resets(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);
    dvz_panzoom_zoom(pz, (vec2){3.0f, 3.0f});
    dvz_panzoom_pan(pz, (vec2){0.5f, 0.5f});

    DvzPointerEvent ev = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    bool consumed = dvz_panzoom_pointer(pz, &ev);
    AT(consumed);
    AT(pz->zoom[0] == 1.0f);
    AT(pz->pan[0] == 0.0f);

    dvz_panzoom_destroy(pz);
    return 0;
}


int test_panzoom_mvp_identity(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = dvz_panzoom(800.0f, 600.0f, 0);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    dvz_panzoom_mvp(pz, &mvp);

    /* At identity panzoom the view matrix should place the camera looking down Z. */
    /* view[3][2] (translation z) should be -2 for lookat from (0,0,2) to (0,0,0). */
    AT(fabsf(mvp.view[3][2] - (-2.0f)) < 1e-4f);

    /* At identity panzoom proj maps NDC [-1,1] → [-1,1]. */
    /* For ortho(-1,1,-1,1,-10,10): proj[0][0] = 1, proj[1][1] = 1. */
    AT(fabsf(mvp.proj[0][0] - 1.0f) < 1e-4f);
    AT(fabsf(mvp.proj[1][1] - 1.0f) < 1e-4f);

    dvz_panzoom_destroy(pz);
    return 0;
}


int test_panel_panzoom_getter(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    AT(dvz_panel_panzoom(panel) == NULL);
    dvz_panel_set_panzoom(panel, NULL, 0);
    DvzPanzoom* pz = dvz_panel_panzoom(panel);
    ANN(pz);
    AT(pz == panel->panzoom);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure panel camera and arcball compose into view/projection and model matrices.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_scene_camera_arcball_mvp_composition(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzCameraDesc desc = dvz_camera_desc();
    desc.eye[0] = 0.0f;
    desc.eye[1] = 0.0f;
    desc.eye[2] = 3.0f;
    desc.target[0] = 0.0f;
    desc.target[1] = 0.0f;
    desc.target[2] = 0.0f;
    desc.fov_y = GLM_PI_4f;
    desc.near = 0.1f;
    desc.far = 100.0f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &desc);
    ANN(camera);
    AT(dvz_panel_camera(panel) == camera);

    dvz_panel_set_arcball(panel, NULL, 0);
    ANN(panel->arcball);
    dvz_arcball_initial(panel->arcball, (vec3){0.4f, -0.8f, 1.2f});

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);

    AT(fabsf(mvp.view[3][2] - (-3.0f)) < 1e-4f);
    AT(mvp.proj[0][0] > 1.0f);
    AT(mvp.proj[1][1] > mvp.proj[0][0]);
    AT(fabsf(mvp.model[0][0] - 1.0f) > 1e-3f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_arcball_create_reset(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 600.0f, 0);
    ANN(arc);

    /* At construction the accumulated matrix should be identity (init angles all zero). */
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_rotate_produces_nonidentity_model(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 800.0f, 0);

    /* Simulate a drag: current position != press position → rotation quaternion not identity. */
    dvz_arcball_rotate(arc, (vec2){0.5f, 0.0f}, (vec2){0.0f, 0.0f});

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) != 0);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_end_commits_rotation(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 800.0f, 0);

    dvz_arcball_rotate(arc, (vec2){0.5f, 0.0f}, (vec2){0.0f, 0.0f});
    mat4 model_before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_before);

    dvz_arcball_end(arc);

    /* After end(), in-flight rotation is identity; mat has been updated.
       dvz_arcball_model should give the same result as before. */
    mat4 model_after = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_after);
    AT(memcmp(model_before, model_after, sizeof(mat4)) == 0);

    /* The in-flight rotation is now identity — another rotate from same positions gives same. */
    versor id = GLM_QUAT_IDENTITY_INIT;
    AT(memcmp(arc->rotation, id, sizeof(versor)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_rotate_axis_is_incremental(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 800.0f, 0);

    dvz_arcball_set(arc, (vec3){0.4f, 0.0f, 0.2f});
    mat4 model_before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_before);

    dvz_arcball_rotate_axis(arc, 0.25f, (vec3){0.0f, 1.0f, 0.0f});

    mat4 model_after = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_after);
    AT(memcmp(model_before, model_after, sizeof(mat4)) != 0);

    dvz_arcball_rotate(arc, (vec2){0.35f, 0.15f}, (vec2){-0.15f, -0.10f});
    dvz_arcball_end(arc);
    mat4 model_dragged = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_dragged);

    dvz_arcball_rotate_axis(arc, 0.25f, (vec3){0.0f, 1.0f, 0.0f});

    mat4 model_resumed = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model_resumed);
    AT(memcmp(model_dragged, model_resumed, sizeof(mat4)) != 0);

    dvz_arcball_destroy(arc);
    return 0;
}


/**
 * Check that wheel events update arcball zoom and model scale.
 *
 * @param suite test suite
 * @param item test item
 * @return test status
 */
int test_arcball_zoom_wheel(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 800.0f, 0);
    ANN(arc);
    AT(fabsf(arc->zoom - 1.0f) < 1e-5f);

    DvzPointerEvent wheel_in = {
        .type = DVZ_POINTER_EVENT_WHEEL,
        .content.w.dir = {0.0f, 1.0f},
        .pos = {400.0f, 400.0f},
    };
    AT(dvz_arcball_pointer(arc, &wheel_in));
    AT(arc->zoom > 1.0f);
    float zoom_in = arc->zoom;

    DvzPointerEvent wheel_out = {
        .type = DVZ_POINTER_EVENT_WHEEL,
        .content.w.dir = {0.0f, -2.0f},
        .pos = {400.0f, 400.0f},
    };
    AT(dvz_arcball_pointer(arc, &wheel_out));
    AT(arc->zoom < zoom_in);

    dvz_arcball_zoom(arc, 2.5f);
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    AT(fabsf(model[0][0] - 2.5f) < 1e-5f);

    DvzPointerEvent reset = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    AT(dvz_arcball_pointer(arc, &reset));
    AT(fabsf(arc->zoom - 1.0f) < 1e-5f);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_interaction_state(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 600.0f, 0);
    ANN(arc);

    AT(!dvz_arcball_is_interacting(NULL));
    AT(!dvz_arcball_is_interacting(arc));

    DvzPointerEvent press = {.type = DVZ_POINTER_EVENT_PRESS, .button = DVZ_POINTER_BUTTON_LEFT};
    AT(dvz_arcball_pointer(arc, &press));
    AT(dvz_arcball_is_interacting(arc));

    DvzPointerEvent release = {
        .type = DVZ_POINTER_EVENT_RELEASE,
        .button = DVZ_POINTER_BUTTON_LEFT,
    };
    AT(dvz_arcball_pointer(arc, &release));
    AT(!dvz_arcball_is_interacting(arc));

    DvzPointerEvent drag_start = {
        .type = DVZ_POINTER_EVENT_DRAG_START,
        .button = DVZ_POINTER_BUTTON_LEFT,
    };
    AT(dvz_arcball_pointer(arc, &drag_start));
    AT(dvz_arcball_is_interacting(arc));

    DvzPointerEvent drag_stop = {
        .type = DVZ_POINTER_EVENT_DRAG_STOP,
        .button = DVZ_POINTER_BUTTON_LEFT,
    };
    AT(dvz_arcball_pointer(arc, &drag_stop));
    AT(!dvz_arcball_is_interacting(arc));

    dvz_arcball_destroy(arc);
    return 0;
}


/*************************************************************************************************/
/*  Camera tests                                                                                 */
int test_arcball_double_click_resets(TstSuite* suite, TstItem* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = dvz_arcball(800.0f, 600.0f, 0);

    /* Apply a rotation then commit it. */
    dvz_arcball_rotate(arc, (vec2){0.5f, 0.3f}, (vec2){-0.2f, -0.1f});
    dvz_arcball_end(arc);

    /* Double-click resets. */
    DvzPointerEvent ev = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    bool consumed = dvz_arcball_pointer(arc, &ev);
    AT(consumed);

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}


/**
 * Register scene panzoom and arcball tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_panzoom_arcball(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TEST_SIMPLE(test_panzoom_create_reset);
    TEST_SIMPLE(test_panzoom_pan_shift);
    TEST_SIMPLE(test_panzoom_zoom_wheel);
    TEST_SIMPLE(test_panzoom_viewport_filters_pointer_events);
    TEST_SIMPLE(test_panzoom_double_click_resets);
    TEST_SIMPLE(test_panzoom_mvp_identity);
    TEST_SIMPLE(test_panel_panzoom_getter);

    TEST_SIMPLE(test_arcball_create_reset);
    TEST_SIMPLE(test_arcball_rotate_produces_nonidentity_model);
    TEST_SIMPLE(test_arcball_end_commits_rotation);
    TEST_SIMPLE(test_arcball_rotate_axis_is_incremental);
    TEST_SIMPLE(test_arcball_zoom_wheel);
    TEST_SIMPLE(test_arcball_interaction_state);
    TEST_SIMPLE(test_arcball_double_click_resets);

    TEST_SIMPLE(test_scene_camera_arcball_mvp_composition);
    return 0;
}
