/*************************************************************************************************/
/*  GPU context holding buffers and textures in video memory                                     */
/*************************************************************************************************/

#ifndef VKL_CONTEXT_HEADER
#define VKL_CONTEXT_HEADER

#include "colormaps.h"
#include "common.h"
#include "fifo.h"
#include "transfers.h"
#include "vklite.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_DEFAULT_WIDTH  800
#define VKL_DEFAULT_HEIGHT 600

#define VKL_BUFFER_TYPE_STAGING_SIZE (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_VERTEX_SIZE  (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_INDEX_SIZE   (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_STORAGE_SIZE (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_UNIFORM_SIZE (4 * 1024 * 1024)

#define VKL_ZERO_OFFSET                                                                           \
    (uvec3) { 0, 0, 0 }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Default queue.
typedef enum
{
    VKL_DEFAULT_QUEUE_TRANSFER,
    VKL_DEFAULT_QUEUE_COMPUTE,
    VKL_DEFAULT_QUEUE_RENDER,
    VKL_DEFAULT_QUEUE_PRESENT,
    VKL_DEFAULT_QUEUE_COUNT,
} VklDefaultQueue;



// Filter type.
typedef enum
{
    VKL_FILTER_MIN,
    VKL_FILTER_MAG,
} VklFilterType;



/*************************************************************************************************/
/*  Typedefs */
/*************************************************************************************************/

typedef struct VklFontAtlas VklFontAtlas;
typedef struct VklColorTexture VklColorTexture;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklFontAtlas
{
    const char* name;
    uint32_t width, height;
    uint32_t cols, rows;
    uint8_t* font_texture;
    float glyph_width, glyph_height;
    const char* font_str;
    VklTexture* texture;
};



struct VklColorTexture
{
    const unsigned char* arr;
    VklTexture* texture;
};



struct VklContext
{
    VklObject obj;
    VklGpu* gpu;

    VklCommands transfer_cmd;

    VklContainer buffers;
    VklContainer images;
    VklContainer samplers;
    VklContainer textures;
    VklContainer computes;

