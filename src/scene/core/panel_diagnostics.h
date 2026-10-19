/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

enum
{
    DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW = 1u << 0,
    DVZ_PANEL_DIAGNOSTIC_CHANGE_LAYOUT = 1u << 1,
    DVZ_PANEL_DIAGNOSTIC_CHANGE_MEMBERSHIP = 1u << 2,
    DVZ_PANEL_DIAGNOSTIC_CHANGE_VISUAL = 1u << 3,
    DVZ_PANEL_DIAGNOSTIC_CHANGE_ADORNMENT = 1u << 4,
    DVZ_PANEL_DIAGNOSTIC_CHANGE_LIGHTS = 1u << 5,
    DVZ_PANEL_DIAGNOSTIC_CHANGE_UNKNOWN_FRAME = 1u << 6,
};

bool _scene_panel_diagnostics_env_enabled(void);

void _scene_panel_diagnostics_begin(DvzFigure* figure);

void _scene_panel_diagnostics_record(
    DvzFigure* figure, uint32_t panel_index, const DvzFramePlan* plan, uint32_t first_node,
    uint64_t elapsed_ns, bool planning_succeeded);

void _scene_panel_diagnostics_finish(DvzFigure* figure, bool stream_emitted);
