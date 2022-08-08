/*************************************************************************************************/
/*  Testing pipelib                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_pipelib.h"
#include "board.h"
#include "fileio.h"
#include "pipe.h"
#include "pipelib.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Pipelib tests                                                                                */
/*************************************************************************************************/

int test_pipelib_1(TstSuite* suite)
{
    ANN(suite);
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    uvec2 size = {WIDTH, HEIGHT};

    // Create the renderpass.
    DvzRenderpass renderpass = offscreen_renderpass(gpu);

    // Create the board.
    DvzBoard board = dvz_board(gpu, &renderpass, WIDTH, HEIGHT, 0);
    dvz_board_create(&board);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create a graphics pipe.
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &renderpass, 1, size, DVZ_GRAPHICS_TRIANGLE,
        DVZ_PIPELIB_FLAGS_CREATE_MVP | DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT);

    // Create the vertex buffer dat.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ANN(dat_vertex);
    dvz_pipe_vertex(pipe, dat_vertex);

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
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(pipe, &cmds, 0, 0, 3);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    uint8_t* rgb = dvz_board_alloc(&board);
    dvz_board_download(&board, board.size, rgb);
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/pipelib.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    dvz_board_free(&board);

    // Destruction.
    dvz_pipelib_destroy(lib);
    dvz_board_destroy(&board);
    dvz_renderpass_destroy(&renderpass);
    dvz_context_destroy(ctx);
    return 0;
}
