/*************************************************************************************************/
/*  Testing image                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_image.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/image.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Image tests                                                                                  */
/*************************************************************************************************/

int test_image_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("image", VISUAL_TEST_PANZOOM, 0);

    // Create the visual.
    DvzVisual* visual = dvz_image(vt.batch, 0);

    // Visual allocation.
    dvz_image_alloc(visual, 1);

    // Image position.
    dvz_image_position(visual, 0, 1, (vec4[]){{-1, +1, +1, -1}}, 0);

    // Image texture coordinates.
    dvz_image_texcoords(visual, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Create and upload the texture.
    uvec3 tex_shape = {0};
    DvzId tex = load_crate_texture(vt.batch, tex_shape);

    dvz_image_texture(visual, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
