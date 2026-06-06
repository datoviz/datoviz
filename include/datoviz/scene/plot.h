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


EXTERN_C_OFF
