/*************************************************************************************************/
/*  Testing fake sphere                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_fake_sphere.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/fake_sphere.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Fake sphere tests                                                                            */
/*************************************************************************************************/

int test_fake_sphere_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("fake_sphere", VISUAL_TEST_ARCBALL, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_fake_sphere(vt.batch, 0);

    // Visual allocation.
    dvz_fake_sphere_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos3D(n, 0.25);
    dvz_fake_sphere_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = dvz_mock_color(n, 128);
    dvz_fake_sphere_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 0.1, 0.3);
    dvz_fake_sphere_size(visual, 0, n, size, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Light position
    dvz_fake_sphere_light_pos(visual, (vec3){-1, +1, +10});

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}
