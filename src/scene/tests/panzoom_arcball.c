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
#include "../_controllers.h"
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

/**
 * Project normalized device coordinates onto the test arcball sphere.
 *
 * @param p normalized input position
 * @param out output homogeneous direction
 */
static void _test_screen_to_arcball(vec2 p, vec4 out)
{
    ANN(out);
    float dist = glm_vec2_dot(p, p);
    if (dist <= 1.0f)
    {
        glm_vec4_copy((vec4){p[0], p[1], sqrtf(1.0f - dist), 0.0f}, out);
    }
    else
    {
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], p[1], 0.0f, 0.0f}, out);
    }
}


int test_panzoom_create_reset(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = _dvz_panzoom(800.0f, 600.0f, 0);
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


int test_panzoom_pan_shift(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = _dvz_panzoom(800.0f, 600.0f, 0);

    /* Shift by half the viewport width → pan[0] should move by 1.0 NDC unit (at zoom=1). */
    dvz_panzoom_pan_shift(pz, (vec2){400.0f, 0.0f}, (vec2){0, 0});
    /* shift[0] = 2 * 400 / 800 = 1.0; pan[0] = pan_center[0] + 1.0 / zoom[0] = 1.0 */
    AT(fabsf(pz->pan[0] - 1.0f) < 1e-5f);
    AT(fabsf(pz->pan[1]) < 1e-5f);

    dvz_panzoom_destroy(pz);
    return 0;
}


int test_panzoom_zoom_wheel(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = _dvz_panzoom(800.0f, 800.0f, 0);

    /* Positive wheel delta should zoom in regardless of platform. */
    dvz_panzoom_zoom_wheel(pz, (vec2){0.0f, 1.0f}, (vec2){400.0f, 400.0f});
    AT(pz->zoom[0] > 1.0f);
    AT(pz->zoom[1] > 1.0f);

    DvzPanzoom* pz2 = _dvz_panzoom(800.0f, 800.0f, 0);
    dvz_panzoom_zoom_wheel(pz2, (vec2){0.0f, -1.0f}, (vec2){400.0f, 400.0f});
    AT(pz2->zoom[0] < 1.0f);
    AT(pz2->zoom[1] < 1.0f);

    dvz_panzoom_destroy(pz);
    dvz_panzoom_destroy(pz2);
    return 0;
}


int test_panzoom_zoom_limits(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = _dvz_panzoom(800.0f, 800.0f, 0);
    ANN(pz);

    AT(dvz_panzoom_zoom_limits(pz, (vec2){0.5f, 0.25f}, (vec2){4.0f, 8.0f}));
    dvz_panzoom_zoom(pz, (vec2){0.01f, 100.0f});
    AT(fabsf(pz->zoom[0] - 0.5f) < 1e-6f);
    AT(fabsf(pz->zoom[1] - 8.0f) < 1e-6f);

    dvz_panzoom_zoom(pz, (vec2){1.0f, 1.0f});
    dvz_panzoom_end(pz);
    dvz_panzoom_zoom_shift(pz, (vec2){100000.0f, -100000.0f}, (vec2){400.0f, 400.0f});
    AT(pz->zoom[0] <= 4.0f);
    AT(pz->zoom[1] >= 0.25f);

    AT(!dvz_panzoom_zoom_limits(pz, (vec2){0.0f, 0.25f}, (vec2){4.0f, 8.0f}));
    AT(!dvz_panzoom_zoom_limits(pz, (vec2){2.0f, 0.25f}, (vec2){1.0f, 8.0f}));

    dvz_panzoom_destroy(pz);
    return 0;
}


int test_panzoom_viewport_filters_pointer_events(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* left = _dvz_panzoom(400.0f, 400.0f, 0);
    DvzPanzoom* right = _dvz_panzoom(400.0f, 400.0f, 0);
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


int test_panzoom_double_click_resets(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = _dvz_panzoom(800.0f, 600.0f, 0);
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


int test_panzoom_mvp_identity(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* pz = _dvz_panzoom(800.0f, 600.0f, 0);

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


int test_panel_panzoom_getter(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    AT(panel->panzoom == NULL);
    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);
    DvzPanzoom* pz = dvz_controller_panzoom(controller);
    ANN(pz);
    AT(pz == panel->panzoom);

    dvz_scene_destroy(scene);
    return 0;
}


int test_shared_panzoom_xy_visible_domains(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    ANN(left);
    ANN(right);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    AT(dvz_panel_bind_controller(left, controller, DVZ_DIM_MASK_XY) == 0);
    AT(dvz_panel_bind_controller(right, controller, DVZ_DIM_MASK_XY) == 0);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(left, router) == 0);
    AT(dvz_panel_connect_input(right, router) == 0);

    DvzInputEvent ev = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_WHEEL,
                .content.w.dir = {0.0f, 1.0f},
                .pos = {200.0f, 200.0f},
            },
    };
    dvz_input_emit_event(router, &ev);
    AT(panzoom->zoom[0] > 1.0f);

    double left_x0 = 0.0, left_x1 = 0.0, right_x0 = 0.0, right_x1 = 0.0;
    double left_y0 = 0.0, left_y1 = 0.0, right_y0 = 0.0, right_y1 = 0.0;
    AT(dvz_panel_visible_domain(left, DVZ_DIM_X, &left_x0, &left_x1));
    AT(dvz_panel_visible_domain(right, DVZ_DIM_X, &right_x0, &right_x1));
    AT(dvz_panel_visible_domain(left, DVZ_DIM_Y, &left_y0, &left_y1));
    AT(dvz_panel_visible_domain(right, DVZ_DIM_Y, &right_y0, &right_y1));
    AC(left_x0, right_x0, 1e-6);
    AC(left_x1, right_x1, 1e-6);
    AC(left_y0, right_y0, 1e-6);
    AC(left_y1, right_y1, 1e-6);

    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}


