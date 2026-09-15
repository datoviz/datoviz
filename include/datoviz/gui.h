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
#include "datoviz/font.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;
typedef struct DvzGuiViewport DvzGuiViewport;
typedef struct DvzGuiTree DvzGuiTree;
typedef struct DvzGuiTable DvzGuiTable;

typedef void (*DvzGuiCallback)(DvzGui* gui, DvzView* view, void* user_data);

/** Column sentinel used by events which do not concern one table column. */
#define DVZ_GUI_COLUMN_NONE UINT32_MAX



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzGuiFlags
{
    DVZ_GUI_FLAGS_NONE = 0,
    DVZ_GUI_FLAGS_DOCKING = 1u << 0,   /* enable ImGui docking support */
    DVZ_GUI_FLAGS_DOCKSPACE = 1u << 1, /* create the default full-view dockspace */
} DvzGuiFlags;



typedef enum DvzGuiViewportFlags
{
    DVZ_GUI_VIEWPORT_FLAGS_NONE = 0,

    /* Forward mouse input from the ImGui image item to the viewport's source input router. */
    DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT = 1u << 0,

    /* Keep rendering the source figure even while the ImGui viewport window is hidden. */
    DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN = 1u << 1,
} DvzGuiViewportFlags;



typedef enum DvzGuiDockSlot
{
    DVZ_GUI_DOCK_SLOT_LEFT,
    DVZ_GUI_DOCK_SLOT_RIGHT,
    DVZ_GUI_DOCK_SLOT_TOP,
    DVZ_GUI_DOCK_SLOT_BOTTOM,
    DVZ_GUI_DOCK_SLOT_CENTER,
} DvzGuiDockSlot;


typedef enum DvzGuiDataEventType
{
    DVZ_GUI_DATA_EVENT_NONE = 0,
    DVZ_GUI_DATA_EVENT_SELECTION_CHANGED,
    DVZ_GUI_DATA_EVENT_ACTIVATED,
    DVZ_GUI_DATA_EVENT_EXPANSION_CHANGED,
    DVZ_GUI_DATA_EVENT_SORT_CHANGED,
    DVZ_GUI_DATA_EVENT_FILTER_CHANGED,
} DvzGuiDataEventType;

typedef enum DvzGuiDataWidgetFlags
{
    DVZ_GUI_DATA_WIDGET_FLAGS_NONE = 0,
    DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT = 1u << 0,
    DVZ_GUI_DATA_WIDGET_FLAGS_FILTER = 1u << 1,
} DvzGuiDataWidgetFlags;

typedef enum DvzGuiDataSetFlags
{
    DVZ_GUI_DATA_SET_FLAGS_NONE = 0,
    DVZ_GUI_DATA_SET_FLAGS_RESET_STATE = 1u << 0,
} DvzGuiDataSetFlags;

typedef enum DvzGuiDataStyleFlags
{
    DVZ_GUI_DATA_STYLE_FLAGS_NONE = 0,
    DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND = 1u << 0,
    DVZ_GUI_DATA_STYLE_FLAGS_BACKGROUND = 1u << 1,
    DVZ_GUI_DATA_STYLE_FLAGS_ACCENT = 1u << 2,
    DVZ_GUI_DATA_STYLE_FLAGS_DISABLED = 1u << 3,
} DvzGuiDataStyleFlags;

typedef enum DvzGuiTableColumnFlags
{
    DVZ_GUI_TABLE_COLUMN_FLAGS_NONE = 0,
    DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE = 1u << 0,
    DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE = 1u << 1,
    DVZ_GUI_TABLE_COLUMN_FLAGS_STRETCH = 1u << 2,
} DvzGuiTableColumnFlags;

