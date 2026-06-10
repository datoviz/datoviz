/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example visual style helpers                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum ExampleStyleColorRole
{
    EXAMPLE_STYLE_COLOR_FRAME_BG,
    EXAMPLE_STYLE_COLOR_PANEL_BG,
    EXAMPLE_STYLE_COLOR_GRID,
    EXAMPLE_STYLE_COLOR_TEXT,
    EXAMPLE_STYLE_COLOR_MINOR_TICK,
    EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
    EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
    EXAMPLE_STYLE_COLOR_WARNING,
    EXAMPLE_STYLE_COLOR_ERROR,
} ExampleStyleColorRole;


typedef enum ExampleStyleTextRole
{
    EXAMPLE_STYLE_TEXT_TITLE,
    EXAMPLE_STYLE_TEXT_PANEL_LABEL,
    EXAMPLE_STYLE_TEXT_DATA_LABEL,
    EXAMPLE_STYLE_TEXT_ANNOTATION,
    EXAMPLE_STYLE_TEXT_SMALL,
} ExampleStyleTextRole;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ExampleAxisStyleOptions
{
    float tick_size_px;
    float label_size_px;
    float tick_gap_px;
    float x_label_gap_px;
    float y_label_gap_px;
    uint8_t minor_tick_alpha;
    uint8_t grid_alpha;
} ExampleAxisStyleOptions;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzColor example_graphite_cyan_color(ExampleStyleColorRole role);

DvzColormap* example_graphite_cyan_colormap(DvzScene* scene);

DvzScale* example_graphite_cyan_color_scale(DvzScene* scene, double min, double max);

DvzTextStyle example_graphite_cyan_text_style(ExampleStyleTextRole role);

void example_graphite_cyan_panel_background(float out_rgba[4]);

void example_graphite_cyan_set_panel_background(DvzPanel* panel);

ExampleAxisStyleOptions example_graphite_cyan_axis_options(void);

DvzAxisStyle example_graphite_cyan_axis_style(
    bool vertical, const ExampleAxisStyleOptions* options);

bool example_graphite_cyan_apply_axis_style(
    DvzAxis* axis, bool vertical, const ExampleAxisStyleOptions* options);
