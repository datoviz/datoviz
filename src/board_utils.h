/*************************************************************************************************/
/*  Board utils                                                                                  */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BOARD_UTILS
#define DVZ_HEADER_BOARD_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "board.h"
#include "resources.h"



/*************************************************************************************************/
/*  Board creation utils                                                                         */
/*************************************************************************************************/

static void
make_images(DvzGpu* gpu, DvzImages* images, DvzFormat format, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(images != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_trace("making images");
    *images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);

    dvz_images_format(images, (VkFormat)format);
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
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_trace("making depth image");
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



static void
make_staging(DvzGpu* gpu, DvzImages* staging, DvzFormat format, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    ASSERT(staging != NULL);
    ASSERT(format != 0);
    ASSERT(width > 0);
    ASSERT(height > 0);

    log_trace("making staging images");
    *staging = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);

    dvz_images_format(staging, (VkFormat)format);
    dvz_images_size(staging, (uvec3){width, height, 1});
    dvz_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // dvz_images_memory(
    //     staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_images_vma_usage(staging, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_images_create(staging);
}



static void make_framebuffers(
    DvzGpu* gpu, DvzFramebuffers* framebuffers, DvzRenderpass* renderpass, //
    DvzImages* images, DvzImages* depth)
{
    ASSERT(gpu != NULL);
    ASSERT(framebuffers != NULL);
    ASSERT(renderpass != NULL);
    ASSERT(images != NULL);
    ASSERT(depth != NULL);
    log_trace("making framebuffers");

    *framebuffers = dvz_framebuffers(gpu);
    dvz_framebuffers_attachment(framebuffers, 0, images);
    dvz_framebuffers_attachment(framebuffers, 1, depth);
    dvz_framebuffers_create(framebuffers, renderpass);
}



#endif
