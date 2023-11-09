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
    DvzId tex = load_crate_texture(vt.batch);
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
    float distanceToCenter = sqrt(x * x + y * y);
    return distanceToCenter - radius;
}

static void _disc_sdf(DvzVisual* visual)
{
    ANN(visual);
    uint32_t width = 64;
    uint32_t height = 64;
    DvzSize texsize = width * height * sizeof(float);
    float* texdata = (float*)calloc(texsize, sizeof(float));
    for (uint32_t i = 0; i < texsize; i++)
    {
        uint32_t x = i % width;
        uint32_t y = i / width;
        float posX = (x - width / 2.0);
        float posY = (y - height / 2.0);
        float value = _sdf(posX, posY, width / 4);
        texdata[i] = value;
    }

    DvzId tex = dvz_tex_image(visual->batch, DVZ_FORMAT_R32_SFLOAT, width, height, texdata);
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
    _disc_sdf(visual);

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
