/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video test helpers                                                                           */
/*************************************************************************************************/

#pragma once

#include <stdint.h>

#include <volk.h>

#define DVZ_TEST_VIDEO_WIDTH   1920
#define DVZ_TEST_VIDEO_HEIGHT  1080
#define DVZ_TEST_VIDEO_FPS     10
#define DVZ_TEST_VIDEO_SECONDS 3
#define DVZ_TEST_VIDEO_FRAMES  (DVZ_TEST_VIDEO_FPS * DVZ_TEST_VIDEO_SECONDS)

#define DVZ_TEST_VIDEO_CLEAR_R 0
#define DVZ_TEST_VIDEO_CLEAR_G 128
#define DVZ_TEST_VIDEO_CLEAR_B 255
#define DVZ_TEST_VIDEO_CLEAR_A 255

VkClearColorValue dvz_test_video_clear_color(uint32_t frame_idx, uint32_t total_frames);
void dvz_test_video_progress(int frame, int total);
