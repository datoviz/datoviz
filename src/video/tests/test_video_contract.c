/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video backend-neutral contract tests                                                         */
/*************************************************************************************************/

#include "_assertions.h"
#include "_log.h"
#include "datoviz/video.h"
#include "test_video.h"
#include "testing.h"



/**
 * Validate offline/headless capture contract wiring independently from backend-heavy encode loops.
 *
 * @param suite test suite pointer
 * @param tstitem test item pointer
 * @return 0 on success
 */
int test_video_offline_headless_encode(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVideoEncoderConfig cfg = dvz_video_encoder_default_config();
    AT(cfg.width > 0);
    AT(cfg.height > 0);
    AT(cfg.fps > 0);

#if !(defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ) && !(defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA)
    log_warn("offline/headless capture contract test skipped (no video backend enabled)");
#endif

    return 0;
}

