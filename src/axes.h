#ifndef VKY_AXES_HEADER
#define VKY_AXES_HEADER

#include "../include/visky/visky.h"



VkyAxesTickRange vky_axes_get_ticks(double dmin, double dmax, VkyAxesContext context);
// dvec2s vky_axes_normalize_pos(VkyAxes* axes, dvec2s pos); // => [-1, 1]

VkyAxes* vky_axes_init(VkyPanel*, VkyAxes2DParams);

VkyVisual* vky_axes_create_tick_visual(VkyScene* scene, VkyAxes* axes);
VkyVisual* vky_axes_create_text_visual(VkyScene* scene, VkyAxes* axes);

void vky_axes_make_vertices(
    VkyAxes* axes, uint32_t* vertex_count, VkyAxesTickVertex* vertices,
    uint32_t* text_vertex_count, VkyAxesTextData* text_data);

// void vky_axes_register_visual(VkyAxes* axis, VkyVisual* visual);
void vky_axes_update_visuals(VkyAxes* axes);
void vky_axes_recompute_ticks(VkyAxes*, VkyPanzoom*, bool);

VKY_EXPORT void vky_axes_reset(VkyAxes* axes);
VKY_EXPORT VkyBox2D vky_axes_get_range(VkyAxes* axes);
VKY_EXPORT void vky_axes_set_range(VkyAxes* axes, VkyBox2D, bool recompute_ticks);


#endif
