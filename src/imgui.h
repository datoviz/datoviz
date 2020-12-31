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

VKY_EXPORT void vkl_imgui_frame(VklCanvas* canvas, VklCommands* cmds, uint32_t cmd_idx);

VKY_EXPORT void vkl_imgui_destroy();



#ifdef __cplusplus
}
#endif

#endif
