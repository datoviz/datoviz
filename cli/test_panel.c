#include "test_panel.h"
#include "../include/visky/builtin_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _canvas_fill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    VklGrid* grid = (VklGrid*)ev.user_data;
    ASSERT(grid != NULL);

    VklViewport viewport = {0};
    VklCommands* cmds = NULL;
    VklPanel* panel = NULL;

    // Go through all the current command buffers.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        cmds = ev.u.rf.cmds[i];

        // We only fill the PANEL command buffers.
        // if (cmds->obj.group_id == VKL_COMMANDS_GROUP_PANELS)
        // {
        //     panel = &grid->panels[cmds->obj.id];
        for (uint32_t j = 0; j < grid->panel_count; j++)
        {
            panel = &grid->panels[j];
            ASSERT(is_obj_created(&panel->obj));
            // Find the panel viewport.
            viewport = vkl_panel_viewport(panel);

            // Go through all visuals in the panel.
            for (uint32_t k = 0; k < panel->visual_count; k++)
            {
                vkl_visual_fill_event(
                    panel->visuals[k], ev.u.rf.clear_color, cmds, ev.u.rf.img_idx, viewport, NULL);
            }
        }
    }
}



/*************************************************************************************************/
/*  Builtin visual tests                                                                         */
/*************************************************************************************************/

int test_panel_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    VklGrid grid = vkl_grid(canvas, 2, 3);

    VklVisual visual = vkl_visual_builtin(canvas, VKL_VISUAL_MARKER, 0);

    vkl_panel_visual(vkl_panel(&grid, 0, 0), &visual, VKL_VIEWPORT_INNER);
    vkl_panel_visual(vkl_panel(&grid, 1, 1), &visual, VKL_VIEWPORT_INNER);

    const uint32_t N = 1000;
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    // Params.
    float param = 10.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    mat4 id = GLM_MAT4_IDENTITY_INIT;
    vkl_visual_data(&visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_PROJ, 0, 1, id);

    vkl_visual_data_texture(&visual, VKL_PROP_COLOR_TEXTURE, 0, 1, 1, 1, NULL);
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    vkl_visual_buffer(&visual, VKL_SOURCE_UNIFORM, 1, br_viewport);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_visual_update(&visual, viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _canvas_fill, &grid);

    vkl_app_run(app, N_FRAMES);
    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}
