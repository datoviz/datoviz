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
#include "_controllers.h"
#include "_scene.h"
#include "core/orientation_gizmo_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "datoviz/scene/arcball.h"
#include "datoviz/scene/panzoom.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

typedef struct ControllerDestroyRequestProbe
{
    uint32_t calls;
    DvzFigure* last_figure;
} ControllerDestroyRequestProbe;


static void _controller_destroy_request_frame_callback(DvzFigure* figure, void* user_data)
{
    ControllerDestroyRequestProbe* probe = (ControllerDestroyRequestProbe*)user_data;
    ANN(probe);
    probe->calls++;
    probe->last_figure = figure;
}



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



/**
 * Return whether two 4x4 matrices match within an absolute tolerance.
 *
 * @param a first matrix
 * @param b second matrix
 * @param eps absolute tolerance
 * @return whether every coefficient is within tolerance
 */
static bool _test_mat4_close(mat4 a, mat4 b, float eps)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            if (fabsf(a[i][j] - b[i][j]) > eps)
                return false;
        }
    }
    return true;
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

    DvzPanzoom* x_only = _dvz_panzoom(800.0f, 800.0f, DVZ_PANZOOM_FLAGS_FIXED_Y);
    dvz_panzoom_zoom_wheel(x_only, (vec2){0.0f, 1.0f}, (vec2){400.0f, 400.0f});
    AT(x_only->zoom[0] > 1.0f);
    AC(x_only->zoom[1], 1.0f, 1e-6f);
    AC(x_only->pan[1], 0.0f, 1e-6f);

    dvz_panzoom_destroy(pz);
    dvz_panzoom_destroy(pz2);
    dvz_panzoom_destroy(x_only);
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


int test_panzoom_keep_aspect_right_drag_diagonal_boundary(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzPanzoom* right = _dvz_panzoom(800.0f, 600.0f, DVZ_PANZOOM_FLAGS_KEEP_ASPECT);
    ANN(right);
    DvzPointerEvent ev = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_RIGHT,
        .content.d.press_pos = {400.0f, 300.0f},
        .content.d.is_press_valid = true,
        .pos = {480.0f, 300.0f},
    };
    ev.content.d.shift[0] = ev.pos[0] - ev.content.d.press_pos[0];
    ev.content.d.shift[1] = ev.pos[1] - ev.content.d.press_pos[1];
    AT(dvz_panzoom_pointer(right, &ev));
    AT(right->zoom[0] > 1.0f);
    AT(right->zoom[1] > 1.0f);

    DvzPanzoom* up = _dvz_panzoom(800.0f, 600.0f, DVZ_PANZOOM_FLAGS_KEEP_ASPECT);
    ANN(up);
    ev.pos[0] = 400.0f;
    ev.pos[1] = 220.0f;
    ev.content.d.shift[0] = ev.pos[0] - ev.content.d.press_pos[0];
    ev.content.d.shift[1] = ev.pos[1] - ev.content.d.press_pos[1];
    AT(dvz_panzoom_pointer(up, &ev));
    AT(up->zoom[0] > 1.0f);
    AT(up->zoom[1] > 1.0f);

    DvzPanzoom* out = _dvz_panzoom(800.0f, 600.0f, DVZ_PANZOOM_FLAGS_KEEP_ASPECT);
    ANN(out);
    ev.pos[0] = 320.0f;
    ev.pos[1] = 380.0f;
    ev.content.d.shift[0] = ev.pos[0] - ev.content.d.press_pos[0];
    ev.content.d.shift[1] = ev.pos[1] - ev.content.d.press_pos[1];
    AT(dvz_panzoom_pointer(out, &ev));
    AT(out->zoom[0] < 1.0f);
    AT(out->zoom[1] < 1.0f);

    DvzPanzoom* boundary = _dvz_panzoom(800.0f, 600.0f, DVZ_PANZOOM_FLAGS_KEEP_ASPECT);
    ANN(boundary);
    ev.pos[0] = 480.0f;
    ev.pos[1] = 380.0f;
    ev.content.d.shift[0] = ev.pos[0] - ev.content.d.press_pos[0];
    ev.content.d.shift[1] = ev.pos[1] - ev.content.d.press_pos[1];
    AT(dvz_panzoom_pointer(boundary, &ev));
    AC(boundary->zoom[0], 1.0f, 1e-6f);
    AC(boundary->zoom[1], 1.0f, 1e-6f);

    dvz_panzoom_destroy(right);
    dvz_panzoom_destroy(up);
    dvz_panzoom_destroy(out);
    dvz_panzoom_destroy(boundary);
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


/**
 * Ensure panzoom wheel anchors use framebuffer/figure coordinates after HiDPI scaling.
 *
 * @param suite test suite
 * @param item test case
 * @return 0 on success
 */
