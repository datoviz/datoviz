/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene annotation preparation internals                                                       */
/*************************************************************************************************/

#pragma once

#include "_scene.h"

void _scene_prepare_axis_visuals(DvzFigure* figure);

void _scene_prepare_colorbar_visuals(DvzFigure* figure, DvzDiagnosticReport* report);

void _scene_prepare_legend_visuals(DvzFigure* figure, DvzDiagnosticReport* report);

void _scene_prepare_text_visuals(DvzFigure* figure);

void _scene_prepare_pinned_readout_cards(DvzFigure* figure);

void _scene_prepare_selection_cards(DvzFigure* figure);

void _scene_prepare_overlay_cards(DvzFigure* figure);