int test_split_panzoom_x_y_bindings(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    ANN(left);
    ANN(right);

    DvzController* shared_x = dvz_panzoom(scene, NULL);
    DvzController* left_y = dvz_panzoom(scene, NULL);
    DvzController* right_y = dvz_panzoom(scene, NULL);
    ANN(shared_x);
    ANN(left_y);
    ANN(right_y);

    DvzPanzoom* px = dvz_controller_panzoom(shared_x);
    DvzPanzoom* ly = dvz_controller_panzoom(left_y);
    DvzPanzoom* ry = dvz_controller_panzoom(right_y);
    ANN(px);
    ANN(ly);
    ANN(ry);
    dvz_panzoom_pan(px, (vec2){0.25f, 0.0f});
    dvz_panzoom_zoom(px, (vec2){2.0f, 1.0f});
    dvz_panzoom_pan(ly, (vec2){0.0f, +0.10f});
    dvz_panzoom_zoom(ly, (vec2){1.0f, 3.0f});
    dvz_panzoom_pan(ry, (vec2){0.0f, -0.20f});
    dvz_panzoom_zoom(ry, (vec2){1.0f, 4.0f});

    AT(dvz_panel_bind_controller(left, shared_x, DVZ_DIM_MASK_X) == 0);
    AT(dvz_panel_bind_controller(left, left_y, DVZ_DIM_MASK_Y) == 0);
    AT(dvz_panel_bind_controller(right, shared_x, DVZ_DIM_MASK_X) == 0);
    AT(dvz_panel_bind_controller(right, right_y, DVZ_DIM_MASK_Y) == 0);

    double left_x0 = 0.0, left_x1 = 0.0, right_x0 = 0.0, right_x1 = 0.0;
    double left_y0 = 0.0, left_y1 = 0.0, right_y0 = 0.0, right_y1 = 0.0;
    AT(dvz_panel_visible_domain(left, DVZ_DIM_X, &left_x0, &left_x1));
    AT(dvz_panel_visible_domain(right, DVZ_DIM_X, &right_x0, &right_x1));
    AT(dvz_panel_visible_domain(left, DVZ_DIM_Y, &left_y0, &left_y1));
    AT(dvz_panel_visible_domain(right, DVZ_DIM_Y, &right_y0, &right_y1));
    AC(left_x0, right_x0, 1e-6);
    AC(left_x1, right_x1, 1e-6);
    AT(fabs(left_y0 - right_y0) > 1e-3);
    AT(fabs(left_y1 - right_y1) > 1e-3);

    DvzController* replacement_x = dvz_panzoom(scene, NULL);
    ANN(replacement_x);
    AT(dvz_panel_bind_controller(left, replacement_x, DVZ_DIM_MASK_X) == 0);
    AT(dvz_panel_controller(left, DVZ_DIM_X) == replacement_x);
    AT(dvz_panel_controller(left, DVZ_DIM_Y) == left_y);

    dvz_scene_destroy(scene);
    return 0;
}


