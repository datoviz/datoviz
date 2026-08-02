/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "presentation_policy.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Functions                                                                                   */
/*************************************************************************************************/

/**
 * Return the default requested presentation mode for app-owned windows.
 *
 * @return FIFO-latest when supported by the compile-time Vulkan headers, otherwise FIFO
 */
VkPresentModeKHR _dvz_app_present_mode_default(void)
{
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    return VK_PRESENT_MODE_FIFO_LATEST_READY_KHR;
#else
    return VK_PRESENT_MODE_FIFO_KHR;
#endif
}



/**
 * Parse the optional presentation-mode environment override.
 *
 * @param[out] present_mode parsed presentation mode
 * @return true when an override was present and valid
 */
bool _dvz_app_present_mode_env(VkPresentModeKHR* present_mode)
{
    if (present_mode == NULL)
        return false;
    const char* value = getenv("DVZ_PRESENT_MODE");
    if (value == NULL || value[0] == '\0')
        return false;
    if (strcmp(value, "immediate") == 0)
        *present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    else if (strcmp(value, "mailbox") == 0)
        *present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    else if (strcmp(value, "fifo") == 0)
        *present_mode = VK_PRESENT_MODE_FIFO_KHR;
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    else if (strcmp(value, "fifo-latest") == 0)
        *present_mode = VK_PRESENT_MODE_FIFO_LATEST_READY_KHR;
#endif
    else
        return false;
    return true;
}



/**
 * Parse the optional app FPS-cap environment override.
 *
 * @param[out] fps_cap parsed positive FPS cap
 * @return true when an override was present and valid
 */
bool _dvz_app_fps_cap_env(double* fps_cap)
{
    if (fps_cap == NULL)
        return false;
    const char* value = getenv("DVZ_FPS_CAP");
    if (value == NULL || value[0] == '\0')
        return false;
    char* end = NULL;
    double parsed = strtod(value, &end);
    if (end == value || *end != '\0' || parsed <= 0)
        return false;
    *fps_cap = parsed;
    return true;
}



/**
 * Parse the optional maximum-frames-in-flight environment override.
 *
 * @param[out] frame_slot_count parsed Canvas frame-slot request
 * @return true when an override was present and valid
 */
bool _dvz_app_frame_slot_count_env(uint32_t* frame_slot_count)
{
    if (frame_slot_count == NULL)
        return false;
    const char* value = getenv("DVZ_MAX_FRAMES_IN_FLIGHT");
    if (value == NULL || value[0] == '\0')
        return false;
    if (strcmp(value, "auto") == 0)
    {
        *frame_slot_count = DVZ_CANVAS_FRAME_SLOT_COUNT_AUTOMATIC;
        return true;
    }
    if (value[0] == '-' || value[0] == '+')
        return false;
    errno = 0;
    char* end = NULL;
    unsigned long parsed = strtoul(value, &end, 10);
    if (
        errno == ERANGE || end == value || *end != '\0' || parsed == 0 ||
        parsed >= DVZ_CANVAS_FRAME_SLOT_COUNT_AUTOMATIC)
        return false;
    *frame_slot_count = (uint32_t)parsed;
    return true;
}



/**
 * Resolve the effective continuous-frame cap for one view.
 *
 * @param app_fps_cap explicit app FPS cap, or zero
 * @param present_mode resolved presentation mode
 * @param refresh_rate_hz window refresh rate, or zero when unavailable
 * @return positive FPS cap, or zero for unlimited/backpressure pacing
 */
double _dvz_app_view_effective_fps_cap(
    double app_fps_cap, VkPresentModeKHR present_mode, uint32_t refresh_rate_hz)
{
    if (app_fps_cap > 0)
        return app_fps_cap;
#if defined(VK_KHR_present_mode_fifo_latest_ready)
    if (present_mode == VK_PRESENT_MODE_FIFO_LATEST_READY_KHR && refresh_rate_hz > 0)
        return (double)refresh_rate_hz;
#else
    (void)present_mode;
    (void)refresh_rate_hz;
#endif
    return 0;
}



/**
 * Return whether a continuous view prevents the scheduler from waiting for another view.
 *
 * @param continuous whether continuous rendering is active for the view
 * @param fps_cap effective FPS cap, or zero for backpressure pacing
 * @param next_frame_ns next paced frame deadline, or zero when immediately eligible
 * @return whether the scheduler must poll before considering any deadline wait
 */
bool _dvz_app_view_requires_scheduler_poll(
    bool continuous, double fps_cap, uint64_t next_frame_ns)
{
    return continuous && (fps_cap <= 0 || next_frame_ns == 0);
}
