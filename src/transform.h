#ifndef VKY_TRANSFORM_HEADER
#define VKY_TRANSFORM_HEADER

#include <../include/visky/scene.h>



VKY_EXPORT void
vky_transform_cartesian(VkyBox2D box, uint32_t item_count, const dvec2* pos_in, vec3* pos_out);

#endif
