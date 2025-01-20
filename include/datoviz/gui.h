/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

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

#define DVZ_DIALOG_DEFAULT_PIVOT                                                                  \
    (vec2) { 0, 0 }



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
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGui
{
    DvzGpu* gpu;
    DvzRenderpass renderpass;
    DvzContainer gui_windows;
    int flags;
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
DvzGui* dvz_gui(DvzGpu* gpu, uint32_t queue_idx, int flags);



/**
 * Destroy the GUI renderer.
 *
 * @param gui the GUI
 */
void dvz_gui_destroy(DvzGui* gui);



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
DvzGuiWindow*
dvz_gui_window(DvzGui* gui, DvzWindow* window, DvzImages* images, uint32_t queue_idx);



/**
 * Initialize an offscreen GUI.
 *
 * @param gui the GUI
 * @returns gui_window
 */
DvzGuiWindow* dvz_gui_offscreen(DvzGui* gui, DvzImages* images, uint32_t queue_idx);



/**
 * To be called at the beginning of the command buffer recording.
 *
 * @param gui the GUI
 * @param window the window
 */
void dvz_gui_window_begin(DvzGuiWindow* gui_window, uint32_t cmd_idx);



/**
 * To be called at the end of the command buffer recording.
 *
 * @param gui the GUI
 * @param cmds the command buffer set
 * @param idx the command buffer index within the set
 */
void dvz_gui_window_end(DvzGuiWindow* gui_window, uint32_t cmd_idx);



/**
 * To be called at the end of the command buffer recording.
 *
 * @param gui the GUI
 * @param cmds the command buffer set
 * @param idx the command buffer index within the set
 */
void dvz_gui_window_resize(DvzGuiWindow* gui_window, uint32_t width, uint32_t height);



/**
 * Destroy a GUI window.
 *
 * @param gui the GUI window
 */
void dvz_gui_window_destroy(DvzGuiWindow* gui_window);



EXTERN_C_OFF

#endif
