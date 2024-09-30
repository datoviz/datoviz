/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing glyph                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_glyph.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/atlas.h"
#include "scene/font.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/glyph.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Glyph tests                                                                                  */
/*************************************************************************************************/

static void _on_timer(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);
    float t = ev.content.t.time;

    // float x = sin(t);
    // float z = cos(t);
    // dvz_glyph_axis(visual, 0, 1, (vec3[]){{x, 0, z}}, 0);

    dvz_glyph_angle(visual, 0, 1, (float[]){M_PI * t}, 0);
    dvz_visual_update(visual);
}

int test_glyph_1(TstSuite* suite)
{
    ANN(suite);

#if !HAS_MSDF
    return 1;
#endif
    VisualTest vt = visual_test_start("glyph", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    // DEBUG
    // const char* text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    // const char* text = "abcdefghijklmnopqrstuvwxyz";
    // const char* text = "dfghijkl!01234";
    const char* text = "Hello world! gih";
    float font_size = 96;
    const uint32_t n = strnlen(text, 4096);
    AT(n > 0);

    // Create the visual.
    DvzVisual* visual = dvz_glyph(vt.batch, 0);

    // Visual allocation.
    dvz_glyph_alloc(visual, n);


    // Glyph positions.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3)); // all zeros
    dvz_glyph_position(visual, 0, n, pos, 0);
    FREE(pos);

    // Glyph colors.
    cvec4* color = dvz_mock_color(n, 255);
    dvz_glyph_color(visual, 0, n, color, 0);
    FREE(color);

    // Background color.
    // dvz_glyph_bgcolor(visual, (vec4){1, 1, 1, .5});


    // Create the atlas.
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);
    DvzAtlas* atlas = dvz_atlas(ttf_size, ttf_bytes);

    // Generate the atlas.
    dvz_atlas_generate(atlas);

    // Upload the atlas texture to the glyph visual.
    dvz_glyph_atlas(visual, atlas);

    // Set the texture coordinates.
    dvz_glyph_ascii(visual, text);


    // For shift and size, we will use a font to compute the layout (positions of the letters).
    DvzFont* font = dvz_font(ttf_size, ttf_bytes);
    dvz_font_size(font, font_size);
    vec4* xywh = dvz_font_ascii(font, text);
    // for (uint32_t i = 0; i < n; i++)
    // {
    //     log_info(
    //         "%c: %.1f\t%.1f\t%.1f\t%.1f", text[i], xywh[i][0], xywh[i][1], xywh[i][2],
    //         xywh[i][3]);
    // }

    // Now we can use the xywh array returned by the font to set the size and shift properties
    // of the glyph vsual.
    dvz_glyph_xywh(visual, 0, n, xywh, (vec2){-250, -20}, 0);
    FREE(xywh);


    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // LATER
    // // Animation.
    // vt.visual = visual;
    // dvz_app_timer(vt.app, 0, 1. / 60., 0);
    // dvz_app_ontimer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_font_destroy(font);
    dvz_atlas_destroy(atlas);

    return 0;
}
