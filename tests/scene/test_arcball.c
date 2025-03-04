/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing arcball                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_arcball.h"
#include "datoviz.h"
#include "scene/arcball.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Arcball test utils                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Arcball tests                                                                                */
/*************************************************************************************************/

int test_arcball_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzArcball* arcball = dvz_arcball(WIDTH, HEIGHT, 0);

    vec2 cur_pos = {0};
    vec2 last_pos = {0};
    vec3 angles = {0, 0, 0};

    // Identity.
    dvz_arcball_set(arcball, angles);
    dvz_arcball_print(arcball);

    dvz_arcball_rotate(arcball, cur_pos, last_pos);
    dvz_arcball_print(arcball);

    // From angles.
    angles[0] = M_PI / 4;
    dvz_arcball_set(arcball, angles);
    dvz_arcball_print(arcball);

    // Reset
    dvz_arcball_reset(arcball);

    // Rotation.
    last_pos[0] = +10;
    last_pos[1] = -10;
    dvz_arcball_rotate(arcball, cur_pos, last_pos);
    dvz_arcball_print(arcball);

    dvz_arcball_reset(arcball);
    dvz_arcball_set(arcball, (vec3){.1, .2, .3});
    dvz_arcball_angles(arcball, angles);
    AC(angles[0], .1, 1e-3);
    AC(angles[1], .2, 1e-3);
    AC(angles[2], .3, 1e-3);

    dvz_arcball_destroy(arcball);
    return 0;
}
