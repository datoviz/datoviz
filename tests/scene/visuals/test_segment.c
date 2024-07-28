/*************************************************************************************************/
/*  Testing segment                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_segment.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/segment.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Segment tests                                                                                */
/*************************************************************************************************/

int test_segment_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("segment", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 32;

    // Create the visual.
    DvzVisual* visual = dvz_segment(vt.batch, 0);

    // Visual allocation.
    dvz_segment_alloc(visual, n);

    // Setting the visual's data.
    float t = 0, r = .75;
    float aspect = WIDTH / (float)HEIGHT;
    AT(aspect > 0);

    vec3* initial = (vec3*)calloc(n, sizeof(vec3));
    vec3* terminal = (vec3*)calloc(n, sizeof(vec3));
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    float* linewidth = (float*)calloc(n, sizeof(float));
    DvzCapType* initial_cap = (DvzCapType*)calloc(n, sizeof(DvzCapType));
    DvzCapType* terminal_cap = (DvzCapType*)calloc(n, sizeof(DvzCapType));

    for (uint32_t i = 0; i < n; i++)
    {
        t = .5 * i / (float)n;
        initial[i][0] = r * cos(M_2PI * t);
        initial[i][1] = aspect * r * sin(M_2PI * t);

        terminal[i][0] = -initial[i][0];
        terminal[i][1] = -initial[i][1];

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, color[i]);
        color[i][3] = 216;

        linewidth[i] = 10.0f;

        initial_cap[i] = i % DVZ_CAP_COUNT;
        terminal_cap[i] = i % DVZ_CAP_COUNT;
    }

    dvz_segment_position(visual, 0, n, initial, terminal, 0);
    dvz_segment_color(visual, 0, n, color, 0);
    dvz_segment_linewidth(visual, 0, n, linewidth, 0);
    dvz_segment_cap(visual, 0, n, initial_cap, terminal_cap, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(initial);
    FREE(terminal);
    FREE(color);
    FREE(linewidth);

    return 0;
}
