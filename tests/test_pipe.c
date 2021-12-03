/*************************************************************************************************/
/*  Testing pipe                                                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_pipe.h"
#include "board.h"
#include "context.h"
#include "fileio.h"
#include "graphics.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_pipe_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);

    // Create the board.
    DvzBoard board = dvz_board(gpu, WIDTH, HEIGHT);
    dvz_board_create(&board);
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);

    // Create the graphics.
    DvzGraphics graphics = triangle_graphics(gpu, &board.renderpass, "");
    // Create the graphics pipeline.
    dvz_graphics_create(&graphics);

    // Vertex buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_dat_upload(dat_vertex, 0, size, data, true);

    // Create the pipe.
    DvzPipe pipe = dvz_pipe(gpu);
    dvz_pipe_graphics(&pipe, &graphics, 1);
    dvz_pipe_vertex(&pipe, dat_vertex);
    dvz_pipe_create(&pipe);

    // Command buffer.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_board_begin(&board, &cmds, 0);
    dvz_cmd_viewport(&cmds, 0, viewport.viewport);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, 3);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_end(&cmds, 0);

    // Render.
    dvz_cmd_submit_sync(&cmds, 0);

    // Retrieve the rendered image.
    uint8_t* rgba = dvz_board_alloc(&board);
    dvz_board_download(&board, board.size, rgba);
    // Save it to a file.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/pipe.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgba);
    dvz_board_free(&board);

    // Destruction.
    dvz_graphics_destroy(&graphics);
    dvz_dat_destroy(dat_vertex);
    dvz_pipe_destroy(&pipe);

    dvz_board_destroy(&board);
    dvz_context_destroy(ctx);
    return 0;
}
