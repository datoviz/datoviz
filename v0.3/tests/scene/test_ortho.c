/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing ortho                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_ortho.h"
#include "datoviz.h"
#include "scene/ortho.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Ortho test utils                                                                             */
/*************************************************************************************************/

#define PAN(x, y)                                                                                 \
    dvz_ortho_pan_shift(ortho, (vec2){WIDTH * x, HEIGHT * y}, (vec2){WIDTH / 2, HEIGHT / 2});     \
    dvz_ortho_end(ortho);

#define ZOOM(x, y, cx, cy)                                                                        \
    dvz_ortho_zoom_shift(ortho, (vec2){WIDTH * x, HEIGHT * y}, (vec2){WIDTH * cx, HEIGHT * cy});  \
    dvz_ortho_end(ortho);

#define RESET dvz_ortho_reset(ortho);

#define AP(x, y)                                                                                  \
    AC(ortho->pan[0], x, EPS);                                                                    \
    AC(ortho->pan[1], y, EPS);

#define SHOW                                                                                      \
    log_info("pan: (%.2f, %.2f)  zoom: (%.2f)", ortho->pan[0], ortho->pan[1], ortho->zoom);       \
    dvz_ortho_mvp(ortho, &mvp);                                                                   \
    glm_mat4_print(mvp.view, stdout);



/*************************************************************************************************/
/*  Ortho tests                                                                                */
/*************************************************************************************************/

int test_ortho_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzOrtho* ortho = dvz_ortho(WIDTH, HEIGHT, 0);
    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);
    float a = (float)WIDTH / HEIGHT;

    // Test pan.
    {
        PAN(0, 0);
        AP(0, 0);
        // SHOW;

        PAN(.5, 0);
        AP(a, 0);
        // SHOW;

        PAN(.5, 0);
        // SHOW;
        AP(2 * a, 0);

        PAN(-1, .5);
        // SHOW;
        AP(0, -1);
    }

    RESET;

    // Test zoom.
    {
        ZOOM(0, 0, .5, .5);
        AP(0, 0);

        ZOOM(.5, -.5, .5, .5);
        // SHOW;
        AT(ortho->zoom > 1);
    }

    // Zoom with shift center.
    RESET;
    {
        ZOOM(5, -5, 1, 0); // top right corner
        // SHOW;
        AT(ortho->zoom > 1e6);
        AP(-a, -1);
    }

    dvz_ortho_destroy(ortho);
    return 0;
}
