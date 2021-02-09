
#include "../include/datoviz/scene.h"
#include "../include/datoviz/demo.h"


void dvz_demo_scatter(uint32_t N, dvec3* pos)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_MARKER, 0);

    cvec4* color = (cvec4*)calloc(N, sizeof(cvec4));
    float* size = (float*)calloc(N, sizeof(float));
    for (uint32_t i = 0; i < N; i++)
    {
        dvz_colormap_scale(DVZ_CMAP_JET, dvz_rand_float(), 0, 1, color[i]);
        size[i] = 2 + 38 * dvz_rand_float();
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, N, size);

    dvz_app_run(app, 0);

    dvz_app_destroy(app);
}
