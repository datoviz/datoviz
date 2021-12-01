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

static int setup_gpu(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ASSERT(ctx != NULL);

    log_debug("setup: creating GPU");
    ctx->host = dvz_host(DVZ_BACKEND_GLFW);

    DvzGpu* gpu = dvz_gpu_best(ctx->host);
    _default_queues(gpu, false);
    dvz_gpu_create(gpu, 0);
    ctx->gpu = gpu;

    return 0;
}



static int teardown_gpu(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ASSERT(ctx != NULL);

    log_debug("teardown: destroying GPU");
    ASSERT(ctx->gpu != NULL);
    dvz_gpu_destroy(ctx->gpu);

    ASSERT(ctx->host != NULL);
    dvz_host_destroy(ctx->host);

    return 0;
}



// Get or create the host from the suite's context.
static DvzGpu* get_gpu(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ASSERT(ctx != NULL);
    DvzGpu* gpu = ctx->gpu;
    if (gpu == NULL)
    {
        log_error("you need to add the setup fixture setup_gpu()");
        // setup_host(suite);
    }
    ASSERT(gpu != NULL);
    return gpu;
}



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_resources_1(TstSuite*);

int test_resources_dat_1(TstSuite*);

int test_resources_tex_1(TstSuite*);



/*************************************************************************************************/
/*  Resources data transfers tests                                                               */
/*************************************************************************************************/

int test_resources_dat_transfers(TstSuite* suite);

int test_resources_dat_resize(TstSuite* suite);

int test_resources_tex_transfers(TstSuite* suite);

int test_resources_tex_resize(TstSuite* suite);



#endif
