/*************************************************************************************************/
/*  Testing simple loop                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_loop.h"
#include "../src/vklite_utils.h"
#include "canvas.h"
#include "loop.h"
#include "test.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_loop_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);
    ASSERT(host != NULL);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);

    dvz_loop_run(loop, N_FRAMES);

    dvz_loop_destroy(loop);
    dvz_gpu_destroy(gpu);
    return 0;
}
