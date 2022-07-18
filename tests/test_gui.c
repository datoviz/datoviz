/*************************************************************************************************/
/*  Testing vklite                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_gui.h"
#include "../src/vklite_utils.h"
#include "fileio.h"
#include "gui.h"
#include "resources.h"
#include "surface.h"
#include "test.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_gui(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Create the renderpass.
    // DvzRenderpass renderpass =
    //     dvz_gpu_renderpass(gpu, DVZ_DEFAULT_CLEAR_COLOR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    TestCanvas canvas = offscreen_canvas(gpu);

    // Need to init the GUI engine.
    DvzGui* gui = dvz_gui(gpu, 0);

    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_begin_renderpass(&cmds, 0, &canvas.renderpass, &canvas.framebuffers);

    // Mark the beginning and end of the frame.
    dvz_gui_frame_offscreen(WIDTH, HEIGHT);
    // The GUI code goes here.

    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200});
    dvz_gui_text("Hello world");
    dvz_gui_dialog_end();
    // dvz_gui_demo();

    dvz_gui_frame_end(&cmds, 0);

    dvz_cmd_end_renderpass(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    uint8_t* rgb = screenshot(framebuffers->attachments[0], 1);
    char path[1024];
    snprintf(path, sizeof(path), "%s/imgui.ppm", ARTIFACTS_DIR);
    log_debug("saving screenshot to %s", path);
    dvz_write_ppm(path, WIDTH, HEIGHT, rgb);
    FREE(rgb);

    test_canvas_destroy(&canvas);

    // Destroy the GUI engine.
    dvz_gui_destroy(gui);

    dvz_gpu_destroy(gpu);
    return 0;
}



static void _fill_gui(TestCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    ASSERT(cmds != NULL);

    // Begin the command buffer and renderpass.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);

    // Begin the GUI frame.
    dvz_gui_frame_begin(canvas->window);

    // GUI code.
    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200});
    dvz_gui_text("Hello world");
    dvz_gui_dialog_end();

    // End the GUI frame.
    dvz_gui_frame_end(cmds, idx);

    // End the renderpass and command buffer.
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

int test_vklite_canvas_gui(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzWindow window = dvz_window(host->backend, WIDTH, HEIGHT, 0);
    VkSurfaceKHR surface = dvz_window_surface(host, &window);
    AT(surface != VK_NULL_HANDLE);

    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_PRESENT);
    dvz_gpu_create(gpu, surface);

    DvzGui* gui = dvz_gui(gpu, 0);

    TestCanvas canvas = desktop_canvas(gpu, &window, surface);
    canvas.always_refill = true;

    dvz_gui_window(gui, &window, canvas.swapchain.images, 0);

    test_canvas_show(&canvas, _fill_gui, N_FRAMES);

    test_canvas_destroy(&canvas);
    // dvz_gui_destroy(gui);
    dvz_gpu_destroy(gpu);
    return 0;
}
