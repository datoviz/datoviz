/*************************************************************************************************/
/*  Testing monoglyph                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_monoglyph.h"
#include "renderer.h"
#include "request.h"
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

int test_monoglyph_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("monoglyph", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_monoglyph(vt.batch, 0);

    // Visual allocation.
    dvz_monoglyph_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos2D(n, 0.25);
    dvz_monoglyph_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = dvz_mock_color(n, 128);
    dvz_monoglyph_color(visual, 0, n, color, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);

    return 0;
}
