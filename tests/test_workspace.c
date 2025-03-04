/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing workspace                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_workspace.h"
#include "board.h"
#include "canvas.h"
#include "fileio.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "testing_utils.h"
#include "vklite.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Workspace tests                                                                              */
/*************************************************************************************************/

int test_workspace_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzWorkspace* ws = dvz_workspace(gpu, DVZ_RENDERER_FLAGS_WHITE_BACKGROUND);
    DvzCanvas* board = dvz_workspace_board(ws, WIDTH, HEIGHT, 0);
    ANN(board);

    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_begin_renderpass(&cmds, 0, board->render.renderpass, &board->render.framebuffers);
    dvz_cmd_end_renderpass(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Retrieve the rendered image.
    uint8_t* rgb = dvz_board_alloc(board);
    dvz_board_download(board, board->size, rgb);
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/workspace.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);

    for (uint32_t i = 0; i < WIDTH * HEIGHT * 3; i++)
        AT(rgb[i] == 255)
    dvz_board_free(board);

    dvz_workspace_destroy(ws);
    return 0;
}
