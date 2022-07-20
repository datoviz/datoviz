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
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests with vklite                                                                            */
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

    // // Begin the GUI frame.
    // dvz_gui_frame_begin(canvas->, cmds, idx);

    // // GUI code.
    // dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200});
    // dvz_gui_text("Hello world");
    // dvz_gui_dialog_end();

    // // End the GUI frame.
    // dvz_gui_frame_end(cmds, idx);

    // End the renderpass and command buffer.
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

int test_vklite_canvas_gui(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    // Create a window.
    DvzWindow window = dvz_window(host->backend, WIDTH, HEIGHT, 0);

    // Create a surface.
    VkSurfaceKHR surface = dvz_window_surface(host, &window);
    AT(surface != VK_NULL_HANDLE);

    // Create a GPU with surface support.
    DvzGpu* gpu = dvz_gpu_best(host);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_PRESENT);
    dvz_gpu_create(gpu, surface);

    // Create the GUI interface on queue #0.
    DvzGui* gui = dvz_gui(gpu, 0);

    // Create a canvas.
    TestCanvas canvas = desktop_canvas(gpu, &window, surface);
    canvas.always_refill = true;

    // Prepare the window for the GUI.
    dvz_gui_window(gui, &window, canvas.swapchain.images, 0);

    // Simple event loop with GUI callback.
    test_canvas_show(&canvas, _fill_gui, N_FRAMES);

    // Destroy objects.
    test_canvas_destroy(&canvas);
    dvz_gui_destroy(gui);
    dvz_gpu_destroy(gpu);
    return 0;
}



/*************************************************************************************************/
/*  Tests GUI                                                                                    */
/*************************************************************************************************/

static void _refill(DvzCanvas* canvas, DvzCommands* cmds, uint32_t cmd_idx, void* user_data)
{
    ASSERT(canvas != NULL);

    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    dvz_cmd_begin(cmds, cmd_idx);
    dvz_cmd_begin_renderpass(
        cmds, cmd_idx, canvas->render.renderpass, &canvas->render.framebuffers);
    dvz_cmd_end_renderpass(cmds, cmd_idx);
    dvz_cmd_end(cmds, cmd_idx);
}

static void _fill_overlay(DvzCanvas* canvas, void* user_data)
{
    ASSERT(canvas != NULL);

    DvzGuiWindow* gui_window = (DvzGuiWindow*)user_data;
    ASSERT(gui_window != NULL);

    DvzCommands* cmds = &gui_window->cmds;
    ASSERT(cmds != NULL);

    uint32_t img_idx = canvas->render.swapchain.img_idx;

    // Overlay renderpass.
    DvzRenderpass* renderpass = &gui_window->gui->renderpass;
    ASSERT(renderpass != NULL);

    // Overlay framebuffers.
    DvzFramebuffers* framebuffers = &gui_window->framebuffers;
    ASSERT(framebuffers != NULL);

    // Begin recording the command buffer.
    dvz_cmd_begin(cmds, img_idx);
    dvz_cmd_begin_renderpass(cmds, img_idx, renderpass, framebuffers);

    // Make a GUI.
    dvz_gui_frame_begin(gui_window, cmds, img_idx);
    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200});
    dvz_gui_text("Hello world");
    dvz_gui_dialog_end();
    dvz_gui_frame_end(cmds, img_idx);

    // Stop recording the command buffer.
    dvz_cmd_end_renderpass(cmds, img_idx);
    dvz_cmd_end(cmds, img_idx);

    // Add the command buffer to the current submission.
    DvzSubmit* submit = &canvas->render.submit;
    dvz_submit_commands(submit, &gui_window->cmds);
}

int test_gui_1(TstSuite* suite)
{
    // Test the GUI renderpass integration.
    // WARNING: resizing is not expected to work here because we need to recreate the overlay
    // framebuffers. Proper GUI integration is done in the presenter part.

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

    // Need to init the GUI engine.
    DvzGui* gui = dvz_gui(gpu, 0);

    // Create the canvas.
    DvzCanvas canvas = dvz_canvas(gpu, &renderpass, WIDTH, HEIGHT, 0);
    dvz_canvas_refill(&canvas, _refill, NULL);
    dvz_canvas_create(&canvas, surface);

    // Prepare the window for the GUI.
    DvzGuiWindow* gui_window =
        dvz_gui_window(gui, &window, canvas.render.swapchain.images, DVZ_DEFAULT_QUEUE_RENDER);

    dvz_canvas_fill_overlay(&canvas, _fill_overlay, gui_window);

    // Basic event loop.
    // TODO: use dvz_loop()

    // Destroy objects.
    dvz_canvas_destroy(&canvas);
    dvz_window_destroy(&window);
    dvz_surface_destroy(host, surface);
    dvz_renderpass_destroy(&renderpass);
    dvz_gui_window_destroy(gui_window);
    dvz_gui_destroy(gui);
    dvz_gpu_destroy(gpu);
    return 0;
}