typedef enum DvzGuiTableColumnType
{
    DVZ_GUI_TABLE_COLUMN_TEXT = 0,
    DVZ_GUI_TABLE_COLUMN_INT64,
    DVZ_GUI_TABLE_COLUMN_DOUBLE,
    DVZ_GUI_TABLE_COLUMN_BOOL,
    DVZ_GUI_TABLE_COLUMN_COLOR,
} DvzGuiTableColumnType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzGuiConfig
{
    uint32_t struct_size;
    uint32_t flags;     /* Reserved for future GUI config flags; must be 0 in v0.4. */
    uint32_t gui_flags; /* Bitwise OR of DvzGuiFlags. */
    /* Initial width hint for ordinary dvz_gui_begin() windows; 0 keeps ImGui's auto width. */
    uint32_t default_window_width;
    /* ImGui .ini path, or NULL to disable persisted GUI window state. */
    const char* ini_path;
} DvzGuiConfig;



typedef struct DvzGuiViewportConfig
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t viewport_flags;

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


typedef struct DvzGuiDataEvent
{
    uint32_t type;
    uint32_t flags;
    uint64_t row_key;
    uint32_t column_id;
    int32_t detail;
    uint64_t revision;
} DvzGuiDataEvent;


typedef struct DvzGuiTableColumnDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t column_id;
    uint32_t type;
    float initial_width;
    const char* title;
    const char* format;
    uint32_t reserved[4];
} DvzGuiTableColumnDesc;


typedef struct DvzGuiDataStyle
{
    uint32_t struct_size;
    uint32_t flags;
    uint64_t row_key;
    DvzColor foreground;
    DvzColor background;
    DvzColor accent;
    uint32_t reserved[4];
} DvzGuiDataStyle;



/** Compact, font-relative layout parameters for a retained tree. */
typedef struct DvzGuiTreeLayout
{
    uint32_t struct_size;
    uint32_t flags;
    float indent_em;
    float row_padding_em;
    float item_spacing_em;
    float disclosure_gap_em;
    float swatch_gap_em;
    float secondary_gap_em;
    uint32_t reserved[2];
} DvzGuiTreeLayout;



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
 * Return a default table column descriptor.
 *
 * @return a zero-initialized, size-versioned descriptor
 */
DVZ_EXPORT DvzGuiTableColumnDesc dvz_gui_table_column_desc(void);

/**
 * Return a default data row style.
 *
 * @return a zero-initialized, size-versioned style
 */
DVZ_EXPORT DvzGuiDataStyle dvz_gui_data_style(void);



/**
 * Attach a Dear ImGui overlay to a GLFW view.
 *
 * @param view the view
 * @param config optional GUI configuration
 * @return the GUI overlay, or NULL on failure
 */
DVZ_EXPORT DvzGui* dvz_view_gui(DvzView* view, const DvzGuiConfig* config);



/**
 * Register a GUI callback called while building each ImGui frame.
 *
 * The callback can be registered only after `dvz_view_gui()` has created the overlay; calling this
 * before overlay creation returns DVZ_ERROR.
 * The callback and user data are retained until replaced, cleared, or the view is destroyed; the
 * caller must keep them valid for that lifetime.
 *
 * @param view the view
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult
dvz_view_set_gui_callback(DvzView* view, DvzGuiCallback callback, void* user_data);



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
 * Dock an ImGui window into the default full-view dockspace the first time it is shown.
 *
 * Call before dvz_gui_begin() with the same title. The request is applied only once per title, so
 * user-driven docking changes are not overwritten on later frames.
 *
 * @param gui the GUI overlay
 * @param title the window title
 * @param slot side or remaining center of the full-view dockspace
 * @param size_px initial docked size in logical pixels along the split axis, or 0 for default
 * @return DVZ_OK if the request was accepted, DVZ_ERROR otherwise
 */
DVZ_EXPORT DvzResult
dvz_gui_dock_window_once(DvzGui* gui, const char* title, DvzGuiDockSlot slot, float size_px);



/**
 * Return whether the current ImGui window is docked.
 *
 * Call between dvz_gui_begin() and dvz_gui_end().
 *
 * @param gui the GUI overlay
 * @return whether the current ImGui window is docked
 */
DVZ_EXPORT bool dvz_gui_current_window_docked(DvzGui* gui);



/**
 * Return the current ImGui window rectangle in host-window logical pixels.
 *
 * Call between dvz_gui_begin() and dvz_gui_end().
 *
 * @param gui the GUI overlay
 * @param out output rectangle
 * @return whether the rectangle was written
 */
