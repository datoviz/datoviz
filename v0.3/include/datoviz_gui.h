/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */



/*************************************************************************************************/
/*  Public GUI API                                                                               */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_GUI
#define DVZ_HEADER_PUBLIC_GUI



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "datoviz_app.h"
#include "datoviz_keycodes.h"
#include "datoviz_macros.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "datoviz_version.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzGuiWindow DvzGuiWindow;
typedef struct DvzTex DvzTex;



EXTERN_C_ON

/*************************************************************************************************/
/*************************************************************************************************/
/*  GUI functions                                                                                */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Capture a GUI window.
 *
 * @param gui_window the GUI window
 * @param is_captured whether the windows should be captured
 *
 */
DVZ_EXPORT void dvz_gui_window_capture(DvzGuiWindow* gui_window, bool is_captured);



/**
 * Return whether a dialog is being moved.
 *
 * @returns whether the dialog is being moved
 */
DVZ_EXPORT bool dvz_gui_moving(void);



/**
 * Return whether a dialog is being resized
 *
 * @returns whether the dialog is being resized
 */
DVZ_EXPORT bool dvz_gui_resizing(void);



/**
 * Return whether a dialog has just moved.
 *
 * @returns whether the dialog has just moved
 */
DVZ_EXPORT bool dvz_gui_moved(void);



/**
 * Return whether a dialog has just been resized.
 *
 * @returns whether the dialog has just been resized
 */
DVZ_EXPORT bool dvz_gui_resized(void);



/**
 * Return whether a dialog is collapsed.
 *
 * @returns whether the dialog is collapsed
 */
DVZ_EXPORT bool dvz_gui_collapsed(void);



/**
 * Return whether a dialog has just been collapsed or uncollapsed.
 *
 * @returns whether the dialog has just been collapsed or uncollapsed.
 */
DVZ_EXPORT bool dvz_gui_collapse_changed(void);



/**
 * Return whether mouse is dragging.
 *
 * @returns whether the mouse is dragging
 */
DVZ_EXPORT bool dvz_gui_dragging(void);



/**
 * Set the position of the next GUI dialog.
 *
 * @param pos the dialog position
 * @param pivot the pivot
 *
 */
DVZ_EXPORT void dvz_gui_pos(vec2 pos, vec2 pivot);



/**
 * Set a fixed position for a GUI dialog.
 *
 * @param pos the dialog position
 * @param pivot the pivot
 *
 */
DVZ_EXPORT void dvz_gui_fixed(vec2 pos, vec2 pivot);



/**
 * Get the position and size of the current dialog.
 *
 * @param viewport the x, y, w, h values
 *
 */
DVZ_EXPORT void dvz_gui_viewport(vec4 viewport);



/**
 * Set the corner position of the next GUI dialog.
 *
 * @param corner which corner
 * @param pad the pad
 *
 */
DVZ_EXPORT void dvz_gui_corner(DvzCorner corner, vec2 pad);



/**
 * Set the size of the next GUI dialog.
 *
 * @param size the size
 *
 */
DVZ_EXPORT void dvz_gui_size(vec2 size);



/**
 * Set the color of an element.
 *
 * @param type the element type for which to change the color
 * @param color the color
 *
 */
DVZ_EXPORT void dvz_gui_color(int type, DvzColor color);



/**
 * Set the style of an element.
 *
 * @param type the element type for which to change the style
 * @param value the value
 *
 */
DVZ_EXPORT void dvz_gui_style(int type, float value);



/**
 * Set the flags of the next GUI dialog.
 *
 * @param flags the flags
 *
 */
DVZ_EXPORT int dvz_gui_flags(int flags);



/**
 * Set the alpha transparency of the next GUI dialog.
 *
 * @param alpha the alpha transparency value
 *
 */
DVZ_EXPORT void dvz_gui_alpha(float alpha);



/**
 * Start a new dialog.
 *
 * @param title the dialog title
 * @param flags the flags
 */
DVZ_EXPORT void dvz_gui_begin(const char* title, int flags);



/**
 * Add a text item in a dialog.
 *
 * @param fmt the format string
 */
DVZ_EXPORT void dvz_gui_text(const char* fmt, ...);



/**
 * Add a text box in a dialog.
 *
 * @param label the label
 * @param str_len the size of the str buffer
 * @param str the modified string
 * @param flags the flags
 * @returns whether the text has changed
 */
DVZ_EXPORT bool dvz_gui_textbox(const char* label, uint32_t str_len, char* str, int flags);



