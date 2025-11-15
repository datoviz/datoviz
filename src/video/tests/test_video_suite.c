/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video test suite                                                                             */
/*************************************************************************************************/

#include "test_video.h"

#include "_log.h"
#include "testing.h"

int test_video_1(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    return 0;
}

#if !(defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA)
int test_video_nvenc(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    log_warn("NVENC backend disabled at build time; skipping nvenc video test");
    return 0;
}
#endif

#if !(defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
int test_video_kvazaar(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    log_warn("kvazaar backend disabled at build time; skipping CPU fallback video test");
    return 0;
}
#endif

int test_video(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "video";
    (void)tags;

    TEST_SIMPLE(test_video_1);

#if defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA
    TEST_SIMPLE(test_video_nvenc);
#endif

#if defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ
    TEST_SIMPLE(test_video_kvazaar);
#endif

    return 0;
}
