/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing axis                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axis.h"
#include "datoviz.h"
#include "scene/axis.h"
#include "scene/box.h"
#include "scene/ticks.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"
#include "visuals/visual_test.h"



/*************************************************************************************************/
/*  Axis tests                                                                                   */
/*************************************************************************************************/

static void _on_frame(DvzApp* app, DvzId window_id, DvzFrameEvent ev)
{
    ANN(app);

    // The timer callbacks are called here.
    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzPanzoom* pz = vt->panzoom;
    ANN(pz);

    DvzAxis* axis = vt->haxis;
    ANN(axis);

    // Update the axis if the panzoom has been updated and if the ticks have changed.
    dvz_axis_onpanzoom(axis, pz);
}

int test_axis_1(TstSuite* suite)
{
    ANN(suite);

#if !HAS_MSDF
    return 1;
#endif
    VisualTest vt = visual_test_start("axis", VISUAL_TEST_PANZOOM, DVZ_APP_FLAGS_WHITE_BACKGROUND);

    // Margins.
    float m = 100;
    dvz_panel_margins(vt.panel, m, m, m, m);


    // Parameters.
    float font_size = 18;
    DvzDim dim = DVZ_DIM_X;
    double dmin = -5; // -102.5;
    double dmax = +5; //-92.5;
    // float inner_viewport_size = WIDTH;
    double range_size = WIDTH - 2 * m;
    double glyph_size = font_size;


    // Create the atlas.
    DvzAtlasFont af = dvz_atlas_font(font_size);


    // Create the reference frame.
    DvzRef* ref = dvz_ref(0);
    dvz_ref_set(ref, dim, dmin, dmax);


    // Create the glyph visual.
    DvzVisual* glyph = dvz_glyph(vt.batch, 0);
    dvz_glyph_atlas_font(glyph, &af);


    // Create the segment visual.
    DvzVisual* segment = dvz_segment(vt.batch, 0);


    // Create the glyph visual for the exponent and offset (factorized formats only).
    DvzVisual* factor = dvz_glyph(vt.batch, 0);
    dvz_glyph_atlas_font(factor, &af);

    DvzAtlasFont af_label = dvz_atlas_font(28);
    DvzVisual* label = dvz_glyph(vt.batch, 0);
    dvz_glyph_atlas_font(label, &af_label);


    // Create the axis.
    DvzAxis* axis = dvz_axis(glyph, segment, factor, label, dim, 0);
    dvz_axis_ref(axis, ref);
    dvz_axis_size(axis, range_size, glyph_size);
    dvz_axis_horizontal(axis, 0);
    dvz_axis_label(axis, "Axis", 10, DVZ_ORIENTATION_DEFAULT);
    vt.haxis = axis;


    // Compute ticks.
    dvz_axis_update(axis, dmin, dmax);

    dvz_app_onframe(vt.app, _on_frame, &vt);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, glyph, 0);
    dvz_panel_visual(vt.panel, segment, 0);
    dvz_panel_visual(vt.panel, factor, 0);
    dvz_panel_visual(vt.panel, label, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_font_destroy(af.font);
    dvz_atlas_destroy(af.atlas);
    dvz_ref_destroy(ref);

    return 0;
}
