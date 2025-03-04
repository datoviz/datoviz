/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing alloc                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_resources.h"
#include "context.h"
#include "host.h"
#include "test.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_resources_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    // Create the resources object.
    DvzResources res = {0};
    dvz_resources(gpu, &res);

    // Create some GPU objects, which should be automatically destroyed upon context destruction
    // (resources destruction).
    DvzBuffer* buffer = dvz_resources_buffer(&res, DVZ_BUFFER_TYPE_VERTEX, false, 64);
    ANN(buffer);

    uvec3 shape = {2, 4, 1};
    DvzImages* img = dvz_resources_image(&res, DVZ_TEX_2D, shape, DVZ_FORMAT_R8G8B8A8_UNORM);
    ANN(img);

    DvzSampler* sampler =
        dvz_resources_sampler(&res, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    ANN(sampler);

    dvz_resources_destroy(&res);
    return 0;
}



int test_resources_dat_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    // Allocate a dat.
    DvzSize size = 128;
    DvzDat* dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    ANN(dat);

    dvz_dat_destroy(dat);
    dvz_context_destroy(ctx);
    return 0;
}



int test_resources_tex_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    // Create the context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    uvec3 shape = {2, 4, 1};
    DvzFormat format = DVZ_FORMAT_R8G8B8A8_UNORM;

    // Allocate a tex.
    DvzTex* tex = dvz_tex(ctx, DVZ_TEX_2D, shape, format, 0);
    ANN(tex);

    dvz_tex_destroy(tex);

    dvz_context_destroy(ctx);
    return 0;
}



/*************************************************************************************************/
/*  Resources data transfers tests                                                               */
/*************************************************************************************************/

int test_resources_dat_transfers(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);
    ctx->res.img_count = 3;

    // Allocate a dat.
    DvzSize size = 128;
    uint8_t data[3] = {1, 2, 3};
    uint8_t data1[3] = {0};
    DvzDat* dat = NULL;

    int flags_tests[] = {
        DVZ_DAT_FLAGS_NONE,       //
        DVZ_DAT_FLAGS_STANDALONE, //
        DVZ_DAT_FLAGS_MAPPABLE,   //
        DVZ_DAT_FLAGS_DUP,
    };

    for (uint32_t i = 0; i < sizeof(flags_tests) / sizeof(int); i++)
    {
        // dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, DVZ_DAT_FLAGS_MAPPABLE);
        dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, flags_tests[i]);
        ANN(dat);

        // Upload some data.
        dvz_dat_upload(dat, 0, sizeof(data), data, true);

        // Download back the data.
        dvz_dat_download(dat, 0, sizeof(data1), data1, true);
        // log_info("%d %d %d", data1[0], data1[1], data1[2]);
        AT(data1[0] == 1);
        AT(data1[1] == 2);
        AT(data1[2] == 3);

        dvz_dat_destroy(dat);
    }

    dvz_context_destroy(ctx);
    return 0;
}



int test_resources_dat_resize(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);
    ctx->res.img_count = 3;

    // Allocate a dat.
    DvzSize size = 16;
    uint8_t data[16] = {0};
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;

    // Upload.

    // NOTE: we need to set DVZ_DAT_FLAGS_MAPPABLE in case the Vulkan implementation has all
    // buffers mappable (like swiftshader). Otherwise there will be a mismatch between the intended
    // mappable flag, and the actual mappable property of the underlying buffer. The library
    // will find the wrong buffer and the resize will trigger the creation of a new buffer
    // that will no longer hold the data, and the test will fail.
    DvzDat* dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, DVZ_DAT_FLAGS_MAPPABLE);
    dvz_dat_upload(dat, 0, sizeof(data), data, true);

    // Resize.
    DvzSize new_size = 32;
    // should reuse the same underlying buffer so the data should still be there
    dvz_dat_resize(dat, new_size);

    // Download back the data.
    uint8_t data1[32] = {0};
    dvz_dat_download(dat, 0, sizeof(data1), data1, true);

    AT(memcmp(data1, data, size) == 0);

    dvz_dat_destroy(dat);

    dvz_context_destroy(ctx);
    return 0;
}



int test_resources_tex_transfers(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    uvec3 shape = {2, 4, 1};
    DvzFormat format = DVZ_FORMAT_R8G8B8A8_UNORM;
    DvzSize size = 4 * shape[0] * shape[1] * shape[2];

    // Create a data array.
    uint8_t data[32] = {0};
    uint8_t data1[32 * 4] = {0};
    AT(size == 32);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;

    // Allocate a tex.
    DvzTex* tex = dvz_tex(ctx, DVZ_TEX_2D, shape, format, 0);
    ANN(tex);

    // Upload some data.
    dvz_tex_upload(tex, DVZ_ZERO_OFFSET, shape, size, data, true);

    // Resize.
    uvec3 new_shape = {4, 8, 1};
    dvz_tex_resize(tex, new_shape);

    // Download back the data.
    dvz_tex_download(tex, DVZ_ZERO_OFFSET, shape, size, data1, true);

    // NOTE: there is NO guarantee that the data will be kept upon resize. Whether that is the case
    // or not is undefined behavior. for (uint32_t i = 0; i < size; i++)
    //     AT(data1[i] == i);

    dvz_tex_destroy(tex);

    dvz_context_destroy(ctx);
    return 0;
}



int test_resources_tex_resize(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    uvec3 shape = {2, 4, 1};
    DvzFormat format = DVZ_FORMAT_R8G8B8A8_UNORM;
    DvzSize size = 4 * shape[0] * shape[1] * shape[2];

    // Create a data array.
    uint8_t data[32] = {0};
    uint8_t data1[32 * 4] = {0};
    AT(size == 32);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;

    // Allocate a tex.
    DvzTex* tex = dvz_tex(ctx, DVZ_TEX_2D, shape, format, 0);
    ANN(tex);

    // Upload some data.
    dvz_tex_upload(tex, DVZ_ZERO_OFFSET, shape, size, data, true);

    // Resize.
    uvec3 new_shape = {4, 8, 1};
    dvz_tex_resize(tex, new_shape);

    // Download back the data.
    dvz_tex_download(tex, DVZ_ZERO_OFFSET, shape, size, data1, true);

    // NOTE: there is NO guarantee that the data will be kept upon resize. Whether that is the case
    // or not is undefined behavior. for (uint32_t i = 0; i < size; i++)
    //     AT(data1[i] == i);

    dvz_tex_destroy(tex);

    dvz_context_destroy(ctx);
    return 0;
}
