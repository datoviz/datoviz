#include "../include/visky/visuals2.h"
#include "utils.h"


#define N_FRAMES 10



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define RANDN_POS(x)                                                                              \
    x[0] = .25 * randn();                                                                         \
    x[1] = .25 * randn();                                                                         \
    x[2] = .25 * randn();

#define RAND_COLOR(x)                                                                             \
    x[0] = rand_byte();                                                                           \
    x[1] = rand_byte();                                                                           \
    x[2] = rand_byte();                                                                           \
    x[3] = 255;



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

static void _visual_fill() {}

static int vklite2_visuals_1(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    VklVisual visual = vkl_visual(canvas);

    // Props.
    vkl_visual_prop(
        &visual, VKL_PROP_POS, 0, VKL_DTYPE_VEC3, VKL_PROP_LOC_ATTRIBUTE, 0, 0,
        offsetof(VklVertex, pos));

    vkl_visual_prop(
        &visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_PROP_LOC_ATTRIBUTE, 0, 1,
        offsetof(VklVertex, color));

    // Grpahics.
    vkl_visual_graphics(&visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS, 0));
    vkl_visual_fill(&visual, _visual_fill);

    // Generate data.
    const uint32_t N = 1000;
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_size(&visual, N, 0);
    vkl_visual_data(&visual, VKL_PROP_POS, 0, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, color);

    // Run and end.
    vkl_app_run(app, 0);
    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}
