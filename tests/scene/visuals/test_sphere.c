/*************************************************************************************************/
/*  Testing sphere                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_sphere.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/sphere.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Sphere tests                                                                                 */
/*************************************************************************************************/

int test_sphere_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("sphere", VISUAL_TEST_ARCBALL, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_sphere(vt.batch, 0);

    // Visual allocation.
    dvz_sphere_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos3D(n, 0.25);
    dvz_sphere_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = dvz_mock_color(n, 255);
    dvz_sphere_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 50, 100);
    dvz_sphere_size(visual, 0, n, size, 0);

    // Light position.
    dvz_sphere_light_pos(visual, (vec3){-1, +1, +10});

    // Light parameters.
    dvz_sphere_light_params(visual, (vec4){.3, .6, 2, 32});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}
