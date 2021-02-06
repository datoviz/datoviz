#ifndef DVZ_IMGUI_HEADER
#define DVZ_IMGUI_HEADER

#include "../include/datoviz/canvas.h"
// #include "../include/datoviz/controls.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Dear ImGUI C wrapper                                                                         */
/*************************************************************************************************/

DVZ_EXPORT void dvz_imgui_init(DvzCanvas* canvas);

DVZ_EXPORT void dvz_imgui_dpi_scaling(DvzCanvas* canvas, float scaling);

DVZ_EXPORT void dvz_imgui_destroy();

DVZ_EXPORT void dvz_gui_begin(const char* title, int flags);

DVZ_EXPORT void dvz_gui_end();

DVZ_EXPORT void dvz_gui_callback(DvzCanvas* canvas, DvzEvent event);

DVZ_EXPORT void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent event);



#ifdef __cplusplus
}
#endif

#endif
