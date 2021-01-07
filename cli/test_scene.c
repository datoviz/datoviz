#include "test_scene.h"
#include "../external/exwilk.h"
#include "../include/visky/builtin_visuals.h"
#include "../include/visky/scene.h"
#include "../src/axes.h"
#include "utils.h"



/*************************************************************************************************/
/*  Axes tests                                                                                   */
/*************************************************************************************************/

int test_axes_1(TestContext* context)
{
    VklTickFormat f = {0};
    f.format_type = VKL_TICK_FORMAT_SCIENTIFIC;
    f.precision = 3;
    VklAxesContext ctx = {0};
    ctx.coord = VKL_AXES_COORD_X;
    ctx.size_viewport = 1000;
    ctx.size_glyph = 10;

    const uint32_t N = 16;
    char* labels = calloc(N * MAX_GLYPHS_PER_TICK, sizeof(char));
    make_labels(f, 0, 1, .1, ctx, labels);
    for (uint32_t i = 0; i < N; i++)
    {
        log_debug("%s ", &labels[i * MAX_GLYPHS_PER_TICK]);
    }

    FREE(labels);
    return 0;
}



/*************************************************************************************************/
/*  Scene tests                                                                                  */
/*************************************************************************************************/

static void _panzoom(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklScene* scene = ev.user_data;
    ASSERT(scene != NULL);
    VklGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    VklController* controller = NULL;
    VklInteract* interact = NULL;
    VklTransform tr = {0};
    dvec2 ll = {-1, -1};
    dvec2 ur = {+1, +1};
    dvec2 pos_ll = {0};
    dvec2 pos_ur = {0};


    VklPanel* panel = vkl_container_iter_init(&grid->panels);
    VklPanel* other = NULL;
    uint32_t i = 0;
    while (panel != NULL)
    {
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
            other = (VklPanel*)grid->panels.items[1 - i];
            interact = &other->controller->interacts[0];
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

        panel = vkl_container_iter(&grid->panels);
        i++;
    }
}

int test_scene_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, VKL_CANVAS_FLAGS_FPS);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklScene* scene = vkl_scene(canvas, 2, 3);
    VklPanel* panel = vkl_scene_panel(scene, 0, 0, VKL_CONTROLLER_PANZOOM, 0);
    vkl_panel_mode(panel, VKL_PANEL_FLOATING);
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
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, VKL_CANVAS_FLAGS_FPS);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklScene* scene = vkl_scene(canvas, 1, 1);
    VklPanel* panel = vkl_scene_panel(scene, 0, 0, VKL_CONTROLLER_AXES_2D, 0);

    // Markers.
    VklVisual* visual = vkl_scene_visual(panel, VKL_VISUAL_MARKER, 0);
    const uint32_t N = 10000;
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float param = 10.0f;
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
        color[i][3] = 200;
    }

    vkl_visual_data(visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(visual, VKL_PROP_COLOR, 0, N, color);
    vkl_visual_data(visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    vkl_app_run(app, N_FRAMES);
    vkl_scene_destroy(scene);
    FREE(pos);
    FREE(color);
    TEST_END
}



static void _logistic(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklCommands* cmds = ev.user_data;
    ASSERT(cmds != NULL);
    vkl_queue_wait(canvas->gpu, VKL_DEFAULT_QUEUE_RENDER);
    vkl_cmd_submit_sync(cmds, 0);
    vkl_queue_wait(canvas->gpu, VKL_DEFAULT_QUEUE_COMPUTE);
}

int test_scene_logistic(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, VKL_CANVAS_FLAGS_FPS);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklScene* scene = vkl_scene(canvas, 1, 1);
    VklPanel* panel = vkl_scene_panel(scene, 0, 0, VKL_CONTROLLER_PANZOOM, 0);

    // Markers.
    VklVisual* visual = vkl_scene_visual(panel, VKL_VISUAL_MARKER, 0);

    const uint32_t N = 1000000;
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        pos[i][0] = -1 + 2 * i / (float)(N - 1);
        pos[i][1] = -1 + 2 * rand_float();
        vkl_colormap_scale(VKL_CMAP_HSV, i, 0, N, color[i]);
        color[i][3] = 20;
    }

    vkl_visual_data(visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(visual, VKL_PROP_COLOR, 0, N, color);
    float param = 2.0f;
    vkl_visual_data(visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // Create compute object.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_logistic.comp.spv", SPIRV_DIR);
    VklCompute* compute = vkl_ctx_compute(gpu->context, path);
    vkl_compute_slot(compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_compute_push(compute, 0, sizeof(N), VK_SHADER_STAGE_COMPUTE_BIT);

    VklBindings bindings = vkl_bindings(&compute->slots, 1);
    VklSource* source = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_update(visual, panel->viewport, (VklDataCoords){0}, NULL);
    vkl_bindings_buffer(&bindings, 0, source->u.br);
    vkl_bindings_update(&bindings);
    vkl_compute_bindings(compute, &bindings);
    vkl_compute_create(compute);

    VklCommands* cmds = vkl_canvas_commands(canvas, VKL_DEFAULT_QUEUE_COMPUTE, 1);
    vkl_cmd_begin(cmds, 0);
    vkl_cmd_push(cmds, 0, &compute->slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(N), &N);
    vkl_cmd_compute(cmds, 0, compute, (uvec3){N, 1, 1});
    vkl_cmd_end(cmds, 0);
    vkl_event_callback(canvas, VKL_EVENT_TIMER, 1. / 10, VKL_EVENT_MODE_SYNC, _logistic, cmds);

    vkl_app_run(app, N_FRAMES);
    vkl_compute_destroy(compute);
    vkl_bindings_destroy(&bindings);
    vkl_scene_destroy(scene);
    TEST_END
}
