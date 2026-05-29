/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene frame trace internals                                                                  */
/*************************************************************************************************/

#pragma once

#include <stdint.h>

#include "_scene.h"

typedef enum
{
    DVZ_FRAME_PLAN_TRACE_NONE = 0,
    DVZ_FRAME_PLAN_TRACE_NORMAL,
    DVZ_FRAME_PLAN_TRACE_FULL,
} DvzFramePlanTraceMode;

void _scene_figure_id(const DvzFigure* figure, char* out, uint32_t size);

void _scene_figure_frame_plan_trace_reset(DvzFigure* figure);

DvzFramePlanTraceMode _scene_frame_plan_trace_mode_from_env(const char* value);

uint32_t _scene_frame_plan_trace_flags_from_env(const char* value);

bool _scene_frame_plan_trace_should_print(
    DvzFramePlanTraceMode mode, const DvzFigure* figure, const char* graph);

void _scene_frame_plan_trace(DvzFigure* figure, const DvzFramePlan* plan);
