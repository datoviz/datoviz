/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App presentation policy                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/canvas.h"



/*************************************************************************************************/
/*  Functions                                                                                   */
/*************************************************************************************************/

VkPresentModeKHR _dvz_app_present_mode_default(void);

bool _dvz_app_present_mode_env(VkPresentModeKHR* present_mode);

bool _dvz_app_fps_cap_env(double* fps_cap);

bool _dvz_app_frame_slot_count_env(uint32_t* frame_slot_count);

double _dvz_app_view_effective_fps_cap(
    double app_fps_cap, VkPresentModeKHR present_mode, uint32_t refresh_rate_hz);

bool _dvz_app_view_requires_scheduler_poll(
    bool continuous, double fps_cap, uint64_t next_frame_ns);
