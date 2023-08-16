/*************************************************************************************************/
/*  Testing mesh                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_mesh.h"
#include "renderer.h"
#include "request.h"
#include "scene/dual.h"
#include "scene/scene_testing_utils.h"
#include "scene/shape.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/mesh.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Mesh tests                                                                                   */
/*************************************************************************************************/

int test_mesh_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("mesh", VISUAL_TEST_ARCBALL);

    // Disc shape parameters.
    const uint32_t count = 30;
    cvec4 color = {255, 0, 0, 255};

    // Disc mesh.
    DvzShape disc = dvz_shape_disc(count, color);

    // Create the visual.
    DvzVisual* visual = dvz_mesh_shape(vt.rqr, &disc);

    // Light position
    dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});

    // Light parameters: ambient, diffuse, specular, exponent.
    dvz_mesh_light_params(visual, (vec4){.2, .5, .3, 32});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&disc);

    return 0;
}
