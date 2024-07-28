/*************************************************************************************************/
/*  Testing path                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_path.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/path.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Path tests                                                                                */
/*************************************************************************************************/

int test_path_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("path", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    uint32_t N = 100; // size of each path
    uint32_t n_paths = 20;
    uint32_t total_length = N * n_paths;

    // Path lengths.
    uint32_t* path_lengths = (uint32_t*)calloc(n_paths, sizeof(uint32_t));
    for (uint32_t j = 0; j < n_paths; j++)
        path_lengths[j] = N;

    // Create the visual.
    DvzVisual* visual = dvz_path(vt.batch, 0);

    // Visual allocation.
    dvz_path_alloc(visual, total_length);

    // Allocate the positions array.
    vec3* positions = (vec3*)calloc(total_length, sizeof(vec3));

    // Allocate the colors array.
    cvec4* colors = (cvec4*)calloc(total_length, sizeof(cvec4));

    // Generate the path data.
    double t = 0;
    double d = 1.0 / (double)(N - 1);
    double a = .15;
    double offset = 0;
    uint32_t k = 0;
    for (uint32_t j = 0; j < n_paths; j++)
    {
        offset = n_paths > 1 ? -.75 + 1.5 * j / (double)(n_paths - 1) : 0;
        for (int32_t i = 0; i < (int32_t)N; i++)
        {
            t = -.9 + 1.8 * i * d;
            positions[k][0] = t;
            positions[k][1] = a * sin(M_2PI * t / .9) + offset;

            dvz_colormap_scale(DVZ_CMAP_HSV, j, 0, n_paths - 1, colors[k]);

            k++;
        }
    }

    // Set the visual's position and color data.
    dvz_path_position(visual, total_length, positions, n_paths, path_lengths, 0);
    dvz_path_color(visual, 0, total_length, colors, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(path_lengths);
    FREE(positions);
    FREE(colors);

    return 0;
}



static void _on_timer(DvzApp* app, DvzId window_id, DvzTimerEvent ev)
{
    ANN(app);

    VisualTest* vt = (VisualTest*)ev.user_data;
    ANN(vt);

    DvzVisual* visual = vt->visual;
    ANN(visual);

    // Allocate the colors array.
    cvec4* colors = (cvec4*)vt->user_data;
    ANN(colors);

    // Generate the path data.
    uint32_t n_paths = vt->n;
    uint32_t N = vt->m;
    uint32_t k = 0;
    int64_t step = (int64_t)ev.step_idx;
    for (uint32_t j = 0; j < n_paths; j++)
    {
        for (int32_t i = 0; i < (int32_t)N; i++)
        {
            colors[k++][3] = (uint8_t)(20 * ((-step + i + 7 * j) % 10));
        }
    }

    dvz_path_color(visual, 0, N * n_paths, colors, 0);

    dvz_visual_update(visual);
}

int test_path_2(TstSuite* suite)
{
    VisualTest vt = visual_test_start("path", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    uint32_t N = 100; // size of each path
    uint32_t n_paths = 10;
    uint32_t total_length = N * n_paths;

    // Path lengths.
    uint32_t* path_lengths = (uint32_t*)calloc(n_paths, sizeof(uint32_t));
    for (uint32_t j = 0; j < n_paths; j++)
        path_lengths[j] = N;

    // Create the visual.
    DvzVisual* visual = dvz_path(vt.batch, 0);

    // Visual allocation.
    dvz_path_alloc(visual, total_length);

    // Allocate the positions array.
    vec3* positions = (vec3*)calloc(total_length, sizeof(vec3));

    // Allocate the colors array.
    cvec4* colors = (cvec4*)calloc(total_length, sizeof(cvec4));

    // Generate the path data.
    double t = 0;
    double r = .1;
    double R = .9;
    double a = 0;
    uint32_t k = 0;
    for (uint32_t j = 0; j < n_paths; j++)
    {
        a = r + (R - r) * j / (double)(n_paths - 1);
        for (int32_t i = 0; i < (int32_t)N; i++)
        {
            t = M_2PI * i / (double)(N);
            positions[k][0] = HEIGHT / (double)WIDTH * a * cos(t);
            positions[k][1] = a * sin(t);

            colors[k][0] = 128;
            colors[k][1] = 128;
            colors[k][2] = 128;
            // colors[k][3] = 255;

            k++;
        }
    }

    // Set the visual's position and color data.
    dvz_path_position(visual, total_length, positions, n_paths, path_lengths, 0);
    dvz_path_color(visual, 0, total_length, colors, 0);
    dvz_path_linewidth(visual, 5);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Animation.
    vt.n = n_paths;
    vt.m = N;
    vt.visual = visual;
    vt.user_data = (void*)colors;
    dvz_app_timer(vt.app, 0, 1. / 60., 0);
    dvz_app_ontimer(vt.app, _on_timer, &vt);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(path_lengths);
    FREE(positions);
    FREE(colors);

    return 0;
}
