#ifndef VKL_IMGUI_HEADER
#define VKL_IMGUI_HEADER

#include "../include/visky/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************************************/
/*  ImGUI                                                                                        */
/*************************************************************************************************/

VKY_EXPORT void vkl_imgui_init(VklCanvas* canvas);

VKY_EXPORT void vkl_imgui_destroy();

VKY_EXPORT void vkl_imgui_callback_fps(VklCanvas* canvas, VklPrivateEvent);



#ifdef __cplusplus
}
#endif

#endif
