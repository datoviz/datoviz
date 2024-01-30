/*************************************************************************************************/
/*  Testing axis                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_axis.h"
#include "scene/axis.h"
#include "scene/panzoom.h"
#include "scene/viewport.h"
#include "scene/visuals/marker.h"
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

    VisualTest vt = visual_test_start("axis_1", VISUAL_TEST_PANZOOM, DVZ_CANVAS_FLAGS_FPS);

    // Create the visual.
    int flags = 0;
    DvzAxis* axis = dvz_axis(vt.batch, flags);

    // Global parameters.
    float font_size = 48;
    vec2 anchor = {-.5, -1.25};

    float a = .8;
    vec3 p0 = {-a, -a, 0};
    vec3 p1 = {+a, -a, 0};
    vec3 p2 = {-a, +a, 0};
    vec3 p3 = {+a, +a, 0};

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
    dvz_axis_anchor(axis, anchor);
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

    float m = 40;
    dvz_panel_margins(vt.panel, m, m, m, m);
    dvz_visual_clip(axis->glyph, DVZ_VIEWPORT_CLIP_OUTER);
    dvz_visual_clip(axis->segment, DVZ_VIEWPORT_CLIP_OUTER);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_axis_panel(axis, vt.panel);

    // Run the test.
    visual_test_end(vt);

    dvz_axis_destroy(axis);
    return 0;
}



int test_axis_2(TstSuite* suite)
{
    ANN(suite);

    VisualTest vt = visual_test_start(
        "axis_2", VISUAL_TEST_PANZOOM, DVZ_CANVAS_FLAGS_FPS | DVZ_RENDERER_FLAGS_WHITE_BACKGROUND);


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


    // Create the visual.
    int flags = 0;
    DvzAxis* haxis = dvz_axis(vt.batch, flags);
    DvzAxis* vaxis = dvz_axis(vt.batch, flags);


    // Global parameters.
    float font_size = 32;

    cvec4 color_glyph = {0, 0, 0, 255};
    cvec4 color_lim = {0, 0, 0, 255};
    cvec4 color_grid = {0, 0, 0, 255};
    cvec4 color_major = {0, 0, 0, 255};
    cvec4 color_minor = {0, 0, 0, 255};

    float width_lim = 4;
    float width_grid = 2;
    float width_major = 4;
    float width_minor = 2;

    float length_lim = 1;
    float length_grid = 1;
    float length_major = 40;
    float length_minor = 20;


    // Horizontal
    float ha = 1.0;
    vec3 hp0 = {-ha, -ha, 0};
    vec3 hp1 = {+ha, -ha, 0};
    vec3 hp2 = {-ha, +ha, 0};
    vec3 hp3 = {+ha, +ha, 0};
    vec2 hanchor = {-.5, -1.25};
    vec2 hoffset = {0, -50};

    dvz_axis_size(haxis, font_size);
    dvz_axis_anchor(haxis, hanchor);
    dvz_axis_offset(haxis, hoffset);
    dvz_axis_width(haxis, width_lim, width_grid, width_major, width_minor);
    dvz_axis_length(haxis, length_lim, length_grid, length_major, length_minor);
    dvz_axis_color(haxis, color_glyph, color_lim, color_grid, color_major, color_minor);

    dvz_axis_pos(haxis, dmin, dmax, hp0, hp1, hp2, hp3);
    dvz_axis_set(haxis, tick_count, values, glyph_count, glyphs, index, length);

    dvz_visual_fixed(haxis->glyph, false, true, false);
    dvz_visual_fixed(haxis->segment, false, true, false);
    dvz_visual_clip(haxis->glyph, DVZ_VIEWPORT_CLIP_BOTTOM);
    dvz_visual_clip(haxis->segment, DVZ_VIEWPORT_CLIP_BOTTOM);


    // Vertical.
    float va = 1.0;
    vec3 vp0 = {-va, -va, 0};
    vec3 vp1 = {-va, +va, 0};
    vec3 vp2 = {+va, -va, 0};
    vec3 vp3 = {+va, +va, 0};
    vec2 vanchor = {-1, 0};
    vec2 voffset = {-50, -18};

    dvz_axis_size(vaxis, font_size);
    dvz_axis_anchor(vaxis, vanchor);
    dvz_axis_offset(vaxis, voffset);
    dvz_axis_width(vaxis, width_lim, width_grid, width_major, width_minor);
    dvz_axis_length(vaxis, length_lim, length_grid, length_major, length_minor);
    dvz_axis_color(vaxis, color_glyph, color_lim, color_grid, color_major, color_minor);

    dvz_axis_pos(vaxis, dmin, dmax, vp0, vp1, vp2, vp3);
    dvz_axis_set(vaxis, tick_count, values, glyph_count, glyphs, index, length);

    dvz_visual_fixed(vaxis->glyph, true, false, false);
    dvz_visual_fixed(vaxis->segment, true, false, false);
    dvz_visual_clip(vaxis->glyph, DVZ_VIEWPORT_CLIP_LEFT);
    dvz_visual_clip(vaxis->segment, DVZ_VIEWPORT_CLIP_LEFT);



    // Number of items.
    const uint32_t n = 1000000;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_FILLED);
    dvz_marker_shape(visual, DVZ_MARKER_SHAPE_DISC);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 192;
    }
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        size[i] = 20 + 30 * dvz_rand_float();
    }
    dvz_marker_size(visual, 0, n, size, 0);

    // Parameters.
    dvz_marker_edge_color(visual, (cvec4){255, 255, 255, 255});
    dvz_marker_edge_width(visual, (float){3.0});

    dvz_visual_clip(visual, DVZ_VIEWPORT_CLIP_OUTER);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);



    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_margins(vt.panel, 20, 20, 120, 120);
    dvz_axis_panel(haxis, vt.panel);
    dvz_axis_panel(vaxis, vt.panel);

    // Run the test.
    visual_test_end(vt);

    dvz_axis_destroy(haxis);
    dvz_axis_destroy(vaxis);
    return 0;
}
