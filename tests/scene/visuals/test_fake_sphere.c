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
    VisualTest vt = visual_test_start("fake_sphere", VISUAL_TEST_ARCBALL, DVZ_CANVAS_FLAGS_VSYNC);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_fake_sphere(vt.batch, 0);

    // Visual allocation.
    dvz_fake_sphere_alloc(visual, n);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
        pos[i][2] = .25 * dvz_rand_normal();
    }
    dvz_fake_sphere_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
    }
    dvz_fake_sphere_color(visual, 0, n, color, 0);

    // Size.
    float* size = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        size[i] = .1 + .2 * dvz_rand_float();
    }
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
