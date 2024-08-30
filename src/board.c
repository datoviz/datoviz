/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "board.h"
#include "render_utils.h"
#include "resources.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzBoard
dvz_board(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t width, uint32_t height, int flags)
{
    ANN(gpu);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzBoard board = {0};
    board.obj.type = DVZ_OBJECT_TYPE_BOARD;
    board.gpu = gpu;
    board.flags = flags;
    board.width = width;
    board.height = height;
    board.size = width * height * 3 * sizeof(uint8_t);
    board.renderpass = renderpass;
    ASSERT(dvz_obj_is_created(&renderpass->obj));

    dvz_board_format(&board, DVZ_DEFAULT_FORMAT);
    // dvz_board_clear_color(&board, DVZ_DEFAULT_CLEAR_COLOR);

    board.cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);

    dvz_obj_init(&board.obj);
    return board;
}



void dvz_board_format(DvzBoard* board, DvzFormat format)
{
    ANN(board);
    board->format = format;
    // NOTE: for now only 4 bytes per pixel. Otherwise need to update board.size as a function of
    // the format.
    log_trace("changing board format, need to recreate the board");
}



void dvz_board_create(DvzBoard* board)
{
    ANN(board);

    DvzGpu* gpu = board->gpu;
    ANN(gpu);

    log_trace("creating the board");

    // Make images.
    make_images(gpu, &board->images, board->format, board->width, board->height);

    // Make depth buffer image.
    make_depth(gpu, &board->depth, 1, board->width, board->height);

    // Make staging image.
    make_staging(gpu, &board->staging, board->format, board->width, board->height);

    // Make framebuffers.
    make_framebuffers(gpu, &board->framebuffers, board->renderpass, &board->images, &board->depth);

    dvz_obj_created(&board->obj);
    log_trace("board created");
}



void dvz_board_recreate(DvzBoard* board)
{
    ANN(board);
    log_trace("recreating the board");

    // NOTE: we do not call dvz_board_destroy() because we do not want to destroy the rgb pointer
    // if it is allocated. It is reallocated in dvz_board_resize() (which calls
    // dvz_board_recreate()).
    dvz_images_destroy(&board->images);
    dvz_images_destroy(&board->depth);
    dvz_images_destroy(&board->staging);
    dvz_framebuffers_destroy(&board->framebuffers);

    dvz_board_create(board);
}



void dvz_board_resize(DvzBoard* board, uint32_t width, uint32_t height)
{
    ANN(board);
    board->width = width;
    board->height = height;
    DvzSize new_size = width * height * 3 * sizeof(uint8_t);
    DvzSize old_size = board->size;
    board->size = new_size;
    // Realloc the RGBA CPU buffer storing the downloaded image.
    if (board->rgb != NULL && new_size > old_size)
    {
        REALLOC(board->rgb, new_size)
    }
    dvz_board_recreate(board);
}



void dvz_board_begin(DvzBoard* board, DvzCommands* cmds, uint32_t idx)
{
    ANN(board);

    DvzGpu* gpu = board->gpu;
    ANN(gpu);

    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, board->renderpass, &board->framebuffers);
}



void dvz_board_viewport(DvzBoard* board, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size)
{
    ANN(board);

    // A value of 0 = full canvas.
    if (size[0] == 0)
        size[0] = board->width;
    if (size[1] == 0)
        size[1] = board->height;

    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            .x = offset[0],
            .y = offset[1],
            .width = size[0],
            .height = size[1],
            // WARNING: do not forget this otherwise depth testing may not work!
            .minDepth = 0,
            .maxDepth = 1});
}



void dvz_board_end(DvzBoard* board, DvzCommands* cmds, uint32_t idx)
{
    ANN(board);
    ANN(cmds);

    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



uint8_t* dvz_board_alloc(DvzBoard* board)
{
    ANN(board);
    ASSERT(board->width > 0);
    ASSERT(board->height > 0);
    if (board->rgb == NULL)
        board->rgb = (uint8_t*)calloc(board->width * board->height, 3 * sizeof(uint8_t));
    ANN(board->rgb);
    return board->rgb;
}



void dvz_board_free(DvzBoard* board)
{
    ANN(board);
    if (board->rgb != NULL)
        FREE(board->rgb);
}



void dvz_board_download(DvzBoard* board, DvzSize size, uint8_t* rgb)
{
    ANN(board);
    ASSERT(size > 0);
    if (rgb == NULL)
        rgb = board->rgb;
    ANN(rgb);

    DvzGpu* gpu = board->gpu;
    ANN(gpu);

    // Start the image transition command buffers.
    log_trace("starting board download");
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_TRANSFER, 1);
    dvz_cmd_begin(&cmds, 0);

    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, &board->staging);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    dvz_cmd_copy_image(&cmds, 0, &board->images, &board->staging);

    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    dvz_images_download(&board->staging, 0, 1, true, false, rgb);
}



void dvz_board_destroy(DvzBoard* board)
{
    ANN(board); //
    log_trace("destroy board");

    dvz_images_destroy(&board->images);
    dvz_images_destroy(&board->depth);
    dvz_images_destroy(&board->staging);
    dvz_framebuffers_destroy(&board->framebuffers);

    dvz_board_free(board);
    dvz_obj_destroyed(&board->obj);
}
