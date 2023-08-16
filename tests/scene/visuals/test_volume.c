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
#include "scene/visuals/volume.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Volume tests                                                                                 */
/*************************************************************************************************/

int test_volume_1(TstSuite* suite)
{
    // Create app objects.
    DvzApp* app = dvz_app(0);
    DvzRequester* rqr = dvz_app_requester(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(rqr);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_VSYNC);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Arcball.
    DvzArcball* arcball = dvz_panel_arcball(app, panel);
    ANN(arcball);

    // Perspective camera.
    DvzCamera* camera = dvz_panel_camera(panel);

    // Volume visual.
    DvzVisual* volume = dvz_volume(rqr, 0);
    dvz_volume_alloc(volume, 1);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, volume);

    // Create texture.
    uint32_t a = 7;
    uint32_t b = a;
    uint32_t c = a;
    uvec3 shape = {a, b, c};
    DvzId tex = dvz_create_tex(rqr, DVZ_TEX_3D, DVZ_FORMAT_R8_UNORM, shape, 0).id;
    DvzId sampler =
        dvz_create_sampler(rqr, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT).id;

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
    dvz_upload_tex(rqr, tex, DVZ_ZERO_OFFSET, shape, size, tex_data);

    // Run the scene.
    dvz_scene_run(scene, app, N_FRAMES);

    // Cleanup.
    dvz_camera_destroy(camera);
    dvz_panel_destroy(panel);
    dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    FREE(tex_data);
    return 0;
}
