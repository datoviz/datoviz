/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scientific plotting building blocks                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Guide lines and spans                                                                        */
/*************************************************************************************************/

/**
 * Return the default guide-line descriptor.
 *
 * @return default guide-line descriptor
 */
DVZ_EXPORT DvzGuideLineDesc dvz_guide_line_desc(void);


/**
 * Return the default guide-span descriptor.
 *
 * @return default guide-span descriptor
 */
DVZ_EXPORT DvzGuideSpanDesc dvz_guide_span_desc(void);


/**
 * Return the default bars descriptor.
 *
 * @return default bars descriptor
 */
DVZ_EXPORT DvzBarsDesc dvz_bars_desc(void);


/**
 * Return the default band/ribbon descriptor.
 *
 * @return default band descriptor
 */
DVZ_EXPORT DvzBandDesc dvz_band_desc(void);


/**
 * Create a retained guide line attached to one panel.
 *
 * @param panel the panel
 * @param desc guide-line descriptor
 * @return the guide line, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGuideLine* dvz_guide_line(DvzPanel* panel, const DvzGuideLineDesc* desc);


/**
 * Create a horizontal guide line at one Y data coordinate.
 *
 * @param panel the panel
 * @param y Y data coordinate
 * @param desc optional guide-line descriptor; NULL uses defaults
 * @return the guide line, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGuideLine* dvz_hline(DvzPanel* panel, double y, const DvzGuideLineDesc* desc);


/**
 * Create a vertical guide line at one X data coordinate.
 *
 * @param panel the panel
 * @param x X data coordinate
 * @param desc optional guide-line descriptor; NULL uses defaults
 * @return the guide line, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGuideLine* dvz_vline(DvzPanel* panel, double x, const DvzGuideLineDesc* desc);


/**
 * Return a borrowed role visual from a guide line.
 *
 * Only `DVZ_PLOT_ROLE_LINE` is valid for guide lines.
 *
 * @param guide the guide line
 * @param role visual role
 * @return borrowed visual, or NULL when unavailable
 */
DVZ_EXPORT DvzVisual* dvz_guide_line_visual(DvzGuideLine* guide, DvzPlotRole role);


/**
 * Update a guide line data-coordinate value.
 *
 * @param guide the guide line
 * @param value new X or Y value, depending on orientation
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_guide_line_set_value(DvzGuideLine* guide, double value);


/**
 * Create a retained guide span attached to one panel.
 *
 * @param panel the panel
 * @param desc guide-span descriptor
 * @return the guide span, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGuideSpan* dvz_guide_span(DvzPanel* panel, const DvzGuideSpanDesc* desc);


/**
 * Create a horizontal guide span between two Y data coordinates.
 *
 * @param panel the panel
 * @param y0 first Y data coordinate
 * @param y1 second Y data coordinate
 * @param desc optional guide-span descriptor; NULL uses defaults
 * @return the guide span, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGuideSpan*
dvz_hspan(DvzPanel* panel, double y0, double y1, const DvzGuideSpanDesc* desc);


/**
 * Create a vertical guide span between two X data coordinates.
 *
 * @param panel the panel
 * @param x0 first X data coordinate
 * @param x1 second X data coordinate
 * @param desc optional guide-span descriptor; NULL uses defaults
 * @return the guide span, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzGuideSpan*
dvz_vspan(DvzPanel* panel, double x0, double x1, const DvzGuideSpanDesc* desc);


/**
 * Return a borrowed role visual from a guide span.
 *
 * Valid roles are `DVZ_PLOT_ROLE_FILL` and `DVZ_PLOT_ROLE_OUTLINE`.
 *
 * @param span the guide span
 * @param role visual role
 * @return borrowed visual, or NULL when unavailable
 */
DVZ_EXPORT DvzVisual* dvz_guide_span_visual(DvzGuideSpan* span, DvzPlotRole role);


