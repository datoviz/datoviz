/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/canvas.h"
#include "datoviz/common/macros.h"

#define DVZ_CANVAS_PRESENTATION_SLOT_COUNT 3

EXTERN_C_ON

/** Completed source handles may be retained while this slot is pinned for display or host use. */
typedef struct DvzCanvasPresentationFrame
{
    DvzCanvasLiveImageFrame frame;
    uint32_t slot;
    uint64_t serial;
} DvzCanvasPresentationFrame;

int _dvz_canvas_presentation_configure(DvzCanvas* canvas, bool enabled);
int _dvz_canvas_presentation_try_admit(DvzCanvas* canvas);
VkCommandBuffer _dvz_canvas_presentation_admitted_command(DvzCanvas* canvas);
VkCommandBuffer _dvz_canvas_presentation_slot_command(DvzCanvas* canvas, uint32_t slot);
int _dvz_canvas_presentation_latest_completed(
    DvzCanvas* canvas, DvzCanvasPresentationFrame* frame);
int _dvz_canvas_presentation_pin_display(DvzCanvas* canvas, uint32_t slot);
int _dvz_canvas_presentation_mark_host_use(
    DvzCanvas* canvas, uint32_t slot, DvzCanvas* host, uint64_t value);
int _dvz_canvas_presentation_release_host(DvzCanvas* canvas, DvzCanvas* host);
int _dvz_canvas_presentation_pending(DvzCanvas* canvas);
void _dvz_canvas_presentation_reject_submission(DvzCanvas* canvas);

EXTERN_C_OFF
