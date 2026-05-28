/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text annotation internals                                                              */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

void _text_style_color(const DvzTextStyle* style, uint8_t out_color[4]);

void _text_panel_pixel_to_clip(
    const DvzPanel* panel, float x, float y, float z, float out[3]);

bool _text_visual_prepare(
    DvzFigure* figure, DvzPanel* panel, const DvzPanelAttach* attach, DvzVisual* visual);

bool _scalebar_prepare_visual(DvzFigure* figure, DvzAnnotation* annotation);
