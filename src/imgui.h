#ifndef VKL_IMGUI_HEADER
#define VKL_IMGUI_HEADER

#include "../include/visky/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  ImGui style                                                                                  */
/*************************************************************************************************/

typedef enum
{
    VKL_GUI_STANDARD = 0,
    VKL_GUI_PROMPT = 1,
    VKL_GUI_FIXED_TL = 10,
    VKL_GUI_FIXED_TR = 11,
    VKL_GUI_FIXED_LL = 12,
    VKL_GUI_FIXED_LR = 13,
} VklGuiStyle;



/*************************************************************************************************/
/*  ImGui functions                                                                              */
/*************************************************************************************************/

VKY_EXPORT void vkl_imgui_init(VklCanvas* canvas);

VKY_EXPORT void vkl_imgui_begin(const char* title, VklGuiStyle style);

VKY_EXPORT void vkl_imgui_end();

VKY_EXPORT void vkl_imgui_destroy();

VKY_EXPORT void vkl_imgui_callback_fps(VklCanvas* canvas, VklEvent);



#ifdef __cplusplus
}
#endif

#endif