int test_panzoom_panel_input_uses_hidpi_figure_coordinates(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzController* controller = dvz_panzoom(scene, NULL);
    ANN(controller);
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    ANN(panzoom);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) == 0);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(panel, router) == 0);

    DvzInputResizeEvent resize = {
        .framebuffer_width = 800,
        .framebuffer_height = 400,
        .window_width = 400,
        .window_height = 200,
        .content_scale_x = 2.0f,
        .content_scale_y = 2.0f,
    };
    dvz_input_emit_resize(router, &resize);

    DvzPanzoom* expected = _dvz_panzoom(800.0f, 400.0f, 0);
    ANN(expected);
    dvz_panzoom_zoom_wheel(expected, (vec2){0.0f, 1.0f}, (vec2){600.0f, 100.0f});

    DvzInputEvent wheel = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_WHEEL,
                .content.w.dir = {0.0f, 1.0f},
                .pos = {300.0f, 50.0f},
                .content_scale = 2.0f,
            },
    };
    dvz_input_emit_event(router, &wheel);

    AC(panzoom->zoom[0], expected->zoom[0], 1e-6f);
    AC(panzoom->zoom[1], expected->zoom[1], 1e-6f);
    AC(panzoom->pan[0], expected->pan[0], 1e-6f);
    AC(panzoom->pan[1], expected->pan[1], 1e-6f);

    dvz_panzoom_destroy(expected);
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
 * Ensure a rotation-only arcball link follows drag and release without copying pan or zoom.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_arcball_rotation_only_keeps_target_centered(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* main_panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.75f, 1.0f});
    DvzPanel* gizmo_panel = dvz_panel(figure, (DvzPanelDesc){0.75f, 0.0f, 0.25f, 0.25f});
    ANN(main_panel);
    ANN(gizmo_panel);

    DvzCameraDesc main_camera_desc = dvz_camera_desc();
    main_camera_desc.eye[0] = 1.8f;
    main_camera_desc.eye[1] = -2.2f;
    main_camera_desc.eye[2] = 1.5f;
    main_camera_desc.target[0] = 0.0f;
    main_camera_desc.target[1] = 0.0f;
    main_camera_desc.target[2] = 0.0f;
    main_camera_desc.up[0] = 0.0f;
    main_camera_desc.up[1] = 0.0f;
    main_camera_desc.up[2] = 1.0f;
    DvzCamera* main_camera = dvz_panel_set_camera(main_panel, &main_camera_desc);
    ANN(main_camera);

    DvzCameraDesc gizmo_camera_desc = dvz_camera_desc();
    gizmo_camera_desc.eye[0] = 0.0f;
    gizmo_camera_desc.eye[1] = 0.0f;
    gizmo_camera_desc.eye[2] = 3.0f;
    gizmo_camera_desc.target[0] = 0.0f;
    gizmo_camera_desc.target[1] = 0.0f;
    gizmo_camera_desc.target[2] = 0.0f;
    gizmo_camera_desc.up[0] = 0.0f;
    gizmo_camera_desc.up[1] = 1.0f;
    gizmo_camera_desc.up[2] = 0.0f;
    DvzCamera* gizmo_camera = dvz_panel_set_camera(gizmo_panel, &gizmo_camera_desc);
    ANN(gizmo_camera);

    DvzController* main_controller = dvz_arcball(scene, NULL);
    DvzController* gizmo_controller = dvz_arcball(scene, NULL);
    ANN(main_controller);
    ANN(gizmo_controller);
    DvzArcball* main_arcball = dvz_controller_arcball(main_controller);
    DvzArcball* gizmo_arcball = dvz_controller_arcball(gizmo_controller);
    ANN(main_arcball);
    ANN(gizmo_arcball);
    AT(dvz_panel_bind_controller(main_panel, main_controller, DVZ_DIM_MASK_XYZ) == 0);
    AT(dvz_panel_bind_controller(gizmo_panel, gizmo_controller, DVZ_DIM_MASK_XYZ) == 0);

    dvz_arcball_pan(main_arcball, (vec2){0.25f, -0.15f});
    dvz_arcball_zoom(main_arcball, 2.25f);
    DvzControllerLink* link = dvz_controller_link(
        scene, main_controller, gizmo_controller, DVZ_CONTROLLER_LINK_ROTATION,
        DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(link);
    AC(gizmo_arcball->pan[0], 0.0f, 1e-6f);
    AC(gizmo_arcball->pan[1], 0.0f, 1e-6f);
    AC(gizmo_arcball->zoom, 1.0f, 1e-6f);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(main_panel, router) == 0);

    DvzInputEvent drag = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_DRAG,
                .button = DVZ_POINTER_BUTTON_LEFT,
                .content.d =
                    {
                        .press_pos = {220.0f, 210.0f},
                        .last_pos = {220.0f, 210.0f},
                        .shift = {160.0f, -40.0f},
                        .is_press_valid = true,
                    },
                .pos = {380.0f, 170.0f},
            },
    };
    dvz_input_emit_event(router, &drag);

    DvzMVP main_mvp = {0};
    DvzMVP gizmo_mvp = {0};
    _scene_panel_apply_mvp(main_panel, &main_mvp);
    _scene_panel_apply_mvp(gizmo_panel, &gizmo_mvp);
    AT(_test_mat4_close(main_mvp.model, gizmo_mvp.model, 1e-5f));
    AT(main_arcball->interacting);
    AT(!gizmo_arcball->interacting);
    AC(gizmo_arcball->pan[0], 0.0f, 1e-6f);
    AC(gizmo_arcball->pan[1], 0.0f, 1e-6f);
    AC(gizmo_arcball->zoom, 1.0f, 1e-6f);

    DvzInputEvent drag_stop = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_DRAG_STOP,
                .button = DVZ_POINTER_BUTTON_LEFT,
                .pos = {380.0f, 170.0f},
            },
    };
    dvz_input_emit_event(router, &drag_stop);

    _scene_panel_apply_mvp(main_panel, &main_mvp);
    _scene_panel_apply_mvp(gizmo_panel, &gizmo_mvp);
    AT(_test_mat4_close(main_mvp.model, gizmo_mvp.model, 1e-5f));
    AC(main_arcball->rotation[0], gizmo_arcball->rotation[0], 1e-6f);
    AC(main_arcball->rotation[1], gizmo_arcball->rotation[1], 1e-6f);
    AC(main_arcball->rotation[2], gizmo_arcball->rotation[2], 1e-6f);
    AC(main_arcball->rotation[3], gizmo_arcball->rotation[3], 1e-6f);
    AC(gizmo_arcball->pan[0], 0.0f, 1e-6f);
    AC(gizmo_arcball->pan[1], 0.0f, 1e-6f);
    AC(gizmo_arcball->zoom, 1.0f, 1e-6f);

    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure all-links propagation does not echo a passive target back into an active arcball.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_arcball_bidirectional_does_not_accumulate_drag(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzController* left = dvz_arcball(scene, NULL);
    DvzController* right = dvz_arcball(scene, NULL);
    ANN(left);
    ANN(right);
    DvzArcball* left_arcball = dvz_controller_arcball(left);
    DvzArcball* right_arcball = dvz_controller_arcball(right);
    ANN(left_arcball);
    ANN(right_arcball);

    DvzControllerLink* left_to_right = dvz_controller_link(
        scene, left, right,
        DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
        DVZ_CONTROLLER_LINK_ONE_WAY);
    DvzControllerLink* right_to_left = dvz_controller_link(
        scene, right, left,
        DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM,
        DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(left_to_right);
    ANN(right_to_left);

    left_arcball->interacting = true;
    dvz_arcball_rotate(left_arcball, (vec2){0.36f, -0.18f}, (vec2){-0.22f, +0.14f});
    dvz_arcball_pan(left_arcball, (vec2){0.08f, -0.04f});
    dvz_arcball_zoom(left_arcball, 1.35f);

    mat4 left_before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(left_arcball, left_before);

    _dvz_scene_controller_links_propagate(scene);
    _dvz_scene_controller_links_propagate(scene);

    mat4 left_after = GLM_MAT4_IDENTITY_INIT;
    mat4 right_after = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(left_arcball, left_after);
    dvz_arcball_model(right_arcball, right_after);

    AT(_test_mat4_close(left_before, left_after, 1e-5f));
    AT(_test_mat4_close(left_after, right_after, 1e-5f));
    AT(left_arcball->interacting);
    AT(!right_arcball->interacting);
    AC(left_arcball->zoom, right_arcball->zoom, 1e-6f);
    AC(left_arcball->pan[0], right_arcball->pan[0], 1e-6f);
    AC(left_arcball->pan[1], right_arcball->pan[1], 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


int test_orientation_gizmo_create_place_resize_and_visibility(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 1000, 800, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel_full(figure);
    ANN(panel);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 1.8f;
    camera_desc.eye[1] = -2.2f;
    camera_desc.eye[2] = 1.5f;
    camera_desc.target[0] = 0.0f;
    camera_desc.target[1] = 0.0f;
    camera_desc.target[2] = 0.0f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);

    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);
    dvz_arcball_set(arcball, (vec3){0.35f, -0.20f, 0.15f});

    DvzOrientationGizmoDesc desc = dvz_orientation_gizmo_desc();
    desc.placement = dvz_placement_panel_corner(
        DVZ_HORIZONTAL_ANCHOR_RIGHT, DVZ_VERTICAL_ANCHOR_BOTTOM, 150.0f, 150.0f, -16.0f,
        -16.0f);
    DvzOrientationGizmo* gizmo = dvz_orientation_gizmo(panel, &desc);
    ANN(gizmo);
    ANN(gizmo->panel);
    ANN(gizmo->axes_visual);
    ANN(gizmo->rings_visual);
    ANN(gizmo->axes_positions);
    ANN(gizmo->axes_normals);
    ANN(gizmo->axes_colors);
    ANN(gizmo->ring_positions);
    ANN(gizmo->ring_colors);
    ANN(gizmo->ring_widths);
    AT(scene->orientation_gizmo_count == 1);
    bool axes_attached = false;
    bool rings_attached = false;
    for (uint32_t vi = 0; vi < gizmo->panel->visual_count; vi++)
    {
        if (gizmo->panel->visuals[vi].visual == gizmo->axes_visual)
        {
            axes_attached = true;
            AT(gizmo->panel->visuals[vi].controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ);
        }
        if (gizmo->panel->visuals[vi].visual == gizmo->rings_visual)
        {
            rings_attached = true;
            AT(gizmo->panel->visuals[vi].controller_mode == DVZ_CONTROLLER_APPLY_VIEW_PROJ);
        }
    }
    AT(axes_attached);
    AT(rings_attached);
    AC(gizmo->panel->desc.x, 0.834f, 1e-6f);
    AC(gizmo->panel->desc.y, 0.7925f, 1e-6f);
    AC(gizmo->panel->desc.width, 0.15f, 1e-6f);
    AC(gizmo->panel->desc.height, 0.1875f, 1e-6f);

    _scene_prepare_orientation_gizmos(figure);
    DvzMVP source_mvp = {0};
    DvzMVP gizmo_mvp = {0};
    _scene_panel_apply_mvp(panel, &source_mvp);
    _scene_panel_apply_mvp(gizmo->panel, &gizmo_mvp);
    mat4 source_view = GLM_MAT4_IDENTITY_INIT;
    mat4 source_model = GLM_MAT4_IDENTITY_INIT;
    mat4 gizmo_view = GLM_MAT4_IDENTITY_INIT;
    mat4 inv_gizmo_view = GLM_MAT4_IDENTITY_INIT;
    mat4 expected = GLM_MAT4_IDENTITY_INIT;
    mat4 tmp = GLM_MAT4_IDENTITY_INIT;
    for (uint32_t col = 0; col < 3; col++)
    {
        for (uint32_t row = 0; row < 3; row++)
        {
            source_view[col][row] = source_mvp.view[col][row];
            source_model[col][row] = source_mvp.model[col][row];
            gizmo_view[col][row] = gizmo_mvp.view[col][row];
        }
    }
    glm_mat4_inv(gizmo_view, inv_gizmo_view);
    glm_mat4_mul(source_view, source_model, tmp);
    glm_mat4_mul(inv_gizmo_view, tmp, expected);
    mat4 axes_transform = GLM_MAT4_IDENTITY_INIT;
    mat4 rings_transform = GLM_MAT4_IDENTITY_INIT;
    AT(dvz_visual_get_transform(gizmo->axes_visual, axes_transform) == 0);
    AT(dvz_visual_get_transform(gizmo->rings_visual, rings_transform) == 0);
    AT(_test_mat4_close(axes_transform, expected, 1e-5f));
    AT(_test_mat4_close(rings_transform, expected, 1e-5f));

    dvz_arcball_reset(arcball);
    _scene_prepare_orientation_gizmos(figure);
    _scene_panel_apply_mvp(panel, &source_mvp);
    _scene_panel_apply_mvp(gizmo->panel, &gizmo_mvp);
    glm_mat4_identity(source_view);
    glm_mat4_identity(source_model);
    glm_mat4_identity(gizmo_view);
    for (uint32_t col = 0; col < 3; col++)
    {
        for (uint32_t row = 0; row < 3; row++)
        {
            source_view[col][row] = source_mvp.view[col][row];
            source_model[col][row] = source_mvp.model[col][row];
            gizmo_view[col][row] = gizmo_mvp.view[col][row];
        }
    }
    glm_mat4_inv(gizmo_view, inv_gizmo_view);
    glm_mat4_mul(source_view, source_model, tmp);
    glm_mat4_mul(inv_gizmo_view, tmp, expected);
    AT(dvz_visual_get_transform(gizmo->axes_visual, axes_transform) == 0);
    AT(_test_mat4_close(axes_transform, expected, 1e-5f));

    dvz_figure_resize(figure, 1200, 900);
    _scene_prepare_orientation_gizmos(figure);
    AC(gizmo->panel->desc.x, 1034.0f / 1200.0f, 1e-6f);
    AC(gizmo->panel->desc.y, 734.0f / 900.0f, 1e-6f);
    AC(gizmo->panel->desc.width, 150.0f / 1200.0f, 1e-6f);
    AC(gizmo->panel->desc.height, 150.0f / 900.0f, 1e-6f);

    dvz_orientation_gizmo_set_visible(gizmo, false);
    AT(!gizmo->visible);
    AT(!gizmo->axes_visual->visible);
    AT(!gizmo->rings_visual->visible);
    dvz_orientation_gizmo_set_visible(gizmo, true);
    AT(gizmo->visible);
    AT(gizmo->axes_visual->visible);
    AT(gizmo->rings_visual->visible);

    dvz_orientation_gizmo_destroy(gizmo);
    AT(!scene->orientation_gizmos[0].active);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure panzoom extent links can copy only one axis.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_panzoom_extent_x_only(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzController* source = dvz_panzoom(scene, NULL);
    DvzController* target = dvz_panzoom(scene, NULL);
    ANN(source);
    ANN(target);
    DvzPanzoom* source_pz = dvz_controller_panzoom(source);
    DvzPanzoom* target_pz = dvz_controller_panzoom(target);
    ANN(source_pz);
    ANN(target_pz);

    dvz_panzoom_pan(source_pz, (vec2){0.30f, -0.40f});
    dvz_panzoom_zoom(source_pz, (vec2){2.0f, 3.0f});

    DvzControllerLink* link = dvz_controller_link(
        scene, source, target, DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(link);
    AC(target_pz->pan[0], source_pz->pan[0], 1e-6f);
    AC(target_pz->zoom[0], source_pz->zoom[0], 1e-6f);
    AC(target_pz->pan[1], 0.0f, 1e-6f);
    AC(target_pz->zoom[1], 1.0f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure panzoom extent links resolve through each panel's equal-aspect base extent.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_panzoom_extent_x_equal_aspect_panels(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 600, 200, 0);
    DvzPanel* source_panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 2.0f / 3.0f, 1.0f});
    DvzPanel* target_panel = dvz_panel(figure, (DvzPanelDesc){2.0f / 3.0f, 0.0f, 1.0f / 3.0f, 1.0f});
    ANN(source_panel);
    ANN(target_panel);

    DvzPanelView2D view = dvz_panel_view2d();
    view.aspect = DVZ_PANEL_VIEW2D_ASPECT_EQUAL;
    AT(dvz_panel_set_view2d(source_panel, &view) == 0);
    AT(dvz_panel_set_view2d(target_panel, &view) == 0);

    DvzController* source = dvz_panzoom(scene, NULL);
    DvzController* target = dvz_panzoom(scene, NULL);
    ANN(source);
    ANN(target);
    AT(dvz_panel_bind_controller(source_panel, source, DVZ_DIM_MASK_XY) == 0);
    AT(dvz_panel_bind_controller(target_panel, target, DVZ_DIM_MASK_XY) == 0);

    DvzPanzoom* source_pz = dvz_controller_panzoom(source);
    DvzPanzoom* target_pz = dvz_controller_panzoom(target);
    ANN(source_pz);
    ANN(target_pz);
    dvz_panzoom_zoom(source_pz, (vec2){2.0f, 1.0f});

    DvzControllerLink* link = dvz_controller_link(
        scene, source, target, DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(link);
    AC(target_pz->pan[0], 0.0f, 1e-6f);
    AC(target_pz->zoom[0], 1.0f, 1e-6f);
    AC(target_pz->zoom[1], 1.0f, 1e-6f);

    float extent[4] = {0};
    AT(_scene_panel_panzoom_extent(target_panel, extent));
    AC(extent[0], -1.0f, 1e-6f);
    AC(extent[1], +1.0f, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure all-links propagation does not echo passive panzoom extent state into an active drag.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_panzoom_extent_x_bidirectional_does_not_accumulate_drag(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* top = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 0.5f});
    DvzPanel* bottom = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.5f, 1.0f, 0.5f});
    ANN(top);
    ANN(bottom);

    DvzController* top_x = dvz_panzoom(scene, NULL);
    DvzController* bottom_x = dvz_panzoom(scene, NULL);
    ANN(top_x);
    ANN(bottom_x);
    AT(dvz_panel_bind_controller(top, top_x, DVZ_DIM_MASK_X) == 0);
    AT(dvz_panel_bind_controller(bottom, bottom_x, DVZ_DIM_MASK_X) == 0);

    DvzControllerLink* top_to_bottom = dvz_controller_link(
        scene, top_x, bottom_x, DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
    DvzControllerLink* bottom_to_top = dvz_controller_link(
        scene, bottom_x, top_x, DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(top_to_bottom);
    ANN(bottom_to_top);

    DvzPanzoom* top_pz = dvz_controller_panzoom(top_x);
    DvzPanzoom* bottom_pz = dvz_controller_panzoom(bottom_x);
    ANN(top_pz);
    ANN(bottom_pz);
    top_pz->interacting = true;
    top_pz->pan[0] = 0.24f;
    top_pz->pan_center[0] = 0.0f;
    top_pz->zoom[0] = 1.80f;
    top_pz->zoom_center[0] = 1.0f;

    _dvz_scene_controller_links_propagate(scene);
    _dvz_scene_controller_links_propagate(scene);

    AC(top_pz->pan[0], 0.24f, 1e-6f);
    AC(top_pz->pan_center[0], 0.0f, 1e-6f);
    AC(top_pz->zoom[0], 1.80f, 1e-6f);
    AC(top_pz->zoom_center[0], 1.0f, 1e-6f);
    AC(bottom_pz->pan[0], top_pz->pan[0], 1e-6f);
    AC(bottom_pz->zoom[0], top_pz->zoom[0], 1e-6f);
    AC(bottom_pz->pan_center[0], top_pz->pan[0], 1e-6f);
    AC(bottom_pz->zoom_center[0], top_pz->zoom[0], 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure invalid controller link combinations are rejected.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_validation(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzScene* other = dvz_scene();
    ANN(scene);
    ANN(other);
    DvzController* arc_a = dvz_arcball(scene, NULL);
    DvzController* arc_b = dvz_arcball(scene, NULL);
    DvzController* panzoom = dvz_panzoom(scene, NULL);
    DvzController* other_arc = dvz_arcball(other, NULL);
    ANN(arc_a);
    ANN(arc_b);
    ANN(panzoom);
    ANN(other_arc);

    AT(dvz_controller_link(
           NULL, arc_a, arc_b, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, NULL, arc_b, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, NULL, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, arc_a, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, arc_b, DVZ_CONTROLLER_LINK_NONE, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, panzoom, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, other_arc, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, arc_b, DVZ_CONTROLLER_LINK_CAMERA, DVZ_CONTROLLER_LINK_ONE_WAY) ==
       NULL);
    AT(dvz_controller_link(
           scene, arc_a, arc_b, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_TWO_WAY) ==
       NULL);

    dvz_scene_destroy(other);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure destroying a controller link stops future propagation.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_link_destroy_stops_arcball_propagation(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzController* source = dvz_arcball(scene, NULL);
    DvzController* target = dvz_arcball(scene, NULL);
    ANN(source);
    ANN(target);
    DvzArcball* source_arcball = dvz_controller_arcball(source);
    DvzArcball* target_arcball = dvz_controller_arcball(target);
    ANN(source_arcball);
    ANN(target_arcball);

    DvzControllerLink* link = dvz_controller_link(
        scene, source, target, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(link);
    dvz_arcball_rotate_axis(source_arcball, 0.35f, (vec3){0.0f, 1.0f, 0.0f});
    _dvz_scene_controller_links_propagate(scene);

    mat4 source_model = GLM_MAT4_IDENTITY_INIT;
    mat4 target_model = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(source_arcball, source_model);
    dvz_arcball_model(target_arcball, target_model);
    AT(_test_mat4_close(source_model, target_model, 1e-5f));

    dvz_controller_link_destroy(link);
    mat4 target_before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(target_arcball, target_before);

    dvz_arcball_rotate_axis(source_arcball, 0.45f, (vec3){1.0f, 0.0f, 0.0f});
    _dvz_scene_controller_links_propagate(scene);
    dvz_arcball_model(source_arcball, source_model);
    dvz_arcball_model(target_arcball, target_model);
    AT(_test_mat4_close(target_before, target_model, 1e-5f));
    AT(!_test_mat4_close(source_model, target_model, 1e-5f));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure destroying a controller detaches panels, destroys links, and reuses slots.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_destroy_detaches_panels_links_and_reuses_slot(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    dvz_controller_destroy(NULL);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* left = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
    DvzPanel* right = dvz_panel(figure, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});
    ANN(left);
    ANN(right);

    DvzController* source = dvz_arcball(scene, NULL);
    DvzController* target = dvz_arcball(scene, NULL);
    ANN(source);
    ANN(target);
    DvzArcball* source_arcball = dvz_controller_arcball(source);
    ANN(source_arcball);
    AT(dvz_panel_bind_controller(left, source, DVZ_DIM_MASK_XYZ) == 0);
    AT(dvz_panel_bind_controller(right, source, DVZ_DIM_MASK_XYZ) == 0);

    DvzControllerLink* link = dvz_controller_link(
        scene, source, target, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY);
    ANN(link);
    AT(link->active);

    ControllerDestroyRequestProbe probe = {0};
    AT(_scene_add_request_frame_callback(
        scene, _controller_destroy_request_frame_callback, &probe));

    dvz_controller_destroy(source);
    AT(probe.calls == 1);
    AT(probe.last_figure == figure);
    AT(dvz_controller_type(source) == DVZ_CONTROLLER_TYPE_NONE);
    AT(dvz_controller_arcball(source) == NULL);
    AT(!link->active);
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        AT(left->controllers[dim] == NULL);
        AT(right->controllers[dim] == NULL);
    }
    AT(left->arcball == NULL);
    AT(right->arcball == NULL);

    dvz_controller_destroy(source);
    AT(probe.calls == 1);

    DvzController* reused = dvz_arcball(scene, NULL);
    AT(reused == source);
    AT(dvz_controller_type(reused) == DVZ_CONTROLLER_TYPE_ARCBALL);
    DvzControllerLink* reused_link = dvz_controller_link(
        scene, reused, target, DVZ_CONTROLLER_LINK_ROTATION, DVZ_CONTROLLER_LINK_ONE_WAY);
    AT(reused_link == link);

    _scene_remove_request_frame_callback(scene, _controller_destroy_request_frame_callback, &probe);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure destroying one partial panzoom binding leaves other dimensions intact.
 *
 * @param suite the test suite
 * @param item the test item
 * @return 0 on success
 */
int test_controller_destroy_preserves_other_panel_bindings(
    TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 400, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);

    DvzController* x_controller = dvz_panzoom(scene, NULL);
    DvzController* y_controller = dvz_panzoom(scene, NULL);
    ANN(x_controller);
    ANN(y_controller);
    DvzPanzoom* y_panzoom = dvz_controller_panzoom(y_controller);
    ANN(y_panzoom);
    AT(dvz_panel_bind_controller(panel, x_controller, DVZ_DIM_MASK_X) == 0);
    AT(dvz_panel_bind_controller(panel, y_controller, DVZ_DIM_MASK_Y) == 0);
    AT(panel->controllers[DVZ_DIM_X] == x_controller);
    AT(panel->controllers[DVZ_DIM_Y] == y_controller);
    AT(panel->panzoom == y_panzoom);

    dvz_controller_destroy(x_controller);
    AT(panel->controllers[DVZ_DIM_X] == NULL);
    AT(panel->controllers[DVZ_DIM_Y] == y_controller);
    AT(panel->panzoom == y_panzoom);
    AT(dvz_controller_type(y_controller) == DVZ_CONTROLLER_TYPE_PANZOOM);

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
    desc.near_clip = 0.1f;
    desc.far_clip = 100.0f;
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

int test_orbit_camera_changes_view_not_model(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});
    DvzCamera* camera = dvz_panel_set_camera(panel, NULL);
    ANN(camera);

    DvzController* controller = dvz_orbit_camera(scene, NULL);
    ANN(controller);
    DvzOrbitCamera* orbit = dvz_controller_orbit_camera(controller);
    ANN(orbit);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    vec3 eye0 = {0}, target0 = {0}, up0 = {0};
    dvz_camera_get_view(camera, eye0, target0, up0);
    dvz_orbit_camera_set_camera(orbit, camera);
    dvz_orbit_camera_resize(orbit, 800.0f, 600.0f);

    vec3 pivot = {0};
    AT(dvz_orbit_camera_get_pivot(orbit, pivot) == 0);
    AC(pivot[0], target0[0], 1e-6);
    AC(pivot[1], target0[1], 1e-6);
    AC(pivot[2], target0[2], 1e-6);
    AT(dvz_orbit_camera_get_distance(orbit) > 0.0f);

    DvzPointerEvent ev = {0};
    ev.type = DVZ_POINTER_EVENT_DRAG;
    ev.button = DVZ_POINTER_BUTTON_LEFT;
    ev.pos[0] = 500.0f;
    ev.pos[1] = 300.0f;
    ev.content.d.press_pos[0] = 400.0f;
    ev.content.d.press_pos[1] = 300.0f;
    ev.content.d.last_pos[0] = 400.0f;
    ev.content.d.last_pos[1] = 300.0f;
    ev.content.d.is_press_valid = true;
    AT(dvz_orbit_camera_pointer(orbit, &ev));

    vec3 eye1 = {0}, target1 = {0}, up1 = {0};
    dvz_camera_get_view(camera, eye1, target1, up1);
    vec3 orbit_eye = {0}, orbit_target = {0}, orbit_up = {0};
    AT(dvz_orbit_camera_get_view(orbit, orbit_eye, orbit_target, orbit_up) == 0);
    AC(orbit_eye[0], eye1[0], 1e-6);
    AC(orbit_eye[1], eye1[1], 1e-6);
    AC(orbit_eye[2], eye1[2], 1e-6);
    AC(orbit_target[0], target1[0], 1e-6);
    AC(orbit_target[1], target1[1], 1e-6);
    AC(orbit_target[2], target1[2], 1e-6);
    AC(orbit_up[0], up1[0], 1e-6);
    AC(orbit_up[1], up1[1], 1e-6);
    AC(orbit_up[2], up1[2], 1e-6);
    AT(fabsf(eye1[0] - eye0[0]) > 1e-3f || fabsf(eye1[2] - eye0[2]) > 1e-3f);
    AC(target1[0], target0[0], 1e-6);
    AC(target1[1], target0[1], 1e-6);
    AC(target1[2], target0[2], 1e-6);

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    AC(mvp.model[0][0], 1.0f, 1e-6);
    AC(mvp.model[1][1], 1.0f, 1e-6);
    AC(mvp.model[2][2], 1.0f, 1e-6);
    AC(mvp.model[3][0], 0.0f, 1e-6);

    dvz_scene_destroy(scene);
    return 0;
}


int test_orbit_camera_drag_axes_match_upright_planet(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = -3.0f;
    camera_desc.eye[2] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;

    DvzCamera* camera = dvz_camera_create(&camera_desc);
    ANN(camera);
    DvzOrbitCamera* orbit = dvz_orbit_camera_create(NULL);
    ANN(orbit);
    dvz_orbit_camera_viewport(orbit, 0.0f, 0.0f, 800.0f, 600.0f);
    dvz_orbit_camera_set_camera(orbit, camera);

    DvzPointerEvent horizontal = {
        .type = DVZ_POINTER_EVENT_DRAG,
        .button = DVZ_POINTER_BUTTON_LEFT,
        .pos = {600.0f, 300.0f},
        .content.d.press_pos = {400.0f, 300.0f},
        .content.d.last_pos = {400.0f, 300.0f},
        .content.d.shift = {200.0f, 0.0f},
        .content.d.is_press_valid = true,
    };
    AT(dvz_orbit_camera_pointer(orbit, &horizontal));

    vec3 eye = {0}, target = {0}, up = {0};
    dvz_camera_get_view(camera, eye, target, up);
    AT(eye[0] < -1e-3f);
    AC(eye[2], 0.0f, 1e-5f);
    AC(up[0], 0.0f, 1e-5f);
    AC(up[1], 0.0f, 1e-5f);
    AC(up[2], 1.0f, 1e-5f);
    AC(target[0], 0.0f, 1e-6f);
    AC(target[1], 0.0f, 1e-6f);
    AC(target[2], 0.0f, 1e-6f);

    dvz_camera_set_view(camera, camera_desc.eye, camera_desc.target, camera_desc.up);
    dvz_orbit_camera_set_camera(orbit, camera);
    DvzPointerEvent vertical = horizontal;
    vertical.pos[0] = 400.0f;
    vertical.pos[1] = 100.0f;
    vertical.content.d.shift[0] = 0.0f;
    vertical.content.d.shift[1] = -200.0f;
    AT(dvz_orbit_camera_pointer(orbit, &vertical));

    dvz_camera_get_view(camera, eye, target, up);
    AT(eye[2] < -1e-3f);
    AC(up[0], 0.0f, 1e-5f);

    camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = 1.2f;
    camera_desc.eye[2] = 1.25f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 1.0f;
    camera_desc.up[2] = 0.0f;
    dvz_camera_set_view(camera, camera_desc.eye, camera_desc.target, camera_desc.up);
    dvz_orbit_camera_set_camera(orbit, camera);
    AT(dvz_orbit_camera_pointer(orbit, &horizontal));

    dvz_camera_get_view(camera, eye, target, up);
    AC(eye[1], 1.2f, 1e-5f);
    AC(up[0], 0.0f, 1e-5f);
    AC(up[1], 1.0f, 1e-5f);
    AC(up[2], 0.0f, 1e-5f);

    dvz_orbit_camera_destroy(orbit);
    dvz_camera_destroy(camera);
    return 0;
}


int test_orbit_camera_scene_drag_uses_stable_baseline(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = -3.0f;
    camera_desc.eye[2] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);

    DvzController* controller = dvz_orbit_camera(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(panel, router) == 0);

    DvzInputEvent start = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_DRAG_START,
                .button = DVZ_POINTER_BUTTON_LEFT,
                .pos = {404.0f, 300.0f},
                .content.d.press_pos = {400.0f, 300.0f},
                .content.d.is_press_valid = true,
            },
    };
    dvz_input_emit_event(router, &start);

    DvzInputEvent drag = start;
    drag.content.pointer.type = DVZ_POINTER_EVENT_DRAG;
    drag.content.pointer.pos[0] = 500.0f;
    drag.content.pointer.content.d.last_pos[0] = 404.0f;
    drag.content.pointer.content.d.last_pos[1] = 300.0f;
    drag.content.pointer.content.d.shift[0] = 100.0f;
    drag.content.pointer.content.d.shift[1] = 0.0f;
    dvz_input_emit_event(router, &drag);

    drag.content.pointer.pos[0] = 600.0f;
    drag.content.pointer.content.d.last_pos[0] = 500.0f;
    drag.content.pointer.content.d.shift[0] = 200.0f;
    dvz_input_emit_event(router, &drag);

    vec3 actual_eye = {0}, actual_target = {0}, actual_up = {0};
    dvz_camera_get_view(camera, actual_eye, actual_target, actual_up);

    DvzCamera* expected_camera = dvz_camera_create(&camera_desc);
    ANN(expected_camera);
    DvzOrbitCamera* expected_orbit = dvz_orbit_camera_create(NULL);
    ANN(expected_orbit);
    dvz_orbit_camera_viewport(expected_orbit, 0.0f, 0.0f, 800.0f, 600.0f);
    dvz_orbit_camera_set_camera(expected_orbit, expected_camera);
    AT(dvz_orbit_camera_pointer(expected_orbit, &drag.content.pointer));

    vec3 expected_eye = {0}, expected_target = {0}, expected_up = {0};
    dvz_camera_get_view(expected_camera, expected_eye, expected_target, expected_up);
    for (uint32_t i = 0; i < 3; i++)
    {
        AC(actual_eye[i], expected_eye[i], 1e-5f);
        AC(actual_target[i], expected_target[i], 1e-5f);
        AC(actual_up[i], expected_up[i], 1e-5f);
    }

    dvz_orbit_camera_destroy(expected_orbit);
    dvz_camera_destroy(expected_camera);
    dvz_input_router_destroy(router);
    dvz_scene_destroy(scene);
    return 0;
}


int test_orbit_camera_double_click_restores_initial_view(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = -3.0f;
    camera_desc.eye[2] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    ANN(panel);
    DvzCamera* camera = dvz_panel_set_camera(panel, &camera_desc);
    ANN(camera);

    vec3 initial_eye = {0}, initial_target = {0}, initial_up = {0};
    dvz_camera_get_view(camera, initial_eye, initial_target, initial_up);

    DvzController* controller = dvz_orbit_camera(scene, NULL);
    ANN(controller);
    AT(dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) == 0);

    DvzInputRouter* router = dvz_input_router();
    ANN(router);
    AT(dvz_panel_connect_input(panel, router) == 0);

    DvzInputEvent drag = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_DRAG,
                .button = DVZ_POINTER_BUTTON_LEFT,
                .pos = {400.0f, 100.0f},
                .content.d.press_pos = {400.0f, 300.0f},
                .content.d.last_pos = {400.0f, 300.0f},
                .content.d.shift = {0.0f, -200.0f},
                .content.d.is_press_valid = true,
            },
    };
    dvz_input_emit_event(router, &drag);

    DvzInputEvent stop = drag;
    stop.content.pointer.type = DVZ_POINTER_EVENT_DRAG_STOP;
    dvz_input_emit_event(router, &stop);

    vec3 dragged_eye = {0}, dragged_target = {0}, dragged_up = {0};
    dvz_camera_get_view(camera, dragged_eye, dragged_target, dragged_up);
    AT(fabsf(dragged_eye[0] - initial_eye[0]) > 1e-3f ||
       fabsf(dragged_eye[1] - initial_eye[1]) > 1e-3f ||
       fabsf(dragged_eye[2] - initial_eye[2]) > 1e-3f);

    DvzInputEvent reset = {
        .type = DVZ_INPUT_EVENT_POINTER,
        .content.pointer =
            {
                .type = DVZ_POINTER_EVENT_DOUBLE_CLICK,
                .pos = {400.0f, 300.0f},
            },
    };
    dvz_input_emit_event(router, &reset);

    vec3 reset_eye = {0}, reset_target = {0}, reset_up = {0};
    dvz_camera_get_view(camera, reset_eye, reset_target, reset_up);
    for (uint32_t i = 0; i < 3; i++)
    {
        AC(reset_eye[i], initial_eye[i], 1e-5f);
        AC(reset_target[i], initial_target[i], 1e-5f);
        AC(reset_up[i], initial_up[i], 1e-5f);
    }

    dvz_input_router_destroy(router);
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
 * Check that wheel events update arcball zoom and camera dolly.
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
    AT(fabsf(model[0][0] - 1.0f) < 1e-5f);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_arcball_mvp(arc, &mvp);
    AT(fabsf(mvp.model[0][0] - 1.0f) < 1e-5f);
    AT(fabsf(mvp.view[3][2] - logf(2.5f)) < 1e-5f);
    AT(fabsf(mvp.proj[0][0] - 1.0f) < 1e-5f);
    AT(fabsf(mvp.proj[1][1] - 1.0f) < 1e-5f);

    DvzPointerEvent reset = {.type = DVZ_POINTER_EVENT_DOUBLE_CLICK};
    AT(dvz_arcball_pointer(arc, &reset));
    AT(fabsf(arc->zoom - 1.0f) < 1e-5f);

    dvz_arcball_destroy(arc);
    return 0;
}


