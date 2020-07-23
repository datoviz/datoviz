#ifndef VKY_DEMO_HEADER
#define VKY_DEMO_HEADER

typedef struct VkyPanel VkyPanel;

VKY_EXPORT void vky_demo_blank(void);
VKY_EXPORT void vky_demo_param(vec4 clear_color);
VKY_EXPORT void vky_demo_scatter(size_t point_count, const dvec2* points);

// The raytracing demo is also used in tests and in the scene_raytracing example.
VKY_EXPORT void raytracing_demo(VkyPanel* panel);
VKY_EXPORT void vky_demo_raytracing(void);

#endif
