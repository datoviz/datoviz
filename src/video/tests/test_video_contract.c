/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video backend-neutral contract tests                                                         */
/*************************************************************************************************/

#include "_assertions.h"
#include "../encoder.h"
#include "../encoder_backend.h"
#include "datoviz/video.h"
#include "test_video.h"
#include "testing.h"


static int _video_failing_output_submit(DvzVideoEncoder* enc, uint64_t wait_value)
{
    (void)wait_value;
    enc->output_failed = true;
    return 0;
}



static int _video_stop_error(DvzVideoEncoder* enc)
{
    (void)enc;
    return -7;
}



/**
 * Validate offline/headless capture contract wiring independently from backend-heavy encode loops.
 *
 * @param suite test suite pointer
 * @param tstitem test item pointer
 * @return 0 on success
 */
int test_video_offline_headless_encode(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVideoEncoderConfig cfg = dvz_video_encoder_config();
    AT(cfg.width == 0);
    AT(cfg.height == 0);
    AT(cfg.fps == 0);

    DvzVideoSinkConfig sink_cfg = dvz_video_sink_config();
    AT(sink_cfg.encoder.width == 0);
    AT(sink_cfg.encoder.height == 0);
    AT(sink_cfg.encoder.fps == 0);

#if !(defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ) && !(defined(DVZ_HAS_NVENC) && DVZ_HAS_NVENC)
    tst_skip(suite, "no video backend enabled");
#endif

    return 0;
}



int test_video_output_errors_propagate(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVideoBackend backend = {
        .name = "failing-output",
        .submit = _video_failing_output_submit,
        .stop = _video_stop_error,
    };
    DvzVideoEncoder enc = {
        .started = true,
        .backend = &backend,
        .memory_fd = -1,
        .wait_semaphore_fd = -1,
    };
    AT(dvz_video_encoder_submit(&enc, 1) != 0);
    AT(enc.frame_idx == 0);
    AT(dvz_video_encoder_submit(&enc, 2) != 0);
    AT(dvz_video_encoder_stop(&enc) == -1);

#if defined(__linux__)
    FILE* full = fopen("/dev/full", "wb");
    ANN(full);
    DvzVideoEncoder flush = {
        .fp = full,
        .mux = DVZ_VIDEO_MUX_MP4_POST,
        .memory_fd = -1,
        .wait_semaphore_fd = -1,
    };
    const uint8_t data[4] = {1, 2, 3, 4};
    AT(fwrite(data, 1, sizeof(data), full) == sizeof(data));
    AT(dvz_video_encoder_stop(&flush) != 0);
    fclose(full);
#endif
    return 0;
}
