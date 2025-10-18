/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing window                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "backend.h"
#include "host.h"
#include "test_window.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_window_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzBackend backend = DVZ_BACKEND_GLFW;

    DvzWindow window = dvz_window(backend, 100, 100, 0);
    DvzWindow window2 = dvz_window(backend, 100, 100, 0);

    dvz_window_destroy(&window);
    dvz_window_destroy(&window2);

    glfwTerminate();
    return 0;
}
