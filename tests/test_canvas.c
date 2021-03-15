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



int test_canvas_multiple(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);

    DvzCanvas* canvas0 = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    DvzCanvas* canvas1 = dvz_canvas(gpu, WIDTH, HEIGHT, 0);

    uvec2 size = {0};
    dvz_canvas_size(canvas0, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);

    dvz_canvas_clear_color(canvas0, 1, 0, 0);
    dvz_canvas_clear_color(canvas1, 0, 1, 0);

    dvz_app_run(app, N_FRAMES);

    // Check canvas background color.
    uint8_t* rgb0 = dvz_screenshot(canvas0, false);
    uint8_t* rgb1 = dvz_screenshot(canvas1, false);
    for (uint32_t i = 0; i < size[0] * size[1] * 3 * sizeof(uint8_t); i++)
    {
        AT(rgb0[i] == (i % 3 == 0 ? 255 : 0));
        AT(rgb1[i] == (i % 3 == 1 ? 255 : 0));
    }
    FREE(rgb0);
    FREE(rgb1);

    return 0;
}
