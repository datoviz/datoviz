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
/*  Types                                                                                       */
/*************************************************************************************************/

/** Internal scheduler pacing intent, independent of resolved Vulkan present mode. */
typedef enum DvzAppPacingMode
{
    DVZ_APP_PACING_REFRESH,
    DVZ_APP_PACING_FIXED,
    DVZ_APP_PACING_UNBOUNDED,
    DVZ_APP_PACING_HOST_DRIVEN,
} DvzAppPacingMode;


/** Resolved pacing policy for one app view. */
typedef struct DvzAppPacingPolicy
{
    DvzAppPacingMode mode;
    double fps_cap;
} DvzAppPacingPolicy;


/** Pending frame admission state for one scheduler view. */
typedef struct DvzAppPacingRequest
{
    bool needs_frame;
    DvzAppPacingPolicy policy;
    uint64_t next_frame_ns;
} DvzAppPacingRequest;



/*************************************************************************************************/
/*  Functions                                                                                   */
/*************************************************************************************************/

VkPresentModeKHR _dvz_app_present_mode_default(void);

bool _dvz_app_present_mode_env(VkPresentModeKHR* present_mode);

bool _dvz_app_fps_cap_env(double* fps_cap);

bool _dvz_app_frame_slot_count_env(uint32_t* frame_slot_count);

/** Resolve native scheduler pacing from requested policy inputs. */
DvzAppPacingPolicy _dvz_app_pacing_policy_resolve(
    bool app_owned_native, bool immediate_requested, double app_fps_cap, uint32_t refresh_rate_hz);

/** Return whether a policy admits a frame at the current scheduler time. */
bool _dvz_app_pacing_policy_admits(
    const DvzAppPacingPolicy* policy, uint64_t next_frame_ns, uint64_t now_ns);

/** Advance a paced deadline after a successfully submitted frame. */
uint64_t _dvz_app_pacing_policy_advance(
    const DvzAppPacingPolicy* policy, uint64_t next_frame_ns, uint64_t now_ns);

/** Resolve the earliest admission deadline across pending view requests. */
bool _dvz_app_pacing_requests_deadline(
    uint32_t count, const DvzAppPacingRequest* requests, uint64_t now_ns,
    uint64_t* deadline_ns);

double _dvz_app_view_effective_fps_cap(
    double app_fps_cap, VkPresentModeKHR present_mode, uint32_t refresh_rate_hz);

bool _dvz_app_view_requires_scheduler_poll(
    bool continuous, double fps_cap, uint64_t next_frame_ns);
