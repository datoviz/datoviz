/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 linear recordings                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2RecordingInfo DvzDrp2RecordingInfo;

struct DvzDrp2RecordingInfo
{
    uint32_t width;
    uint32_t height;
    double duration_s;
    double t_present;
    const char* backend_hint;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Write a raw linear DRP2 recording directory.
 *
 * This first recording slice preserves command order and stores payload bytes as blobs. It is an
 * ABI-local development format, not yet the portable DVZR command encoding.
 *
 * @param path recording directory path
 * @param stream the command stream to record
 * @param info optional recording metadata
 * @return whether the recording was written
 */
DVZ_EXPORT bool dvz_drp2_recording_write_stream(
    const char* path, const DvzDrp2CommandStream* stream, const DvzDrp2RecordingInfo* info);


/**
 * Read a raw linear DRP2 recording directory.
 *
 * @param path recording directory path
 * @return a reconstructed command stream, or NULL on error
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_drp2_recording_read_stream(const char* path);


EXTERN_C_OFF
