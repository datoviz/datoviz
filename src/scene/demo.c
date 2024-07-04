/*************************************************************************************************/
/*  Demo                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_demo(void)
{
    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(scene, panel);

    // Create a visual.
    DvzVisual* visual = dvz_point(batch, 0);

    // Allocate a number of points.
    const uint32_t n = 10000;
    dvz_point_alloc(visual, n);

    // Set the point positions.
    {
        vec3* pos = (vec3*)calloc(n, sizeof(vec3));
        for (uint32_t i = 0; i < n; i++)
        {
            pos[i][0] = .25 * dvz_rand_normal();
            pos[i][1] = .25 * dvz_rand_normal();
        }
        dvz_point_position(visual, 0, n, pos, 0);
        FREE(pos);
    }

    // Set the point RGBA colors.
    {
        cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
        for (uint32_t i = 0; i < n; i++)
        {
            dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
            color[i][3] = 128;
        }
        dvz_point_color(visual, 0, n, color, 0);
        FREE(color);
    }

    // Set the point sizes.
    {
        float* size = (float*)calloc(n, sizeof(float));
        for (uint32_t i = 0; i < n; i++)
        {
            size[i] = 25 + 25 * dvz_rand_float();
        }
        dvz_point_size(visual, 0, n, size, 0);
        FREE(size);
    }

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, visual);

    // Run the app.
    dvz_scene_run(scene, app, 0);

    // Cleanup.
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return;
}
