/*************************************************************************************************/
/*  Testing app                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_app.h"
#include "scene/app.h"
#include "scene/scene.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  App tests                                                                                    */
/*************************************************************************************************/

int test_app_1(TstSuite* suite)
{
    ANN(suite);

    // Create app objects.
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzDevice* device = dvz_device(app);

    // Create scene objects.
    DvzScene* scene = dvz_scene();
    DvzFigure* fig = dvz_figure(scene, WIDTH, HEIGHT, 1, 1, 0);
    DvzPanel* panel = dvz_panel(fig, 0, 0, DVZ_PANEL_TYPE_NONE, 0);
    DvzVisual* visual = dvz_visual(scene, DVZ_VISUAL_MARKER, 0);



    // Create the visual properties.
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

    // Add the visual to the panel.
    dvz_panel_visual(panel, visual, 0);



    // Run the presenter.
    dvz_device_run(device, scene, N_FRAMES);



    // Destroy the scene objects.
    dvz_visual_destroy(visual);
    dvz_panel_destroy(panel);
    dvz_figure_destroy(fig);
    dvz_scene_destroy(scene);

    // Destroy the app objects.
    dvz_device_destroy(device);
    dvz_app_destroy(app);

    return 0;
}
