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

int test_video_1(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    return 0;
}

#if !(defined(DVZ_HAS_NVENC) && DVZ_HAS_NVENC) || defined(_WIN32)
int test_video_nvenc(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
#if defined(_WIN32) && defined(DVZ_HAS_NVENC) && DVZ_HAS_NVENC
    tst_skip(suite, "NVENC Vulkan external HANDLE interop is not implemented on Windows");
#else
    tst_skip(suite, "NVENC backend disabled at build time");
#endif
    return 0;
}
#endif

#if !(defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
int test_video_kvazaar(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    tst_skip(suite, "kvazaar backend disabled at build time");
    return 0;
}
#endif



#define TST_VIDEO_CASE(test, resource_flags, isolation_mode, selection_flags)                     \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = (resource_flags);                                                   \
        _tst_desc.isolation = (isolation_mode);                                                   \
        _tst_desc.run_flags = (selection_flags);                                                  \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

int test_video(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "video";
    TST_MODULE(suite, tags);
    (void)tags;

    TST_CASE(test_video_1);

    TST_VIDEO_CASE(
        test_video_nvenc, TST_RES_GPU | TST_RES_VULKAN | TST_RES_VIDEO | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS, TST_RUN_CASE_ADAPTER_EXEMPT);
    TST_VIDEO_CASE(
        test_video_kvazaar, TST_RES_GPU | TST_RES_VULKAN | TST_RES_VIDEO | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS, TST_RUN_CASE_ADAPTER_SUPPORTED);
    TST_VIDEO_CASE(
        test_video_offline_headless_encode, TST_RES_VIDEO | TST_RES_FILESYSTEM,
        TST_ISOLATION_PROCESS, 0);

    return 0;
}
