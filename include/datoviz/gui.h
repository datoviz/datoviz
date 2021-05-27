#ifndef DVZ_IMGUI_HEADER
#define DVZ_IMGUI_HEADER

#include "../include/datoviz/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGuiContext DvzGuiContext;

// Forward declarations.
typedef void* ImTextureID;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGuiContext
{
    ImTextureID colormap_texture;
};



/*************************************************************************************************/
/*  Dear ImGUI C wrapper                                                                         */
/*************************************************************************************************/

/**
 * Enable Dear ImGui support in a canvas.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_imgui_enable(DvzCanvas* canvas);

/**
 * Set the DPI scaling for Dear ImGui in a canvas.
 *
 * @param canvas the canvas
 * @param scaling the DPI scaling factor
 */
DVZ_EXPORT void dvz_imgui_dpi_scaling(DvzCanvas* canvas, float scaling);

/**
 * Show the Dear ImGui demo in a canvas.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_imgui_demo(DvzCanvas* canvas);



/**
 * Start a GUI dialog.
 *
 * @param title the dialog title
 * @param flags the Dear ImGui flags
 */
DVZ_EXPORT void dvz_gui_begin(const char* title, int flags);

/**
 * End a GUI dialog.
 */
DVZ_EXPORT void dvz_gui_end();

/**
 * Callback function that creates all GUIs in a canvas.
 *
 * @param canvas the canvas
 * @param ev the IMGUI event struct
 */
DVZ_EXPORT void dvz_gui_callback(DvzCanvas* canvas, DvzEvent ev);

/**
 * Callback function that creates a FPS GUI.
 *
 * @param canvas the canvas
 * @param ev the IMGUI event struct
 */
DVZ_EXPORT void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent ev);

/**
 * Callback function that creates a demo GUI.
 *
 * @param canvas the canvas
 * @param ev the IMGUI event struct
 */
DVZ_EXPORT void dvz_gui_callback_demo(DvzCanvas* canvas, DvzEvent ev);

/**
 * Callback function that creates a video record control bar.
 *
 * @param canvas the canvas
 * @param ev the IMGUI event struct
 */
DVZ_EXPORT void dvz_gui_callback_player(DvzCanvas* canvas, DvzEvent ev);



#ifdef __cplusplus
}
#endif

#endif
