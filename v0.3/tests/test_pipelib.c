/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing pipelib                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_pipelib.h"
#include "board.h"
#include "canvas.h"
#include "fileio.h"
#include "pipe.h"
#include "pipelib.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Pipelib tests                                                                                */
/*************************************************************************************************/

int test_pipelib_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    // Create the renderpass.
    DvzRenderpass renderpass = offscreen_renderpass(gpu);

    // Create the board.
    DvzCanvas board = dvz_board(gpu, &renderpass, WIDTH, HEIGHT, 0);
    dvz_board_create(&board);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create a graphics pipe.
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &renderpass, DVZ_GRAPHICS_TRIANGLE,
        DVZ_PIPELIB_FLAGS_CREATE_MVP | DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT);

    VkShaderStageFlagBits stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    dvz_graphics_push(&pipe->u.graphics, stages, 0, sizeof(float));

    // NOTE: we now have to create the pipe manually (or automatically when using recorder.c).
    dvz_pipe_create(pipe);

    // Create the vertex buffer dat.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ANN(dat_vertex);
    dvz_pipe_vertex(pipe, 0, dat_vertex, 0);

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
    // dvz_cmd_push(&cmds, 0, &pipe->u.graphics.dslots, stages, 0, sizeof(float), (float[]){1});
    dvz_pipe_draw(pipe, &cmds, 0, 0, 3, 0, 1);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    uint8_t* rgb = dvz_board_alloc(&board);
    dvz_board_download(&board, board.size, rgb);
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/pipelib.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    dvz_board_free(&board);

    // Shader.
    char* glsl = "void main() {gl_Position = vec4(0,0,0,0);}";
    dvz_pipelib_shader(lib, DVZ_SHADER_GLSL, DVZ_SHADER_VERTEX, strnlen(glsl, 1024), glsl, NULL);

    // Destruction.
    dvz_pipelib_destroy(lib);
    dvz_board_destroy(&board);
    dvz_renderpass_destroy(&renderpass);
    dvz_context_destroy(ctx);
    return 0;
}
