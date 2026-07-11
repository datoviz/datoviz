#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

#define N 10000

int main(int argc, char** argv)
{
    srand((unsigned)time(NULL));

    /* Each point is described by three arrays with the same length.
     * pos stores x/y/z positions. z is 0, so this is a 2D scatter plot.
     * color stores one 8-bit RGBA color per point.
     * diameter_px stores one point size per point, measured in screen pixels. */
    float pos[N * 3], diameter_px[N];
    uint8_t color[N * 4];
    for (int i = 0; i < N; i++)
    {
        pos[3 * i + 0] = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
        pos[3 * i + 1] = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
        pos[3 * i + 2] = 0;
        color[4 * i + 0] = rand() % 256;
        color[4 * i + 1] = rand() % 256;
        color[4 * i + 2] = rand() % 256;
        color[4 * i + 3] = 255;
        diameter_px[i] = 5.0f;
    }

    /* Create the scene structure: one scene, one figure, and one full-size panel.
     * The panel is the drawing area where the scatter plot will appear. */
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel* panel = dvz_panel_full(figure);

    /* Add mouse interaction to the panel. Pan/zoom is limited to X and Y because
     * the points are flat, with z = 0. */
    DvzController* controller = dvz_panzoom(scene, NULL);
    dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

    /* Create one point visual for the whole dataset. Each data call attaches
     * one C array to a named visual attribute. */
    DvzVisual* visual = dvz_point(scene, 0);
    dvz_visual_set_data(visual, "position", pos, N);
    dvz_visual_set_data(visual, "color", color, N);
    dvz_visual_set_data(visual, "diameter_px", diameter_px, N);

    /* Uploading arrays is not enough by itself: the visual must be added to a
     * panel before it becomes part of the figure. */
    dvz_panel_add_visual(panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    dvz_view_window(app, figure, 800, 600, "Scatter plot");
    uint32_t frame_count =
        argc == 3 && strcmp(argv[1], "--frames") == 0 ? (uint32_t)strtoul(argv[2], NULL, 10) : 0;
    dvz_app_run(app, frame_count);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
