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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MOUSE_W 320
#define MOUSE_H 456
#define MOUSE_D 528



/*************************************************************************************************/
/*  Volume tests                                                                                 */
/*************************************************************************************************/

int test_volume_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("volume", VISUAL_TEST_ARCBALL);

    // Volume visual.
    DvzVisual* visual =
        dvz_volume(vt.batch, DVZ_VOLUME_FLAGS_COLORMAP | DVZ_VOLUME_FLAGS_BACK_FRONT);
    dvz_volume_alloc(visual, 1);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Texture parameters.
    uint32_t a = 7;
    uint32_t b = a;
    uint32_t c = a;
    uint32_t a0 = a / 2;
    uint32_t d0 = 1;
    DvzSize size = a * b * c * sizeof(uint8_t);

    // Generate the texture data.
    uint8_t* tex_data = (uint8_t*)calloc(size, sizeof(uint8_t));
    memset(tex_data, 2, size);
    for (uint32_t i = a0 - d0; i <= a0 + d0; i++)
        for (uint32_t j = a0 - d0; j <= a0 + d0; j++)
            for (uint32_t k = a0 - d0; k <= a0 + d0; k++)
            {
                tex_data[b * c * i + c * j + k] = 10;
            }

    // Create the texture and upload the volume data.
    DvzId tex = dvz_tex_volume(vt.batch, DVZ_FORMAT_R8_UNORM, a, b, c, tex_data);

    // Bind the volume texture to the visual.
    dvz_volume_texture(visual, tex, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);

    visual_test_end(vt);

    FREE(tex_data);
    return 0;
}



static DvzId _load_brain_volume(VisualTest* vt, bool use_rgb_volume)
{
    char path[1024];
    snprintf(
        path, sizeof(path), "%s/%s%s.npy", DATA_DIR, "allen_mouse_brain",
        use_rgb_volume ? "_rgba" : "");

    DvzSize size = 0;
    char* volume = dvz_read_npy(path, &size);

    if (!volume)
    {
        log_error("file not found: %s", path);
        visual_test_end(*vt);
        return 0;
    }

    log_info("load the Allen Mouse Brain volume (%s)", pretty_size(size));
    DvzFormat format = use_rgb_volume ? DVZ_FORMAT_R8G8B8A8_UNORM : DVZ_FORMAT_R16_UNORM;
    DvzId tex = dvz_tex_volume(vt->batch, format, MOUSE_W, MOUSE_H, MOUSE_D, volume);
    FREE(volume);

    return tex;
}

int test_volume_2(TstSuite* suite)
{
    VisualTest vt = visual_test_start("volume", VISUAL_TEST_ARCBALL);

    // Volume visual.
    DvzVisual* visual = dvz_volume(vt.batch, DVZ_VOLUME_FLAGS_RGBA);
    dvz_volume_alloc(visual, 1);

    // Volume parameters.
    double scaling = 1 / (float)MOUSE_D;
    dvz_volume_size(visual, MOUSE_W * scaling, MOUSE_H * scaling, 1);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Create the texture and upload the volume data.
    DvzId tex = _load_brain_volume(&vt, true);

    // Bind the volume texture to the visual.
    dvz_volume_texture(visual, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    visual_test_end(vt);

    return 0;
}
