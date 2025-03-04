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
#include "backend.h"
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
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TEST_FLAGS_NONE = 0x00,
    DVZ_TEST_FLAGS_BACKEND = 0x01,
    DVZ_TEST_FLAGS_HOST = 0x02,
    DVZ_TEST_FLAGS_GPU = 0x04,

    DVZ_TEST_FLAGS_DESKTOP = DVZ_TEST_FLAGS_BACKEND | DVZ_TEST_FLAGS_HOST | DVZ_TEST_FLAGS_GPU,
    DVZ_TEST_FLAGS_OFFSCREEN = DVZ_TEST_FLAGS_HOST | DVZ_TEST_FLAGS_GPU,
} DvzTestFlags;



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
    GET_HOST                                                                                      \
                                                                                                  \
    DvzGpu* gpu = ((DvzTestCtx*)suite->context)->gpu;                                             \
    ANN(gpu);



/*************************************************************************************************/
/*  Fixtures                                                                                     */
/*************************************************************************************************/

static int setup(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    bool is_offscreen = !(tstitem->flags & DVZ_TEST_FLAGS_BACKEND);

    if (tstitem->flags & DVZ_TEST_FLAGS_BACKEND)
    {
        log_debug("test fixture: init backend");
        dvz_backend_init(DVZ_BACKEND_GLFW);
    }

    if (tstitem->flags & DVZ_TEST_FLAGS_HOST)
    {
        if (!ctx->host)
        {
            log_debug("test fixture: init host");
            ctx->host = dvz_default_host(is_offscreen ? DVZ_BACKEND_OFFSCREEN : DVZ_BACKEND_GLFW);
        }
        ANN(ctx->host);
    }

    if (tstitem->flags & DVZ_TEST_FLAGS_GPU)
    {
        if (!ctx->gpu)
        {
            ANN(ctx->host);

            ctx->gpu = dvz_default_gpu(ctx->host);
            ANN(ctx->gpu);

            if (is_offscreen)
            {
                log_debug("test fixture: init GPU without surface");
                dvz_gpu_create(ctx->gpu, NULL);
            }
            else
            {
                log_debug("test fixture: init GPU with surface");
                dvz_gpu_create_with_surface(ctx->gpu);
            }
        }
        ANN(ctx->gpu);
        ASSERT(dvz_obj_is_created(&ctx->gpu->obj));
    }

    return 0;
}



static int teardown(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    if (tstitem->flags & DVZ_TEST_FLAGS_GPU)
    {
        if (ctx->gpu)
        {
            log_debug("test fixture: destroy GPU");
            dvz_gpu_destroy(ctx->gpu);
            ctx->gpu = NULL;
        }
    }

    if (tstitem->flags & DVZ_TEST_FLAGS_HOST)
    {
        if (ctx->host)
        {
            log_debug("test fixture: destroy host");
            dvz_host_destroy(ctx->host);
            ctx->host = NULL;
        }
    }

    if (tstitem->flags & DVZ_TEST_FLAGS_BACKEND)
    {
        log_debug("test fixture: destroy backend");
        dvz_backend_terminate(DVZ_BACKEND_GLFW);
    }

    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests(const char* match);



#endif