DVZ_EXPORT bool dvz_gui_current_window_rect(DvzGui* gui, DvzRect* out);



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
DVZ_EXPORT bool dvz_gui_slider_int(DvzGui* gui, const char* label, int* value, int min, int max);



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
 * Show a float range slider with two handles.
 *
 * @param gui the GUI overlay
 * @param label slider label
 * @param current_min minimum value edited in place
 * @param current_max maximum value edited in place
 * @param min lower clamp value
 * @param max upper clamp value
 * @param format optional printf-style value format
 * @return whether either value changed
 */
DVZ_EXPORT bool dvz_gui_slider_range_float(
    DvzGui* gui, const char* label, float* current_min, float* current_max, float min, float max,
    const char* format);



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
    DvzGui* gui, const char* label, float* current_min, float* current_max, float speed, float min,
    float max, const char* format);



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
DVZ_EXPORT bool dvz_gui_color_edit_dvz(DvzGui* gui, const char* label, DvzColor* color, int flags);



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
 * offscreen view used to render that figure, then displays the latest source image in an
 * ImGui window created by dvz_gui_viewport_window().
 * The GUI overlay and figure are borrowed and must outlive the returned viewport. Destroying the
 * viewport disables its internally created source view but does not destroy the figure.
 *
 * @param gui the borrowed GUI overlay; must not be NULL
 * @param figure the borrowed figure to render; must not be NULL
 * @param config optional viewport configuration borrowed for the call, or NULL for defaults
 * @return a newly allocated GUI viewport owned by the caller, or NULL on failure
 */
DVZ_EXPORT DvzGuiViewport*
dvz_gui_viewport(DvzGui* gui, DvzFigure* figure, const DvzGuiViewportConfig* config);



/**
 * Create a dockable ImGui viewport from an existing offscreen view.
 *
 * This is the advanced path for callers that already own the source view. Most users should
 * prefer dvz_gui_viewport(), which creates the offscreen source from a figure. The source window
 * must use offscreen canvas rendering. The GUI overlay and source view are borrowed and must
 * outlive the returned viewport; destroying the viewport does not destroy or disable the source.
 *
 * @param gui the borrowed GUI overlay; must not be NULL
 * @param source the borrowed offscreen view providing the rendered image; must not be NULL
 * @param config optional viewport configuration borrowed for the call, or NULL for defaults
 * @return a newly allocated GUI viewport owned by the caller, or NULL on failure
 */
DVZ_EXPORT DvzGuiViewport*
dvz_gui_viewport_from_window(DvzGui* gui, DvzView* source, const DvzGuiViewportConfig* config);



/**
 * Return the input router used by a GUI viewport's offscreen view.
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
 * An internally created source view is disabled; a caller-provided source view remains enabled.
 * The associated GUI overlay, figure, and source view are not destroyed.
 *
 * @param viewport the owned GUI viewport to destroy, or NULL
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

/**
 * Create a retained tree widget which owns copies of all supplied model data.
 *
 * @param widget_id stable, non-empty UTF-8 ImGui identifier
 * @param flags bitwise OR of DvzGuiDataWidgetFlags
 * @return a caller-owned tree, or NULL on validation or allocation failure
 */
DVZ_EXPORT DvzGuiTree* dvz_gui_tree(const char* widget_id, uint32_t flags);

/**
 * Return compact default layout parameters for a retained tree.
 *
 * Distances use em units and therefore follow the attached GUI's device and user scale.
 *
 * @return default tree layout
 */
DVZ_EXPORT DvzGuiTreeLayout dvz_gui_tree_layout(void);

/**
 * Set font-relative layout parameters for a retained tree.
 *
 * @param tree retained tree
 * @param layout size-versioned tree layout
 * @return DVZ_OK on success, DVZ_ERROR on invalid input
 */
DVZ_EXPORT DvzResult dvz_gui_tree_set_layout(DvzGuiTree* tree, const DvzGuiTreeLayout* layout);

