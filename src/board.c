/*************************************************************************************************/
/*  Board: offscreen surface to render on                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "board.h"
#include "resources.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_BOARD_DEFAULT_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define DVZ_BOARD_DEFAULT_CLEAR_COLOR                                                             \
    (cvec4) { 0, 8, 18, 255 }



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void make_renderpass(
    DvzGpu* gpu, DvzRenderpass* renderpass, VkFormat format, VkClearColorValue clear_color)
{
    ASSERT(gpu != NULL);
    ASSERT(renderpass != NULL);
    *renderpass = dvz_renderpass(gpu);

    VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;
    dvz_renderpass_clear(renderpass, (VkClearValue){.color = clear_color});
    dvz_renderpass_clear(renderpass, clear_depth);

    // Color attachment.
    dvz_renderpass_attachment(
        renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    dvz_renderpass_attachment_ops(
        renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    dvz_renderpass_attachment(
        renderpass, 1, //
        DVZ_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        renderpass, 1, //
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    dvz_renderpass_subpass_attachment(renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(renderpass, 0, 1);
}



static void
make_images(DvzGpu* gpu, DvzImages* images, VkFormat format, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(images != NULL);
    *images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);

    dvz_images_format(images, format);
    dvz_images_size(images, (uvec3){width, height, 1});
    dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_queue_access(images, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_images_create(images);
}


static void make_depth(DvzGpu* gpu, DvzImages* depth, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(depth != NULL);
    *depth = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);

    dvz_images_format(depth, VK_FORMAT_D32_SFLOAT);
    dvz_images_size(depth, (uvec3){width, height, 1});
    dvz_images_tiling(depth, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_memory(depth, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(depth, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth, 0);
    dvz_images_create(depth);
}



static inline VkClearColorValue get_clear_color(cvec4 color)
{
    VkClearColorValue value = {0};
    value.float32[0] = color[0] * M_INV_255;
    value.float32[1] = color[1] * M_INV_255;
    value.float32[2] = color[2] * M_INV_255;
    value.float32[3] = color[3] * M_INV_255;
    return value;
}



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzBoard dvz_board(DvzGpu* gpu, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);

    DvzBoard board = {0};

    // Renderpass.
    make_renderpass(
        gpu, &board.renderpass, DVZ_BOARD_DEFAULT_FORMAT,
        get_clear_color(DVZ_BOARD_DEFAULT_CLEAR_COLOR));

    // Make images.
    make_images(gpu, &board.images, DVZ_BOARD_DEFAULT_FORMAT, width, height);

    // Make depth buffer image.
    make_depth(gpu, &board.depth, width, height);

    return board;
}



void dvz_board_format(DvzBoard* board, VkFormat format)
{
    ASSERT(board != NULL); //
}



void dvz_board_clear_color(DvzBoard* board, cvec4 color)
{
    ASSERT(board != NULL); //
}



void dvz_board_create(DvzBoard* board)
{
    ASSERT(board != NULL); //
}



void dvz_board_resize(DvzBoard* board, uint32_t width, uint32_t height)
{
    ASSERT(board != NULL); //
}



void dvz_board_begin(DvzBoard* board, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(board != NULL); //
}



void dvz_board_end(DvzBoard* board, DvzCommands* cmds)
{
    ASSERT(board != NULL);
    ASSERT(cmds != NULL);
}



void dvz_board_download(DvzBoard* board, DvzSize size, uint8_t* rgba)
{
    ASSERT(board != NULL);
    ASSERT(size > 0);
    ASSERT(rgba != NULL);
}



void dvz_board_destroy(DvzBoard* board)
{
    ASSERT(board != NULL); //

    dvz_images_destroy(&board->images);
    dvz_images_destroy(&board->depth);
    dvz_renderpass_destroy(&board->renderpass);
}
