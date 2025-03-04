/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing canvas                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_canvas.h"
#include "backend.h"
#include "canvas.h"
#include "context.h"
#include "pipelib.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "testing_utils.h"
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

int test_canvas_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    ANN(host);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create the window and surface.
    DvzWindow window = dvz_window(host->backend, WIDTH, HEIGHT, 0);
    DvzSurface surface = dvz_window_surface(host, &window);

    // Create the renderpass.
    DvzRenderpass renderpass = desktop_renderpass(gpu);

    // Create the canvas.
    DvzCanvas canvas =
        dvz_canvas(gpu, &renderpass, window.framebuffer_width, window.framebuffer_height, 0);
    dvz_canvas_create(&canvas, surface);

    dvz_canvas_recreate(&canvas);

    uint8_t* rgb = dvz_canvas_download(&canvas);
    ANN(rgb);

    // Save it to a file.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/canvas.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);

    dvz_canvas_destroy(&canvas);
    dvz_surface_destroy(host, surface);
    dvz_window_destroy(&window);
    dvz_renderpass_destroy(&renderpass);
    dvz_gpu_destroy(gpu);
    return 0;
}
