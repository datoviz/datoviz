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



/**
 * Open the MP4 muxer and writer so the encoder can stream MP4 output directly.
 *
 * @param enc encoder instance that owns the MP4 writer
 * @returns true on success or false when the muxer/writer initialization fails
 */
bool dvz_video_encoder_open_mp4_stream(DvzVideoEncoder* enc);



/**
 * Close the MP4 muxer, release writers, and drop the output file handle.
 *
 * @param enc encoder instance whose MP4 resources should be released
 */
void dvz_video_encoder_close_mp4(DvzVideoEncoder* enc);



/**
 * Flush tracked samples to disk after streaming into a temporary bitstream.
 *
 * @param enc encoder that accumulated mux samples while in post-processing mode
 * @returns 0 when muxing succeeds or -1 when MP4 creation fails
 */
int dvz_video_encoder_mux_post(DvzVideoEncoder* enc);



/**
 * Stream a single encoded sample directly into the MP4 writer.
 *
 * @param enc encoder responsible for writing
 * @param data encoded bytes to write
 * @param size size of the encoded buffer
 * @param duration sample duration in 90 kHz ticks
 */
void dvz_video_encoder_mux_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint32_t duration);



/**
 * Record a sample entry for later muxing when streaming is deferred.
 *
 * @param enc encoder that will later write this sample
 * @param offset byte offset in the temporary bitstream
 * @param size sample size in bytes
 * @param duration sample duration in 90 kHz ticks
 */
void dvz_video_encoder_record_sample(
    DvzVideoEncoder* enc, uint64_t offset, uint32_t size, uint32_t duration);
