#include "../include/datoviz/context.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Buffer                                                                                       */
/*************************************************************************************************/

int test_context_buffer(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);
    DvzGpu* gpu = ctx->gpu;
    ASSERT(gpu != NULL);

    // Allocate buffers.
    DvzBufferRegions br = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, 1, 1024);
    VkDeviceSize offset = br.alignment;
    // DBG(br.alignment);
    // AT(br.aligned_size == 128);
    AT(br.count == 1);

    // Upload data.
    uint8_t data[128] = {0};
    for (uint32_t i = 0; i < 128; i++)
        data[i] = i;
    dvz_buffer_upload(br.buffer, offset, 32, data);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    // Resize buffer.
    dvz_ctx_buffers_resize(ctx, &br, br.alignment);
    dvz_ctx_buffers_resize(ctx, &br, 1024 * 2);

    // Download data.
    uint8_t data_2[32] = {0};
    dvz_buffer_download(br.buffer, br.alignment, 32, data_2);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    for (uint32_t i = 0; i < 32; i++)
        AT(data_2[i] == i);

    return 0;
}



int test_context_transfer_buffer(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);

    // Allocate buffers.
    DvzBufferRegions br = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_STORAGE, 1, 128);
    AT(br.count == 1);

    // Upload data.
    uint8_t data[128] = {0};
    for (uint32_t i = 0; i < 128; i++)
        data[i] = i;

    // NOTE: when the app main loop is not running (which is the case here), these transfer
    // functions process the transfers immediately.
    dvz_upload_buffer(ctx, br, 64, 32, data);

    // Download data.
    uint8_t data_2[32] = {0};
    dvz_download_buffer(ctx, br, 64, 32, data_2);
    for (uint32_t i = 0; i < 32; i++)
        AT(data_2[i] == i);

    return 0;
}



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

int test_context_compute(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);

    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/test_double.comp.spv", SPIRV_DIR);
    dvz_ctx_compute(ctx, path);

    return 0;
}



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

int test_context_texture(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);
    DvzGpu* gpu = ctx->gpu;
    ASSERT(gpu != NULL);

    uvec3 size = {16, 48, 1};
    uvec3 offset = {0, 16, 0};
    uvec3 shape = {16, 16, 1};
    VkFormat format = VK_FORMAT_R8G8B8A8_UINT;

    // Texture.
    DvzTexture* tex = dvz_ctx_texture(ctx, 2, size, format);
    dvz_texture_filter(tex, DVZ_FILTER_MAG, VK_FILTER_LINEAR);
    dvz_texture_address_mode(tex, DVZ_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    // Texture data.
    uint8_t data[256] = {0};
    for (uint32_t i = 0; i < 256; i++)
        data[i] = i;
    dvz_texture_upload(tex, offset, shape, 256, data);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    // Download data.
    uint8_t data_2[256] = {0};
    dvz_texture_download(tex, offset, shape, 256, data_2);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    for (uint32_t i = 0; i < 256; i++)
        AT(data_2[i] == i);

    // Second texture.
    log_debug("copy texture");
    DvzTexture* tex_2 = dvz_ctx_texture(ctx, 2, shape, format);
    dvz_texture_copy(tex, offset, tex_2, DVZ_ZERO_OFFSET, shape);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    // Download data.
    memset(data_2, 0, 256);
    dvz_texture_download(tex, offset, shape, 256, data_2);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    for (uint32_t i = 0; i < 256; i++)
        AT(data_2[i] == i);

    // Resize the texture.
    // NOTE: for now, the texture data is LOST when resizing.
    size[1] = 64;
    dvz_texture_resize(tex, size);
    dvz_texture_download(tex, offset, shape, 256, data_2);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    // for (uint32_t i = 0; i < 256; i++)
    //     AT(data_2[i] == i);

    return 0;
}



int test_context_transfer_texture(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);
    DvzGpu* gpu = ctx->gpu;
    ASSERT(gpu != NULL);

    uvec3 size = {16, 48, 1};
    uvec3 offset = {0, 16, 0};
    uvec3 shape = {16, 16, 1};
    VkFormat format = VK_FORMAT_R8G8B8A8_UINT;

    // Texture.
    DvzTexture* tex = dvz_ctx_texture(ctx, 2, size, format);

    // Texture data.
    uint8_t data[256] = {0};
    for (uint32_t i = 0; i < 256; i++)
        data[i] = i;
    dvz_upload_texture(ctx, tex, offset, shape, 256, data);

    // Download data.
    uint8_t data_2[256] = {0};
    dvz_download_texture(ctx, tex, offset, shape, 256, data_2);
    for (uint32_t i = 0; i < 256; i++)
        AT(data_2[i] == i);

    return 0;
}



/*************************************************************************************************/
/*  Colormap                                                                                     */
/*************************************************************************************************/

int test_context_colormap_custom(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);
    DvzGpu* gpu = ctx->gpu;
    ASSERT(gpu != NULL);

    // Make a custom colormap.
    uint8_t cmap = CMAP_CUSTOM;
    uint8_t color_count = 3;
    cvec4 colors[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };

    // Update it on the CPU.
    dvz_colormap_custom(cmap, color_count, colors);

    // Check that the CPU array has been updated.
    cvec4 out = {0};
    for (uint32_t i = 0; i < 3; i++)
    {
        dvz_colormap(cmap, i, out);
        AT(memcmp(out, colors[i], sizeof(cvec4)) == 0);
    }

    // Update the colormap texture on the GPU.
    dvz_context_colormap(ctx);

    // Check that the GPU texture has been updated.
    cvec4* arr = calloc(256 * 256, sizeof(cvec4));
    dvz_texture_download(
        ctx->color_texture.texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, //
        256 * 256 * sizeof(cvec4), arr);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    cvec2 ij = {0};
    dvz_colormap_idx(cmap, 0, ij);
    AT(memcmp(&arr[256 * ij[0] + ij[1]], colors, 3 * sizeof(cvec4)) == 0);
    FREE(arr);

    return 0;
}
