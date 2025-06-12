/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Axis                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_AXIS
#define DVZ_HEADER_AXIS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz_enums.h"
#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAxis DvzAxis;
typedef struct DvzAxisSpec DvzAxisSpec;
typedef struct DvzLayout DvzLayout;

// Forward declarations.
typedef struct DvzVisual DvzVisual;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzPanel DvzPanel;
typedef struct DvzRef DvzRef;
typedef struct DvzTicks DvzTicks;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxisSpec
{
    // Tick width.
    vec4 tick_width;  // lim, grid, major, minor
    vec4 tick_length; // lim, grid, major, minor

    vec2 tick_dir;
    vec2 anchor;
    vec2 offset;
    float pos;
    float extra;

    // Color.
    DvzColor color_glyph;
    DvzColor color_lim;
    DvzColor color_grid;
    DvzColor color_major;
    DvzColor color_minor;
};



struct DvzLayout
{
    vec2 pos;
    vec2 size;
    vec2 offset;
    DvzAlign align[2];
};



struct DvzAxis
{
    DvzAxisSpec spec;
    DvzLayout factor_layout;
    DvzLayout label_layout;

    DvzBatch* batch;
    // Axis visuals.
    DvzVisual* glyph;   // tick labels
    DvzVisual* segment; // major and minor ticks
    DvzVisual* factor;  // exponent and offset with factored formats
    DvzVisual* label;   // axis label
    DvzVisual* spine;   // spine

    DvzTicks* ticks;
    DvzPanel* panel; // HACK: avoid putting an axis several times to a given panel

    DvzDim dim;
    int flags;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzAxis* dvz_axis(DvzBatch* batch, DvzAtlasFont* af, DvzDim dim, int flags);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_size(DvzAxis* axis, double range_size, double glyph_size);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_glyph(DvzAxis* axis, uint32_t tick_count, char** labels, vec3* positions);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_segment(DvzAxis* axis, uint32_t tick_count, vec3* positions);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_factor(DvzAxis* axis, int32_t exponent, double offset);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void
dvz_axis_label(DvzAxis* axis, char* text, float margin, DvzOrientation orientation);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT bool dvz_axis_update(DvzAxis* axis, DvzRef* ref, double dmin, double dmax);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT bool dvz_axis_on_panzoom(DvzAxis* axis, DvzPanzoom* pz, DvzRef* ref, bool force);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_panel(DvzAxis* axis, DvzPanel* panel);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_destroy(DvzAxis* axis);



/*************************************************************************************************/
/*  Axis spec                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_axis_extra(DvzAxis* axis, float extra);



/**
 *
 */
DVZ_EXPORT void dvz_axis_pos(DvzAxis* axis, float pos); // x0 for y axis, y0 for x axis



/**
 *
 */
DVZ_EXPORT void dvz_axis_width(DvzAxis* axis, float lim, float grid, float major, float minor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_length(DvzAxis* axis, float lim, float grid, float major, float minor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_color(
    DvzAxis* axis, DvzColor glyph, DvzColor lim, DvzColor grid, DvzColor major, DvzColor minor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_anchor(DvzAxis* axis, vec2 anchor);



/**
 *
 */
DVZ_EXPORT void dvz_axis_offset(DvzAxis* axis, vec2 offset);



/**
 *
 */
DVZ_EXPORT void dvz_axis_dir(DvzAxis* axis, vec2 tick_dir);



/**
 *
 */
DVZ_EXPORT void
dvz_axis_factor_layout(DvzAxis* axis, DvzAlign align, float xoffset, float yoffset);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axis_label_layout(DvzAxis* axis, DvzAlign align, float xoffset, float yoffset);



/*************************************************************************************************/
/*  Axis helpers                                                                                 */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_axis_horizontal(DvzAxis* axis, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_axis_vertical(DvzAxis* axis, int flags);



#endif
