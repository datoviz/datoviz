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
    int flags = DVZ_MESH_FLAGS_TEXTURED | DVZ_MESH_FLAGS_LIGHTING;
    DvzVisual* visual = dvz_mesh_shape(vt.batch, &shape, flags);


    // Lighting.
    if (flags & DVZ_MESH_FLAGS_LIGHTING)
    {
        dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});

        // Light parameters: ambient, diffuse, specular, exponent.
        dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16});
    }

    // Create and upload the texture.
    if (flags & DVZ_MESH_FLAGS_TEXTURED)
    {
        unsigned long jpg_size = 0;
        unsigned char* jpg_bytes = dvz_resource_texture("crate", &jpg_size);
        ASSERT(jpg_size > 0);
        ANN(jpg_bytes);

        uint32_t jpg_width = 0, jpg_height = 0;
        uint8_t* crate_data = dvz_read_jpg(jpg_size, jpg_bytes, &jpg_width, &jpg_height);
        ASSERT(jpg_width > 0);
        ASSERT(jpg_height > 0);

        dvz_mesh_texture(
            visual, (uvec3){jpg_width, jpg_height, 1}, DVZ_FORMAT_R8G8B8A8_UNORM,
            DVZ_FILTER_NEAREST, jpg_size * sizeof(cvec4), crate_data);

        FREE(crate_data);
    }


    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    dvz_shape_destroy(&shape);

    return 0;
}
