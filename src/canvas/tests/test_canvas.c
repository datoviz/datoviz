/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas tests                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include "_assertions.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Validate the default canvas configuration.
 */
int test_canvas_defaults(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasConfig cfg = dvz_canvas_default_config();
    AT(cfg.window == NULL);
    AT(cfg.device == NULL);
    AT(cfg.color_format == VK_FORMAT_B8G8R8A8_UNORM);
    AT(!cfg.enable_video_sink);
    AT(cfg.timing_history == DVZ_CANVAS_DEFAULT_TIMING_HISTORY);
    return 0;
}



/**
 * Ensure the frame pool rotates across entries.
 */
int test_canvas_frame_pool(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasFramePool pool = {0};
    dvz_canvas_frame_pool_init(&pool, 3);
    DvzStreamFrame* a = dvz_canvas_frame_pool_current(&pool);
    DvzStreamFrame* b = dvz_canvas_frame_pool_rotate(&pool);
    DvzStreamFrame* c = dvz_canvas_frame_pool_rotate(&pool);
    AT(a != NULL);
    AT(b != NULL);
    AT(c != NULL);
    AT(a != b);
    AT(b != c);
    dvz_canvas_frame_pool_release(&pool);
    return 0;
}



/**
 * Check that timing samples wrap around the configured capacity.
 */
int test_canvas_timings(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    (void)item;
    DvzCanvasTimingState timings = {0};
    dvz_canvas_timings_init(&timings, 4);
    for (uint64_t i = 0; i < 6; ++i)
    {
        dvz_canvas_timings_record(&timings, i, 100.0 + (double)i);
    }
    size_t count = 0;
    const DvzFrameTiming* samples = dvz_canvas_timings_view(&timings, &count);
    AT(samples != NULL);
    AT(count == 4);
    dvz_canvas_timings_release(&timings);
    return 0;
}



/**
 * Register the canvas tests.
 */
int test_canvas(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "canvas";
    TEST_SIMPLE(test_canvas_defaults);
    TEST_SIMPLE(test_canvas_frame_pool);
    TEST_SIMPLE(test_canvas_timings);
    return 0;
}
