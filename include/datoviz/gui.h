/*************************************************************************************************/
/*  Dear ImGUI wrapper                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GUI
#define DVZ_HEADER_GUI



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;

// Forward declarations.
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzCommands DvzCommands;
typedef struct DvzWindow DvzWindow;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGui
{
    DvzGpu* gpu;
    bool use_glfw;
    ImGuiIO* io;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

// TODO: docstrings

DVZ_EXPORT DvzGui dvz_gui(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzWindow* window, uint32_t width, uint32_t height);



DVZ_EXPORT void dvz_gui_frame_begin(DvzGui* gui);



DVZ_EXPORT void dvz_gui_dialog_begin(DvzGui* gui, vec2 pos, vec2 size);



DVZ_EXPORT void dvz_gui_dialog_end(DvzGui* gui);



DVZ_EXPORT void dvz_gui_demo(DvzGui* gui);



DVZ_EXPORT void dvz_gui_frame_end(DvzGui* gui, DvzCommands* cmds, uint32_t idx);



DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



EXTERN_C_OFF

#endif
