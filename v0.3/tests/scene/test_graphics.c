/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing graphics                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_graphics.h"
#include "_cglm.h"
#include "board.h"
#include "context.h"
#include "datoviz.h"
#include "fileio.h"
#include "host.h"
#include "pipe.h"
#include "scene/graphics.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "testing_utils.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

#define GRAPHICS_BEGIN                                                                            \
    ANN(suite);                                                                                   \
                                                                                                  \
    DvzHost* host = dvz_host();                                                                   \
    ANN(host);                                                                                    \
    dvz_host_backend(host, DVZ_BACKEND_GLFW);                                                     \
    dvz_host_create(host);                                                                        \
                                                                                                  \
    DvzGpu* gpu = dvz_gpu_best(host);                                                             \
    _default_queues(gpu, false);                                                                  \
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});          \
    dvz_gpu_create(gpu, 0);                                                                       \
                                                                                                  \
    DvzContext* ctx = dvz_context(gpu);                                                           \
    ANN(ctx);                                                                                     \
                                                                                                  \
    DvzRenderpass renderpass = offscreen_renderpass(gpu);                                         \
                                                                                                  \
    DvzCanvas board = dvz_board(gpu, &renderpass, WIDTH, HEIGHT, 0);                              \
    dvz_board_create(&board);

#define GRAPHICS_MVP                                                                              \
    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);                   \
    ANN(dat_mvp);                                                                                 \
    DvzMVP mvp = {0};                                                                             \
    glm_mat4_identity(mvp.model);                                                                 \
    glm_mat4_identity(mvp.view);                                                                  \
    glm_mat4_identity(mvp.proj);                                                                  \
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

#define GRAPHICS_VIEWPORT                                                                         \
    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);         \
    ANN(dat_viewport);                                                                            \
    DvzViewport viewport = {0};                                                                   \
    dvz_viewport_default(WIDTH, HEIGHT, &viewport);                                               \
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);

#define GRAPHICS_SCREENSHOT(name)                                                                 \
    uint8_t* rgb = dvz_board_alloc(&board);                                                       \
    dvz_board_download(&board, board.size, rgb);                                                  \
    char imgpath[1024] = {0};                                                                     \
    snprintf(imgpath, sizeof(imgpath), "%s/graphics_%s.png", ARTIFACTS_DIR, (name));              \
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);                                                   \
    dvz_board_free(&board);

#define GRAPHICS_END                                                                              \
    dvz_dat_destroy(dat_vertex);                                                                  \
    dvz_dat_destroy(dat_mvp);                                                                     \
    dvz_dat_destroy(dat_viewport);                                                                \
    dvz_pipe_destroy(&pipe);                                                                      \
    dvz_board_destroy(&board);                                                                    \
    dvz_renderpass_destroy(&renderpass);                                                          \
    dvz_context_destroy(ctx);                                                                     \
    dvz_gpu_destroy(gpu);                                                                         \
    dvz_host_destroy(host);



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TstSuite* suite, TstItem* tstitem)
{
    // Create the board and context
    GRAPHICS_BEGIN

    // Create the graphics.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe);
    dvz_graphics_builtin(&renderpass, graphics, DVZ_GRAPHICS_POINT, 0);

    const uint32_t n = 50;

    // Create the dats.
    DvzDat* dat_vertex =
        dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, n * sizeof(DvzGraphicsPointVertex), 0);
    ANN(dat_vertex);

    // Slots 0 and 1.
    GRAPHICS_MVP
    GRAPHICS_VIEWPORT

    // Create the descriptors.
    dvz_pipe_vertex(&pipe, 0, dat_vertex, 0);
    dvz_pipe_dat(&pipe, 0, dat_mvp);
    dvz_pipe_dat(&pipe, 1, dat_viewport);
    dvz_pipe_create(&pipe);

    // Upload the data.
    DvzGraphicsPointVertex* data = calloc(n, sizeof(DvzGraphicsPointVertex));
    double t = 0;
    double aspect = WIDTH / (double)HEIGHT;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        data[i].pos[0] = .5 * cos(M_2PI * t);
        data[i].pos[1] = aspect * .5 * sin(M_2PI * t);

        data[i].size = 50;

        dvz_colormap(DVZ_CMAP_HSV, ALPHA_F2D(t), data[i].color);
        data[i].color[3] = ALPHA_U2D(128);
    }
    dvz_dat_upload(dat_vertex, 0, n * sizeof(DvzGraphicsPointVertex), data, true);
    FREE(data)

    // Commands.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    dvz_board_begin(&board, &cmds, 0);
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, n, 0, 1);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    GRAPHICS_SCREENSHOT("point")

    // Destruction
    GRAPHICS_END

    return 0;
}



int test_graphics_triangle(TstSuite* suite, TstItem* tstitem)
{
    // Create the board and context
    GRAPHICS_BEGIN

    // Create the graphics.
    DvzPipe pipe = dvz_pipe(gpu);
    DvzGraphics* graphics = dvz_pipe_graphics(&pipe);
    dvz_graphics_builtin(&renderpass, graphics, DVZ_GRAPHICS_TRIANGLE, 0);

    // Create the dats.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ANN(dat_vertex);

    // Slots 0 and 1.
    GRAPHICS_MVP
    GRAPHICS_VIEWPORT

    // Create the descriptors.
    dvz_pipe_vertex(&pipe, 0, dat_vertex, 0);
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
    dvz_board_viewport(&board, &cmds, 0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pipe_draw(&pipe, &cmds, 0, 0, 3, 0, 1);
    dvz_board_end(&board, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);

    // Screenshot.
    GRAPHICS_SCREENSHOT("triangle")

    // Destruction
    GRAPHICS_END

    return 0;
}
