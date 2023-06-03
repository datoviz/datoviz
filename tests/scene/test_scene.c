/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_scene.h"
#include "canvas.h"
#include "scene/app.h"
#include "scene/scene.h"
#include "scene/viewset.h"
#include "scene/visuals/pixel.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene tests                                                                                  */
/*************************************************************************************************/

int test_scene_1(TstSuite* suite)
{
    ANN(suite);

    // Create app object.
    DvzApp* app = dvz_app();
    DvzRequester* rqr = dvz_app_requester(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(rqr);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_VSYNC);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Panel contains.
    AT(dvz_panel_contains(panel, (vec2){0, 0}));
    AT(!dvz_panel_contains(panel, (vec2){WIDTH, HEIGHT}));
    ASSERT(dvz_panel_at(figure, (vec2){WIDTH / 2, HEIGHT / 2}) == panel);
    ASSERT(dvz_panel_at(figure, (vec2){WIDTH / 2, -1}) == NULL);

    // Upload the data.
    const uint32_t n = 10000;

    // Create a visual.
    DvzVisual* pixel = dvz_pixel(rqr, 0);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_pixel_position(pixel, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 128;
    }
    dvz_pixel_color(pixel, 0, n, color, 0);

    // Add the visual to the panel.
    dvz_panel_visual(panel, pixel);

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(app, panel);
    ANN(pz);

    // Run the app.
    dvz_scene_run(scene, app, N_FRAMES);

    // Cleanup.
    dvz_panel_destroy(panel);
    dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    FREE(pos);
    FREE(color);
    return 0;
}



int test_scene_2(TstSuite* suite)
{
    ANN(suite);

    // Create app object.
    DvzApp* app = dvz_app();
    DvzRequester* rqr = dvz_app_requester(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(rqr);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_FPS);

    // Upload the data.
    const uint32_t n = 10000;

    // Create a visual.
    DvzVisual* pixel = dvz_pixel(rqr, 0);

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_pixel_position(pixel, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 128;
    }
    dvz_pixel_color(pixel, 0, n, color, 0);

    // Create two panels.
    DvzPanel* panel_0 = dvz_panel(figure, 0, 0, WIDTH / 2, HEIGHT);
    DvzPanel* panel_1 = dvz_panel(figure, WIDTH / 2, 0, WIDTH / 2, HEIGHT);

    // Add the visual to the panel.
    dvz_panel_visual(panel_0, pixel);

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(app, panel_0);
    ANN(pz);

    // Run the app.
    dvz_scene_run(scene, app, N_FRAMES);

    // Cleanup.
    dvz_panel_destroy(panel_0);
    dvz_panel_destroy(panel_1);
    dvz_figure_destroy(figure);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);
    FREE(pos);
    FREE(color);
    return 0;
}
