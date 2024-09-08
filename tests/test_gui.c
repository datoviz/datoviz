/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing GUI                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_gui.h"
#include "canvas.h"
#include "gui.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Tests GUI                                                                                    */
/*************************************************************************************************/

int test_gui_offscreen(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create the renderpass.
    // DvzRenderpass renderpass =
    //     dvz_gpu_renderpass(gpu, DVZ_DEFAULT_CLEAR_COLOR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    TestCanvas canvas = offscreen_canvas(gpu);
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    // Need to init the GUI engine.
    DvzGui* gui = dvz_gui(gpu, 0, DVZ_GUI_FLAGS_OFFSCREEN);

    // Mark the beginning and end of the frame.
    DvzGuiWindow* gui_window = dvz_gui_offscreen(gui, canvas.images, 0);

    // Start the recording of the GUI code.
    dvz_gui_window_begin(gui_window, 0);

    // The GUI code goes here.

    dvz_gui_pos((vec2){100, 100}, DVZ_DIALOG_DEFAULT_PIVOT);
    dvz_gui_size((vec2){200, 200});
    dvz_gui_begin("Hello", 0);

    dvz_gui_text("Hello offscreen");
    dvz_gui_end();
    // dvz_gui_demo();

    // Stop the recording of the GUI code.
    dvz_gui_window_end(gui_window, 0);
    dvz_cmd_submit_sync(&gui_window->cmds, 0);

    // Save a screenshot.
    uint8_t* rgb = screenshot(framebuffers->attachments[0], 1);
    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/imgui.ppm", ARTIFACTS_DIR);
    log_debug("saving screenshot to %s", path);
    dvz_write_ppm(path, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT, rgb));
    FREE(rgb);

    canvas_destroy(&canvas);

    // Destroy the GUI engine.
    dvz_gui_window_destroy(gui_window);
    dvz_gui_destroy(gui);

    dvz_gpu_destroy(gpu);
    return 0;
}
