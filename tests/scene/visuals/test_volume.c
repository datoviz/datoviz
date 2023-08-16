/*************************************************************************************************/
/*  Testing volume                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_volume.h"
#include "renderer.h"
#include "request.h"
#include "scene/app.h"
#include "scene/camera.h"
#include "scene/dual.h"
#include "scene/scene.h"
#include "scene/scene_testing_utils.h"
#include "scene/shape.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/visual_test.h"
#include "scene/visuals/volume.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Volume tests                                                                                 */
/*************************************************************************************************/

int test_volume_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start(VISUAL_TEST_ARCBALL);

    // Volume visual.
    DvzVisual* volume = dvz_volume(vt.rqr, 0);
    dvz_volume_alloc(volume, 1);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, volume);

    // Create texture.
    uint32_t a = 7;
    uint32_t b = a;
    uint32_t c = a;
    uvec3 shape = {a, b, c};
    DvzId tex = dvz_create_tex(vt.rqr, DVZ_TEX_3D, DVZ_FORMAT_R8_UNORM, shape, 0).id;
    DvzId sampler =
        dvz_create_sampler(vt.rqr, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT).id;

    // Bind texture to the visual.
    dvz_visual_tex(volume, 3, tex, sampler, DVZ_ZERO_OFFSET);

    // Update the texture data.
    DvzSize size = a * b * c;
    uint8_t* tex_data = (uint8_t*)calloc(size, sizeof(uint8_t));
    memset(tex_data, 2, size);
    uint32_t a0 = a / 2;
    uint32_t d0 = 1; // 3;
    for (uint32_t i = a0 - d0; i <= a0 + d0; i++)
        for (uint32_t j = a0 - d0; j <= a0 + d0; j++)
            for (uint32_t k = a0 - d0; k <= a0 + d0; k++)
            {
                tex_data[b * c * i + c * j + k] = 10;
            }

    // Upload the texture data.
    dvz_upload_tex(vt.rqr, tex, DVZ_ZERO_OFFSET, shape, size, tex_data);

    visual_test_end(vt);

    FREE(tex_data);
    return 0;
}
