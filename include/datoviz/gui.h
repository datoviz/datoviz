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
typedef struct DvzGuiViewport DvzGuiViewport;

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



typedef enum DvzGuiViewportFlags
{
    DVZ_GUI_VIEWPORT_FLAGS_NONE = 0,

    /* Forward mouse input from the ImGui image item to the viewport's source input router. */
    DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT = 1u << 0,

    /* Keep rendering the source figure even while the ImGui viewport window is hidden. */
    DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN = 1u << 1,
} DvzGuiViewportFlags;



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



typedef struct DvzGuiViewportConfig
{
    uint32_t flags;

    /* Initial size of the owned offscreen source window created by dvz_gui_viewport(). */
    uint32_t initial_width;
    uint32_t initial_height;

    /* Minimum source size after the ImGui content region is resized. */
    uint32_t min_width;
    uint32_t min_height;

    /* Resize quantization and debounce policy for the source offscreen window. */
    uint32_t resize_step;
    uint32_t resize_delay_frames;
} DvzGuiViewportConfig;



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
 * Return the default dockable Datoviz GUI viewport configuration.
 *
 * @return default GUI viewport configuration
 */
DVZ_EXPORT DvzGuiViewportConfig dvz_gui_viewport_config(void);



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



/**
 * Create a dockable ImGui viewport that renders a figure into an owned offscreen window.
 *
 * A GUI viewport is an ImGui-hosted Datoviz render target. It is not a scene DvzPanel. The
 * supplied figure may contain any scene panels and visuals; the viewport creates and manages the
 * offscreen app-window used to render that figure, then displays the latest source image in an
 * ImGui window created by dvz_gui_viewport_window().
 *
 * @param gui the GUI overlay
 * @param figure figure to render inside the GUI viewport
 * @param config optional viewport configuration
 * @return the GUI viewport, or NULL on failure
 */
DVZ_EXPORT DvzGuiViewport*
dvz_gui_viewport(DvzGui* gui, DvzFigure* figure, const DvzGuiViewportConfig* config);



/**
 * Create a dockable ImGui viewport from an existing offscreen app window.
 *
 * This is the advanced path for callers that already own the source app-window. Most users should
 * prefer dvz_gui_viewport(), which creates the offscreen source from a figure. The source window
 * must use offscreen canvas rendering.
 *
 * @param gui the GUI overlay
 * @param source app window providing the rendered image
 * @param config optional viewport configuration
 * @return the GUI viewport, or NULL on failure
 */
DVZ_EXPORT DvzGuiViewport*
dvz_gui_viewport_from_window(
    DvzGui* gui, DvzAppWindow* source, const DvzGuiViewportConfig* config);



/**
 * Return the input router used by a GUI viewport's offscreen app window.
 *
 * Pass the returned router to dvz_panel_set_panzoom() or dvz_panel_set_arcball() to attach
 * controllers to scene panels rendered in the GUI viewport.
 *
 * @param viewport the GUI viewport
 * @return the input router, or NULL
 */
DVZ_EXPORT struct DvzInputRouter* dvz_gui_viewport_input(DvzGuiViewport* viewport);



/**
 * Destroy a dockable ImGui viewport.
 *
 * @param viewport the GUI viewport
 */
DVZ_EXPORT void dvz_gui_viewport_destroy(DvzGuiViewport* viewport);



/**
 * Show a dockable ImGui window containing a Datoviz-rendered viewport image.
 *
 * Hidden or collapsed viewport windows stop rendering their source figure by default after the
 * first image is available. Set DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN to keep the source
 * rendering continuously.
 *
 * @param viewport the GUI viewport
 * @param title the ImGui window title
 * @param open optional open flag, or NULL
 * @param flags Dear ImGui window flags
 * @return whether the Datoviz image was visible this frame
 */
DVZ_EXPORT bool
dvz_gui_viewport_window(DvzGuiViewport* viewport, const char* title, bool* open, int flags);



EXTERN_C_OFF