/**
 * Replace the tree rows atomically.
 *
 * @param tree tree
 * @param row_count row count
 * @param keys keys
 * @param parents parents
 * @param labels labels
 * @param secondary_labels secondary labels
 * @param flags flags
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_set_rows(
    DvzGuiTree* tree, uint32_t row_count, const uint64_t* keys, const uint32_t* parents,
    const char* const* labels, const char* const* secondary_labels, uint32_t flags);

/**
 * Replace copied tree swatches atomically.
 *
 * @param tree tree
 * @param row_count row count
 * @param colors colors
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_tree_set_swatches(DvzGuiTree* tree, uint32_t row_count, const DvzColor* colors);

/**
 * Replace copied keyed tree row styles atomically.
 *
 * @param tree tree
 * @param style_count style count
 * @param styles styles
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_tree_set_styles(DvzGuiTree* tree, uint32_t style_count, const DvzGuiDataStyle* styles);

/**
 * Replace tree selection by stable row key.
 *
 * @param tree tree
 * @param key_count key count
 * @param keys keys
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_tree_set_selection(DvzGuiTree* tree, uint32_t key_count, const uint64_t* keys);

/**
 * Copy selected tree keys up to capacity.
 *
 * @param tree tree
 * @param capacity capacity
 * @param keys keys
 * @return the full selected count
 */
DVZ_EXPORT uint32_t
dvz_gui_tree_get_selection(const DvzGuiTree* tree, uint32_t capacity, uint64_t* keys);

/**
 * Replace tree expansion state by stable row key.
 *
 * @param tree tree
 * @param key_count key count
 * @param keys keys
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_tree_set_expanded(DvzGuiTree* tree, uint32_t key_count, const uint64_t* keys);

/**
 * Copy expanded tree keys up to capacity.
 *
 * @param tree tree
 * @param capacity capacity
 * @param keys keys
 * @return the full expanded count
 */
DVZ_EXPORT uint32_t
dvz_gui_tree_get_expanded(const DvzGuiTree* tree, uint32_t capacity, uint64_t* keys);

/**
 * Expand all tree rows.
 *
 * @param tree tree
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_expand_all(DvzGuiTree* tree);

/**
 * Expand tree rows above the requested depth.
 *
 * @param tree tree
 * @param depth depth
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_expand_to_depth(DvzGuiTree* tree, uint32_t depth);

/**
 * Collapse all tree rows.
 *
 * @param tree tree
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_collapse_all(DvzGuiTree* tree);

/**
 * Expand ancestors and schedule scrolling to one row.
 *
 * @param tree tree
 * @param key key
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_reveal(DvzGuiTree* tree, uint64_t key);

/**
 * Set the strict tree visibility mask.
 *
 * @param tree tree
 * @param row_count row count
 * @param visible visible
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_tree_set_visible(DvzGuiTree* tree, uint32_t row_count, const bool* visible);

/**
 * Set the external tree match mask.
 *
 * @param tree tree
 * @param row_count row count
 * @param matches matches
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_tree_set_matches(DvzGuiTree* tree, uint32_t row_count, const bool* matches);

/**
 * Copy a UTF-8 native filter into the tree.
 *
 * @param tree tree
 * @param filter filter
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_set_filter(DvzGuiTree* tree, const char* filter);

/**
 * Copy the tree filter up to capacity.
 *
 * @param tree tree
 * @param capacity capacity
 * @param filter filter
 * @return the required byte count including the terminator
 */
DVZ_EXPORT uint32_t
dvz_gui_tree_get_filter(const DvzGuiTree* tree, uint32_t capacity, char* filter);

/**
 * Draw a tree and drain interaction events.
 *
 * @param gui gui
 * @param tree tree
 * @param events events
 * @param capacity capacity
 * @param written written
 * @param dropped dropped
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_tree_draw(
    DvzGui* gui, DvzGuiTree* tree, DvzGuiDataEvent* events, uint32_t capacity, uint32_t* written,
    uint32_t* dropped);

/**
 * Destroy a retained tree.
 *
 * @param tree tree
 */
DVZ_EXPORT void dvz_gui_tree_destroy(DvzGuiTree* tree);

