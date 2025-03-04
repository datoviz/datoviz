/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_RESOURCES
#define DVZ_HEADER_TEST_RESOURCES



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../src/resources_utils.h"
#include "test.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  GPU fixture                                                                                  */
/*************************************************************************************************/

static int setup_gpu(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    log_debug("setup: creating GPU");
    ctx->host = dvz_host(DVZ_BACKEND_OFFSCREEN);
    ANN(ctx->host);
    dvz_host_create(ctx->host);

    DvzGpu* gpu = dvz_gpu_best(ctx->host);
    _default_queues(gpu, false);

    // HACK
    // dvz_gpu_extension(gpu, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    // dvz_gpu_external(gpu, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);

    dvz_gpu_create(gpu, 0);
    ctx->gpu = gpu;

    return 0;
}



static int teardown_gpu(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    log_debug("teardown: destroying GPU");
    ANN(ctx->gpu);
    dvz_gpu_destroy(ctx->gpu);

    ANN(ctx->host);
    dvz_host_destroy(ctx->host);

    return 0;
}



// Get or create the host from the suite's context.
static DvzGpu* get_gpu(TstSuite* suite)
{
    ANN(suite);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);
    DvzGpu* gpu = ctx->gpu;
    if (gpu == NULL)
    {
        log_error("you need to add the setup fixture setup_gpu()");
        // setup_host(suite);
    }
    ANN(gpu);
    return gpu;
}



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_resources_1(TstSuite* suite, TstItem* tstitem);

int test_resources_dat_1(TstSuite* suite, TstItem* tstitem);

int test_resources_tex_1(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Resources data transfers tests                                                               */
/*************************************************************************************************/

int test_resources_dat_transfers(TstSuite* suite, TstItem* tstitem);

int test_resources_dat_resize(TstSuite* suite, TstItem* tstitem);

int test_resources_tex_transfers(TstSuite* suite, TstItem* tstitem);

int test_resources_tex_resize(TstSuite* suite, TstItem* tstitem);



#endif
