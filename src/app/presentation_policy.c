/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "presentation_policy.h"

#include "_assertions.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Constants                                                                                   */
/*************************************************************************************************/

#define DVZ_APP_FIFO_LATEST_UNKNOWN_REFRESH_FALLBACK_HZ 60.0



/*************************************************************************************************/
/*  Helpers                                                                                     */
/*************************************************************************************************/

static uint64_t _pacing_period_ns(const DvzAppPacingPolicy* policy)
{
    ANN(policy);
    if (policy->fps_cap <= 0)
        return 0;
    double period = 1000000000.0 / policy->fps_cap;
    if (period >= (double)UINT64_MAX)
        return UINT64_MAX;
    uint64_t period_ns = (uint64_t)period;
    return period_ns > 0 ? period_ns : 1;
}



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
 * Map a public app presentation mode to Vulkan.
 *
 * @param present_mode public app presentation mode
 * @return requested Vulkan presentation mode
 */
VkPresentModeKHR _dvz_app_present_mode_resolve(DvzAppPresentMode present_mode)
{
    switch (present_mode)
    {
    case DVZ_APP_PRESENT_MODE_AUTOMATIC:
        return _dvz_app_present_mode_default();
    case DVZ_APP_PRESENT_MODE_FIFO:
        return VK_PRESENT_MODE_FIFO_KHR;
    case DVZ_APP_PRESENT_MODE_FIFO_LATEST:
#if defined(VK_KHR_present_mode_fifo_latest_ready)
        return VK_PRESENT_MODE_FIFO_LATEST_READY_KHR;
#else
        return VK_PRESENT_MODE_FIFO_KHR;
#endif
    case DVZ_APP_PRESENT_MODE_MAILBOX:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case DVZ_APP_PRESENT_MODE_IMMEDIATE:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    default:
        return _dvz_app_present_mode_default();
    }
}



/**
 * Resolve an app presentation request, including the diagnostic environment override.
 *
 * @param present_mode public app presentation mode
 * @param prefer_latest_ready whether automatic mode should prefer FIFO-latest
 * @param[out] explicit_mode whether the resolved mode was explicitly requested
 * @param[out] invalid_env_value invalid nonempty environment override, or NULL
 * @return requested Vulkan presentation mode
 */
VkPresentModeKHR _dvz_app_present_mode_config(
    DvzAppPresentMode present_mode, bool prefer_latest_ready, bool* explicit_mode,
    const char** invalid_env_value)
{
    VkPresentModeKHR resolved = VK_PRESENT_MODE_FIFO_KHR;
    bool is_explicit = present_mode != DVZ_APP_PRESENT_MODE_AUTOMATIC;
    if (is_explicit)
        resolved = _dvz_app_present_mode_resolve(present_mode);
    else if (prefer_latest_ready)
        resolved = _dvz_app_present_mode_default();
    const char* value = getenv("DVZ_PRESENT_MODE");
    const bool env_override_applied = _dvz_app_present_mode_parse(value, &resolved);
    if (env_override_applied)
        is_explicit = true;
    if (explicit_mode != NULL)
        *explicit_mode = is_explicit;
    if (invalid_env_value != NULL)
        *invalid_env_value =
            value != NULL && value[0] != '\0' && !env_override_applied ? value : NULL;
    return resolved;
}



/**
 * Parse an optional presentation-mode override value.
 *
 * @param value override value, or NULL
 * @param[out] present_mode parsed presentation mode
 * @return true when an override was present and valid
 */