int test_arcball_scene_binding_uses_panel_input(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    ANN(panel);

    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

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
    AC(arcball->zoom, 1.0f, 1e-6f);

    DvzInputEvent inside = outside;
    inside.content.pointer.pos[0] = 200.0f;
    dvz_input_emit_event(router, &inside);
    AT(arcball->zoom > 1.0f);

    dvz_panel_destroy(panel);
    AT(dvz_controller_arcball(controller) == arcball);

    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}


int test_arcball_panel_input_uses_hidpi_figure_coordinates(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    ANN(left);
    ANN(right);

    DvzController* left_controller = dvz_arcball(scene, NULL);
    DvzController* right_controller = dvz_arcball(scene, NULL);
    ANN(left_controller);
    ANN(right_controller);
    DvzArcball* left_arcball = dvz_controller_arcball(left_controller);
    DvzArcball* right_arcball = dvz_controller_arcball(right_controller);
    ANN(left_arcball);
    ANN(right_arcball);
    AT(dvz_panel_bind_controller(left, left_controller, DVZ_DIM_MASK_XYZ) == 0);
    AT(dvz_panel_bind_controller(right, right_controller, DVZ_DIM_MASK_XYZ) == 0);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(left, router) == 0);
    AT(dvz_panel_connect_input(right, router) == 0);

    DvzInputResizeEvent resize = {
        .framebuffer_width = 800,
        .framebuffer_height = 400,
        .window_width = 400,
        .window_height = 200,
        .content_scale_x = 2.0f,
        .content_scale_y = 2.0f,
    };
    dvz_input_emit_resize(router, &resize);

    DvzInputEvent wheel = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_WHEEL,
                .content.w.dir = {0.0f, 1.0f},
                .pos = {250.0f, 100.0f},
                .content_scale = 2.0f,
            },
    };
    dvz_input_emit_event(router, &wheel);
    AC(left_arcball->zoom, 1.0f, 1e-6f);
    AT(right_arcball->zoom > 1.0f);

    dvz_input_router_destroy(router);
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
int test_scene_camera_arcball_mvp_composition(TstContext* suite, const TstCase* item)
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

    DvzController* arcball_controller = dvz_arcball(scene, NULL);
    ANN(arcball_controller);
    DvzArcball* arcball = dvz_controller_arcball(arcball_controller);
    ANN(arcball);
    AT(dvz_panel_bind_controller(panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0);
    dvz_arcball_initial(arcball, (vec3){0.4f, -0.8f, 1.2f});

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);

    AT(fabsf(mvp.view[3][2] - (-3.0f)) < 1e-4f);
    AT(mvp.proj[0][0] > 1.0f);
    AT(mvp.proj[1][1] > mvp.proj[0][0]);
    AT(fabsf(mvp.model[0][0] - 1.0f) > 1e-3f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_arcball_create_reset(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 600.0f, 0);
    ANN(arc);

    /* At construction the accumulated matrix should be identity (init angles all zero). */
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) == 0);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_rotate_produces_nonidentity_model(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 800.0f, 0);

    /* Simulate a drag: current position != press position → rotation quaternion not identity. */
    dvz_arcball_rotate(arc, (vec2){0.5f, 0.0f}, (vec2){0.0f, 0.0f});

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(memcmp(model, identity, sizeof(mat4)) != 0);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_end_commits_rotation(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 800.0f, 0);

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


int test_arcball_rotate_axis_is_incremental(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 800.0f, 0);

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
 * Ensure a camera-backed drag keeps the pressed virtual-sphere point under the cursor.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_arcball_camera_view_preserves_drag_anchor(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 600.0f, 0);
    ANN(arc);

    mat4 view = GLM_MAT4_IDENTITY_INIT;
    glm_lookat(
        (vec3){1.8f, -2.2f, 1.5f}, (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 0.0f, 1.0f}, view);
    _dvz_arcball_view(arc, view);

    vec2 press_ndc = {-0.20f, 0.0f};
    vec2 cursor_ndc = {0.25f, 0.0f};
    vec4 press_ball = {0};
    vec4 cursor_ball = {0};
    _test_screen_to_arcball(press_ndc, press_ball);
    _test_screen_to_arcball(cursor_ndc, cursor_ball);

    mat4 inv_view = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(view, inv_view);
    vec4 object_anchor = {0};
    glm_mat4_mulv(inv_view, press_ball, object_anchor);

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_LEFT,
        .content.d =
            {
                .press_pos = {320.0f, 300.0f},
                .last_pos = {320.0f, 300.0f},
                .shift = {180.0f, 0.0f},
                .is_press_valid = true,
            },
        .pos = {500.0f, 300.0f},
    };
    AT(dvz_arcball_pointer(arc, &drag));

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);

    vec4 moved_anchor = {0};
    vec4 moved_view = {0};
    glm_mat4_mulv(model, object_anchor, moved_anchor);
    glm_mat4_mulv(view, moved_anchor, moved_view);
    AC(moved_view[0], cursor_ball[0], 1e-4f);
    AC(moved_view[1], cursor_ball[1], 1e-4f);
    AC(moved_view[2], cursor_ball[2], 1e-4f);

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
int test_arcball_zoom_wheel(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 800.0f, 0);
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
#if defined(__APPLE__)
    AT(zoom_in < 1.02f);
