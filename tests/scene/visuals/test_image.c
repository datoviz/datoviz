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
    DvzVisual* visual = dvz_image(vt.batch, 0);

    // Visual allocation.
    dvz_image_alloc(visual, n);

    // Image position.
    dvz_image_position(visual, 0, 1, (vec4[]){{-1, +1, +1, -1}}, 0);

    // Image texture coordinates.
    dvz_image_texcoords(visual, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Texture parameters.
    const uint32_t w = 16;
    const uint32_t h = 8;
    uvec3 shape = {w, h, 1};
    DvzSize size = w * h;

    // Generate the texture data.
    cvec4* tex_data = (cvec4*)calloc(size, sizeof(cvec4));
    for (uint32_t i = 0; i < w * h; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i * 256 / (w * h), tex_data[i]);
    }

    // Create and upload the texture.
    dvz_image_texture(
        visual, shape, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_FILTER_NEAREST, //
        size * sizeof(cvec4), tex_data);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(tex_data);

    return 0;
}
