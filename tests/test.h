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

#include "host.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTestCtx DvzTestCtx;

// Forward declarations.
typedef struct DvzGpu DvzGpu;
typedef struct DvzResources DvzResources;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTestCtx
{
    DvzHost* host;
    DvzGpu* gpu;
    // DvzResources* res;
};



/*************************************************************************************************/
/*  Host fixture                                                                                 */
/*************************************************************************************************/

static int setup_host(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    log_debug("setup: creating host");
    ctx->host = dvz_host(DVZ_BACKEND_GLFW);
    ANN(ctx->host);
    dvz_host_create(ctx->host);

    return 0;
}



static int teardown_host(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);

    log_debug("teardown: destroying host");
    ANN(ctx->host);
    dvz_host_destroy(ctx->host);

    return 0;
}



// Get or create the host from the suite's context.
static DvzHost* get_host(TstSuite* suite)
{
    ANN(suite);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ANN(ctx);
    DvzHost* host = ctx->host;
    if (host == NULL)
    {
        log_error("you need to add the setup fixture setup_host()");
        // setup_host(suite);
    }
    ANN(host);
    return host;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests(const char* match);



#endif
