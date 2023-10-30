/*************************************************************************************************/
/*  Testing glyph                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_glyph.h"
#include "renderer.h"
#include "request.h"
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

int test_glyph_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("glyph", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 1;

    // Create the visual.
    DvzVisual* visual = dvz_glyph(vt.batch, 0);

    // Visual allocation.
    dvz_glyph_alloc(visual, n);

    // Image position.
    dvz_glyph_position(visual, 0, 1, (vec3[]){{0, 0, 0}}, 0);

    dvz_glyph_size(visual, (vec2){.1, .1});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
