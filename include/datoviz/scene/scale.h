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
 * Create a scene-owned colormap object.
 *
 * @param scene the scene
 * @param desc the colormap descriptor, or NULL for defaults
 * @return the colormap
 */
DVZ_EXPORT DvzColormap* dvz_colormap(DvzScene* scene, const DvzColormapDesc* desc);


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
 * Sample a scene-owned colormap at a normalized coordinate.
 *
 * @param colormap the colormap, or NULL for grayscale fallback
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
DVZ_EXPORT bool dvz_colormap_sample(const DvzColormap* colormap, double t, DvzColor out);


/**
 * Sample a built-in colormap at a normalized coordinate.
 *
 * @param builtin the built-in colormap selector
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
DVZ_EXPORT bool dvz_colormap_builtin_sample(
    DvzBuiltinColormap builtin, double t, DvzColor out);


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
 * detached placement, flags, and title when a non-NULL title is supplied. Attached colorbar anchors
 * must match the requested orientation.
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
/*  Visual scale bindings                                                                        */
/*************************************************************************************************/

/**
 * Bind a scene-owned scale to a named visual slot.
 *
 * First retained slice: image visuals accept the `"colormap"` slot. Other
 * visual families and slot names are rejected until their retained scale
 * wiring is implemented. Volume visuals also accept the `"colormap"` slot as retained transfer
 * function state for the volume renderer.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param scale the scale, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
DVZ_EXPORT int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name, DvzScale* scale);


EXTERN_C_OFF
