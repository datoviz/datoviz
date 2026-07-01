/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene axis internals                                                                         */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define AXIS_EPS 1e-12
#define AXIS_TEXT_TICK_SIZE 11.0f
#define AXIS_TEXT_LABEL_SIZE 13.0f
#define AXIS_TEXT_TICK_GAP 6.0f
#define AXIS_TEXT_LABEL_GAP 16.0f
#define AXIS_TEXT_Y_LABEL_ANGLE +1.57079632679f



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzAxisTickPolicy _axis_default_tick_policy(void);

DvzAxisStyle _axis_default_style(void);

bool _axis_dim_supported(DvzDim dim);

void _axis_mark_dirty(DvzAxis* axis);

DvzAxis* _panel_axis_slot(DvzPanel* panel, DvzDim dim);

void _axis_init(DvzAxis* axis, DvzPanel* panel, DvzDim dim);

void _axis_plot_interval(const DvzAxis* axis, float* out_min, float* out_max);

float _axis_data_to_visual(
    double value, double min, double max, float visual_min, float visual_max);

float _axis_inverse_panzoom_coord(
    const float extent[4], uint32_t lo_idx, uint32_t hi_idx, float value);

float _axis_forward_panzoom_coord(
    const float extent[4], uint32_t lo_idx, uint32_t hi_idx, float value);

float _axis_data_to_source_visual(const DvzAxis* axis, double value);

float _axis_panzoom_scale(const float extent[4], DvzDim dim);

bool _axis_visible_domain(const DvzAxis* axis, double* out_min, double* out_max);

bool _axis_visible_sorted_interval(const DvzAxis* axis, double* out_min, double* out_max);

void _axis_compute_ticks(DvzAxis* axis);

void _axis_hide_text(DvzAxis* axis);

void _axis_update_text(
    DvzAxis* axis, const DvzPanelFrameResolved* snapshot, float x0, float x1, float y0, float y1,
    double visible_min, double visible_max);

void _axis_update_visual(DvzAxis* axis);
