/*************************************************************************************************/
/*  Testing alloc                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "resources.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_resources_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Create the resources object.
    DvzResources res = {0};
    dvz_resources(gpu, &res);

    // Create some GPU objects, which should be automatically destroyed upon context destruction
    // (resources destruction).
    DvzBuffer* buffer = dvz_resources_buffer(&res, DVZ_BUFFER_TYPE_VERTEX, false, 64);
    ASSERT(buffer != NULL);

    uvec3 shape = {2, 4, 1};
    DvzImages* img = dvz_resources_image(&res, DVZ_TEX_2D, shape, VK_FORMAT_R8G8B8A8_UNORM);
    ASSERT(img != NULL);

    DvzSampler* sampler =
        dvz_resources_sampler(&res, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    ASSERT(sampler != NULL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_double.comp.spv", SPIRV_DIR);
    DvzCompute* compute = dvz_resources_compute(&res, path);
    ASSERT(compute != NULL);
    dvz_resources_destroy(&res);

    return 0;
}



int test_resources_dat_1(TstSuite* suite)
{
    // ASSERT(tc != NULL);
    // DvzContext* ctx = tc->context;
    // ASSERT(ctx != NULL);

    // DvzGpu* gpu = ctx->gpu;
    // ASSERT(gpu != NULL);

    // uint32_t img_count = 3;
    // dvz_context_img_count(ctx, img_count);

    // // Allocate a dat.
    // VkDeviceSize size = 128;
    // uint8_t data[3] = {1, 2, 3};
    // uint8_t data1[3] = {0};
    // DvzDat* dat = NULL;

    // int flags_tests[] = {
    //     DVZ_DAT_OPTIONS_NONE,       //
    //     DVZ_DAT_OPTIONS_STANDALONE, //
    //     DVZ_DAT_OPTIONS_MAPPABLE,   //
    //     DVZ_DAT_OPTIONS_DUP,
    // };

    // for (uint32_t i = 0; i < sizeof(flags_tests) / sizeof(int); i++)
    // {
    //     // dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, DVZ_DAT_OPTIONS_MAPPABLE);
    //     dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, flags_tests[i]);
    //     ASSERT(dat != NULL);

    //     // Upload some data.
    //     dvz_dat_upload(dat, 0, sizeof(data), data, true);

    //     // Download back the data.
    //     dvz_dat_download(dat, 0, sizeof(data1), data1, true);
    //     // log_info("%d %d %d", data1[0], data1[1], data1[2]);
    //     AT(data1[0] == 1);
    //     AT(data1[1] == 2);
    //     AT(data1[2] == 3);

    //     dvz_dat_destroy(dat);
    // }

    return 0;
}



int test_resources_tex_1(TstSuite* suite)
{
    // ASSERT(tc != NULL);
    // DvzContext* ctx = tc->context;
    // ASSERT(ctx != NULL);

    // DvzGpu* gpu = ctx->gpu;
    // ASSERT(gpu != NULL);

    // uvec3 shape = {2, 4, 1};
    // VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    // VkDeviceSize size = 4 * shape[0] * shape[1] * shape[2];

    // // Create a data array.
    // uint8_t data[32] = {0};
    // uint8_t data1[32] = {0};
    // ASSERT(size == 32);
    // for (uint32_t i = 0; i < size; i++)
    //     data[i] = i;

    // // Allocate a tex.
    // DvzTex* tex = dvz_tex(ctx, DVZ_TEX_2D, shape, format, 0);
    // ASSERT(tex != NULL);

    // // Upload some data.
    // dvz_tex_upload(tex, DVZ_ZERO_OFFSET, shape, size, data, true);

    // // Download back the data.
    // dvz_tex_download(tex, DVZ_ZERO_OFFSET, shape, size, data1, true);

    // for (uint32_t i = 0; i < size; i++)
    //     // log_info("%d", data1[i]);
    //     AT(data1[i] == i);

    // dvz_tex_destroy(tex);

    return 0;
}
