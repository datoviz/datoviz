/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App terminal status internals                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_status.h"

#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a terminal status-line state.
 *
 * @param status the status state
 */
void _dvz_app_status_init(DvzAppStatus* status)
{
    ANN(status);
    dvz_memset(status, sizeof(DvzAppStatus), 0, sizeof(DvzAppStatus));
}



/**
 * Clear an open terminal status line before printing a multi-line block.
 *
 * @param status the status state
 */
void _dvz_app_status_break_line(DvzAppStatus* status)
{
    if (status == NULL || !status->line_open)
        return;
    dvz_fprintf(stderr, "\n");
    status->line_open = false;
}



/**
 * Finish an open terminal status line with a newline.
 *
 * @param status the status state
 */
void _dvz_app_status_finish(DvzAppStatus* status)
{
    _dvz_app_status_break_line(status);
}



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
    uint32_t semantic_count, bool changed)
{
    ANN(status);
    status->trace_valid = true;
    status->trace_changed = changed;
    status->trace_frame_index = frame_index;
    status->trace_command_count = command_count;
    status->trace_semantic_count = semantic_count;
}



/**
 * Update the FPS portion of the terminal status line.
 *
 * @param status the status state
 * @param fps frames per second
 * @param frames frames in the measurement window
 * @param elapsed_s measurement-window duration in seconds
 */
void _dvz_app_status_fps(
    DvzAppStatus* status, double fps, uint32_t frames, double elapsed_s)
{
    ANN(status);
    status->fps_valid = true;
    status->fps = fps;
    status->fps_frames = frames;
    status->fps_elapsed_s = elapsed_s;
}



/**
 * Format the current terminal status line.
 *
 * @param status the status state
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false when there is nothing to print or on truncation
 */
bool _dvz_app_status_line(const DvzAppStatus* status, char* out, uint32_t size)
{
    ANN(status);
    ANN(out);
    ASSERT(size > 0);

    int written = 0;
    if (status->trace_valid && status->fps_valid)
    {
        written = dvz_snprintf(
            out, size,
            "\r\x1b[2Kframe %08llu | %s | %u cmds | %u semantic | FPS %6.1f"
            " (%u frames in %.3f s)",
            (unsigned long long)status->trace_frame_index,
            status->trace_changed ? "changed" : "unchanged", status->trace_command_count,
            status->trace_semantic_count, status->fps, status->fps_frames,
            status->fps_elapsed_s);
    }
    else if (status->trace_valid)
    {
        written = dvz_snprintf(
            out, size, "\r\x1b[2Kframe %08llu | %s | %u cmds | %u semantic",
            (unsigned long long)status->trace_frame_index,
            status->trace_changed ? "changed" : "unchanged", status->trace_command_count,
            status->trace_semantic_count);
    }
    else if (status->fps_valid)
    {
        written = dvz_snprintf(
            out, size, "\r\x1b[2KFPS %6.1f (%u frames in %.3f s)", status->fps,
            status->fps_frames, status->fps_elapsed_s);
    }
    else
    {
        return false;
    }
    return written >= 0 && (uint32_t)written < size;
}



/**
 * Render the current terminal status line in place.
 *
 * @param status the status state
 */
void _dvz_app_status_render(DvzAppStatus* status)
{
    ANN(status);
    char line[192] = {0};
    if (!_dvz_app_status_line(status, line, sizeof(line)))
        return;
    dvz_fprintf(stderr, "%s", line);
    fflush(stderr);
    status->line_open = true;
}
