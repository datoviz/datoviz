/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing axes                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axes.h"
#include "datoviz.h"
#include "scene/axes.h"
#include "scene/box.h"
#include "scene/ticks.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"
#include "visuals/visual_test.h"



/*************************************************************************************************/
/*  Axes tests                                                                                   */
/*************************************************************************************************/

int test_axes_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

#if !HAS_MSDF
    return 1;
#endif
    VisualTest vt = visual_test_start("axes", VISUAL_TEST_PANZOOM, DVZ_APP_FLAGS_WHITE_BACKGROUND);

    DvzAxes* axes = dvz_panel_axes_2D(vt.panel, 0, 10, -1, 1);

    DvzVisual* visual = dvz_demo_panel_2D(vt.panel);

    visual_test_end(vt);
    dvz_axes_destroy(axes);
    return 0;
}
