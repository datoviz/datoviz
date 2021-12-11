/*************************************************************************************************/
/*  Testing graphics                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_graphics.h"
#include "board.h"
#include "context.h"
#include "fileio.h"
#include "graphics.h"
#include "host.h"
#include "pipe.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_triangle(TstSuite* suite)
{
    ASSERT(suite != NULL);

    // Host.
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);

    // GPU.
    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, false);
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});
    dvz_gpu_create(gpu, 0);

    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);

    // Create the board.
    DvzBoard board = dvz_board(gpu, WIDTH, HEIGHT);
    dvz_board_create(&board);

    // Create the graphics.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe, 1);
    dvz_graphics_builtin(&board.renderpass, graphics, DVZ_GRAPHICS_TRIANGLE, 0);

    // Create the dats.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ASSERT(dat_vertex != NULL);

    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    ASSERT(dat_mvp != NULL);
    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    ASSERT(dat_viewport != NULL);
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);

    // Create the bindings.
    dvz_pipe_vertex(&pipe, dat_vertex);
    dvz_pipe_dat(&pipe, 0, dat_mvp);
    dvz_pipe_dat(&pipe, 1, dat_viewport);
    dvz_pipe_create(&pipe);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    dvz_dat_upload(dat_vertex, 0, sizeof(data), data, true);

    // Commands.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_board_begin(&board, &cmds, 0);
    dvz_board_viewport(&board, &cmds, 0, DVZ_VIEWPORT_DEFAULT, DVZ_VIEWPORT_DEFAULT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, 3);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    uint8_t* rgb = dvz_board_alloc(&board);
    dvz_board_download(&board, board.size, rgb);
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/graphics_triangle.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    dvz_board_free(&board);

    // Destruction
    dvz_dat_destroy(dat_vertex);
    dvz_dat_destroy(dat_mvp);
    dvz_dat_destroy(dat_viewport);
    dvz_pipe_destroy(&pipe);
    dvz_board_destroy(&board);
    dvz_context_destroy(ctx);
    dvz_gpu_destroy(gpu);
    dvz_host_destroy(host);

    return 0;
}
