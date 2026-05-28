/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan lowering internals                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "frame_plan/frame_plan.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void _scene_emit_visual_uploads(
    DvzFigure* figure, DvzFramePlan* plan, DvzDiagnosticReport* report);

bool _scene_emit_panel_render(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id);

bool _scene_emit_panel_render_ex(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id,
    DvzDiagnosticReport* report);
