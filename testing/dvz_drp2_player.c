/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Minimal DRP2 recording player                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_compat.h"
#include "_time_utils.h"
#include "datoviz/drp2.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Print command-line usage.
 *
 * @param argv0 executable path
 */
static void _usage(const char* argv0)
{
    dvz_fprintf(
        stderr,
        "usage: %s [--fast] recording.dvzr\n"
        "\n"
        "Replays a DRP2 recording through the semantic runtime.\n"
        "Default playback is paced by recorded presentation timestamps.\n"
        "Use --fast to execute frames without timestamp waits.\n",
        argv0 != NULL ? argv0 : "dvz_drp2_player");
}



/**
 * Sleep until the recorded presentation timestamp unless fast mode is enabled.
 *
 * @param clock playback clock
 * @param frame recorded frame metadata
 * @param fast whether playback is unpaced
 */
static void _pace_frame(DvzClock* clock, const DvzDrp2RecordedFrame* frame, bool fast)
{
    if (clock == NULL || frame == NULL || fast || frame->t_present <= 0)
        return;

    double now = dvz_clock_get(clock);
    double delay = frame->t_present - now;
    if (delay <= 0)
        return;

    double delay_us = delay * 1000000.0;
    int sleep_us = delay_us > (double)INT_MAX ? INT_MAX : (int)delay_us;
    dvz_sleep_us(sleep_us);
}



/**
 * Play a recording frame by frame through the semantic DRP2 runtime.
 *
 * @param recording loaded recording
 * @param fast whether playback is unpaced
 * @return process exit code
 */
static int _play_recording(const DvzDrp2Recording* recording, bool fast)
{
    if (recording == NULL)
        return 1;

    uint32_t raw_fallback_count = dvz_drp2_recording_raw_fallback_count(recording);
    if (raw_fallback_count > 0)
    {
        dvz_fprintf(
            stderr,
            "dvz_drp2_player: warning: recording uses %" PRIu32
            " ABI-local raw fallback command(s); portability is not guaranteed\n",
            raw_fallback_count);
        const DvzDrp2RawFallback* fallback = dvz_drp2_recording_raw_fallback(recording, 0);
        if (fallback != NULL)
        {
            dvz_fprintf(
                stderr,
                "dvz_drp2_player: first raw fallback at command %" PRIu32
                " type %d\n",
                fallback->command_index, (int)fallback->command_type);
        }
    }

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    if (runtime == NULL)
    {
        dvz_fprintf(stderr, "dvz_drp2_player: failed to create semantic runtime\n");
        return 1;
    }

    uint32_t frame_count = dvz_drp2_recording_frame_count(recording);
    DvzClock clock = dvz_clock();
    for (uint32_t i = 0; i < frame_count; i++)
    {
        const DvzDrp2RecordedFrame* frame = dvz_drp2_recording_frame(recording, i);
        if (frame == NULL)
        {
            dvz_fprintf(stderr, "dvz_drp2_player: missing frame %" PRIu32 "\n", i);
            dvz_drp2_runtime_destroy(runtime);
            return 1;
        }

        _pace_frame(&clock, frame, fast);
        DvzDrp2ValidationResult result =
            dvz_drp2_recording_execute_frame(recording, runtime, i);
        if (!result.ok)
        {
            dvz_fprintf(
                stderr,
                "dvz_drp2_player: frame %" PRIu32 " failed at command %" PRIu32
                " with code %d\n",
                i, result.command_index, (int)result.code);
            dvz_drp2_runtime_destroy(runtime);
            return 1;
        }
    }

    dvz_fprintf(
        stdout, "dvz_drp2_player: replayed %" PRIu32 " frame(s) (%s)\n", frame_count,
        fast ? "fast" : "paced");
    dvz_drp2_runtime_destroy(runtime);
    return 0;
#else
    (void)fast;
    dvz_fprintf(stderr, "dvz_drp2_player: DRP2 vklite runtime is not available\n");
    return 1;
#endif
}



/**
 * Program entry point.
 *
 * @param argc argument count
 * @param argv argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    bool fast = false;
    const char* path = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--fast") == 0)
        {
            fast = true;
            continue;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            _usage(argv[0]);
            return 0;
        }
        if (path == NULL)
        {
            path = argv[i];
            continue;
        }
        _usage(argv[0]);
        return 1;
    }

    if (path == NULL)
    {
        _usage(argv[0]);
        return 1;
    }

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    if (recording == NULL)
    {
        dvz_fprintf(stderr, "dvz_drp2_player: failed to open %s\n", path);
        return 1;
    }

    int rc = _play_recording(recording, fast);
    dvz_drp2_recording_close(recording);
    return rc;
}
