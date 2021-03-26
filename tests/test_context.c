#include "../include/datoviz/context.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Buffer                                                                                       */
/*************************************************************************************************/

int test_context_buffer_default(TestContext* tc)
{
    DvzContext* ctx = tc->context;
    ASSERT(ctx != NULL);

    // Allocate buffers.
    DvzBufferRegions br = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, 1, 128);
    AT(br.aligned_size == 128);
    AT(br.count == 1);

    // Upload data.
    uint8_t data[128] = {0};
    for (uint32_t i = 0; i < 128; i++)
        data[i] = i;
    dvz_buffer_upload(br.buffer, 64, 32, data);

    // Resize buffer.
    dvz_ctx_buffers_resize(ctx, &br, 64);
    dvz_ctx_buffers_resize(ctx, &br, 256);

    // Download data.
    uint8_t data_2[32] = {0};
    dvz_buffer_download(br.buffer, 64, 32, data_2);
    for (uint32_t i = 0; i < 32; i++)
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
    cvec2 ij = {0};
    dvz_colormap_idx(cmap, 0, ij);
    AT(memcmp(&arr[256 * ij[0] + ij[1]], colors, 3 * sizeof(cvec4)) == 0);
    FREE(arr);

    return 0;
}
