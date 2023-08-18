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
    VisualTest vt = visual_test_start("path", VISUAL_TEST_PANZOOM);

    // Number of items.
    uint32_t N = 100; // size of each path
    uint32_t n_paths = 20;
    uint32_t total_length = N * n_paths;

    // Path lengths.
    uint32_t* path_lengths = (uint32_t*)calloc(n_paths, sizeof(uint32_t));
    for (uint32_t j = 0; j < n_paths; j++)
        path_lengths[j] = N;

    // Create the visual.
    DvzVisual* visual = dvz_path(vt.rqr, 0);

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
    dvz_panel_visual(vt.panel, visual);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(path_lengths);
    FREE(positions);
    FREE(colors);

    return 0;
}
