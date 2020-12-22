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
    VklInteract* interact = NULL;
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
            // Get box of active panel.
            tr = vkl_transform(panel, VKL_CDS_PANZOOM, VKL_CDS_GPU);
            vkl_transform_apply(&tr, ll, pos_ll);
            vkl_transform_apply(&tr, ur, pos_ur);
            log_debug("(%.3f, %.3f) (%.3f, %.3f)", pos_ll[0], pos_ll[1], pos_ur[0], pos_ur[1]);

            // Set box of other panel.
            interact = &scene->grid.panels[1 - i].controller->interacts[0];
            VklPanzoom* panzoom = &interact->u.p;
            tr = vkl_transform_inv(tr);
            panzoom->camera_pos[0] = tr.shift[0];
            panzoom->camera_pos[1] = tr.shift[1];
            panzoom->zoom[0] = tr.scale[0];
            panzoom->zoom[1] = tr.scale[1];

            // View matrix (depends on the pan).
            {
                vec3 center;
                glm_vec3_copy(panzoom->camera_pos, center);
                center[2] = 0.0f; // only the z coord changes between panel and center.
                vec3 lookup = {0, 1, 0};
                glm_lookat(panzoom->camera_pos, center, lookup, interact->mvp.view);
            }
            // Proj matrix (depends on the zoom).
            {
                float zx = panzoom->zoom[0];
                float zy = panzoom->zoom[1];
                glm_ortho(
                    -1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f,
                    interact->mvp.proj);
            }
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
    vkl_panel_span(panel2, VKL_GRID_VERTICAL, 2);

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



int test_scene_axes(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklScene* scene = vkl_scene(canvas, 1, 1);
    VklPanel* panel = vkl_scene_panel(scene, 0, 0, VKL_CONTROLLER_PANZOOM, 0);
    vkl_panel_margins(panel, (vec4){100, 100, 100, 100});

    VklVisual* visualx = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_X);
    VklVisual* visualy = vkl_scene_visual(panel, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_Y);

    visualx->clip = VKL_VIEWPORT_INNER;
    visualy->clip = VKL_VIEWPORT_INNER;

    visualx->transform = VKL_TRANSFORM_AXIS_X;
    visualy->transform = VKL_TRANSFORM_AXIS_Y;

    const uint32_t N = 4 * 10 + 1;
    float* xticks = calloc(N, sizeof(float));
    float* yticks = calloc(N, sizeof(float));
    char** text = calloc(N, sizeof(char*));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -2 + 4 * (float)i / (N - 1);
        xticks[i] = t;
        yticks[i] = t;
        text[i] = "hello";
    }

    // Minor ticks.
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, xticks);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N, yticks);

    // Major ticks.
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, xticks);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, yticks);

    // Grid.
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, xticks);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, yticks);

    // Lim.
    float lim[] = {-1};
    vkl_visual_data(visualx, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);
    vkl_visual_data(visualy, VKL_PROP_POS, VKL_AXES_LEVEL_LIM, 1, lim);

    // Text.
    vkl_visual_data(visualx, VKL_PROP_TEXT, 0, N, text);

    // Color
    cvec4 color = {0, 0, 0, 255};
    vkl_visual_data(visualx, VKL_PROP_COLOR, 0, 1, color);
    vkl_visual_data(visualy, VKL_PROP_COLOR, 0, 1, color);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);
    vkl_app_run(app, N_FRAMES);
    vkl_visual_destroy(visualx);
    vkl_visual_destroy(visualy);
    vkl_scene_destroy(scene);
    FREE(xticks);
    FREE(yticks);
    TEST_END
}
