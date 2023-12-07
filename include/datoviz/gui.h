/*************************************************************************************************/
/*  Dear ImGUI wrapper                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GUI
#define DVZ_HEADER_GUI



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

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
typedef struct DvzGui DvzGui;
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzCommands DvzCommands;
typedef struct DvzWindow DvzWindow;
typedef struct DvzTex DvzTex;
typedef struct ImGuiIO ImGuiIO;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// GUI flags.
typedef enum
{
    DVZ_GUI_FLAGS_NONE,
    DVZ_GUI_FLAGS_OFFSCREEN,
} DvzGuiFlags;



// GUI dialog flags.
typedef enum
{
    DVZ_DIALOG_FLAGS_NONE = 0x0000,
    DVZ_DIALOG_FLAGS_FPS = 0x0007,
} DvzDialogFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGui
{
    DvzGpu* gpu;
    DvzRenderpass renderpass;
    DvzContainer gui_windows;
};



struct DvzGuiWindow
{
    DvzObject obj;
    DvzGui* gui;
    DvzWindow* window;
    uint32_t width, height; // framebuffer window size
    bool is_offscreen;
    DvzFramebuffers framebuffers;
    DvzCommands cmds;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  GUI functions                                                                                */
/*************************************************************************************************/

/**
 * Initialize the GUI renderer.
 *
 * @param gpu the GPU
 * @param queue_idx the render queue
 */
DVZ_EXPORT DvzGui* dvz_gui(DvzGpu* gpu, uint32_t queue_idx, int flags);



/**
 * Destroy the GUI renderer.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



/*************************************************************************************************/
/*  GUI window                                                                                   */
/*************************************************************************************************/

/**
 * Initialize the GUI for a window .
 *
 * @param gui the GUI
 * @param window the window
 * @returns gui_window
 */
DVZ_EXPORT DvzGuiWindow*
dvz_gui_window(DvzGui* gui, DvzWindow* window, DvzImages* images, uint32_t queue_idx);



/**
 * Initialize an offscreen GUI.
 *
 * @param gui the GUI
 * @returns gui_window
 */
DVZ_EXPORT DvzGuiWindow* dvz_gui_offscreen(DvzGui* gui, DvzImages* images, uint32_t queue_idx);



/**
 * To be called at the beginning of the command buffer recording.
 *
 * @param gui the GUI
 * @param window the window
 */
DVZ_EXPORT void dvz_gui_window_begin(DvzGuiWindow* gui_window, uint32_t cmd_idx);



/**
 * To be called at the end of the command buffer recording.
 *
 * @param gui the GUI
 * @param cmds the command buffer set
 * @param idx the command buffer index within the set
 */
DVZ_EXPORT void dvz_gui_window_end(DvzGuiWindow* gui_window, uint32_t cmd_idx);



/**
 * To be called at the end of the command buffer recording.
 *
 * @param gui the GUI
 * @param cmds the command buffer set
 * @param idx the command buffer index within the set
 */
DVZ_EXPORT void dvz_gui_window_resize(DvzGuiWindow* gui_window, uint32_t width, uint32_t height);



/**
 * Destroy a GUI window.
 *
 * @param gui the GUI window
 */
DVZ_EXPORT void dvz_gui_window_destroy(DvzGuiWindow* gui_window);



/*************************************************************************************************/
/*  DearImGui Wrappers                                                                           */
/*************************************************************************************************/

/**
 * Start a new dialog.
 *
 * @param title the dialog title
 * @param pos the dialog position
 * @param size the dialog size
 * @param flags the flags
 */
DVZ_EXPORT void dvz_gui_dialog_begin(const char* title, vec2 pos, vec2 size, int flags);



/**
 * Add a text item in a dialog.
 *
 * @param fmt the format string
 */
DVZ_EXPORT void dvz_gui_text(const char* fmt, ...);



/**
 */
DVZ_EXPORT bool dvz_gui_slider(const char* name, float vmin, float vmax, float* value);



/**
 */
DVZ_EXPORT void dvz_gui_image(DvzTex* tex, float width, float height);



/**
 * Show the demo GUI.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_demo(void);



/**
 * Stop the creation of the dialog.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_dialog_end(void);



EXTERN_C_OFF

#endif
