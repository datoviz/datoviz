#ifndef DVZ_IMGUI_HEADER
#define DVZ_IMGUI_HEADER

#include "../include/datoviz/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  ImGui style                                                                                  */
/*************************************************************************************************/

typedef enum
{
    DVZ_GUI_STANDARD = 0,
    DVZ_GUI_PROMPT = 1,
    DVZ_GUI_FIXED_TL = 10,
    DVZ_GUI_FIXED_TR = 11,
    DVZ_GUI_FIXED_LL = 12,
    DVZ_GUI_FIXED_LR = 13,
} DvzGuiStyle;



/*************************************************************************************************/
/*  ImGui functions                                                                              */
/*************************************************************************************************/

DVZ_EXPORT void dvz_gui_init(DvzCanvas* canvas);

DVZ_EXPORT void dvz_gui_begin(const char* title, DvzGuiStyle style);

DVZ_EXPORT void dvz_gui_end();

DVZ_EXPORT void dvz_gui_destroy();

DVZ_EXPORT void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent);



#ifdef __cplusplus
}
#endif

#endif
