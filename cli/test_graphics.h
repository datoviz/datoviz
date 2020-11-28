#include "../include/visky/graphics.h"
#include "utils.h"


#define N_FRAMES 10



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

static int vklite2_graphics_points(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    VklGraphics* graphics = vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS);
    // ASSERT(graphics != NULL);

    // vkl_app_run(app, 5);

    TEST_END
}
