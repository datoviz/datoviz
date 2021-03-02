/*************************************************************************************************/
/*  GPU context holding buffers and textures in video memory                                     */
/*************************************************************************************************/

#ifndef DVZ_CONTEXT_HEADER
#define DVZ_CONTEXT_HEADER

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

#define DVZ_DEFAULT_WIDTH  800
#define DVZ_DEFAULT_HEIGHT 600

#define DVZ_BUFFER_TYPE_STAGING_SIZE (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_VERTEX_SIZE  (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_INDEX_SIZE   (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_STORAGE_SIZE (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_UNIFORM_SIZE (4 * 1024 * 1024)

#define DVZ_ZERO_OFFSET                                                                           \
    (uvec3) { 0, 0, 0 }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Default queue.
typedef enum
{
    DVZ_DEFAULT_QUEUE_TRANSFER,
    DVZ_DEFAULT_QUEUE_COMPUTE,
    DVZ_DEFAULT_QUEUE_RENDER,
    DVZ_DEFAULT_QUEUE_PRESENT,
    DVZ_DEFAULT_QUEUE_COUNT,
} DvzDefaultQueue;



// Filter type.
typedef enum
{
    DVZ_FILTER_MIN,
    DVZ_FILTER_MAG,
} DvzFilterType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFontAtlas DvzFontAtlas;
typedef struct DvzColorTexture DvzColorTexture;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFontAtlas
{
    const char* name;
    uint32_t width, height;
    uint32_t cols, rows;
    uint8_t* font_texture;
    float glyph_width, glyph_height;
    const char* font_str;
    DvzTexture* texture;
};



struct DvzColorTexture
{
    unsigned char* arr;
    DvzTexture* texture;
};



