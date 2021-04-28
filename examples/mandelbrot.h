/*************************************************************************************************/
/*  Mandelbrot fractal example.                                                                  */
/*************************************************************************************************/

// NOTE: ignore this.
#ifndef SCREENSHOT
#define SCREENSHOT
#endif
#ifndef NFRAMES
#define NFRAMES 0
#endif

// We include the library header file.
#include <datoviz/datoviz.h>

static int demo_mandelbrot()
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);
    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    SCREENSHOT
    dvz_app_run(app, NFRAMES);

    dvz_app_destroy(app);
    return 0;
}
