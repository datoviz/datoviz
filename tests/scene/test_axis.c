/*************************************************************************************************/
/*  Testing axis                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axis.h"
#include "scene/axis.h"
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

    DvzVisual* segment = dvz_axis_segment(axis);
    DvzVisual* glyph = dvz_axis_glyph(axis);

    // Global parameters.
    float font_size = 32;

    vec3 p0 = {-.9, 0, 0};
    vec3 p1 = {+.9, 0, 0};
    vec3 p2 = {-.9, +.9, 0};
    vec3 p3 = {+.9, +.9, 0};

    cvec4 lim = {255, 0, 0, 255};
    cvec4 grid = {0, 255, 0, 255};
    cvec4 major = {255, 255, 255, 255};
    cvec4 minor = {255, 255, 0, 255};

    dvz_axis_size(axis, font_size);
    dvz_axis_width(axis, 4, 8, 12, 16);
    dvz_axis_color(axis, lim, grid, major, minor);

    // Set the ticks and labels.
    uint32_t tick_count = 3;
    uint32_t glyph_count = 3;
    uint32_t index[3] = {0, 1, 2};
    uint32_t length[3] = {1, 1, 1};

    char* glyphs = "012";
    double values[3] = {0, 1, 2};
    double dmin = 0;
    double dmax = 2;

    dvz_axis_pos(axis, dmin, dmax, p0, p1, p2, p3);
    dvz_axis_set(axis, tick_count, values, glyph_count, glyphs, index, length);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, segment);
    dvz_panel_visual(vt.panel, glyph);

    // Run the test.
    visual_test_end(vt);

    dvz_axis_destroy(axis);
    return 0;
}