/**
 * Check that right-drag pans the arcball camera view.
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
    AT(fabsf(model[3][0]) < 1e-5f);
    AT(fabsf(model[3][1]) < 1e-5f);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_arcball_mvp(arc, &mvp);
    AT(fabsf(mvp.model[3][0]) < 1e-5f);
    AT(fabsf(mvp.model[3][1]) < 1e-5f);
    AT(fabsf(mvp.view[3][0] - 0.2f) < 1e-5f);
    AT(fabsf(mvp.view[3][1] - 0.2f) < 1e-5f);
    AT(fabsf(mvp.proj[3][0]) < 1e-5f);
    AT(fabsf(mvp.proj[3][1]) < 1e-5f);

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
    TST_CASE(test_panzoom_keep_aspect_right_drag_diagonal_boundary);
    TST_CASE(test_panzoom_viewport_filters_pointer_events);
    TST_CASE(test_panzoom_double_click_resets);
    TST_CASE(test_panzoom_mvp_identity);
    TST_CASE(test_panel_panzoom_getter);
    TST_CASE(test_shared_panzoom_xy_visible_domains);
    TST_CASE(test_panzoom_panel_input_uses_hidpi_figure_coordinates);
    TST_CASE(test_split_panzoom_x_y_bindings);

    TST_GROUP("arcball");
    TST_CASE(test_orbit_camera_changes_view_not_model);
    TST_CASE(test_orbit_camera_drag_axes_match_upright_planet);
    TST_CASE(test_orbit_camera_scene_drag_uses_stable_baseline);
    TST_CASE(test_orbit_camera_double_click_restores_initial_view);
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
    TST_CASE(test_controller_link_arcball_rotation_only_keeps_target_centered);
    TST_CASE(test_controller_link_arcball_bidirectional_does_not_accumulate_drag);
    TST_CASE(test_orientation_gizmo_create_place_resize_and_visibility);
    TST_CASE(test_controller_link_panzoom_extent_x_only);
    TST_CASE(test_controller_link_panzoom_extent_x_equal_aspect_panels);
    TST_CASE(test_controller_link_panzoom_extent_x_bidirectional_does_not_accumulate_drag);
    TST_CASE(test_controller_link_validation);
    TST_CASE(test_controller_link_destroy_stops_arcball_propagation);
    TST_CASE(test_controller_destroy_detaches_panels_links_and_reuses_slot);
    TST_CASE(test_controller_destroy_preserves_other_panel_bindings);

    TST_CASE(test_scene_camera_arcball_mvp_composition);
    return 0;
}
