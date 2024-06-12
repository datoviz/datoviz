/*************************************************************************************************/
/*  Testing volume                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_volume.h"
#include "gui.h"
#include "presenter.h"
#include "renderer.h"
#include "request.h"
#include "scene/app.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/dual.h"
#include "scene/scene.h"
#include "scene/scene_testing_utils.h"
#include "scene/shape.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/basic.h"
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
    VisualTest vt = visual_test_start("volume", VISUAL_TEST_ARCBALL, DVZ_CANVAS_FLAGS_VSYNC);

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



static inline void _gui_callback(DvzApp* app, DvzId canvas_id, void* user_data)
{
    DvzVisual* visual = (DvzVisual*)user_data;
    ANN(visual);

    dvz_gui_pos((vec2){50, 50}, DVZ_DIALOG_DEFAULT_PIVOT);
    dvz_gui_size((vec2){250, 80});
    dvz_gui_begin("Parameters", 0);

    float* param = visual->user_data;
    ANN(param);

    if (dvz_gui_slider("param", 0, 10, param))
    {
        dvz_visual_param(visual, 2, 3, (vec4){*param, 0, 0, 0});
        dvz_visual_update(visual);
    }

    dvz_gui_end();
}

int test_volume_2(TstSuite* suite)
{
    VisualTest vt = visual_test_start(
        "volume", VISUAL_TEST_ARCBALL, DVZ_CANVAS_FLAGS_VSYNC | DVZ_CANVAS_FLAGS_IMGUI);

    // Volume visual.
    float param = 1.0;
    {
        DvzVisual* visual = dvz_volume(vt.batch, DVZ_VOLUME_FLAGS_RGBA);
        dvz_volume_alloc(visual, 1);

        // Volume parameters.
        double scaling = 1 / (float)MOUSE_D;
        dvz_volume_size(visual, MOUSE_W * scaling, MOUSE_H * scaling, 1);

        // Add the visual to the panel AFTER setting the visual's data.
        dvz_panel_visual(vt.panel, visual);

        // Create the texture and upload the volume data.
        uvec3 shape = {0};
        DvzId tex = load_brain_volume(vt.batch, shape, true);

        // Bind the volume texture to the visual.
        dvz_volume_texture(visual, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

        // GUI callback.
        visual->user_data = &param;
        dvz_app_gui(vt.app, vt.figure->canvas_id, _gui_callback, visual);
    }

    // Image visual.
    {
        DvzVisual* image = dvz_image(vt.batch, 0);

        // Visual allocation.
        dvz_image_alloc(image, 1);

        // Image position.
        float a = .25;
        dvz_image_position(image, 0, 1, (vec4[]){{-a, +a, +a, -a}}, 0);

        // Image texture coordinates.
        dvz_image_texcoords(image, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

        // Add the visual to the panel AFTER setting the visual's data.
        dvz_panel_visual(vt.panel, image);

        // Create and upload the texture.
        uvec3 tex_shape = {0};
        DvzId tex_img = load_crate_texture(vt.batch, tex_shape);

        dvz_image_texture(image, tex_img, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);
    }

    dvz_arcball_initial(vt.arcball, (vec3){-2.4, +.7, +1.5});
    dvz_camera_initial(vt.camera, (vec3){0, 0, 1.5}, vt.camera->lookat, vt.camera->up);
    dvz_panel_update(vt.panel);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
