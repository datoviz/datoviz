/*************************************************************************************************/
/*  Testing point                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_point.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/point.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Point tests                                                                                  */
/*************************************************************************************************/

int test_point_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("point", VISUAL_TEST_PANZOOM, DVZ_CANVAS_FLAGS_VSYNC);

    // Number of items.
    const uint32_t n = 10000;

    // Create the visual.
    DvzVisual* visual = dvz_point(vt.batch, 0);

    // Visual allocation.
    dvz_point_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos2D(n, 0.25);
    dvz_point_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = dvz_mock_color(n, 128);
    dvz_point_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 1, 50);
    dvz_point_size(visual, 0, n, size, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}