struct DvzContext
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzCommands transfer_cmd;

    DvzContainer buffers;
    DvzContainer images;
    DvzContainer samplers;
    DvzContainer textures;
    DvzContainer computes;

    // Font atlas.
    DvzFontAtlas font_atlas;
    DvzColorTexture color_texture;
    DvzTexture* transfer_texture; // Default linear 1D texture
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Get the staging buffer, and make sure it can contain `size` bytes.
static DvzBuffer* staging_buffer(DvzContext* context, VkDeviceSize size)
{
    DvzBuffer* staging = (DvzBuffer*)dvz_container_get(&context->buffers, DVZ_BUFFER_TYPE_STAGING);
    ASSERT(staging != NULL);
    ASSERT(staging->buffer != VK_NULL_HANDLE);

    // Make sure the staging buffer is idle before using it.
    // TODO: optimize this and avoid hard synchronization here before copying data into
    // the staging buffer.
    dvz_queue_wait(context->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    // Resize the staging buffer is needed.
    // TODO: keep staging buffer fixed and copy parts of the data to staging buffer in several
    // steps?
    if (staging->size < size)
    {
        VkDeviceSize new_size = dvz_next_pow2(size);
        log_info("reallocating staging buffer to %s", pretty_size(new_size));
        dvz_buffer_resize(staging, new_size, &context->transfer_cmd);
    }
    ASSERT(staging->size >= size);
    return staging;
}



static void _copy_buffer_from_staging(
    DvzContext* context, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands* cmds = &context->transfer_cmd;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    VkBufferCopy region = {0};
    region.size = size;
    region.srcOffset = 0;
    region.dstOffset = br.offsets[0] + offset;
    vkCmdCopyBuffer(cmds->cmds[0], staging->buffer, br.buffer->buffer, br.count, &region);
    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we stop all
    // rendering so that we're sure that the buffer we're going to write to is not
    // being used by the GPU.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    log_debug("copy %s from staging buffer", pretty_size(size));
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_buffer_to_staging(
    DvzContext* context, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands* cmds = &context->transfer_cmd;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

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
    dvz_cmd_copy_buffer(cmds, 0, br.buffer, vk_offset, staging, 0, size * n_regions);
    dvz_cmd_end(cmds, 0);

    // Wait for the compute queue to be idle, as we assume the buffer to be copied from may
    // be modified by compute shaders.
    // TODO: more efficient synchronization (semaphores?)
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_COMPUTE);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    log_debug("copy %s to staging buffer", pretty_size(size));
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_texture_from_staging(
    DvzContext* context, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands* cmds = &context->transfer_cmd;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    dvz_barrier_images(&barrier, texture->image);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    dvz_cmd_copy_buffer_to_image(cmds, 0, staging, texture->image);

    // Image transition.
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->image->layout);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_texture_to_staging(
    DvzContext* context, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands* cmds = &context->transfer_cmd;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    dvz_barrier_images(&barrier, texture->image);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    dvz_cmd_copy_image_to_buffer(cmds, 0, texture->image, staging);

    // Image transition.
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image->layout);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
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
DVZ_EXPORT DvzContext* dvz_context(DvzGpu* gpu, DvzWindow* window);

/**
 * Destroy all GPU resources in a GPU context.
 *
 * @param context the context
 */
DVZ_EXPORT void dvz_context_reset(DvzContext* context);

/**
 * Update the colormap texture on the GPU after it has changed on the CPU.
 *
 * @param context the context
 */
DVZ_EXPORT void dvz_context_colormap(DvzContext* context);



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
DVZ_EXPORT DvzBufferRegions dvz_ctx_buffers(
    DvzContext* context, DvzBufferType buffer_type, uint32_t buffer_count, VkDeviceSize size);

/**
 * Resize a set of buffer regions.
 *
 * @param context the context
 * @param br the buffer regions to resize
 * @param new_size the new size of each buffer region, in bytes
 */
DVZ_EXPORT void
dvz_ctx_buffers_resize(DvzContext* context, DvzBufferRegions* br, VkDeviceSize new_size);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

/**
 * Create a new compute pipeline.
 *
 * @param context the context
 * @param shader_path path to the `.spirv` file containing the compute shader
 */
DVZ_EXPORT DvzCompute* dvz_ctx_compute(DvzContext* context, const char* shader_path);



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
DVZ_EXPORT DvzTexture*
dvz_ctx_texture(DvzContext* context, uint32_t dims, uvec3 size, VkFormat format);

/**
 * Resize a texture.
 *
 * !!! warning
 *     This function will delete the texture data.
 *
 * @param texture the texture
 * @param size the new size (width, height, depth)
 */
DVZ_EXPORT void dvz_texture_resize(DvzTexture* texture, uvec3 size);

/**
 * Set the texture filter.
 *
 * @param texture the texture
 * @param type the filter type
 * @param filter the filter
 */
DVZ_EXPORT void dvz_texture_filter(DvzTexture* texture, DvzFilterType type, VkFilter filter);

/**
 * Set the texture address mode.
 *
 * @param texture the texture
 * @param axis the axis
 * @param address_mode the address mode
 */
DVZ_EXPORT void dvz_texture_address_mode(
    DvzTexture* texture, DvzTextureAxis axis, VkSamplerAddressMode address_mode);

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
DVZ_EXPORT void dvz_texture_upload(
    DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, const void* data);

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
DVZ_EXPORT void dvz_texture_download(
    DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data);

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
DVZ_EXPORT void dvz_texture_copy(
    DvzTexture* src, uvec3 src_offset, DvzTexture* dst, uvec3 dst_offset, uvec3 shape);

/**
 * Destroy a texture.
 *
 * @param texture the texture
 */
DVZ_EXPORT void dvz_texture_destroy(DvzTexture* texture);



static DvzTexture* _default_transfer_texture(DvzContext* context)
{
    uvec3 shape = {256, 1, 1};
    DvzTexture* texture = dvz_ctx_texture(context, 1, shape, VK_FORMAT_R32_SFLOAT);
    float* tex_data = (float*)calloc(256, sizeof(float));
    for (uint32_t i = 0; i < 256; i++)
        tex_data[i] = i / 255.0;
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    uvec3 offset = {0, 0, 0};
    dvz_texture_upload(texture, offset, offset, 256 * sizeof(float), tex_data);
    FREE(tex_data);
    return texture;
}



#ifdef __cplusplus
}
#endif

#endif
