/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scale                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Scales and colormaps                                                                         */
/*************************************************************************************************/

/**
 * Return a scene-owned builtin unit ladder.
 *
 * @param scene the scene
 * @param builtin builtin ladder kind
 * @return the ladder, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzUnitLadder* dvz_unit_ladder_builtin(
    DvzScene* scene, DvzUnitLadderBuiltin builtin);


/**
 * Create a scene-owned custom unit ladder.
 *
 * @param scene the scene
 * @param canonical_unit canonical unit label
 * @return the ladder, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzUnitLadder* dvz_unit_ladder_create(
    DvzScene* scene, const char* canonical_unit);


/**
 * Add one display entry to a custom unit ladder.
 *
 * Duplicate factors or labels are rejected.
 *
 * @param ladder the ladder
 * @param factor display-unit factor in canonical units
 * @param label display unit label
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_unit_ladder_add(DvzUnitLadder* ladder, double factor, const char* label);


/**
 * Clear all entries from a custom unit ladder.
 *
 * Builtin ladders ignore this call.
 *
 * @param ladder the ladder
 */
DVZ_EXPORT void dvz_unit_ladder_clear(DvzUnitLadder* ladder);


/**
 * Create a scene-owned units object.
 *
 * @param scene the scene
 * @return the units object, or NULL on allocation error
 */
DVZ_EXPORT DvzUnits* dvz_units_create(DvzScene* scene);


/**
 * Create a scene-owned units object using a builtin ladder.
 *
 * @param scene the scene
 * @param builtin builtin ladder kind
 * @param data_to_canonical factor from data coordinates to canonical units
 * @return the units object, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzUnits* dvz_units_builtin(
    DvzScene* scene, DvzUnitLadderBuiltin builtin, double data_to_canonical);


/**
 * Set the factor from data coordinates to canonical units.
 *
 * @param units the units object
 * @param factor finite positive conversion factor
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_units_data_to_canonical(DvzUnits* units, double factor);


/**
 * Attach a ladder to a units object.
 *
 * @param units the units object
 * @param ladder the display ladder
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_units_ladder(DvzUnits* units, DvzUnitLadder* ladder);


/**
 * Set the unit display mode.
 *
 * @param units the units object
 * @param mode display mode
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_units_display_mode(DvzUnits* units, DvzUnitDisplayMode mode);


/**
 * Force a display label for fixed display mode.
 *
 * @param units the units object
 * @param label ladder entry label
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_units_fixed_label(DvzUnits* units, const char* label);


/**
 * Return a scene-owned builtin datetime format.
 *
 * @param scene the scene
 * @param builtin builtin datetime format kind
 * @return the format, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzDateTimeFormat* dvz_datetime_format_builtin(
    DvzScene* scene, DvzDateTimeBuiltin builtin);


/**
 * Create a scene-owned datetime format.
 *
 * @param scene the scene
 * @return the datetime format, or NULL on allocation error
 */
DVZ_EXPORT DvzDateTimeFormat* dvz_datetime_format_create(DvzScene* scene);


/**
 * Set the timezone used by a datetime format.
 *
 * The first v0.4 slice supports only UTC.
 *
 * @param format the datetime format
 * @param timezone timezone name
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_datetime_format_timezone(
    DvzDateTimeFormat* format, const char* timezone);


/**
 * Set the formatting rule for a calendar interval.
 *
 * @param format the datetime format
 * @param interval interval kind
 * @param strftime_format C strftime-compatible format
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_datetime_format_rule(
    DvzDateTimeFormat* format, DvzTimeInterval interval, const char* strftime_format);


/**
 * Return the default format descriptor.
 *
 * @return default format descriptor
 */
DVZ_EXPORT DvzFormatDesc dvz_format_desc(void);


/**
 * Return the default scale descriptor.
 *
 * @return default scale descriptor
 */
DVZ_EXPORT DvzScaleDesc dvz_scale_desc(void);


/**
 * Create a scene-owned scale object.
 *
 * @param scene the scene
 * @param desc the scale descriptor, or NULL for defaults
 * @return the scale
 */
DVZ_EXPORT DvzScale* dvz_scale(DvzScene* scene, const DvzScaleDesc* desc);


/**
 * Destroy a scale object.
 *
 * @param scale the scale
 */
DVZ_EXPORT void dvz_scale_destroy(DvzScale* scale);


/**
 * Set the semantic domain on a scale.
 *
 * @param scale the scale
 * @param min the domain minimum
 * @param max the domain maximum
 */
DVZ_EXPORT void dvz_scale_set_domain(DvzScale* scale, double min, double max);


/**
 * Set the current visible range on a scale.
 *
 * @param scale the scale
 * @param min the view-range minimum
 * @param max the view-range maximum
 */
DVZ_EXPORT void dvz_scale_set_view_range(DvzScale* scale, double min, double max);


