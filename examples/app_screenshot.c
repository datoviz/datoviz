#include <visky/visky.h>

static void fcb(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, (VkClearColorValue){{0.5, 0.8, 0.2, 1}});
    vky_end_render_pass(cmd_buf, canvas);
}

int main()
{
    log_set_level_env();
    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, 200, 100);
    canvas->cb_fill_command_buffer = fcb;

    vky_fill_command_buffers(canvas);
    vky_offscreen_frame(canvas, 0);

    // Screenshot.
    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    vky_begin_screenshot(screenshot);
    uint8_t* image = vky_screenshot_to_rgb(screenshot, false);
    write_ppm("artifacts/blank.ppm", screenshot->width, screenshot->height, image);
    free(image);
    vky_end_screenshot(screenshot);
    vky_destroy_screenshot(screenshot);

    vky_destroy_app(app);
    return 0;
}
