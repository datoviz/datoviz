/*************************************************************************************************/
/*  Testing renderer                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_runner.h"
#include "_glfw.h"
#include "context.h"
#include "renderer.h"
#include "runner.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Renderer tests                                                                               */
/*************************************************************************************************/

int test_runner_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Renderer.
    DvzRenderer* rd = dvz_renderer_glfw(gpu);


    // Destroy the renderer.
    dvz_renderer_destroy(rd);
    return 0;
}
