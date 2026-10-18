/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#include "datoviz/canvas.h"
#include "datoviz/common/macros.h"

EXTERN_C_ON

typedef void (*DvzCanvasCommandsCompletedCallback)(VkCommandBuffer command_buffer, void* user_data);

uint32_t _dvz_canvas_frame_slot(const DvzCanvas* canvas);

void _dvz_canvas_set_commands_completed_callback(
    DvzCanvas* canvas, DvzCanvasCommandsCompletedCallback callback, void* user_data);

/** Owner-only notification after completion proof and before command-buffer reset or destruction.
 * Abandoned submissions additionally require earlier MAIN work to finish before retiring shared
 * runtime resources recorded against the unsent command buffer. Device-loss teardown is exempt.
 */
void _dvz_canvas_notify_commands_completed(DvzCanvas* canvas, VkCommandBuffer command_buffer);

EXTERN_C_OFF
