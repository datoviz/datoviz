/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App trace internals                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_APP_TRACE_NONE,
    DVZ_APP_TRACE_NORMAL,
    DVZ_APP_TRACE_FULL,
} DvzAppTraceMode;


typedef enum
{
    DVZ_APP_TRACE_EVENT_NONE,
    DVZ_APP_TRACE_EVENT_CHANGED,
    DVZ_APP_TRACE_EVENT_UNCHANGED,
} DvzAppTraceEventKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzAppTracePlan DvzAppTracePlan;

struct DvzAppTracePlan
{
    DvzAppTraceEventKind event_kind;
    bool prepend_newline;
    bool rewrite_in_place;
    bool status_line_open_after;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse the `DVZ_DRP2_TRACE` environment variable into an internal trace mode.
 *
 * @param value environment variable value, or NULL
 * @return the parsed trace mode
 */
DvzAppTraceMode _dvz_app_trace_mode_from_env(const char* value);


/**
 * Compute the terminal-behavior plan for one trace update.
 *
 * @param mode active trace mode
 * @param status_line_open whether an in-place unchanged line is currently open
 * @param changed whether the newly emitted stream differs from the previous one
 * @return the trace update plan
 */
DvzAppTracePlan
_dvz_app_trace_plan(DvzAppTraceMode mode, bool status_line_open, bool changed);


/**
 * Format one in-place unchanged status line.
 *
 * The line begins with carriage-return plus ANSI erase-line so repeated updates reuse one
 * terminal row instead of stacking.
 *
 * @param frame_index 0-based frame index
 * @param command_count emitted command count
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false on error or truncation
 */
bool _dvz_app_trace_status_line(
    uint64_t frame_index, uint32_t command_count, char* out, uint32_t size);


/**
 * Format the stable serializer name used for duplicate detection.
 *
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false on error or truncation
 */
bool _dvz_app_trace_fingerprint_name(char* out, uint32_t size);
