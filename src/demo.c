
#include "../include/datoviz/demo.h"
#include "../include/datoviz/gui.h"
#include "../include/datoviz/scene.h"


int dvz_demo_scatter(int32_t n, dvec3* pos)
{
    // HACK: some wrapping languages do not support uint types well
    uint32_t N = (uint32_t)n;
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    dvz_canvas_clear_color(canvas, 1, 1, 1);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_MARKER, 0);

    cvec4* color = (cvec4*)calloc(N, sizeof(cvec4));
    float* size = (float*)calloc(N, sizeof(float));

    for (uint32_t i = 0; i < N; i++)
    {
        dvz_colormap_scale(DVZ_CMAP_VIRIDIS, dvz_rand_float(), 0, 1, color[i]);
        size[i] = 10 + 40 * dvz_rand_float();
        color[i][3] = 200;
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, N, size);

    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    return 0;
}



int dvz_demo_gui(void)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, DVZ_CANVAS_FLAGS_IMGUI);
    dvz_imgui_demo(canvas);
    dvz_app_run(app, 0);
    dvz_app_destroy(app);
    return 0;
}
