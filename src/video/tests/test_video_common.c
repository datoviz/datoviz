/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video test helpers                                                                           */
/*************************************************************************************************/

#include "test_video_common.h"

#include <stdio.h>

VkClearColorValue dvz_test_video_clear_color(uint32_t frame_idx, uint32_t total_frames)
{
    VkClearColorValue clr = {.float32 = {DVZ_TEST_VIDEO_CLEAR_R / 255.0f,
                                         DVZ_TEST_VIDEO_CLEAR_G / 255.0f,
                                         DVZ_TEST_VIDEO_CLEAR_B / 255.0f,
                                         1.0f}};
    if (total_frames == 0)
    {
        return clr;
    }

    float u = (float)(frame_idx % total_frames) / (float)total_frames;
    float h = u * 6.0f;
    uint32_t sector = (uint32_t)h;
    if (sector >= 6)
    {
        sector = 5;
    }
    float f = h - (float)sector;

    float r = 0.0f, g = 0.0f, b = 0.0f;
    switch (sector)
    {
    case 0:
        r = 1.0f;
        g = f;
        b = 0.0f;
        break;
    case 1:
        r = 1.0f - f;
        g = 1.0f;
        b = 0.0f;
        break;
    case 2:
        r = 0.0f;
        g = 1.0f;
        b = f;
        break;
    case 3:
        r = 0.0f;
        g = 1.0f - f;
        b = 1.0f;
        break;
    case 4:
        r = f;
        g = 0.0f;
        b = 1.0f;
        break;
    default:
        r = 1.0f;
        g = 0.0f;
        b = 1.0f - f;
        break;
    }

    clr.float32[0] = r;
    clr.float32[1] = g;
    clr.float32[2] = b;
    clr.float32[3] = 1.0f;
    return clr;
}

void dvz_test_video_progress(int frame, int total)
{
    const int width = 40;
    float ratio = (total > 0) ? (float)frame / (float)total : 1.0f;
    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }
    int filled = (int)(ratio * width);
    printf("\r[");
    for (int i = 0; i < width; ++i)
    {
        putchar(i < filled ? '#' : ' ');
    }
    printf("] %d/%d", frame, total);
    fflush(stdout);
    if (frame >= total)
    {
        printf("\n");
    }
}
