/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing monoglyph                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_monoglyph.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/monoglyph.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Monoglyph tests */
/*************************************************************************************************/

int test_monoglyph_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("monoglyph", VISUAL_TEST_PANZOOM, 0);

    // Create the visual.
    DvzVisual* visual = dvz_monoglyph(vt.batch, 0);

    // Text area.
    const char text0[] = "Hello world.\nThis is a new line.\nAnd yet another line here!\n";
    char text[98 + 62] = {0};
    memcpy(text, text0, sizeof(text0));
    uint32_t j = 0, idx = 60;
    for (uint32_t i = 0; i < 96; i++)
    {
        ASSERT(idx < 96 + 2 + 61);
        ASSERT(i + 32 < 128);
        text[idx++] = (char)(i + 32);
        if (i % 32 == 0 && i > 0)
        {
            text[idx++] = '\n';
        }
    }

    dvz_monoglyph_textarea(visual, (vec3){0, 0, 0}, (DvzColor){RED}, 5.0f, text);
    dvz_monoglyph_anchor(visual, (vec2){+40, -10});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
