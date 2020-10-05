#ifndef VKY_AXES_HEADER
#define VKY_AXES_HEADER

#include "../include/visky/visky.h"


typedef enum
{
    VKY_AXES_TICK_NONE = 0x0000,

    VKY_AXES_TICK_MINOR = 0x0001,
    VKY_AXES_TICK_MAJOR = 0x0002,
    VKY_AXES_TICK_GRID = 0x0004,
    VKY_AXES_TICK_LIM = 0x0008,

    VKY_AXES_TICK_USER_0 = 0x0010,
    VKY_AXES_TICK_USER_1 = 0x0020,
    VKY_AXES_TICK_USER_2 = 0x0040,
    VKY_AXES_TICK_USER_3 = 0x0080,

    VKY_AXES_TICK_ALL = 0x00FF,
} VkyAxesTickFlag;


VkyAxes* vky_axes_init(VkyPanel*, VkyAxes2DParams);
VKY_EXPORT void vky_axes_reset(VkyAxes* axes);

void vky_axes_panzoom_update(VkyAxes*); // update the outer and inner panzooms
bool vky_axes_refill_needed(VkyAxes*);  // whether a refill is needed
void vky_axes_rescale(VkyAxes*);        // recompute the new axis scale when a refill is needed
void vky_axes_compute_ticks(VkyAxes*);  // compute the ticks after a rescale
void vky_axes_update_visuals(VkyAxes*); // update the tick visuals after a tick recompute

VKY_EXPORT void vky_axes_toggle_tick(VkyAxes* axes, int tick_element);
VKY_EXPORT void vky_axes_set_tick_params(VkyAxes* axes);
VKY_EXPORT void vky_axes_set_text_params(VkyAxes* axes);

VKY_EXPORT VkyBox2D vky_axes_get_range(VkyAxes* axes);
VKY_EXPORT void vky_axes_set_range(VkyAxes* axes, VkyBox2D, bool refill);
VKY_EXPORT void vky_axes_set_initial_range(VkyAxes* axes, VkyBox2D box);


#endif
