#include "test_panel.h"
#include "../include/datoviz/builtin_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _canvas_fill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzGrid* grid = (DvzGrid*)ev.user_data;
    ASSERT(grid != NULL);

    DvzViewport viewport = {0};
    DvzCommands* cmds = NULL;
    DvzPanel* panel = NULL;
    DvzContainerIterator iter;
    uint32_t img_idx = 0;

    // Go through all the current command buffers.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        cmds = ev.u.rf.cmds[i];
        img_idx = ev.u.rf.img_idx;

        dvz_visual_fill_begin(canvas, cmds, img_idx);

        iter = dvz_container_iterator(&grid->panels);
        while (iter.item != NULL)
        {
            panel = iter.item;
            ASSERT(dvz_obj_is_created(&panel->obj));
            // Find the panel viewport.
            viewport = dvz_panel_viewport(panel);
            dvz_cmd_viewport(cmds, img_idx, viewport.viewport);

            // Go through all visuals in the panel.
            for (uint32_t k = 0; k < panel->visual_count; k++)
            {
                dvz_visual_fill_event(
                    panel->visuals[k], ev.u.rf.clear_color, cmds, img_idx, viewport, NULL);
            }
            dvz_container_iter(&iter);
        }

        dvz_visual_fill_end(canvas, cmds, img_idx);
    }
}



/*************************************************************************************************/
/*  Panel tests                                                                                  */
/*************************************************************************************************/

static void _canvas_click(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzGrid* grid = (DvzGrid*)ev.user_data;
    ASSERT(grid != NULL);
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    float x = ev.u.c.pos[0] / size[0];
    float y = ev.u.c.pos[1] / size[1];
    uint32_t col = (uint32_t)(x * 2);
    uint32_t row = (uint32_t)(y * 3);
    dvz_panel_cell((DvzPanel*)grid->panels.items[0], row, col);
}

int test_panel_1(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    DvzGrid grid = dvz_grid(canvas, 2, 3);

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_POINT, 0);

    dvz_panel_visual(dvz_panel(&grid, 0, 0), &visual);

    // Visual data.
    const uint32_t N = 1000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float param = 10.0f;
    float zero = 0.0f;
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    {
        dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
        dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

        dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

        dvz_visual_data(&visual, DVZ_PROP_MODEL, 0, 1, id);
        dvz_visual_data(&visual, DVZ_PROP_VIEW, 0, 1, id);
        dvz_visual_data(&visual, DVZ_PROP_PROJ, 0, 1, id);
        dvz_visual_data(&visual, DVZ_PROP_TIME, 0, 1, &zero);

        dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, 0, 1, 1, NULL);
    }

    dvz_visual_update(&visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _canvas_fill, &grid);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_CLICK, 0, DVZ_EVENT_MODE_SYNC, _canvas_click, &grid);

    dvz_app_run(app, N_FRAMES);
    dvz_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}
