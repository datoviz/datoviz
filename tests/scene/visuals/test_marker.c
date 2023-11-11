/*************************************************************************************************/
/*  Testing marker                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_marker.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/sdf.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/marker.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Marker tests                                                                                 */
/*************************************************************************************************/

int test_marker_code(TstSuite* suite)
{
    VisualTest vt = visual_test_start("marker_code", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);
    dvz_marker_shape(visual, DVZ_MARKER_SHAPE_HEART);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 192;
    }
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        size[i] = 10 + 40 * dvz_rand_float();
    }
    dvz_marker_size(visual, 0, n, size, 0);

    // Angle.
    float* angle = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        angle[i] = M_2PI * dvz_rand_float();
    }
    dvz_marker_angle(visual, 0, n, size, 0);

    // Parameters.
    dvz_marker_edge_color(visual, (cvec4){255, 255, 255, 255});
    dvz_marker_edge_width(visual, (float){3.0});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);
    FREE(angle);

    return 0;
}



int test_marker_bitmap(TstSuite* suite)
{
    VisualTest vt = visual_test_start("marker_bitmap", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 100;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);

    // Bitmap marker.
    dvz_marker_mode(visual, DVZ_MARKER_MODE_BITMAP);

    // Create and upload the texture.
    uvec3 tex_shape = {0};
    DvzId tex = load_crate_texture(vt.batch, tex_shape);
    DvzId sampler = dvz_create_sampler(
                        visual->batch, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                        .id;
    dvz_marker_tex(visual, tex, sampler);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 128;
    }
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        size[i] = 50 + 50 * dvz_rand_float();
    }
    dvz_marker_size(visual, 0, n, size, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

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

    DvzId tex = dvz_tex_image(visual->batch, DVZ_FORMAT_R32_SFLOAT, size, size, texdata);
    DvzId sampler = dvz_create_sampler(
                        visual->batch, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                        .id;
    dvz_marker_tex(visual, tex, sampler);
    FREE(texdata);
}

int test_marker_sdf(TstSuite* suite)
{
    VisualTest vt = visual_test_start("marker_sdf", VISUAL_TEST_PANZOOM);

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
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 192;
    }
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        size[i] = 25 + 75 * dvz_rand_float();
    }
    dvz_marker_size(visual, 0, n, size, 0);

    // Angle.
    float* angle = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        angle[i] = M_2PI * dvz_rand_float();
    }
    dvz_marker_angle(visual, 0, n, size, 0);

    // Parameters.
    dvz_marker_edge_color(visual, (cvec4){255, 255, 255, 255});
    dvz_marker_edge_width(visual, (float){2.0});
    // IMPORTANT: need to specify the size of the texture when using SDFs.
    dvz_marker_tex_scale(visual, (float){(float)tex_size});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);
    FREE(angle);

    return 0;
}



int test_marker_msdf(TstSuite* suite)
{
    VisualTest vt = visual_test_start("marker_msdf", VISUAL_TEST_PANZOOM);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_aspect(visual, DVZ_MARKER_ASPECT_OUTLINE);

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
    //     char imgpath[1024];
    //     snprintf(imgpath, sizeof(imgpath), "%s/msdf.png", ARTIFACTS_DIR);
    //     dvz_write_png(imgpath, w, h, rgb);
    //     FREE(rgb);
    // }

    // NOTE: the first argument is the number of vec3 items, not the number of floats.
    float* msdf_alpha = dvz_rgb_to_rgba_float(w * h, msdf);
    FREE(msdf);

    // Upload the MSDF image to a texture.
    DvzId tex = dvz_tex_image(visual->batch, DVZ_FORMAT_R32G32B32A32_SFLOAT, w, h, msdf_alpha);
    DvzId sampler = dvz_create_sampler(
                        visual->batch, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                        .id;
    dvz_marker_tex(visual, tex, sampler);
    FREE(msdf_alpha);

    // Visual allocation.
    dvz_marker_alloc(visual, n);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_marker_position(visual, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 192;
    }
    dvz_marker_color(visual, 0, n, color, 0);

    // Size.
    float* size = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        size[i] = 50 + 50 * dvz_rand_float();
    }
    dvz_marker_size(visual, 0, n, size, 0);

    // Parameters.
    dvz_marker_edge_color(visual, (cvec4){255, 255, 255, 255});
    dvz_marker_edge_width(visual, (float){3.0});
    // IMPORTANT: need to specify the size of the texture when using SDFs.
    dvz_marker_tex_scale(visual, (float){w});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}



static void _on_timer(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);
    float t = ev.content.t.time;

    dvz_marker_angle(visual, 0, 1, (float[]){.25 * M_PI * t}, 0);
    dvz_visual_update(visual);
}

// Check glyph adaptive size depending on the marker rotation.
int test_marker_rotation(TstSuite* suite)
{
    VisualTest vt = visual_test_start("marker_rotation", VISUAL_TEST_PANZOOM);

    // Create the visual.
    DvzVisual* visual = dvz_marker(vt.batch, 0);
    dvz_marker_shape(visual, DVZ_MARKER_SHAPE_SQUARE);

    // Visual allocation.
    dvz_marker_alloc(visual, 1);

    dvz_marker_position(visual, 0, 1, (vec3[]){{0, 0, 0}}, 0);
    dvz_marker_size(visual, 0, 1, (float[]){250}, 0);
    dvz_marker_angle(visual, 0, 1, (float[]){0}, 0);
    dvz_marker_color(visual, 0, 1, (cvec4[]){{255, 0, 0, 255}}, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual);

    // Animation.
    vt.visual = visual;
    dvz_app_timer(vt.app, 0, 1. / 60., 0);
    dvz_app_ontimer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