/**
 * Bind a colormap to a scale.
 *
 * @param scale the scale
 * @param colormap the colormap
 */
DVZ_EXPORT void dvz_scale_set_colormap(DvzScale* scale, DvzColormap* colormap);


/**
 * Override shared formatting policy on a scale.
 *
 * @param scale the scale
 * @param format the format descriptor, or NULL to clear the override
 */
DVZ_EXPORT void dvz_scale_set_format(DvzScale* scale, const DvzFormatDesc* format);


/**
 * Replace retained categorical entries on a scale.
 *
 * Entries are valid only on categorical scales. Passing NULL or count=0 clears explicit entries
 * and restores palette-index fallback for categorical mapping.
 *
 * @param scale the scale
 * @param categories category entry array, or NULL to clear
 * @param count the number of category entries
 * @return true when the category table was accepted
 */
DVZ_EXPORT bool dvz_scale_set_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count);


/**
 * Update or append retained categorical entries on a scale.
 *
 * Entries are valid only on categorical scales. Existing entries with matching category ids are
 * replaced in place. New entries are appended. Duplicate ids in the input batch are rejected.
 *
 * @param scale the scale
 * @param categories category entry array
 * @param count the number of category entries
 * @return true when the category table was accepted
 */
DVZ_EXPORT bool dvz_scale_update_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count);


/**
 * Remove retained categorical entries from a scale.
 *
 * Entries are valid only on categorical scales. Missing ids are ignored.
 *
 * @param scale the scale
 * @param ids category ids to remove
 * @param count the number of ids
 * @return true when the category table was updated
 */
DVZ_EXPORT bool dvz_scale_remove_categories(
    DvzScale* scale, const DvzCategoryId* ids, uint32_t count);


/**
 * Create a scene-owned colormap object.
 *
 * @param scene the scene
 * @param desc the colormap descriptor, or NULL for defaults
 * @return the colormap
 */
DVZ_EXPORT DvzColormap* dvz_colormap(DvzScene* scene, const DvzColormapDesc* desc);


/**
 * Return the default colormap descriptor.
 *
 * @return default colormap descriptor
 */
DVZ_EXPORT DvzColormapDesc dvz_colormap_desc(void);


/**
 * Create a scene-owned built-in colormap object.
 *
 * @param scene the scene
 * @param builtin the built-in colormap selector
 * @return the colormap
 */
DVZ_EXPORT DvzColormap* dvz_colormap_builtin(
    DvzScene* scene, DvzBuiltinColormap builtin);


/**
 * Create a scene-owned custom LUT colormap.
 *
 * The color array is copied. Samples use linear interpolation between neighboring entries after
 * mapping the normalized scalar coordinate to [0, count - 1].
 *
 * @param scene the scene
 * @param label optional colormap label
 * @param colors RGBA8 lookup table
 * @param count number of colors in the lookup table
 * @return the colormap, or NULL on error
 */
DVZ_EXPORT DvzColormap* dvz_colormap_custom(
    DvzScene* scene, const char* label, const DvzColor* colors, uint32_t count);


/**
 * Sample a scene-owned colormap at a normalized coordinate.
 *
 * @param colormap the colormap, or NULL for grayscale fallback
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
DVZ_EXPORT bool dvz_colormap_sample(const DvzColormap* colormap, double t, DvzColor* out);


/**
 * Sample a built-in colormap at a normalized coordinate.
 *
 * @param builtin the built-in colormap selector
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
DVZ_EXPORT bool dvz_colormap_builtin_sample(
    DvzBuiltinColormap builtin, double t, DvzColor* out);


/**
 * Destroy a colormap object.
 *
 * @param colormap the colormap
 */
DVZ_EXPORT void dvz_colormap_destroy(DvzColormap* colormap);


/**
 * Set custom color stops on a colormap.
 *
 * @param colormap the colormap
 * @param stops the color stops
 * @param count the number of stops
 */
DVZ_EXPORT void dvz_colormap_set_stops(
    DvzColormap* colormap, const DvzColormapStop* stops, uint32_t count);


/**
 * Set the diverging center on a colormap.
 *
 * @param colormap the colormap
 * @param center the semantic center value
 */
DVZ_EXPORT void dvz_colormap_set_center(DvzColormap* colormap, double center);



/*************************************************************************************************/
/*  Colorbars                                                                                    */
/*************************************************************************************************/

/**
 * Return the default colorbar descriptor.
 *
 * @return default colorbar descriptor
 */
DVZ_EXPORT DvzColorbarDesc dvz_colorbar_desc(void);


/**
 * Create a panel-attached colorbar bound to a scale.
 *
 * @param panel the panel
 * @param scale the scale
 * @param desc the colorbar descriptor, or NULL for defaults
 * @return the colorbar
 */
DVZ_EXPORT DvzColorbar* dvz_colorbar(
    DvzPanel* panel, DvzScale* scale, const DvzColorbarDesc* desc);


