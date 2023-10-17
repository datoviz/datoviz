/*************************************************************************************************/
/*  Testing basic                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_basic.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/basic.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Basic tests                                                                                  */
/*************************************************************************************************/

int test_basic_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("basic", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 30000;

    // Create the visual.
    DvzVisual* visual = dvz_basic(vt.batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);

    // Visual allocation.
    dvz_basic_alloc(visual, n);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_basic_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 128;
    }
    dvz_basic_color(visual, 0, n, color, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);

    return 0;
}
