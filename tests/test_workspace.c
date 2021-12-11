/*************************************************************************************************/
/*  Testing workspace                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_workspace.h"
#include "board.h"
#include "fileio.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Workspace tests                                                                              */
/*************************************************************************************************/

int test_workspace_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    DvzWorkspace* ws = dvz_workspace(gpu);
    DvzBoard* board = dvz_workspace_board(ws, WIDTH, HEIGHT, 0);
    ASSERT(board != NULL);

    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_begin_renderpass(&cmds, 0, &board->renderpass, &board->framebuffers);
    dvz_cmd_end_renderpass(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Retrieve the rendered image.
    uint8_t* rgb = dvz_board_alloc(board);
    dvz_board_download(board, board->size, rgb);
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/workspace.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);

    AT(board->clear_color[2] > 0);
    for (uint32_t i = 0; i < WIDTH * HEIGHT * 3; i++)
        AT(rgb[i] == board->clear_color[i % 3])
    dvz_board_free(board);

    dvz_workspace_destroy(ws);
    return 0;
}
