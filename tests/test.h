/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST
#define DVZ_HEADER_TEST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "app.h"
#include "testing.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTestCtx DvzTestCtx;

// Forward declarations.
typedef struct DvzHost DvzHost;
typedef struct DvzGpu DvzGpu;
typedef struct DvzResources DvzResources;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTestCtx
{
    DvzHost* host;
    DvzGpu* gpu;
};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define GET_HOST                                                                                  \
    ANN(suite);                                                                                   \
                                                                                                  \
    DvzHost* host = ((DvzTestCtx*)suite->context)->host;                                          \
    ANN(host);

#define GET_HOST_GPU                                                                              \
    ANN(suite);                                                                                   \
                                                                                                  \
    DvzHost* host = ((DvzTestCtx*)suite->context)->host;                                          \
    ANN(host);                                                                                    \
                                                                                                  \
    DvzGpu* gpu = ((DvzTestCtx*)suite->context)->gpu;                                             \
    ANN(gpu);



/*************************************************************************************************/
/*  Fixtures                                                                                     */
/*************************************************************************************************/

static int setup_host(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    if (ctx->host == NULL)
    {
        ctx->host = dvz_default_host(dvz_default_backend(0));
    }
    ANN(ctx->host);

    return 0;
}

static int teardown_host(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    if (ctx->host != NULL)
    {
        dvz_host_destroy(ctx->host);
    }

    return 0;
}



static int setup_gpu(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    // Get or create the host.
    setup_host(suite, tstitem);
    ANN(ctx->host);

    // Get or create the GPU.
    if (ctx->gpu == NULL)
    {
        ctx->gpu = dvz_default_gpu(ctx->host);
    }
    ANN(ctx->gpu);

    return 0;
}

static int teardown_gpu(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    if (ctx->gpu != NULL)
    {
        dvz_gpu_destroy(ctx->gpu);
    }

    teardown_host(suite, tstitem);

    return 0;
}



static int setup_offscreen(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    // TODO

    return 0;
}

static int teardown_offscreen(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    // TODO

    return 0;
}



static int setup_external(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    // TODO

    // dvz_gpu_extension(gpu, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    // dvz_gpu_external(gpu, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);

    return 0;
}

static int teardown_external(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    // TODO

    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests(const char* match);



#endif
