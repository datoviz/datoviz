#ifndef VKY_BASIC_HEADER
#define VKY_BASIC_HEADER

#include "scene.h"


/*************************************************************************************************/
/*  Basic API                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VkyVisual* vky_basic_visual(
    VkyControllerType, VkyVisualType, uint32_t item_count, vec3* pos, VkyColor* color);
VKY_EXPORT VkyVisual*
vky_basic_scatter(uint32_t item_count, vec3* pos, VkyColor* color, float* size);
VKY_EXPORT VkyVisual* vky_basic_plot(uint32_t item_count, vec3* pos, VkyColor* color);
VKY_EXPORT VkyVisual* vky_basic_segments(uint32_t item_count, vec3* pos, VkyColor* color);
VKY_EXPORT VkyVisual* vky_basic_imshow(uint32_t width, uint32_t height, const uint8_t*);
VKY_EXPORT VkyVisual* vky_basic_mesh(uint32_t item_count, vec3* pos, VkyColor* color);

VKY_EXPORT VkyPanel* vky_basic_panel(void);
VKY_EXPORT void vky_basic_run(void);



#endif