/**
 * Add a slider.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider(const char* name, float vmin, float vmax, float* value);



/**
 * Add a slider with 2 values.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_vec2(const char* name, float vmin, float vmax, vec2 value);



/**
 * Add a slider with 3 values.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_vec3(const char* name, float vmin, float vmax, vec3 value);



/**
 * Add a slider with 4 values.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_vec4(const char* name, float vmin, float vmax, vec4 value);



/**
 * Add an integer slider.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_int(const char* name, int vmin, int vmax, int* value);



/**
 * Add an integer slider with 2 values.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_ivec2(const char* name, int vmin, int vmax, ivec2 value);



/**
 * Add an integer slider with 3 values.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_ivec3(const char* name, int vmin, int vmax, ivec3 value);



/**
 * Add an integer slider with 4 values.
 *
 * @param name the slider name
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] value the pointer to the value
 * @returns whether the value has changed
 */
DVZ_EXPORT bool dvz_gui_slider_ivec4(const char* name, int vmin, int vmax, ivec4 value);



/**
 * Add a button.
 *
 * @param name the button name
 * @param width the button width
 * @param height the button height
 * @returns whether the button was pressed
 */
DVZ_EXPORT bool dvz_gui_button(const char* name, float width, float height);



/**
 * Add a checkbox.
 *
 * @param name the button name
 * @param[out] checked whether the checkbox is checked
 * @returns whether the checkbox's state has changed
 */
DVZ_EXPORT bool dvz_gui_checkbox(const char* name, bool* checked);



/**
 * Add a dropdown menu.
 *
 * @param name the menu name
 * @param count the number of menu items
 * @param items the item labels
 * @param[out] selected a pointer to the selected index
 * @param flags the dropdown menu flags
 */
DVZ_EXPORT bool dvz_gui_dropdown(
    const char* name, uint32_t count, const char** items, uint32_t* selected, int flags);



/**
 * Add a progress widget.
 *
 * @param fraction the fraction between 0 and 1
 * @param width the widget width
 * @param height the widget height
 * @param fmt the format string
 */
DVZ_EXPORT void dvz_gui_progress(float fraction, float width, float height, const char* fmt, ...);



/**
 * Add an image in a GUI dialog.
 *
 * @param tex the texture
 * @param width the image width
 * @param height the image height
 */
DVZ_EXPORT void dvz_gui_image(DvzTex* tex, float width, float height);



/**
 * Add a color picker
 *
 * @param name the widget name
 * @param color the color
 * @param flags the widget flags
 */
DVZ_EXPORT bool dvz_gui_colorpicker(const char* name, vec3 color, int flags);



/**
 * Start a new tree node.
 *
 * @param name the widget name
 */
DVZ_EXPORT bool dvz_gui_node(const char* name);



/**
 * Close the current tree node.
 */
DVZ_EXPORT void dvz_gui_pop(void);



/**
 * Close the current tree node.
 */
DVZ_EXPORT bool dvz_gui_clicked(void);



/**
 * Close the current tree node.
 *
 * @param name the widget name
 */
DVZ_EXPORT bool dvz_gui_selectable(const char* name);



/**
 * Display a table with selectable rows.
 *
 * @param name the widget name
 * @param row_count the number of rows
 * @param column_count the number of columns
 * @param labels all cell labels
 * @param selected a pointer to an array of boolean indicated which rows are selected
 * @param flags the Dear ImGui flags
 * @returns whether the row selection has changed (in the selected array)
 */
DVZ_EXPORT bool dvz_gui_table(                                   //
    const char* name, uint32_t row_count, uint32_t column_count, //
    const char** labels, bool* selected, int flags);



/**
 * Display a collapsible tree. Assumes the data is in the right order, with level encoding the
 * depth of each row within the tree.
 *
 * Filtering can be implemented with the "visible" parameter. Note that this function automatically
 * propagates the visibility of each node to all its descendents and ascendents, without modifying
 * in-place the "visible" array.
 *
 * @param count the number of rows
 * @param ids short id of each row
 * @param labels full label of each row
 * @param levels a positive integer indicate
 * @param colors the color of each square in each row
 * @param folded whether each row is currently folded (modified by this function)
 * @param selected whether each row is currently selected (modified by this function)
 * @param visible whether each row is visible (used for filtering)
 * @returns whether the selection has changed
 */
DVZ_EXPORT bool dvz_gui_tree(
    uint32_t count, char** ids, char** labels, uint32_t* levels, DvzColor* colors, bool* folded,
    bool* selected, bool* visible);



/**
 * Show the demo GUI.
 */
DVZ_EXPORT void dvz_gui_demo(void);



/**
 * Stop the creation of the dialog.
 */
DVZ_EXPORT void dvz_gui_end(void);



EXTERN_C_OFF

#endif
