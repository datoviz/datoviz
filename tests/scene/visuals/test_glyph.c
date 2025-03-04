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

static void _on_timer(DvzApp* app, DvzId window_id, DvzTimerEvent* ev)
{
    VisualTest* vt = (VisualTest*)ev->user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);
    float t = ev->time;

    uint32_t n = vt->n;
    ASSERT(n > 0);

    float a = cos(.25 * 2 * M_PI * t);
    float b = sin(.25 * 2 * M_PI * t);
    // log_info("%f", b);
    vec2* anchor = (vec2*)_repeat(n, sizeof(vec2), (vec2){a, b});
    dvz_glyph_anchor(visual, 0, n, anchor, 0);
    FREE(anchor);

    dvz_visual_update(visual);
}

int test_glyph_1(TstSuite* suite, TstItem* tstitem)
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
    const char* text = "Hello world!";
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
    DvzColor* color = dvz_mock_color(n, ALPHA_MAX);
    dvz_glyph_color(visual, 0, n, color, 0);
    FREE(color);

    // Background color.
    // dvz_glyph_bgcolor(visual, (vec4){1, 1, 1, .5});


    // Create the atlas.
    DvzAtlasFont af = {0};
    dvz_atlas_font(font_size, &af);

    // Upload the atlas texture to the glyph visual.
    dvz_glyph_atlas_font(visual, &af);

    // Set the texture coordinates.
    dvz_glyph_ascii(visual, text);


    // For shift and size, we will use a font to compute the layout (positions of the letters).
    vec4* xywh = (vec4*)calloc(n, sizeof(vec4));
    dvz_font_ascii(af.font, text, xywh);
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

    vec2* group_size = (vec2*)_repeat(n, sizeof(vec2), (vec2){300, 50});
    dvz_glyph_group_size(visual, 0, n, group_size, 0);
    FREE(group_size);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Animation.
    vt.visual = visual;
    vt.n = n;
    dvz_app_timer(vt.app, 0, 1. / 60., 0);
    dvz_app_on_timer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_font_destroy(af.font);
    dvz_atlas_destroy(af.atlas);

    return 0;
}



static void _set_strings_1(DvzVisual* visual)
{
    uint32_t string_count = 5;
    char* strings[] = {"Hello", "world", "how", "are", "you"};
    vec3 string_positions[] = {
        {-.5, +.5, 0}, {+.5, +.5, 0}, {0, 0, 0}, {-.5, -.5, 0}, {+.5, -.5, 0}};
    dvz_glyph_strings(
        visual, string_count, strings, string_positions, NULL, //
        (DvzColor){YELLOW}, (vec2){0, 0}, (vec2){0, 0});
}

static void _set_strings_2(DvzVisual* visual)
{
    uint32_t string_count = 2;
    char* strings[] = {"string 1/2", "string 2/2"};
    vec3 string_positions[] = {{-.5, -.5, 0}, {0, +.5, 0}};

    dvz_glyph_strings(
        visual, string_count, strings, string_positions, NULL, //
        (DvzColor){YELLOW}, (vec2){0, 0}, (vec2){0, 0});
}

static void _switch_strings(DvzApp* app, DvzId window_id, DvzMouseEvent* ev)
{
    ANN(app);

    VisualTest* vt = (VisualTest*)ev->user_data;
    ANN(vt);

    if (ev->type == DVZ_MOUSE_EVENT_CLICK)
    {
        if (vt->m % 2 == 1)
            _set_strings_1(vt->visual);
        else
            _set_strings_2(vt->visual);
        vt->m++;
    }
}

int test_glyph_strings(TstSuite* suite)
{
    ANN(suite);

#if !HAS_MSDF
    return 1;
#endif
    VisualTest vt = visual_test_start("glyph", VISUAL_TEST_PANZOOM, 0);

    // Create the visual.
    DvzVisual* visual = dvz_glyph(vt.batch, 0);

    // Create the atlas.
    float font_size = 48;
    DvzAtlasFont af = {0};
    dvz_atlas_font(font_size, &af);
    dvz_glyph_atlas_font(visual, &af);

    _set_strings_1(visual);

    // Add the visual to the panel AFTER setting the visual's data.
    vt.visual = visual;
    dvz_panel_visual(vt.panel, visual, 0);

    vt.m = 0;
    dvz_app_on_mouse(vt.app, _switch_strings, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_font_destroy(af.font);
    dvz_atlas_destroy(af.atlas);

    return 0;
}
