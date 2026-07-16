/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  SVG tiger prepared-model helpers                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/geom.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SvgTigerPath
{
    uint32_t point_offset;
    uint32_t point_count;
    bool closed;
    bool has_fill;
    bool has_stroke;
    DvzColor fill;
    DvzColor stroke;
    float stroke_width_px;
    uint32_t paint_order;
} SvgTigerPath;


typedef struct SvgTigerData
{
    SvgTigerPath* paths;
    dvec2* points;
    uint32_t path_count;
    uint32_t point_count;
    double width;
    double height;
    double bounds[4];
} SvgTigerData;


typedef struct SvgTigerStrokeData
{
    vec3* positions;
    DvzColor* colors;
    float* widths;
    uint32_t* lengths;
    uint32_t point_count;
    uint32_t subpath_count;
} SvgTigerStrokeData;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool svg_tiger_load(const char* path, SvgTigerData* out);

void svg_tiger_destroy(SvgTigerData* data);

DvzGeometry* svg_tiger_fill_geometry(const SvgTigerData* data);

bool svg_tiger_stroke_data(const SvgTigerData* data, SvgTigerStrokeData* out);

void svg_tiger_stroke_destroy(SvgTigerStrokeData* data);
