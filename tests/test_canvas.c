/*************************************************************************************************/
/*  Testing canvas                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_canvas.h"
#include "_glfw.h"
#include "canvas.h"
#include "context.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Canvas tests                                                                                 */
/*************************************************************************************************/

static DvzGpu* make_gpu(DvzHost* host)
{
    ASSERT(host != NULL);

    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, true);
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});

    // HACK: temporarily create a blank window so that we can create a GPU with surface rendering
    // capabilities.
    DvzWindow* window = dvz_window(host, 100, 100);
    ASSERT(window->surface != VK_NULL_HANDLE);
    dvz_gpu_create(gpu, window->surface);
    dvz_window_destroy(window);

    return gpu;
}

int test_canvas_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);

    // Create the board.
    DvzCanvas canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_create(&canvas);

    dvz_canvas_loop(&canvas, N_FRAMES);

    dvz_canvas_destroy(&canvas);
    // dvz_gpu_destroy(gpu);
    return 0;
}
