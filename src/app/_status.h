/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App terminal status internals                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzAppStatus DvzAppStatus;


struct DvzAppStatus
{
    bool line_open;

    bool trace_valid;
    bool trace_changed;
    uint64_t trace_frame_index;
    uint32_t trace_command_count;
    uint32_t trace_semantic_count;

    bool fps_valid;
    double fps;
    uint32_t fps_frames;
    double fps_elapsed_s;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a terminal status-line state.
 *
 * @param status the status state
 */
void _dvz_app_status_init(DvzAppStatus* status);


/**
 * Clear an open terminal status line before printing a multi-line block.
 *
 * @param status the status state
 */
void _dvz_app_status_break_line(DvzAppStatus* status);


/**
 * Finish an open terminal status line with a newline.
 *
 * @param status the status state
 */
void _dvz_app_status_finish(DvzAppStatus* status);


/**
 * Update the DRP2 trace portion of the terminal status line.
 *
 * @param status the status state
 * @param frame_index 0-based frame index
 * @param command_count emitted command count
 * @param semantic_count normalized semantic-line count
 * @param changed whether the stream changed semantically
 */
void _dvz_app_status_trace(
    DvzAppStatus* status, uint64_t frame_index, uint32_t command_count,
    uint32_t semantic_count, bool changed);


/**
 * Update the FPS portion of the terminal status line.
 *
 * @param status the status state
 * @param fps frames per second
 * @param frames frames in the measurement window
 * @param elapsed_s measurement-window duration in seconds
 */
void _dvz_app_status_fps(
    DvzAppStatus* status, double fps, uint32_t frames, double elapsed_s);


/**
 * Format the current terminal status line.
 *
 * @param status the status state
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false when there is nothing to print or on truncation
 */
bool _dvz_app_status_line(const DvzAppStatus* status, char* out, uint32_t size);


/**
 * Render the current terminal status line in place.
 *
 * @param status the status state
 */
void _dvz_app_status_render(DvzAppStatus* status);