/**
 * Destroy a colorbar.
 *
 * @param colorbar the colorbar
 */
DVZ_EXPORT void dvz_colorbar_destroy(DvzColorbar* colorbar);


/**
 * Override formatting policy on a colorbar.
 *
 * @param colorbar the colorbar
 * @param format the format descriptor, or NULL to clear the override
 */
DVZ_EXPORT void dvz_colorbar_set_format(
    DvzColorbar* colorbar, const DvzFormatDesc* format);


/**
 * Set the colorbar orientation.
 *
 * @param colorbar the colorbar
 * @param orientation the orientation
 */
DVZ_EXPORT void dvz_colorbar_set_orientation(
    DvzColorbar* colorbar, DvzColorbarOrientation orientation);


/**
 * Set the colorbar panel-edge anchor.
 *
 * Panel-left and panel-right anchors use vertical orientation. Panel-top and panel-bottom anchors
 * use horizontal orientation.
 *
 * @param colorbar the colorbar
 * @param anchor the panel-edge anchor
 * @return true when the anchor was accepted
 */
DVZ_EXPORT bool dvz_colorbar_set_anchor(DvzColorbar* colorbar, DvzSceneAnchor anchor);


/**
 * Update colorbar layout and placement parameters.
 *
 * The descriptor updates orientation, placement mode, anchor, reserve size, geometry gaps, explicit
 * detached placement, colorbar flags, and title when a non-NULL title is supplied. Attached
 * colorbar anchors must match the requested orientation.
 *
 * @param colorbar the colorbar
 * @param desc layout descriptor
 * @return true when the layout was accepted
 */
DVZ_EXPORT bool dvz_colorbar_set_layout(DvzColorbar* colorbar, const DvzColorbarDesc* desc);


/**
 * Set the colorbar title.
 *
 * @param colorbar the colorbar
 * @param title the title, or NULL to clear
 */
DVZ_EXPORT void dvz_colorbar_set_title(DvzColorbar* colorbar, const char* title);



/*************************************************************************************************/
/*  Legends                                                                                      */
/*************************************************************************************************/

/**
 * Return the default legend descriptor.
 *
 * @return default legend descriptor
 */
DVZ_EXPORT DvzLegendDesc dvz_legend_desc(void);


/**
 * Create a panel-attached legend bound to a categorical scale.
 *
 * @param panel the panel
 * @param scale the categorical scale
 * @param desc the legend descriptor, or NULL for defaults
 * @return the legend
 */
DVZ_EXPORT DvzLegend* dvz_legend(
    DvzPanel* panel, DvzScale* scale, const DvzLegendDesc* desc);


/**
 * Destroy a legend.
 *
 * @param legend the legend
 */
DVZ_EXPORT void dvz_legend_destroy(DvzLegend* legend);


/**
 * Update legend layout and placement parameters.
 *
 * @param legend the legend
 * @param desc layout descriptor
 * @return true when the layout was accepted
 */
DVZ_EXPORT bool dvz_legend_set_layout(DvzLegend* legend, const DvzLegendDesc* desc);


/**
 * Set the legend title.
 *
 * @param legend the legend
 * @param title the title, or NULL to clear
 */
DVZ_EXPORT void dvz_legend_set_title(DvzLegend* legend, const char* title);


/**
 * Highlight one categorical legend entry.
 *
 * This is presentation state only: the bound scale category color and label are unchanged.
 *
 * @param legend the legend
 * @param id category id to highlight
 * @return true when the highlight state was accepted
 */
DVZ_EXPORT bool dvz_legend_set_highlight(DvzLegend* legend, DvzCategoryId id);


/**
 * Clear all highlighted categorical legend entries.
 *
 * @param legend the legend
 * @return true when the highlight state was accepted
 */
DVZ_EXPORT bool dvz_legend_clear_highlight(DvzLegend* legend);


/**
 * Highlight multiple categorical legend entries.
 *
 * This is presentation state only: the bound scale category colors and labels are unchanged.
 *
 * @param legend the legend
 * @param ids category ids to highlight
 * @param count number of highlighted category ids
 * @return true when the highlight state was accepted
 */
DVZ_EXPORT bool
dvz_legend_set_highlights(DvzLegend* legend, const DvzCategoryId* ids, uint32_t count);



/*************************************************************************************************/
/*  Visual scale bindings                                                                        */
/*************************************************************************************************/

/**
 * Bind a scene-owned scale to a named visual slot.
 *
 * Visual scale slots are semantic attribute names. Point and pixel visuals accept the `"color"`
 * slot with a continuous scale when their `"color"` attribute format is
 * `DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32`. Image and volume visuals use `"color"` for continuous
 * sampled fields. Labels visuals and label-volume render modes use `"labels"` with a categorical
 * scale.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param scale the scale, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name, DvzScale* scale);


EXTERN_C_OFF
