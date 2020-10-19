#ifndef VKY_TRANSFORM_HEADER
#define VKY_TRANSFORM_HEADER

#include "scene.h"



VKY_EXPORT void
vky_transform_cartesian(VkyBox2D box, uint32_t item_count, const dvec2* pos_in, vec3* pos_out);

VKY_EXPORT VkyBox2D vky_transform_compute_box(uint32_t point_count, dvec2* points);



#endif
