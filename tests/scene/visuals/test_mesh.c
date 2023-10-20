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

    // Shape.
    DvzShape shape = dvz_shape_cube((cvec4[]){
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {0, 255, 255, 255},
        {255, 0, 255, 255},
        {255, 255, 0, 255},
    });

    // Create the visual.
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, DVZ_MESH_FLAGS_TEXTURED);

    // Light position
    dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});

    // Light parameters: ambient, diffuse, specular, exponent.
    dvz_mesh_light_params(visual, (vec4){.2, .5, .3, 32});


    // Texture parameters.
    const uint32_t w = 16;
    const uint32_t h = 16;
    uvec3 tex_shape = {w, h, 1};
    DvzSize size = w * h;

    // Generate the texture data.
    cvec4* tex_data = (cvec4*)calloc(size, sizeof(cvec4));
    for (uint32_t i = 0; i < w * h; i++)
    {
        ASSERT(i < 256);
        dvz_colormap(DVZ_CMAP_HSV, i, tex_data[i]);
    }

    // Create and upload the texture.
    dvz_mesh_texture(
        visual, tex_shape, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_FILTER_NEAREST, //
        size * sizeof(cvec4), tex_data);


    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);
    FREE(tex_data);

    return 0;
}
