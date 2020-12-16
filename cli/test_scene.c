#include "test_scene.h"
#include "../include/visky/builtin_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene tests                                                                                  */
/*************************************************************************************************/

int test_scene_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklScene* scene = vkl_scene(canvas, 1, 1);
    VklPanel* panel = vkl_scene_panel(scene, 0, 0, VKL_CONTROLLER_PANZOOM, 0);
    VklVisual* visual = vkl_scene_visual(panel, VKL_VISUAL_MARKER, 0);

    // Visual data.
    const uint32_t N = 1000;
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float param = 10.0f;
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    {
        vkl_visual_data(visual, VKL_PROP_POS, 0, N, pos);
        vkl_visual_data(visual, VKL_PROP_COLOR, 0, N, color);
        vkl_visual_data(visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

        // vkl_visual_data(visual, VKL_PROP_MODEL, 0, 1, id);
        // vkl_visual_data(visual, VKL_PROP_VIEW, 0, 1, id);
        // vkl_visual_data(visual, VKL_PROP_PROJ, 0, 1, id);
        // vkl_visual_data_texture(visual, VKL_PROP_COLOR_TEXTURE, 0, 1, 1, 1, NULL);
        // vkl_visual_data_buffer(visual, VKL_SOURCE_UNIFORM, 1, 0, 1, 1, NULL);
    }

    vkl_app_run(app, N_FRAMES);
    // vkl_visual_destroy(visual);
    vkl_scene_destroy(scene);
    FREE(pos);
    FREE(color);
    TEST_END
}
