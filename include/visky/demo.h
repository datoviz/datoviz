#ifndef VKY_DEMO_HEADER
#define VKY_DEMO_HEADER

typedef struct VkyPanel VkyPanel;

VKY_EXPORT void vky_demo_blank(void);
VKY_EXPORT void vky_demo_param(vec4 clear_color);
VKY_EXPORT void vky_demo_scatter(uint32_t point_count, const dvec2* points);
VKY_EXPORT void vky_demo_raytracing(void);

#endif