/**
 * Update a guide span data-coordinate range.
 *
 * @param span the guide span
 * @param min_value first range endpoint
 * @param max_value second range endpoint
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int
dvz_guide_span_set_range(DvzGuideSpan* span, double min_value, double max_value);


/**
 * Create a retained explicit-interval bar series attached to one panel.
 *
 * @param panel the panel
 * @param desc optional bars descriptor; NULL uses defaults
 * @return the bars object, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzBars* dvz_bars(DvzPanel* panel, const DvzBarsDesc* desc);


/**
 * Set explicit bar intervals and values.
 *
 * For vertical bars, starts/ends are X interval edges and values are Y values from the baseline.
 * For horizontal bars, starts/ends are Y interval edges and values are X values from the baseline.
 * Arrays are copied into scene-owned storage.
 *
 * @param bars the bars object
 * @param count number of bars
 * @param starts interval start values
 * @param ends interval end values
 * @param values bar terminal values
 * @return 0 on success, -1 on validation/allocation error
 */
DVZ_EXPORT int dvz_bars_set_intervals(
    DvzBars* bars, uint32_t count, const double* starts, const double* ends,
    const double* values);


/**
 * Update bar style and rendering options while preserving interval data.
 *
 * The descriptor is copied. Enabling an outline after construction creates the outline role visual.
 *
 * @param bars the bars object
 * @param desc bars descriptor
 * @return 0 on success, -1 on validation/allocation error
 */
DVZ_EXPORT int dvz_bars_set_style(DvzBars* bars, const DvzBarsDesc* desc);


/**
 * Return a borrowed role visual from a bars object.
 *
 * Valid roles are `DVZ_PLOT_ROLE_FILL` and `DVZ_PLOT_ROLE_OUTLINE`.
 *
 * @param bars the bars object
 * @param role visual role
 * @return borrowed visual, or NULL when unavailable
 */
DVZ_EXPORT DvzVisual* dvz_bars_visual(DvzBars* bars, DvzPlotRole role);


/**
 * Create a retained band/ribbon attached to one panel.
 *
 * @param panel the panel
 * @param desc optional band descriptor; NULL uses defaults
 * @return the band object, or NULL on validation/allocation error
 */
DVZ_EXPORT DvzBand* dvz_band(DvzPanel* panel, const DvzBandDesc* desc);


/**
 * Set lower/upper band bounds.
 *
 * Arrays are copied into scene-owned storage. NaN values split the generated fill and paths into
 * gaps.
 *
 * @param band the band object
 * @param count number of samples
 * @param x X coordinates
 * @param lower lower Y coordinates
 * @param upper upper Y coordinates
 * @return 0 on success, -1 on validation/allocation error
 */
DVZ_EXPORT int dvz_band_set_bounds(
    DvzBand* band, uint32_t count, const double* x, const double* lower, const double* upper);


/**
 * Set an explicit center line for the band.
 *
 * If absent and the descriptor enables the center line, the line is derived from the bounds as
 * `0.5 * (lower + upper)`. Arrays are copied into scene-owned storage. NaN values split the line.
 *
 * @param band the band object
 * @param count number of samples
 * @param x X coordinates
 * @param y Y coordinates
 * @return 0 on success, -1 on validation/allocation error
 */
DVZ_EXPORT int
dvz_band_set_center(DvzBand* band, uint32_t count, const double* x, const double* y);


/**
 * Update band style and rendering options while preserving bounds and center data.
 *
 * The descriptor is copied. Enabling center or bound paths after construction creates the
 * corresponding role visual.
 *
 * @param band the band object
 * @param desc band descriptor
 * @return 0 on success, -1 on validation/allocation error
 */
DVZ_EXPORT int dvz_band_set_style(DvzBand* band, const DvzBandDesc* desc);


/**
 * Return a borrowed role visual from a band object.
 *
 * Valid roles are `DVZ_PLOT_ROLE_FILL`, `DVZ_PLOT_ROLE_LINE`, and `DVZ_PLOT_ROLE_BOUNDS`.
 *
 * @param band the band object
 * @param role visual role
 * @return borrowed visual, or NULL when unavailable
 */
DVZ_EXPORT DvzVisual* dvz_band_visual(DvzBand* band, DvzPlotRole role);


EXTERN_C_OFF
