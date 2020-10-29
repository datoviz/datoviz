#include <visky/demo_inc.h>

static int raytracing(VkyTestContext* context)
{
    vky_set_controller(context->panel, VKY_CONTROLLER_FPS, NULL);
    vky_set_constant(VKY_PANZOOM_MIN_ZOOM_ID, 1);
    _demo_raytracing(context->panel);
    return 0;
}
