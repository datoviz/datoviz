/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing pipe                                                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_pipe.h"
#include "board.h"
#include "canvas.h"
#include "context.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "testing_utils.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_pipe_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    // Create the renderpass.
    DvzRenderpass renderpass = offscreen_renderpass(gpu);

    // Create the board.
    DvzCanvas board = dvz_board(gpu, &renderpass, WIDTH, HEIGHT, 0);
    dvz_board_create(&board);

    // Vertex buffer.
    DvzSize size = 3 * sizeof(TestVertex);
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_dat_upload(dat_vertex, 0, size, data, true);

    // Create the graphics pipe.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe);
    dvz_graphics_renderpass(graphics, &renderpass, 0);
    dvz_graphics_primitive(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);
    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/test_triangle.vert.spv", SPIRV_DIR);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle.frag.spv", SPIRV_DIR);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(graphics, 0, sizeof(TestVertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));
    dvz_pipe_vertex(&pipe, 0, dat_vertex, 0);
    dvz_pipe_create(&pipe);

    // Command buffer.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_board_begin(&board, &cmds, 0);
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, 3, 0, 1);
    dvz_board_end(&board, &cmds, 0);

    // Render.
    dvz_cmd_submit_sync(&cmds, 0);

    // Retrieve the rendered image.
    uint8_t* rgb = dvz_board_alloc(&board);
    dvz_board_download(&board, board.size, rgb);
    // Save it to a file.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/pipe.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    dvz_board_free(&board);

    // Destruction.
    dvz_pipe_destroy(&pipe);
    dvz_dat_destroy(dat_vertex);
    dvz_board_destroy(&board);
    dvz_renderpass_destroy(&renderpass);
    dvz_context_destroy(ctx);
    return 0;
}
