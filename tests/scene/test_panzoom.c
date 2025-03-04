/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing panzoom                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_panzoom.h"
#include "datoviz.h"
#include "scene/box.h"
#include "scene/panzoom.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Panzoom test utils                                                                           */
/*************************************************************************************************/

#define PAN(x, y)                                                                                 \
    dvz_panzoom_pan_shift(pz, (vec2){WIDTH * x, HEIGHT * y}, (vec2){WIDTH / 2, HEIGHT / 2});      \
    dvz_panzoom_end(pz);

#define ZOOM(x, y, cx, cy)                                                                        \
    dvz_panzoom_zoom_shift(pz, (vec2){WIDTH * x, HEIGHT * y}, (vec2){WIDTH * cx, HEIGHT * cy});   \
    dvz_panzoom_end(pz);

#define RESET dvz_panzoom_reset(pz);

#define AP(x, y)                                                                                  \
    AC(pz->pan[0], x, EPS);                                                                       \
    AC(pz->pan[1], y, EPS);

#define SHOW                                                                                      \
    log_info(                                                                                     \
        "pan: (%.2f, %.2f)  zoom: (%.2f, %.2f)", pz->pan[0], pz->pan[1], pz->zoom[0],             \
        pz->zoom[1]);                                                                             \
    dvz_panzoom_mvp(pz, &mvp);                                                                    \
    glm_mat4_print(mvp.view, stdout);



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

int test_panzoom_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzPanzoom* pz = dvz_panzoom(WIDTH, HEIGHT, 0);
    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);

    // Test pan.
    {
        PAN(0, 0);
        AP(0, 0);
        // SHOW;

        PAN(.5, 0);
        AP(1, 0);
        // SHOW;

        PAN(.5, 0);
        AP(2, 0);

        PAN(-1, .5);
        AP(0, -1);
    }

    RESET;

    // Test zoom.
    {
        ZOOM(0, 0, .5, .5);
        AP(0, 0);

        ZOOM(.5, .5, .5, .5);
        AT(pz->zoom[0] > 1);
        AT(pz->zoom[1] < 1);
    }

    // Zoom with shift center.
    RESET;
    {
        ZOOM(10, -10, 1, 0); // top right corner
        AT(pz->zoom[0] > 1e6);
        AT(pz->zoom[1] > 1e6);
        AT(pz->zoom[0] == pz->zoom[1]);
        // SHOW;
        AP(-1, -1);
    }

    dvz_panzoom_destroy(pz);
    return 0;
}



int test_panzoom_2(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzPanzoom* pz = dvz_panzoom(WIDTH, HEIGHT, 0);

    DvzBox extent = {0};
    dvz_panzoom_extent(pz, &extent);
    AT(extent.xmin == -1.0f);
    AT(extent.xmax == 1.0f);
    AT(extent.ymin == -1.0f);
    AT(extent.ymax == 1.0f);
    AT(extent.zmin == -1.0f);
    AT(extent.zmax == 1.0f);

    dvz_panzoom_destroy(pz);

    return 0;
}



int test_panzoom_3(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzPanzoom* pz = dvz_panzoom(WIDTH, HEIGHT, 0);
    pz->pan[0] = 1;
    pz->pan[1] = -.5;
    pz->zoom[0] = 2;
    pz->zoom[1] = .5;

    DvzBox extent = {0};
    dvz_panzoom_extent(pz, &extent);
    DvzBox expected_extent = dvz_box(-1.5, -.5, -1.5, +2.5, -1, 1);
    AC(extent.xmin, expected_extent.xmin, EPS);
    AC(extent.xmax, expected_extent.xmax, EPS);
    AC(extent.ymin, expected_extent.ymin, EPS);
    AC(extent.ymax, expected_extent.ymax, EPS);

    dvz_panzoom_set(pz, &extent);
    AC(pz->pan[0], 1, EPS);
    AC(pz->pan[1], -.5, EPS);
    AC(pz->zoom[0], 2, EPS);
    AC(pz->zoom[1], .5, EPS);

    DvzBox new_extent = {0};
    dvz_panzoom_extent(pz, &new_extent);
    AC(new_extent.xmin, extent.xmin, EPS);
    AC(new_extent.xmax, extent.xmax, EPS);
    AC(new_extent.ymin, extent.ymin, EPS);
    AC(new_extent.ymax, extent.ymax, EPS);

    return 0;
}
