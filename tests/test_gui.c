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
#include "datoviz.h"
#include "gui.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Tests GUI                                                                                    */
/*************************************************************************************************/

static inline void _gui_callback(DvzApp* app, DvzId canvas_id, DvzGuiEvent* ev)
{
    dvz_gui_demo();

    dvz_gui_pos((vec2){100, 100}, DVZ_DIALOG_DEFAULT_PIVOT);
    dvz_gui_size((vec2){400, 300});
    dvz_gui_begin("Hello", 0);

    // Tree.
    if (dvz_gui_node("Item 1"))
    {
        dvz_gui_selectable("Hello inside item 1");
        if (dvz_gui_clicked())
            log_info("clicked sub item 1");
        dvz_gui_pop();
    }

    if (dvz_gui_node("Item 2"))
    {
        dvz_gui_selectable("Hello inside item 2");
        if (dvz_gui_clicked())
            log_info("clicked sub item 2");
        dvz_gui_pop();
    }

    // Table.
    uint32_t selected_count = 0;
    bool* selected = (bool*)ev->user_data;
    ANN(selected);
    const char* labels[] = {
        "col0", "col1", "col2", //
        "0",    "1",    "2",    //
        "3",    "4",    "5"     //
    };
    bool sel = dvz_gui_table("table", 2, 3, labels, selected, 0);
    if (sel)
    {
        for (uint32_t i = 0; i < 2; i++)
            if (selected[i])
                selected_count++;
        log_info("selected %d rows", selected_count);
    }

    dvz_gui_end();
}

int test_gui_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Create app objects.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);

    bool selected[2] = {0};
    dvz_app_gui(app, dvz_figure_id(figure), _gui_callback, selected);

    // Run the scene.
    dvz_scene_run(scene, app, N_FRAMES);

    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}



int test_gui_offscreen(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST
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
