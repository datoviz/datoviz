/*************************************************************************************************/
/*  Dear ImGUI wrapper                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GUI
#define DVZ_HEADER_GUI



/*************************************************************************************************/
/*  Includes                                                                                    */
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

/**
 * Initialize the GUI renderer.
 *
 * @param gpu the GPU
 * @param renderpass the renderpass
 * @param window the window, or NULL if using an offscreen renderer
 * @param queue_idx the render queue
 * @param width the viewport width
 * @param height the viewport height
 */
DVZ_EXPORT DvzGui dvz_gui(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzWindow* window, //
    uint32_t queue_idx, uint32_t width, uint32_t height);



/**
 * To be called at the beginning of the command buffer recording.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_frame_begin(DvzGui* gui);



/**
 * Start a new dialog.
 *
 * @param gui the GUI
 * @param pos the dialog position
 * @param size the dialog size
 */
DVZ_EXPORT void dvz_gui_dialog_begin(DvzGui* gui, vec2 pos, vec2 size);



/**
 * Stop the creation of the dialog.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_dialog_end(DvzGui* gui);



/**
 * Show the demo GUI.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_demo(DvzGui* gui);



/**
 * To be called at the end of the command buffer recording.
 *
 * @param gui the GUI
 * @param cmds the command buffer set
 * @param idx the command buffer index within the set
 */
DVZ_EXPORT void dvz_gui_frame_end(DvzGui* gui, DvzCommands* cmds, uint32_t idx);



/**
 * Destroy the GUI renderer.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



EXTERN_C_OFF

#endif
