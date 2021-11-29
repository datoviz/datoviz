/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST
#define DVZ_HEADER_TEST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"
// #include "vklite.h"
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

static int setup_host(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ASSERT(ctx != NULL);

    log_debug("setup: creating host");
    ctx->host = dvz_host(DVZ_BACKEND_GLFW);

    return 0;
}



static int teardown_host(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ASSERT(ctx != NULL);

    log_debug("teardown: destroying host");
    ASSERT(ctx->host != NULL);
    dvz_host_destroy(ctx->host);

    return 0;
}



// Get or create the host from the suite's context.
static DvzHost* get_host(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzTestCtx* ctx = (DvzTestCtx*)suite->context;
    ASSERT(ctx != NULL);
    DvzHost* host = ctx->host;
    if (host == NULL)
    {
        log_error("you need to add the setup fixture setup_host()");
        // setup_host(suite);
    }
    ASSERT(host != NULL);
    return host;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

DVZ_EXPORT int dvz_run_tests(const char* match);



#endif