bool _dvz_app_present_mode_parse(const char* value, VkPresentModeKHR* present_mode)
{
    if (present_mode == NULL)
        return false;
    if (value == NULL || value[0] == '\0')
        return false;
    if (strcmp(value, "immediate") == 0)
        *present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    else if (strcmp(value, "mailbox") == 0)
        *present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    else if (strcmp(value, "fifo") == 0)
        *present_mode = VK_PRESENT_MODE_FIFO_KHR;
    else if (strcmp(value, "fifo-latest") == 0)
    {
#if defined(VK_KHR_present_mode_fifo_latest_ready)
        *present_mode = VK_PRESENT_MODE_FIFO_LATEST_READY_KHR;
#else
        *present_mode = VK_PRESENT_MODE_FIFO_KHR;
#endif
    }
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
 * Resolve native scheduler pacing without consulting the resolved Vulkan present mode.
 *
 * @param app_owned_native whether the app owns the native window and event loop
 * @param immediate_requested whether the caller explicitly requested immediate presentation
 * @param app_fps_cap explicit positive app FPS cap, or zero
 * @param refresh_rate_hz active monitor refresh rate, or zero when unavailable
 * @return resolved pacing policy
 */
DvzAppPacingPolicy _dvz_app_pacing_policy_resolve(
    bool app_owned_native, bool immediate_requested, double app_fps_cap, uint32_t refresh_rate_hz)
{
    if (!app_owned_native)
        return (DvzAppPacingPolicy){.mode = DVZ_APP_PACING_HOST_DRIVEN, .fps_cap = 0};
    if (app_fps_cap > 0)
        return (DvzAppPacingPolicy){.mode = DVZ_APP_PACING_FIXED, .fps_cap = app_fps_cap};
    if (immediate_requested)
        return (DvzAppPacingPolicy){.mode = DVZ_APP_PACING_UNBOUNDED, .fps_cap = 0};
    return (DvzAppPacingPolicy){
        .mode = DVZ_APP_PACING_REFRESH,
        .fps_cap = refresh_rate_hz > 0 ? (double)refresh_rate_hz
                                       : DVZ_APP_FIFO_LATEST_UNKNOWN_REFRESH_FALLBACK_HZ,
    };
}



/**
 * Return whether a pacing policy admits a frame at the current scheduler time.
 *
 * @param policy resolved pacing policy
 * @param next_frame_ns next deadline, or zero before the first submitted frame
 * @param now_ns current scheduler timestamp
 * @return whether a frame may be submitted now
 */
bool _dvz_app_pacing_policy_admits(
    const DvzAppPacingPolicy* policy, uint64_t next_frame_ns, uint64_t now_ns)
{
    ANN(policy);
    return policy->fps_cap <= 0 || next_frame_ns == 0 || now_ns >= next_frame_ns;
}



/**
 * Advance a pacing deadline after a successfully submitted frame.
 *
 * Keep an on-time deadline phase-stable. When the scheduler misses a deadline, resume one full
 * period after the completed frame instead of generating catch-up submissions.
 *
 * @param policy resolved pacing policy
 * @param next_frame_ns previous deadline, or zero before the first submitted frame
 * @param now_ns completion timestamp
 * @return next deadline, or zero for unbounded and host-driven policies
 */
uint64_t _dvz_app_pacing_policy_advance(
    const DvzAppPacingPolicy* policy, uint64_t next_frame_ns, uint64_t now_ns)
{
    ANN(policy);
    uint64_t period_ns = _pacing_period_ns(policy);
    if (period_ns == 0)
        return 0;

    if (next_frame_ns > 0 && UINT64_MAX - next_frame_ns >= period_ns)
    {
        uint64_t deadline = next_frame_ns + period_ns;
        if (deadline > now_ns)
            return deadline;
    }
    return UINT64_MAX - now_ns >= period_ns ? now_ns + period_ns : UINT64_MAX;
}



/**
 * Resolve the earliest scheduler admission deadline across pending view requests.
 *
 * Host-driven views do not participate in the native scheduler. A zero deadline means at least one
 * scheduler-owned view is immediately eligible.
 *
 * @param count request count
 * @param requests request array, or NULL when count is zero
 * @param now_ns current scheduler timestamp
 * @param[out] deadline_ns earliest future deadline, or zero when immediately eligible
 * @return whether at least one scheduler-owned view needs a frame
 */
bool _dvz_app_pacing_requests_deadline(
    uint32_t count, const DvzAppPacingRequest* requests, uint64_t now_ns,
    uint64_t* deadline_ns)
{
    if (deadline_ns == NULL || (count > 0 && requests == NULL))
        return false;
    *deadline_ns = 0;
    bool has_work = false;
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzAppPacingRequest* request = &requests[i];
        if (!request->needs_frame || request->policy.mode == DVZ_APP_PACING_HOST_DRIVEN)
            continue;
        has_work = true;
        if (_dvz_app_pacing_policy_admits(
                &request->policy, request->next_frame_ns, now_ns))
        {
            *deadline_ns = 0;
            return true;
        }
        if (*deadline_ns == 0 || request->next_frame_ns < *deadline_ns)
            *deadline_ns = request->next_frame_ns;
    }
    return has_work;
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
    if (present_mode == VK_PRESENT_MODE_FIFO_LATEST_READY_KHR)
    {
        if (refresh_rate_hz > 0)
            return (double)refresh_rate_hz;
        return DVZ_APP_FIFO_LATEST_UNKNOWN_REFRESH_FALLBACK_HZ;
    }
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
