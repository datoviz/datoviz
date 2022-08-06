/*************************************************************************************************/
/*  Testing canvas                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_canvas.h"
#include "canvas.h"
#include "context.h"
#include "glfw_utils.h"
#include "pipelib.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCanvasStruct TestCanvasStruct;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestCanvasStruct
{
    DvzPipe* pipe;
    DvzBufferRegions br;
};



/*************************************************************************************************/
/*  Canvas tests                                                                                 */
/*************************************************************************************************/

int test_canvas_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);
    ASSERT(host != NULL);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Create the window and surface.
    DvzWindow window = dvz_window(host->backend, WIDTH, HEIGHT, 0);
    VkSurfaceKHR surface = dvz_window_surface(host, &window);

    // Create the renderpass.
    DvzRenderpass renderpass = desktop_renderpass(gpu);

    // Create the canvas.
    DvzCanvas canvas = dvz_canvas(gpu, &renderpass, WIDTH, HEIGHT, 0);
    dvz_canvas_create(&canvas, surface);

    dvz_canvas_destroy(&canvas);
    dvz_window_destroy(&window);
    dvz_surface_destroy(host, surface);
    dvz_renderpass_destroy(&renderpass);
    dvz_gpu_destroy(gpu);
    return 0;
}
