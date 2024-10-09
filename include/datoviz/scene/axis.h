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
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAxis DvzAxis;
typedef struct DvzTickSpec DvzTickSpec;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzFont DvzFont;
typedef struct DvzMVP DvzMVP;
typedef struct DvzPanel DvzPanel;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTickSpec
{
    vec3 p0, p1, vector;

    uint32_t tick_count;
    uint32_t glyph_count;

    double dmin, dmax;
    double* values;

    char* glyphs;
    uint32_t* index;
    uint32_t* length;
};



struct DvzAxis
{
    int flags;

    DvzVisual* segment;
    DvzVisual* glyph;
    DvzAtlas* atlas;
    DvzFont* font;

    vec3 p0_ref, p1_ref;
    DvzTickSpec tick_spec;

    // Color.
    DvzColor color_glyph;
    DvzColor color_lim;
    DvzColor color_grid;
    DvzColor color_major;
    DvzColor color_minor;

    // Tick width.
    vec4 tick_width;  // lim, grid, major, minor
    vec4 tick_length; // lim, grid, major, minor

    // Glyphs
    // uint32_t tick_count; // = glyph->group_count
    // uint32_t glyph_count; // = glyph->item_count
    // uint32_t* group_size; // = glyph->group_sizes
    vec2 anchor;
    vec2 offset;

    void* user_data;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/


EXTERN_C_ON

/*************************************************************************************************/
/*  General functions                                                                            */
/*************************************************************************************************/

/**
 *
 */
DvzAxis* dvz_axis(DvzBatch* batch, int flags);



/**
 *
 */
DvzVisual* dvz_axis_segment(DvzAxis* axis);



/**
 *
 */
DvzVisual* dvz_axis_glyph(DvzAxis* axis);


/**
 *
 */
int dvz_axis_direction(DvzAxis* axis, DvzMVP* mvp);
// returns 0 for horizontal, 1 for vertical. depends on the intersection or not
// of two projected boxes with maximal label length



/**
 *
 */
void dvz_axis_panel(DvzAxis* axis, DvzPanel* panel);



/**
 *
 */
void dvz_axis_update(DvzAxis* axis);



/**
 *
 */
void dvz_axis_destroy(DvzAxis* axis);



/*************************************************************************************************/
/*  Visual properties                                                                            */
/*************************************************************************************************/

/**
 *
 */
void dvz_axis_size(DvzAxis* axis, float font_size);


// voir old datoviz params: number of minor ticks, disable grid


/**
 *
 */
void dvz_axis_width(DvzAxis* axis, float lim, float grid, float major, float minor);



/**
 *
 */
void dvz_axis_length(DvzAxis* axis, float lim, float grid, float major, float minor);



/**
 *
 */
void dvz_axis_color(
    DvzAxis* axis, DvzColor glyph, DvzColor lim, DvzColor grid, DvzColor major, DvzColor minor);



/**
 *
 */
void dvz_axis_anchor(DvzAxis* axis, vec2 anchor);



/**
 *
 */
void dvz_axis_offset(DvzAxis* axis, vec2 offset);



/*************************************************************************************************/
/*  MVP                                                                                          */
/*************************************************************************************************/

/**
 *
 */
void dvz_axis_mvp(DvzAxis* axis, DvzMVP* mvp, dvec2 range_data, vec2 range_ndc);
// compute dmin, dmax of the visible viewbox



/*************************************************************************************************/
/*  Ticks and glyphs                                                                             */
/*************************************************************************************************/

/**
 *
 */
DvzTickSpec dvz_tick_spec(
    vec3 p0, vec3 p1, vec3 vector,                                 // positions in NDC
    double dmin, double dmax, uint32_t tick_count, double* values, // tick positions and values
    uint32_t glyph_count, char* glyphs, uint32_t* index, uint32_t* length); // tick labels



/**
 *
 */
void dvz_axis_ticks(DvzAxis* axis, DvzTickSpec* tick_spec);



EXTERN_C_OFF

#endif
