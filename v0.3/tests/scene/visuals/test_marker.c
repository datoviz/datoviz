/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing marker                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_marker.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/marker.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"

// #include "_profile.h"



/*************************************************************************************************/
/*  Marker tests                                                                                 */
/*************************************************************************************************/

int test_marker_code(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("marker_code", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);
    dvz_marker_shape(visual, DVZ_MARKER_SHAPE_HEART);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(192));
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 10, 50);
    dvz_marker_size(visual, 0, n, size, 0);

    // Angle.
    float* angle = dvz_mock_uniform(n, 0, M_2PI);
    dvz_marker_angle(visual, 0, n, size, 0);

    // Parameters.
    dvz_marker_edgecolor(visual, (DvzColor){WHITE});
    dvz_marker_linewidth(visual, (float){3.0});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);
    FREE(angle);

    return 0;
}



int test_marker_bitmap(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("marker_bitmap", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 100;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);

    // Bitmap marker.
    dvz_marker_mode(visual, DVZ_MARKER_MODE_BITMAP);

    // Create and upload the texture.
    uvec3 tex_shape = {0};
    DvzTexture* texture = load_crate_texture(vt.batch, tex_shape);
    dvz_marker_texture(visual, texture);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 50, 100);
    dvz_marker_size(visual, 0, n, size, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}



static inline float _sdf(float x, float y, float radius)
{
    float d = sqrt(x * x + y * y);
    return d - radius;
}

static void _disc_sdf(DvzVisual* visual, uint32_t size)
{
    ANN(visual);
    DvzSize texsize = size * size * sizeof(float);
    float* texdata = (float*)calloc(texsize, sizeof(float));
    for (uint32_t i = 0; i < texsize; i++)
    {
        uint32_t x = i % size;         // [0, w]
        uint32_t y = i / size;         // [0, w]
        float posX = (x - size / 2.0); // [-w/2, +w/2]
        float posY = (y - size / 2.0); // [-w/2, +w/2]
        float k = 4;
        float value = _sdf(posX / k, posY / k, .25 * size / k);
        texdata[i] = value;
    }

    DvzTexture* texture = dvz_texture_2D(
        visual->batch, DVZ_FORMAT_R32_SFLOAT, DVZ_FILTER_LINEAR,
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, size, size, texdata, 0);
    dvz_marker_texture(visual, texture);
    FREE(texdata);
}

int test_marker_sdf(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("marker_sdf", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);

    // Texture-based disc SDF.
    dvz_marker_mode(visual, DVZ_MARKER_MODE_SDF);

    // Create a SDF texture and assign it to the visual.
    uint32_t tex_size = 100;
    _disc_sdf(visual, tex_size);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(192));
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 25, 100);
    dvz_marker_size(visual, 0, n, size, 0);

    // Angle.
    float* angle = dvz_mock_uniform(n, 0, M_2PI);
    dvz_marker_angle(visual, 0, n, angle, 0);

    // Parameters.
    dvz_marker_edgecolor(visual, (DvzColor){WHITE});
    dvz_marker_linewidth(visual, (float){2.0});
    // IMPORTANT: need to specify the size of the texture when using SDFs.
    dvz_marker_tex_scale(visual, (float){(float)tex_size});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);
    FREE(angle);

    return 0;
}



int test_marker_msdf(TstSuite* suite, TstItem* tstitem)
{
#if !HAS_MSDF
    return 1;
#endif
    VisualTest vt = visual_test_start("marker_msdf", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);
    // dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);

    // Bitmap marker.
    dvz_marker_mode(visual, DVZ_MARKER_MODE_MSDF);

    // Create the MSDF image.
    const char* svg_path =
        "M50,10 L61.8,35.5 L90,42 L69,61 L75,90 L50,75 L25,90 L31,61 L10,42 L38.2,35.5 Z";
    uint32_t w = 100;
    uint32_t h = 100;
    float* msdf = dvz_msdf_from_svg(svg_path, w, h); // 3 floats per pixel, need 4 for Vulkan

    // Save the MSDF to a PNG file.
    // {
    //     uint8_t* rgb = dvz_msdf_to_rgb(msdf, w, h);
    //     char imgpath[1024] = {0};
    //     snprintf(imgpath, sizeof(imgpath), "%s/msdf.png", ARTIFACTS_DIR);
    //     dvz_write_png(imgpath, w, h, rgb);
    //     FREE(rgb);
    // }

    // NOTE: the first argument is the number of vec3 items, not the number of floats.
    float* msdf_alpha = (float*)calloc(w * h * 4, sizeof(float));
    dvz_rgb_to_rgba_float(w * h, msdf, msdf_alpha);
    FREE(msdf);

    // Upload the MSDF image to a texture.
    DvzTexture* texture = dvz_texture_2D(
        visual->batch, DVZ_FORMAT_R32G32B32A32_SFLOAT, DVZ_FILTER_LINEAR,
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, w, h, msdf_alpha, 0);
    dvz_marker_texture(visual, texture);
    FREE(msdf_alpha);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_3D(n, 0.25);
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(192));
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 50, 100);
    dvz_marker_size(visual, 0, n, size, 0);

    // Parameters.
    dvz_marker_edgecolor(visual, (DvzColor){WHITE});
    dvz_marker_linewidth(visual, (float){3.0});
    // IMPORTANT: need to specify the size of the texture when using SDFs.
    dvz_marker_tex_scale(visual, (float){w});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}



static void _on_timer(DvzApp* app, DvzId window_id, DvzTimerEvent* ev)
{
    ANN(app);

    VisualTest* vt = (VisualTest*)ev->user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);
    float t = ev->time;

    dvz_marker_angle(visual, 0, 1, (float[]){.25 * M_PI * t}, 0);

    // uint32_t mem = dvz_memory();
    // log_info("%s", pretty_size(mem));
}

// Check glyph adaptive size depending on the marker rotation.
int test_marker_rotation(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("marker_rotation", VISUAL_TEST_PANZOOM, 0);

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_shape(visual, DVZ_MARKER_SHAPE_SQUARE);

    // Visual allocation.
    dvz_marker_alloc(visual, 1);

    dvz_marker_position(visual, 0, 1, (vec3[]){{0, 0, 0}}, 0);
    dvz_marker_size(visual, 0, 1, (float[]){250}, 0);
    dvz_marker_angle(visual, 0, 1, (float[]){0}, 0);
    dvz_marker_color(visual, 0, 1, (DvzColor[]){{RED}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Animation.
    vt.visual = visual;
    dvz_app_timer(vt.app, 0, 1. / 60., 0);
    dvz_app_on_timer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
