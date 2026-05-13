/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App trace helpers                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_trace.h"

#include <string.h>

#include "_assertions.h"
#include "_compat.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse the `DVZ_DRP2_TRACE` environment variable into an internal trace mode.
 *
 * @param value environment variable value, or NULL
 * @return the parsed trace mode
 */
DvzAppTraceMode _dvz_app_trace_mode_from_env(const char* value)
{
    if (value == NULL)
        return DVZ_APP_TRACE_NONE;
    if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
        strcmp(value, "FALSE") == 0 || strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0)
    {
        return DVZ_APP_TRACE_NONE;
    }
    if (strcmp(value, "full") == 0 || strcmp(value, "FULL") == 0)
        return DVZ_APP_TRACE_FULL;
    return DVZ_APP_TRACE_NORMAL;
}



/**
 * Compute the terminal-behavior plan for one trace update.
 *
 * @param mode active trace mode
 * @param status_line_open whether an in-place unchanged line is currently open
 * @param changed whether the newly emitted stream differs from the previous one
 * @return the trace update plan
 */
DvzAppTracePlan
_dvz_app_trace_plan(DvzAppTraceMode mode, bool status_line_open, bool changed)
{
    DvzAppTracePlan plan = {0};
    if (mode == DVZ_APP_TRACE_NONE)
        return plan;

    if (mode == DVZ_APP_TRACE_FULL)
    {
        plan.event_kind = DVZ_APP_TRACE_EVENT_CHANGED;
        plan.prepend_newline = status_line_open;
        return plan;
    }

    if (changed)
    {
        plan.event_kind = DVZ_APP_TRACE_EVENT_CHANGED;
        plan.prepend_newline = status_line_open;
        return plan;
    }

    plan.event_kind = DVZ_APP_TRACE_EVENT_UNCHANGED;
    plan.rewrite_in_place = true;
    plan.status_line_open_after = true;
    return plan;
}



/**
 * Format one in-place unchanged status line.
 *
 * @param frame_index 0-based frame index
 * @param command_count emitted command count
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false on error or truncation
 */
bool _dvz_app_trace_status_line(
    uint64_t frame_index, uint32_t command_count, char* out, uint32_t size)
{
    ANN(out);
    ASSERT(size > 0);
    int written = dvz_snprintf(
        out, size, "\r\x1b[2Kframe %08llu | unchanged | %u cmds",
        (unsigned long long)frame_index, command_count);
    return written >= 0 && (uint32_t)written < size;
}



/**
 * Format the stable serializer name used for duplicate detection.
 *
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false on error or truncation
 */
bool _dvz_app_trace_fingerprint_name(char* out, uint32_t size)
{
    ANN(out);
    ASSERT(size > 0);
    int written = dvz_snprintf(out, size, "live_frame");
    return written >= 0 && (uint32_t)written < size;
}