    // Font atlas.
    VklFontAtlas font_atlas;
    VklColorTexture color_texture;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Get the staging buffer, and make sure it can contain `size` bytes.
static VklBuffer* staging_buffer(VklContext* context, VkDeviceSize size)
{
    VklBuffer* staging = (VklBuffer*)vkl_container_get(&context->buffers, VKL_BUFFER_TYPE_STAGING);
    ASSERT(staging != NULL);
    ASSERT(staging->buffer != VK_NULL_HANDLE);

    // Make sure the staging buffer is idle before using it.
    // TODO: optimize this and avoid hard synchronization here before copying data into
    // the staging buffer.
    vkl_queue_wait(context->gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Resize the staging buffer is needed.
    // TODO: keep staging buffer fixed and copy parts of the data to staging buffer in several
    // steps?
    if (staging->size < size)
    {
        VkDeviceSize new_size = vkl_next_pow2(size);
        log_info("reallocating staging buffer to %s", pretty_size(new_size));
        vkl_buffer_resize(staging, new_size, &context->transfer_cmd);
    }
    ASSERT(staging->size >= size);
    return staging;
}



static void _copy_buffer_from_staging(
    VklContext* context, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    VkBufferCopy region = {0};
    region.size = size;
    region.srcOffset = 0;
    region.dstOffset = br.offsets[0] + offset;
    vkCmdCopyBuffer(cmds->cmds[0], staging->buffer, br.buffer->buffer, br.count, &region);
    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we stop all
    // rendering so that we're sure that the buffer we're going to write to is not
    // being used by the GPU.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    log_debug("copy %s from staging buffer", pretty_size(size));
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_buffer_to_staging(
    VklContext* context, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Determine the offset in the source buffer.
    // Should be consecutive offsets.
    VkDeviceSize vk_offset = br.offsets[0];
    uint32_t n_regions = br.count;
    for (uint32_t i = 1; i < n_regions; i++)
    {
        ASSERT(br.offsets[i] == vk_offset + i * size);
    }
    // Take into account the transfer offset.
    vk_offset += offset;

    // Copy to staging buffer
    ASSERT(br.buffer != 0);
    vkl_cmd_copy_buffer(cmds, 0, br.buffer, vk_offset, staging, 0, size * n_regions);
    vkl_cmd_end(cmds, 0);

    // Wait for the compute queue to be idle, as we assume the buffer to be copied from may
    // be modified by compute shaders.
    // TODO: more efficient synchronization (semaphores?)
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_COMPUTE);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    log_debug("copy %s to staging buffer", pretty_size(size));
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_texture_from_staging(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    vkl_barrier_images(&barrier, texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_buffer_to_image(cmds, 0, staging, texture->image);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->image->layout);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_texture_to_staging(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    vkl_barrier_images(&barrier, texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_image_to_buffer(cmds, 0, texture->image, staging);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image->layout);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

/**
 * Create a context associated to a GPU.
 *
 * @param gpu the GPU
 * @param window the window with the surface attached to the GPU
 */
VKY_EXPORT VklContext* vkl_context(VklGpu* gpu, VklWindow* window);

/**
 * Destroy all GPU resources in a GPU context.
 *
 * @param context the context
 */
VKY_EXPORT void vkl_context_reset(VklContext* context);



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

/**
 * Allocate one of several buffer regions on the GPU.
 *
 * @param context the context
 * @param buffer_type the type of buffer to allocate the regions on
 * @param buffer_count the number of buffer regions to allocate
 * @param size the size of each region to allocate, in bytes
 */
VKY_EXPORT VklBufferRegions vkl_ctx_buffers(
    VklContext* context, VklBufferType buffer_type, uint32_t buffer_count, VkDeviceSize size);

/**
 * Resize a set of buffer regions.
 *
 * @param context the context
 * @param br the buffer regions to resize
 * @param new_size the new size of each buffer region, in bytes
 */
VKY_EXPORT void
vkl_ctx_buffers_resize(VklContext* context, VklBufferRegions* br, VkDeviceSize new_size);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

/**
 * Create a new compute pipeline.
 *
 * @param context the context
 * @param shader_path path to the `.spirv` file containing the compute shader
 */
VKY_EXPORT VklCompute* vkl_ctx_compute(VklContext* context, const char* shader_path);



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

/**
 * Create a new GPU texture.
 *
 * @param context the context
 * @param dims the number of dimensions of the texture (1, 2, or 3)
 * @param size the width, height, and depth
 * @param format the format of each pixel
 */
VKY_EXPORT VklTexture*
vkl_ctx_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format);

/**
 * Resize a texture.
 *
 * !!! warning
 *     This function will delete the texture data.
 *
 * @param texture the texture
 * @param size the new size (width, height, depth)
 */
VKY_EXPORT void vkl_texture_resize(VklTexture* texture, uvec3 size);

/**
 * Set the texture filter.
 *
 * @param texture the texture
 * @param type the filter type
 * @param filter the filter
 */
VKY_EXPORT void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter);

/**
 * Set the texture address mode.
 *
 * @param texture the texture
 * @param axis the axis
 * @param address_mode the address mode
 */
VKY_EXPORT void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode);

/**
 * Upload data to a GPU texture.
 *
 * !!! note
 *     This function should not be used to update a texture that is being used for rendering in the
 *     main event loop, otherwise full GPU synchronization needs to be done. Look at the Transfers
 *     API instead.
 *
 * @param texture the texture
 * @param offset offset within the texture
 * @param shape shape of the part of the texture to update
 * @param size size of the data to upload, in bytes
 * @param data pointer to the data to upload
 */
VKY_EXPORT void vkl_texture_upload(
    VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, const void* data);

/**
 * Download a texture from the GPU to the CPU.
 *
 * !!! note
 *     This function should not be used to download from a texture that is being used for rendering
 *     in the main event loop, otherwise full GPU synchronization needs to be done. Look at the
 *     Transfers API instead.
 *
 * @param texture the texture
 * @param offset offset within the texture
 * @param shape shape of the part of the texture to download
 * @param size size of the data to download, in bytes
 * @param data pointer to the buffer to download to (should be already allocated)
 */
VKY_EXPORT void vkl_texture_download(
    VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data);

/**
 * Copy part of a texture to another texture.
 *
 * This function does not involve CPU-GPU data transfers.
 *
 * @param src the source texture
 * @param src_offset offset within the source texture
 * @param dst the target texture
 * @param dst_offset offset within the target texture
 * @param shape shape of the part of the texture to copy
 */
VKY_EXPORT void vkl_texture_copy(
    VklTexture* src, uvec3 src_offset, VklTexture* dst, uvec3 dst_offset, uvec3 shape);

/**
 * Destroy a texture.
 *
 * @param texture the texture
 */
VKY_EXPORT void vkl_texture_destroy(VklTexture* texture);



#ifdef __cplusplus
}
#endif

#endif
