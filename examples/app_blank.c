#include <visky/visky.h>

static void fcb(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);
    vky_end_render_pass(cmd_buf, canvas);
}

int main()
{
    log_set_level_env();
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    canvas->cb_fill_command_buffer = fcb;
    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
