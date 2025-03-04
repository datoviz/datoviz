/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing volume                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_volume.h"
#include "app.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "gui.h"
#include "presenter.h"
#include "renderer.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/dual.h"
#include "scene/scene.h"
#include "scene/scene_testing_utils.h"
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

int test_volume_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("volume", VISUAL_TEST_ARCBALL, 0);

    // Volume visual.
    DvzVisual* visual = dvz_volume(vt.batch, DVZ_VOLUME_FLAGS_RGBA | DVZ_VOLUME_FLAGS_BACK_FRONT);

    float x = .75;
    float y = .5;
    float z = .25;
    dvz_volume_bounds(visual, (vec2){-x, +x}, (vec2){-y, +y}, (vec2){-z, +z});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Texture data.
    uint8_t a = 10;
    cvec4 tex_data[] = {
        {255, 0, 0, a},   //
        {0, 255, 0, a},   //
        {0, 0, 255, a},   //
        {255, 255, 0, a}, //
        {255, 0, 255, a}, //
        {0, 255, 255, a}, //
    };

    // Create the texture and upload the volume data.
    DvzTexture* texture = dvz_texture_3D(
        vt.batch, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT,
        3, 2, 1, tex_data, 0);

    // Bind the volume texture to the visual.
    dvz_volume_texture(visual, texture);

    visual_test_end(vt);

    return 0;
}



static inline void _gui_callback(DvzApp* app, DvzId canvas_id, DvzGuiEvent* ev)
{
    VisualTest* vt = (VisualTest*)ev->user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);

    dvz_gui_pos((vec2){50, 50}, DVZ_DIALOG_DEFAULT_PIVOT);
    dvz_gui_size((vec2){250, 80});
    dvz_gui_begin("Parameters", 0);

    vec4* param = (vec4*)visual->user_data;
    ANN(param);

    if (dvz_gui_slider("param", 0, 10, &param[0][0]))
    {
        dvz_volume_transfer(visual, *param);
    }

    dvz_gui_end();
}

int test_volume_2(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("volume", VISUAL_TEST_ARCBALL, DVZ_CANVAS_FLAGS_IMGUI);

    // Volume visual.
    vec4 param = {1, 0, 0, 0};
    {
        DvzVisual* visual = dvz_volume(vt.batch, DVZ_VOLUME_FLAGS_RGBA);

        // Volume parameters.
        double scaling = .5 / (float)MOUSE_D;
        float z = MOUSE_W * scaling;
        float x = MOUSE_H * scaling;
        float y = MOUSE_D * scaling;
        dvz_volume_bounds(visual, (vec2){-x, +x}, (vec2){-y, +y}, (vec2){-z, +z});

        dvz_volume_permutation(visual, (ivec3){2, 0, 1});

        // Add the visual to the panel AFTER setting the visual's data.
        dvz_panel_visual(vt.panel, visual, 0);

        // Create the texture and upload the volume data.
        uvec3 shape = {0};
        DvzTexture* texture = load_brain_volume(vt.batch, shape, true);

        if (texture != NULL)
        {
            // Bind the volume texture to the visual.
            dvz_volume_texture(visual, texture);
        }

        // GUI callback.
        vt.visual = visual;
        visual->user_data = &param;
        dvz_app_gui(vt.app, vt.figure->canvas_id, _gui_callback, &vt);
    }

    // Image visual.
    // if (0)
    // {
    //     DvzVisual* image = dvz_image(vt.batch, 0);

    //     // Visual allocation.
    //     dvz_image_alloc(image, 1);

    //     // Image position.
    //     float a = .25;
    //     dvz_image_position(image, 0, 1, (vec4[]){{-a, +a, +a, -a}}, 0);

    //     // Image texture coordinates.
    //     dvz_image_texcoords(image, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);

    //     // Add the visual to the panel AFTER setting the visual's data.
    //     dvz_panel_visual(vt.panel, image, 0);

    //     // Create and upload the texture.
    //     uvec3 tex_shape = {0};
    //     DvzTexture* tex_img = load_crate_texture(vt.batch, tex_shape);

    //     dvz_image_texture(image, tex_img, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);
    // }

    dvz_arcball_gui(vt.arcball, vt.app, vt.figure->canvas_id, vt.panel);

    dvz_arcball_initial(vt.arcball, (vec3){2, .2, .9});
    dvz_camera_initial(vt.camera, (vec3){0, 0, 1.5}, vt.camera->lookat, vt.camera->up);
    dvz_panel_update(vt.panel);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
