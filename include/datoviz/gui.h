/*************************************************************************************************/
/*  Dear ImGUI wrapper                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GUI
#define DVZ_HEADER_GUI



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "common.h"
#include "list.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;
typedef struct DvzGuiWindow DvzGuiWindow;

// Forward declarations.
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzCommands DvzCommands;
typedef struct DvzWindow DvzWindow;
typedef struct ImGuiIO ImGuiIO;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGui
{
    DvzRenderpass renderpass;
};



struct DvzGuiWindow
{
    DvzFramebuffers framebuffers;
    DvzCommands cmds;
    DvzList* callbacks;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions */
/*************************************************************************************************/

/**
 * Initialize the GUI renderer.
 *
 * @param gpu the GPU
 * @param queue_idx the render queue
 */
DVZ_EXPORT DvzGui* dvz_gui(DvzGpu* gpu, uint32_t queue_idx);



/**
 * To be called at the beginning of the command buffer recording.
 *
 * @param gui the GUI
 * @param window the window
 */
DVZ_EXPORT void dvz_gui_frame_begin(DvzGui* gui, DvzWindow* window);



/**
 * To be called at the beginning of the command buffer recording (offscreen version).
 *
 * @param gui the GUI
 * @param width the viewport width
 * @param height the viewport height
 */
DVZ_EXPORT
void dvz_gui_frame_offscreen(uint32_t width, uint32_t height);



/**
 * Start a new dialog.
 *
 * @param gui the GUI
 * @param pos the dialog position
 * @param size the dialog size
 */
DVZ_EXPORT void dvz_gui_dialog_begin(vec2 pos, vec2 size);



/**
 * Add a text item in a dialog.
 *
 * @param gui the GUI
 * @param str the string
 */
DVZ_EXPORT void dvz_gui_text(const char* str);



/**
 * Stop the creation of the dialog.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_dialog_end();



/**
 * Show the demo GUI.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_demo();



/**
 * To be called at the end of the command buffer recording.
 *
 * @param gui the GUI
 * @param cmds the command buffer set
 * @param idx the command buffer index within the set
 */
DVZ_EXPORT void dvz_gui_frame_end(DvzCommands* cmds, uint32_t idx);



/**
 * Destroy the GUI renderer.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



EXTERN_C_OFF

#endif
