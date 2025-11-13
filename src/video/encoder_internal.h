/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video encoder internals                                                                      */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "encoder.h"

bool dvz_video_encoder_open_mp4_stream(DvzVideoEncoder* enc);
void dvz_video_encoder_close_mp4(DvzVideoEncoder* enc);
int dvz_video_encoder_mux_post(DvzVideoEncoder* enc);
void dvz_video_encoder_mux_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint32_t duration);
void dvz_video_encoder_record_sample(
    DvzVideoEncoder* enc, uint64_t offset, uint32_t size, uint32_t duration);
