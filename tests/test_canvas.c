#include "../include/datoviz/canvas.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Test canvas                                                                                  */
/*************************************************************************************************/

int test_canvas_blank(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);

    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    ASSERT(canvas->window != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->window->app != NULL);

    uvec2 size = {0};

    // Framebuffer size.
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);
    log_debug("canvas framebuffer size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    // Screen size.
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    log_debug("canvas screen size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    dvz_app_run(app, N_FRAMES);

    // Check blank canvas.
    uint8_t* rgb = dvz_screenshot(canvas, false);
    for (uint32_t i = 0; i < size[0] * size[1] * 3 * sizeof(uint8_t); i++)
    {
        AT(rgb[i] == (i % 3 == 0 ? 0 : (i % 3 == 1 ? 8 : 18)))
    }
    FREE(rgb);

    return 0;
}