#endif

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


/**
 * Check that right-drag pans the arcball rotation center.
 *
 * @param suite test suite
 * @param item test item
 * @return test status
 */
int test_arcball_pan_right_drag(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 600.0f, 0);
    ANN(arc);

    DvzPointerEvent start = {
        .type = DVZ_POINTER_EVENT_DRAG_START,
        .button = DVZ_POINTER_BUTTON_RIGHT,
        .pos = {400.0f, 300.0f},
    };
    AT(dvz_arcball_pointer(arc, &start));
    AT(dvz_arcball_is_interacting(arc));

    DvzPointerEvent drag = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_RIGHT,
        .content.d.press_pos = {400.0f, 300.0f},
        .content.d.last_pos = {400.0f, 300.0f},
        .content.d.shift = {80.0f, -60.0f},
        .content.d.is_press_valid = true,
        .pos = {480.0f, 240.0f},
    };
    AT(dvz_arcball_pointer(arc, &drag));
    AT(fabsf(arc->pan[0] - 0.2f) < 1e-5f);
    AT(fabsf(arc->pan[1] - 0.2f) < 1e-5f);

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arc, model);
    AT(fabsf(model[3][0] - 0.2f) < 1e-5f);
    AT(fabsf(model[3][1] - 0.2f) < 1e-5f);

    DvzPointerEvent stop = {
        .type = DVZ_POINTER_EVENT_DRAG_STOP,
        .button = DVZ_POINTER_BUTTON_RIGHT,
    };
    AT(dvz_arcball_pointer(arc, &stop));
    AT(!dvz_arcball_is_interacting(arc));
    AT(fabsf(arc->pan_center[0] - arc->pan[0]) < 1e-5f);
    AT(fabsf(arc->pan_center[1] - arc->pan[1]) < 1e-5f);

    DvzPointerEvent reset = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    AT(dvz_arcball_pointer(arc, &reset));
    AT(fabsf(arc->pan[0]) < 1e-5f);
    AT(fabsf(arc->pan[1]) < 1e-5f);

    dvz_arcball_destroy(arc);
    return 0;
}


int test_arcball_interaction_state(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 600.0f, 0);
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
int test_arcball_double_click_resets(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzArcball* arc = _dvz_arcball(800.0f, 600.0f, 0);

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

    TST_MODULE(suite, "scene");

    TST_GROUP("panzoom");
    TST_CASE(test_panzoom_create_reset);
    TST_CASE(test_panzoom_pan_shift);
    TST_CASE(test_panzoom_zoom_wheel);
    TST_CASE(test_panzoom_zoom_limits);
    TST_CASE(test_panzoom_viewport_filters_pointer_events);
    TST_CASE(test_panzoom_double_click_resets);
    TST_CASE(test_panzoom_mvp_identity);
    TST_CASE(test_panel_panzoom_getter);
    TST_CASE(test_shared_panzoom_xy_visible_domains);
    TST_CASE(test_split_panzoom_x_y_bindings);

    TST_GROUP("arcball");
    TST_CASE(test_arcball_create_reset);
    TST_CASE(test_arcball_rotate_produces_nonidentity_model);
    TST_CASE(test_arcball_end_commits_rotation);
    TST_CASE(test_arcball_rotate_axis_is_incremental);
    TST_CASE(test_arcball_camera_view_preserves_drag_anchor);
    TST_CASE(test_arcball_zoom_wheel);
    TST_CASE(test_arcball_pan_right_drag);
    TST_CASE(test_arcball_interaction_state);
    TST_CASE(test_arcball_double_click_resets);
    TST_CASE(test_arcball_scene_binding_uses_panel_input);
    TST_CASE(test_arcball_panel_input_uses_hidpi_figure_coordinates);

    TST_CASE(test_scene_camera_arcball_mvp_composition);
    return 0;
}
