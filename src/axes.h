#ifndef VKY_AXES_HEADER
#define VKY_AXES_HEADER

#include "../include/visky/visky.h"


VkyAxes* vky_axes_init(VkyPanel*, VkyAxes2DParams);
VKY_EXPORT void vky_axes_reset(VkyAxes* axes);

void vky_axes_panzoom_update(VkyAxes*); // update the outer and inner panzooms
bool vky_axes_refill_needed(VkyAxes*);  // whether a refill is needed
void vky_axes_rescale(VkyAxes*);        // recompute the new axis scale when a refill is needed
void vky_axes_compute_ticks(VkyAxes*);  // compute the ticks after a rescale
void vky_axes_update_visuals(VkyAxes*); // update the tick visuals after a tick recompute

VKY_EXPORT VkyBox2D vky_axes_get_range(VkyAxes* axes);
VKY_EXPORT void vky_axes_set_range(VkyAxes* axes, VkyBox2D, bool refill);


#endif