/**
 * Create a retained typed table.
 *
 * @param widget_id widget id
 * @param column_count column count
 * @param columns columns
 * @param flags flags
 * @return a caller-owned table, or NULL on failure
 */
DVZ_EXPORT DvzGuiTable* dvz_gui_table(
    const char* widget_id, uint32_t column_count, const DvzGuiTableColumnDesc* columns,
    uint32_t flags);

/**
 * Replace table row keys and clear column values.
 *
 * @param table table
 * @param row_count row count
 * @param keys keys
 * @param flags flags
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_rows(
    DvzGuiTable* table, uint32_t row_count, const uint64_t* keys, uint32_t flags);

/**
 * Copy a complete UTF-8 text column atomically.
 *
 * @param table table
 * @param column_id column id
 * @param row_count row count
 * @param values values
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_column_text(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count, const char* const* values);

/**
 * Copy a complete signed integer column atomically.
 *
 * @param table table
 * @param column_id column id
 * @param row_count row count
 * @param values values
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_column_int64(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count, const int64_t* values);

/**
 * Copy a complete double column atomically.
 *
 * @param table table
 * @param column_id column id
 * @param row_count row count
 * @param values values
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_column_double(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count, const double* values);

/**
 * Copy a complete boolean column atomically.
 *
 * @param table table
 * @param column_id column id
 * @param row_count row count
 * @param values values
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_column_bool(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count, const bool* values);

/**
 * Copy a complete color column atomically.
 *
 * @param table table
 * @param column_id column id
 * @param row_count row count
 * @param values values
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_column_color(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count, const DvzColor* values);

/**
 * Replace copied keyed table row styles atomically.
 *
 * @param table table
 * @param style_count style count
 * @param styles styles
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_table_set_styles(DvzGuiTable* table, uint32_t style_count, const DvzGuiDataStyle* styles);

/**
 * Replace table selection by stable row key.
 *
 * @param table table
 * @param key_count key count
 * @param keys keys
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_table_set_selection(DvzGuiTable* table, uint32_t key_count, const uint64_t* keys);

/**
 * Copy selected table keys up to capacity.
 *
 * @param table table
 * @param capacity capacity
 * @param keys keys
 * @return the full selected count
 */
DVZ_EXPORT uint32_t
dvz_gui_table_get_selection(const DvzGuiTable* table, uint32_t capacity, uint64_t* keys);

/**
 * Set the strict table visibility mask.
 *
 * @param table table
 * @param row_count row count
 * @param visible visible
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_table_set_visible(DvzGuiTable* table, uint32_t row_count, const bool* visible);

/**
 * Set the external table match mask.
 *
 * @param table table
 * @param row_count row count
 * @param matches matches
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_table_set_matches(DvzGuiTable* table, uint32_t row_count, const bool* matches);

/**
 * Copy a UTF-8 native filter into the table.
 *
 * @param table table
 * @param filter filter
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_set_filter(DvzGuiTable* table, const char* filter);

/**
 * Copy the table filter up to capacity.
 *
 * @param table table
 * @param capacity capacity
 * @param filter filter
 * @return the required byte count including the terminator
 */
DVZ_EXPORT uint32_t
dvz_gui_table_get_filter(const DvzGuiTable* table, uint32_t capacity, char* filter);

/**
 * Return the active stable column ID and direction.
 *
 * @param table table
 * @param column_id column id
 * @param direction direction
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult
dvz_gui_table_get_sort(const DvzGuiTable* table, uint32_t* column_id, int32_t* direction);

/**
 * Draw a table and drain interaction events.
 *
 * @param gui gui
 * @param table table
 * @param events events
 * @param capacity capacity
 * @param written written
 * @param dropped dropped
 * @return DVZ_OK on success, DVZ_ERROR on failure
 */
DVZ_EXPORT DvzResult dvz_gui_table_draw(
    DvzGui* gui, DvzGuiTable* table, DvzGuiDataEvent* events, uint32_t capacity, uint32_t* written,
    uint32_t* dropped);

/**
 * Destroy a retained table.
 *
 * @param table table
 */
DVZ_EXPORT void dvz_gui_table_destroy(DvzGuiTable* table);



EXTERN_C_OFF
