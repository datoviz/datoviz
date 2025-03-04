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
#include "datoviz_defaults.h"
#include "render_utils.h"
#include "resources.h"



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzCanvas
dvz_board(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t width, uint32_t height, int flags)
{
    ANN(gpu);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzCanvas board = {0};
    board.obj.type = DVZ_OBJECT_TYPE_BOARD;
    board.gpu = gpu;
    board.is_offscreen = true; // TODO: really needed? we have obj.type to know
    board.flags = flags;
    board.width = width;
    board.height = height;
    board.size = width * height * 3 * sizeof(uint8_t);
    board.render.renderpass = renderpass;
    ASSERT(dvz_obj_is_created(&renderpass->obj));

    dvz_board_format(&board, DVZ_DEFAULT_FORMAT);
    // dvz_board_clear_color(&board, DVZ_DEFAULT_CLEAR_COLOR);

    board.cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);

    dvz_obj_init(&board.obj);
    return board;
}



void dvz_board_format(DvzCanvas* board, DvzFormat format)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    board->format = format;
    // NOTE: for now only 4 bytes per pixel. Otherwise need to update board.size as a function of
    // the format.
    log_trace("changing board format, need to recreate the board");
}



void dvz_board_create(DvzCanvas* board)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);

    DvzGpu* gpu = board->gpu;
    ANN(gpu);

    log_trace("creating the board");

    // Make images.
    make_images(gpu, &board->render.images, board->format, board->width, board->height);

    // Make depth buffer image.
    make_depth(gpu, &board->render.depth, 1, board->width, board->height);

    // Make staging image.
    make_staging(gpu, &board->render.staging, board->format, board->width, board->height);

    // Make framebuffers.
    make_framebuffers(
        gpu, &board->render.framebuffers, board->render.renderpass, //
        &board->render.images, &board->render.depth);

    dvz_obj_created(&board->obj);
    log_trace("board created");
}



void dvz_board_recreate(DvzCanvas* board)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    log_trace("recreating the board");

    // NOTE: we do not call dvz_board_destroy() because we do not want to destroy the rgb pointer
    // if it is allocated. It is reallocated in dvz_board_resize() (which calls
    // dvz_board_recreate()).
    dvz_images_destroy(&board->render.images);
    dvz_images_destroy(&board->render.depth);
    dvz_images_destroy(&board->render.staging);
    dvz_framebuffers_destroy(&board->render.framebuffers);

    dvz_board_create(board);
}



void dvz_board_resize(DvzCanvas* board, uint32_t width, uint32_t height)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    board->width = width;
    board->height = height;
    DvzSize new_size = width * height * 3 * sizeof(uint8_t);
    DvzSize old_size = board->size;
    board->size = new_size;
    // Realloc the RGB CPU buffer storing the downloaded image.
    if (board->rgb != NULL && new_size > old_size)
    {
        log_debug(
            "reallocating board rgb buffer to %dx%dx3=%s (from %s before)", //
            width, height, pretty_size(new_size), pretty_size(old_size));
        REALLOC(uint8_t*, board->rgb, new_size)
    }
    dvz_board_recreate(board);
}



// NOTE: these 3 functions are aliases to their canvas counterparts, they only exist for historical
// reasons.
void dvz_board_begin(DvzCanvas* board, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    dvz_canvas_begin(board, cmds, idx);
}



void dvz_board_viewport( //
    DvzCanvas* board, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size)
{
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    dvz_canvas_viewport(board, cmds, idx, offset, size);
}



void dvz_board_end(DvzCanvas* board, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    dvz_canvas_end(board, cmds, idx);
}



uint8_t* dvz_board_alloc(DvzCanvas* board)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    ASSERT(board->width > 0);
    ASSERT(board->height > 0);
    if (board->rgb == NULL)
    {
        DvzSize size = board->width * board->height * 3 * sizeof(uint8_t);
        log_debug(
            "allocating board rgb buffer to %dx%dx3=%s", //
            board->width, board->height, pretty_size(size));
        board->rgb = (uint8_t*)calloc(size, 1);
    }
    ANN(board->rgb);
    return board->rgb;
}



void dvz_board_free(DvzCanvas* board)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    if (board->rgb != NULL)
        FREE(board->rgb);
}



void dvz_board_download(DvzCanvas* board, DvzSize size, uint8_t* rgb)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
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
    dvz_barrier_images(&barrier, &board->render.staging);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    dvz_cmd_copy_image(&cmds, 0, &board->render.images, &board->render.staging);

    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    // NOTE: the GPU image is in RGBA but this function converts it into RGB for the passed
    // pointer (has_alpha=false below).
    dvz_images_download(&board->render.staging, 0, 1, true, false, rgb);
}



void dvz_board_destroy(DvzCanvas* board)
{
    ANN(board);
    ASSERT(board->obj.type == DVZ_OBJECT_TYPE_BOARD);
    log_trace("destroy board");

    dvz_images_destroy(&board->render.images);
    dvz_images_destroy(&board->render.depth);
    dvz_images_destroy(&board->render.staging);
    dvz_framebuffers_destroy(&board->render.framebuffers);

    dvz_board_free(board);
    dvz_obj_destroyed(&board->obj);
}
