/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "board.h"
#include "board_utils.h"
#include "resources.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzBoard dvz_board(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    ASSERT(gpu != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzBoard board = {0};
    board.gpu = gpu;
    board.flags = flags;
    board.width = width;
    board.height = height;
    board.size = width * height * 3 * sizeof(uint8_t);

    dvz_board_format(&board, DVZ_DEFAULT_FORMAT);
    dvz_board_clear_color(&board, DVZ_DEFAULT_CLEAR_COLOR);

    // TODO: larger image count?
    board.cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);

    dvz_obj_init(&board.obj);
    return board;
}



void dvz_board_format(DvzBoard* board, DvzFormat format)
{
    ASSERT(board != NULL);
    board->format = format;
    // NOTE: for now only 4 bytes per pixel. Otherwise need to update board.size as a function of
    // the format.
    log_trace("changing board format, need to recreate the board");
}



void dvz_board_clear_color(DvzBoard* board, cvec4 color)
{
    ASSERT(board != NULL);
    ASSERT(sizeof(cvec4) == 4);
    memcpy(board->clear_color, color, sizeof(cvec4));
    log_trace("changing board clear color, need to recreate the board");
}



void dvz_board_create(DvzBoard* board)
{
    ASSERT(board != NULL);

    log_trace("creating the board");

    // Renderpass.
    make_renderpass(
        board->gpu, &board->renderpass, board->format, get_clear_color(board->clear_color));

    // Make images.
    make_images(board->gpu, &board->images, board->format, board->width, board->height);

    // Make depth buffer image.
    make_depth(board->gpu, &board->depth, board->width, board->height);

    // Make staging image.
    make_staging(board->gpu, &board->staging, board->format, board->width, board->height);

    // Make framebuffers.
    make_framebuffers(
        board->gpu, &board->framebuffers, &board->renderpass, &board->images, &board->depth);

    dvz_obj_created(&board->obj);
    log_trace("board created");
}



void dvz_board_recreate(DvzBoard* board)
{
    ASSERT(board != NULL);
    log_trace("recreating the board");
    dvz_board_destroy(board);
    dvz_board_create(board);
}



void dvz_board_resize(DvzBoard* board, uint32_t width, uint32_t height)
{
    ASSERT(board != NULL);
    board->width = width;
    board->height = height;
    board->size = width * height * 3 * sizeof(uint8_t);
    // Realloc the RGBA CPU buffer storing the downloaded image.
    if (board->rgb != NULL)
    {
        dvz_board_free(board);
        dvz_board_alloc(board);
    }
    dvz_board_recreate(board);
}



void dvz_board_begin(DvzBoard* board, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(board != NULL);
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &board->renderpass, &board->framebuffers);
}



void dvz_board_viewport(DvzBoard* board, DvzCommands* cmds, uint32_t idx, vec2 offset, vec2 size)
{
    ASSERT(board != NULL);
    if (size[0] == 0)
        size[0] = board->width;
    if (size[1] == 0)
        size[1] = board->height;
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){.x = offset[0], .y = offset[1], .width = size[0], .height = size[1]});
}



void dvz_board_end(DvzBoard* board, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(board != NULL);
    ASSERT(cmds != NULL);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



uint8_t* dvz_board_alloc(DvzBoard* board)
{
    ASSERT(board != NULL);
    ASSERT(board->width > 0);
    ASSERT(board->height > 0);
    if (board->rgb == NULL)
        board->rgb = calloc(board->width * board->height, 3 * sizeof(uint8_t));
    ASSERT(board->rgb != NULL);
    return board->rgb;
}



void dvz_board_free(DvzBoard* board)
{
    ASSERT(board != NULL);
    if (board->rgb != NULL)
        FREE(board->rgb);
}



void dvz_board_download(DvzBoard* board, DvzSize size, uint8_t* rgb)
{
    ASSERT(board != NULL);
    ASSERT(size > 0);
    if (rgb == NULL)
        rgb = board->rgb;
    ASSERT(rgb != NULL);

    DvzGpu* gpu = board->gpu;
    ASSERT(gpu != NULL);

    // Start the image transition command buffers.
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
    ASSERT(board != NULL); //

    dvz_images_destroy(&board->images);
    dvz_images_destroy(&board->depth);
    dvz_images_destroy(&board->staging);
    dvz_renderpass_destroy(&board->renderpass);
    dvz_framebuffers_destroy(&board->framebuffers);

    dvz_obj_destroyed(&board->obj);
}
