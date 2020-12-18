#include "test_scene.h"
#include "../include/visky/builtin_visuals.h"
#include "../include/visky/scene.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene tests                                                                                  */
/*************************************************************************************************/

static void _panzoom(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklScene* scene = ev.user_data;
    ASSERT(scene != NULL);
    VklPanel* panel = NULL;
    VklController* controller = NULL;
    VklTransform tr = {0};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    dvec2 pos_ll = {0};
    dvec2 pos_ur = {0};
    for (uint32_t i = 0; i < scene->max_controllers; i++)
    {
        panel = &scene->grid.panels[i];
        controller = panel->controller;
        if (controller == NULL || controller->obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        if (controller->interacts[0].is_active && controller->type == VKL_CONTROLLER_PANZOOM)
        {
            tr = vkl_transform(panel, VKL_CDS_PANZOOM, VKL_CDS_GPU);
            vkl_transform_apply(&tr, ll, pos_ll);
            vkl_transform_apply(&tr, ur, pos_ur);
            log_debug("(%.3f, %.3f) (%.3f, %.3f)", pos_ll[0], pos_ll[1], pos_ur[0], pos_ur[1]);
        }
    }
}

int test_scene_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklScene* scene = vkl_scene(canvas, 2, 3);
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

    vkl_visual_data(visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(visual, VKL_PROP_COLOR, 0, N, color);
    vkl_visual_data(visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // Second panel.
    VklPanel* panel2 = vkl_scene_panel(scene, 1, 1, VKL_CONTROLLER_PANZOOM, 0);
    VklVisual* visual2 = vkl_scene_visual(panel2, VKL_VISUAL_MARKER, 0);
    vkl_visual_data(visual2, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(visual2, VKL_PROP_COLOR, 0, N, color);
    vkl_visual_data(visual2, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _panzoom, scene);

    vkl_app_run(app, N_FRAMES);
    vkl_visual_destroy(visual);
    vkl_visual_destroy(visual2);
    vkl_scene_destroy(scene);
    FREE(pos);
    FREE(color);
    TEST_END
}
