/*************************************************************************************************/
/*  Testing axis                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axis.h"
#include "scene/axis.h"
#include "scene/panzoom.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Axis test utils                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Axis tests                                                                                   */
/*************************************************************************************************/

int test_axis_1(TstSuite* suite)
{
    ANN(suite);

    VisualTest vt = visual_test_start("axis", VISUAL_TEST_PANZOOM, DVZ_CANVAS_FLAGS_FPS);

    // Create the visual.
    int flags = 0;
    DvzAxis* axis = dvz_axis(vt.batch, flags);

    // Global parameters.
    float font_size = 48;

    vec3 p0 = {-.9, 0, 0};
    vec3 p1 = {+.9, 0, 0};
    vec3 p2 = {-.9, +.9, 0};
    vec3 p3 = {+.9, +.9, 0};

    cvec4 color_glyph = {255, 255, 0, 255};
    cvec4 color_lim = {255, 0, 0, 255};
    cvec4 color_grid = {0, 255, 0, 255};
    cvec4 color_major = {255, 255, 255, 255};
    cvec4 color_minor = {255, 0, 255, 255};

    float width_lim = 4;
    float width_grid = 2;
    float width_major = 4;
    float width_minor = 2;

    float length_lim = 1;
    float length_grid = 1;
    float length_major = 40;
    float length_minor = 20;

    dvz_axis_size(axis, font_size);
    dvz_axis_width(axis, width_lim, width_grid, width_major, width_minor);
    dvz_axis_length(axis, length_lim, length_grid, length_major, length_minor);
    dvz_axis_color(axis, color_glyph, color_lim, color_grid, color_major, color_minor);

    // Set the ticks and labels.
    double dmin = 0;
    double dmax = 7;

    uint32_t tick_count = 8;
    double values[] = {0, 1, 2, 3, 4, 5, 6, 7};

    char* glyphs = "0 1 2 3 4 5 6 7 ";
    uint32_t glyph_count = strnlen(glyphs, 1024) / 2;
    AT(glyph_count == tick_count);
    uint32_t index[] = {0, 2, 4, 6, 8, 10, 12, 14};
    uint32_t length[] = {1, 1, 1, 1, 1, 1, 1, 1};

    dvz_axis_pos(axis, dmin, dmax, p0, p1, p2, p3);
    dvz_axis_set(axis, tick_count, values, glyph_count, glyphs, index, length);

    dvz_visual_fixed(axis->glyph, false, true, false);
    dvz_visual_fixed(axis->segment, false, true, false);

    dvz_panel_margins(vt.panel, 100, 100, 100, 100);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_axis_panel(axis, vt.panel);

    // Run the test.
    visual_test_end(vt);

    dvz_axis_destroy(axis);
    return 0;
}
