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
    VisualTest vt = visual_test_start("image", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 10000;

    // Create the visual.
    DvzVisual* visual = dvz_image(vt.rqr, 0);

    // Visual allocation.
    dvz_image_alloc(visual, n);

    // Image position.
    dvz_image_position(visual, 0, 1, (vec4[]){{-1, +1, +1, -1}}, 0);

    // Image texture coordinates.
    dvz_image_texcoords(visual, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Create texture.
    const uint32_t w = 16;
    const uint32_t h = 8;
    uvec3 shape = {w, h, 1};
    DvzId tex = dvz_create_tex(vt.rqr, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, shape, 0).id;
    DvzId sampler =
        dvz_create_sampler(vt.rqr, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT).id;

    // Bind texture to the visual.
    dvz_visual_tex(visual, 2, tex, sampler, DVZ_ZERO_OFFSET);

    // Update the texture data.
    DvzSize size = w * h;
    cvec4* tex_data = (cvec4*)calloc(size, sizeof(cvec4));
    for (uint32_t i = 0; i < w * h; i++)
    {
        // dvz_colormap(DVZ_CMAP_HSV, i % 256, tex_data[i]);
        dvz_colormap(DVZ_CMAP_HSV, i * 256 / (w * h), tex_data[i]);
    }
    // dvz_rand_byte();

    // Upload the texture data.
    dvz_upload_tex(vt.rqr, tex, DVZ_ZERO_OFFSET, shape, size * sizeof(cvec4), tex_data);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(tex_data);

    return 0;
}
