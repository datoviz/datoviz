/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Dear ImGui overlay                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;

typedef void (*DvzGuiCallback)(DvzGui* gui, DvzAppWindow* win, void* user_data);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzGuiFlags
{
    DVZ_GUI_FLAGS_NONE = 0,
    DVZ_GUI_FLAGS_DOCKING = 1u << 0,
    DVZ_GUI_FLAGS_DOCKSPACE = 1u << 1,
} DvzGuiFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzGuiConfig
{
    uint32_t flags;
    const char* ini_path;
    float font_size;
    float mono_font_size;
} DvzGuiConfig;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default GUI overlay configuration.
 *
 * @return default GUI configuration
 */
DVZ_EXPORT DvzGuiConfig dvz_gui_config(void);



/**
 * Attach a Dear ImGui overlay to a GLFW app window.
 *
 * @param win the app window
 * @param config optional GUI configuration
 * @return the GUI overlay, or NULL on failure
 */
DVZ_EXPORT DvzGui* dvz_app_window_gui(DvzAppWindow* win, const DvzGuiConfig* config);



/**
 * Register a GUI callback called while building each ImGui frame.
 *
 * @param win the app window
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
DVZ_EXPORT void
dvz_app_window_set_gui_callback(
    DvzAppWindow* win, DvzGuiCallback callback, void* user_data);



/**
 * Start an ImGui window.
 *
 * @param gui the GUI overlay
 * @param title the window title
 * @param open optional open flag, or NULL
 * @param flags Dear ImGui window flags
 * @return whether the window body is visible
 */
DVZ_EXPORT bool dvz_gui_begin(DvzGui* gui, const char* title, bool* open, int flags);



/**
 * End the current ImGui window.
 *
 * @param gui the GUI overlay
 */
DVZ_EXPORT void dvz_gui_end(DvzGui* gui);



/**
 * Show an unformatted text item.
 *
 * @param gui the GUI overlay
 * @param text null-terminated text
 */
DVZ_EXPORT void dvz_gui_text(DvzGui* gui, const char* text);



/**
 * Push the default monospace ImGui font.
 *
 * @param gui the GUI overlay
 * @return whether the monospace font was available and pushed
 */
DVZ_EXPORT bool dvz_gui_push_mono(DvzGui* gui);



/**
 * Pop the current ImGui font.
 *
 * @param gui the GUI overlay
 */
DVZ_EXPORT void dvz_gui_pop_font(DvzGui* gui);



/**
 * Show a button.
 *
 * @param gui the GUI overlay
 * @param label button label
 * @return whether the button was pressed
 */
DVZ_EXPORT bool dvz_gui_button(DvzGui* gui, const char* label);



/**
 * Show a checkbox.
 *
 * @param gui the GUI overlay
 * @param label checkbox label
 * @param value value edited in place
 * @return whether the value changed
 */
DVZ_EXPORT bool dvz_gui_checkbox(DvzGui* gui, const char* label, bool* value);



/**
 * Show a float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value value edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
DVZ_EXPORT bool
dvz_gui_slider_float(DvzGui* gui, const char* label, float* value, float min, float max);



/**
 * Show Dear ImGui's demo window.
 *
 * @param gui the GUI overlay
 * @param open optional open flag, or NULL
 */
DVZ_EXPORT void dvz_gui_demo(DvzGui* gui, bool* open);



EXTERN_C_OFF
