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
#include "datoviz/math/types.h"



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
 * Show a dropdown combo.
 *
 * @param gui the GUI overlay
 * @param label combo label
 * @param current_item selected item index edited in place
 * @param items item labels
 * @param item_count number of item labels
 * @return whether the selection changed
 */
DVZ_EXPORT bool dvz_gui_combo(
    DvzGui* gui, const char* label, int* current_item, const char* const* items, int item_count);



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
 * Show an integer slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value value edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
DVZ_EXPORT bool
dvz_gui_slider_int(DvzGui* gui, const char* label, int* value, int min, int max);



/**
 * Show a two-component float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value two values edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
DVZ_EXPORT bool
dvz_gui_slider_float2(DvzGui* gui, const char* label, float value[2], float min, float max);



/**
 * Show a three-component float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value three values edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
DVZ_EXPORT bool
dvz_gui_slider_float3(DvzGui* gui, const char* label, float value[3], float min, float max);



/**
 * Show a four-component float slider.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value four values edited in place
 * @param min minimum value
 * @param max maximum value
 * @return whether the value changed
 */
DVZ_EXPORT bool
dvz_gui_slider_float4(DvzGui* gui, const char* label, float value[4], float min, float max);



/**
 * Show a float slider with an explicit display format.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param value value edited in place
 * @param min minimum value
 * @param max maximum value
 * @param format printf-style value format
 * @return whether the value changed
 */
DVZ_EXPORT bool dvz_gui_slider_float_format(
    DvzGui* gui, const char* label, float* value, float min, float max, const char* format);



/**
 * Show a float min/max range editor.
 *
 * @param gui the GUI overlay
 * @param label range label
 * @param current_min minimum value edited in place
 * @param current_max maximum value edited in place
 * @param speed drag speed
 * @param min lower clamp value
 * @param max upper clamp value
 * @param format printf-style value format
 * @return whether either value changed
 */
DVZ_EXPORT bool dvz_gui_range_float(
    DvzGui* gui, const char* label, float* current_min, float* current_max, float speed,
    float min, float max, const char* format);



/**
 * Show an RGBA color editor using float channels in [0, 1].
 *
 * @param gui the GUI overlay
 * @param label color label
 * @param rgba RGBA channels edited in place
 * @param flags Dear ImGui color edit flags
 * @return whether the value changed
 */
DVZ_EXPORT bool dvz_gui_color_edit4(DvzGui* gui, const char* label, float rgba[4], int flags);



/**
 * Show an RGBA color editor using a DvzColor value.
 *
 * @param gui the GUI overlay
 * @param label color label
 * @param color color edited in place
 * @param flags Dear ImGui color edit flags
 * @return whether the value changed
 */
DVZ_EXPORT bool dvz_gui_color_edit_dvz(DvzGui* gui, const char* label, DvzColor color, int flags);



/**
 * Show an RGBA color picker using float channels in [0, 1].
 *
 * @param gui the GUI overlay
 * @param label color label
 * @param rgba RGBA channels edited in place
 * @param flags Dear ImGui color edit flags
 * @return whether the value changed
 */
DVZ_EXPORT bool dvz_gui_color_picker4(DvzGui* gui, const char* label, float rgba[4], int flags);



/**
 * Show a labeled separator.
 *
 * @param gui the GUI overlay
 * @param label separator label
 */
DVZ_EXPORT void dvz_gui_separator_text(DvzGui* gui, const char* label);



/**
 * Show a collapsible section header.
 *
 * @param gui the GUI overlay
 * @param label section label
 * @param flags Dear ImGui tree node flags
 * @return whether the section is open
 */
DVZ_EXPORT bool dvz_gui_collapsing_header(DvzGui* gui, const char* label, int flags);



/**
 * Place the next item on the same line.
 *
 * @param gui the GUI overlay
 * @param offset_from_start_x x offset from start, or 0
 * @param spacing spacing between items, or -1 for default
 */
DVZ_EXPORT void dvz_gui_same_line(DvzGui* gui, float offset_from_start_x, float spacing);



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
 * Pass the returned router to dvz_panel_connect_input() to route input through scene panels
 * rendered in the GUI viewport.
 *
 * @param viewport the GUI viewport
 * @return the input router, or NULL
 */
DVZ_EXPORT struct DvzInputRouter* dvz_gui_viewport_input(DvzGuiViewport* viewport);


/**
 * Return the last mouse position over a dockable GUI viewport image.
 *
 * The position and size are in the viewport source window's logical coordinates. The returned
 * state is refreshed by dvz_gui_viewport_window().
 *
 * @param viewport the GUI viewport
 * @param out_pos optional output mouse x/y coordinates
 * @param out_size optional output displayed source width/height
 * @param out_hovered optional output hover state
 * @return whether viewport mouse state was available
 */
DVZ_EXPORT bool dvz_gui_viewport_mouse(
    DvzGuiViewport* viewport, float out_pos[2], float out_size[2], bool* out_hovered);



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
