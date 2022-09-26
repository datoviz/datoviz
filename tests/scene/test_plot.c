/*************************************************************************************************/
/*  Testing plot                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_plot.h"
#include "scene/plot.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Plot tests                                                                                   */
/*************************************************************************************************/

int test_plot_1(TstSuite* suite)
{
    ANN(suite);

    // Create the boilerplate objects: requester, renderer, presenter...
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzDevice* device = dvz_device(app);

    // Generate a bunch of requests.
    DvzScene* scene = dvz_scene();
    DvzFigure* fig = dvz_figure(scene, WIDTH, HEIGHT, 1, 1, 0);
    DvzPanel* panel = dvz_panel(fig, 0, 0, DVZ_PANEL_TYPE_NONE, 0);
    DvzVisual* visual = dvz_visual(scene, DVZ_VISUAL_MARKER, 0);


    // Create the vertex buffer dat.
    const uint32_t n = 50;
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    double t = 0;
    double aspect = WIDTH / (double)HEIGHT;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        pos[i][0] = .5 * cos(M_2PI * t);
        pos[i][1] = aspect * .5 * sin(M_2PI * t);

        // dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), data[i].color);
        // data[i].color[3] = 128;
    }
    dvz_visual_data(visual, DVZ_PROP_POS, 0, n, pos);

    // Run the presenter.
    vz_app_run(app, scene, 0);

    // Destroy the boilerplate objects.
    dvz_visual_destroy(visual);
    dvz_panel_destroy(panel);
    dvz_figure_destroy(fig);
    dvz_device_destroy(device);

    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}
