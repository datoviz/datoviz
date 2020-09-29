#include <visky/visky.h>

int main()
{
    log_set_level_env();
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
