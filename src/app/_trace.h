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

#include "datoviz/drp2/types.h"



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
typedef struct DvzAppTraceLine DvzAppTraceLine;
typedef struct DvzAppTraceSnapshot DvzAppTraceSnapshot;


struct DvzAppTraceLine
{
    char text[192];
};


struct DvzAppTracePlan
{
    DvzAppTraceEventKind event_kind;
    bool prepend_newline;
    bool rewrite_in_place;
    bool status_line_open_after;
};


struct DvzAppTraceSnapshot
{
    uint32_t count;
    uint32_t capacity;
    DvzAppTraceLine* lines;
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


/**
 * Compute a stable semantic fingerprint for one emitted DRP2 stream.
 *
 * The fingerprint ignores volatile per-frame mechanics such as borrowed command-buffer handles,
 * submission ids, payload bytes, and borrowed data pointers. It keeps command order and stable
 * fields such as resource ids, offsets, sizes, draw counts, render targets, and pipeline ids.
 * Transient encoder, pass, command-buffer, and submission ids are deliberately ignored.
 *
 * @param stream the emitted command stream
 * @param out destination fingerprint
 * @return true on success, false on error
 */
bool _dvz_app_trace_fingerprint(const DvzDrp2CommandStream* stream, uint64_t* out);


/**
 * Initialize an empty normalized trace snapshot.
 *
 * @param snapshot the snapshot to initialize
 */
void _dvz_app_trace_snapshot_init(DvzAppTraceSnapshot* snapshot);


/**
 * Destroy a normalized trace snapshot.
 *
 * @param snapshot the snapshot to destroy
 */
void _dvz_app_trace_snapshot_destroy(DvzAppTraceSnapshot* snapshot);


/**
 * Build a compact normalized trace snapshot from one emitted DRP2 stream.
 *
 * @param snapshot destination snapshot
 * @param stream source DRP2 stream
 * @return true on success, false on allocation or stream error
 */
bool _dvz_app_trace_snapshot_build(
    DvzAppTraceSnapshot* snapshot, const DvzDrp2CommandStream* stream);


/**
 * Return whether two normalized trace snapshots contain identical lines in identical order.
 *
 * @param a first snapshot
 * @param b second snapshot
 * @return true if both snapshots are equal
 */
bool _dvz_app_trace_snapshot_equal(
    const DvzAppTraceSnapshot* a, const DvzAppTraceSnapshot* b);


/**
 * Count the occurrences of one normalized line in a snapshot.
 *
 * @param snapshot the snapshot
 * @param text line text
 * @return occurrence count
 */
uint32_t _dvz_app_trace_snapshot_line_count(
    const DvzAppTraceSnapshot* snapshot, const char* text);
