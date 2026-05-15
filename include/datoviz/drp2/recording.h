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
typedef struct DvzDrp2Recorder DvzDrp2Recorder;

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
 * Open a linear DRP2 recorder.
 *
 * @param path recording directory path
 * @param info optional recording metadata
 * @return the recorder, or NULL on error
 */
DVZ_EXPORT DvzDrp2Recorder*
dvz_drp2_recorder_open(const char* path, const DvzDrp2RecordingInfo* info);


/**
 * Append one timestamped command stream to a linear DRP2 recorder.
 *
 * @param recorder the recorder
 * @param t_present presentation timestamp for this stream
 * @param stream the command stream to append
 * @return whether the stream was appended
 */
DVZ_EXPORT bool dvz_drp2_recorder_write_stream(
    DvzDrp2Recorder* recorder, double t_present, const DvzDrp2CommandStream* stream);


/**
 * Close a linear DRP2 recorder.
 *
 * @param recorder the recorder
 * @return whether the recorder was closed cleanly
 */
DVZ_EXPORT bool dvz_drp2_recorder_close(DvzDrp2Recorder* recorder);


/**
 * Write a linear DRP2 recording directory.
 *
 * Supported MVP commands are stored as portable JSON records with payload bytes in blobs.
 * Unsupported commands fall back to ABI-local raw command blobs for development replay.
 *
 * @param path recording directory path
 * @param stream the command stream to record
 * @param info optional recording metadata
 * @return whether the recording was written
 */
DVZ_EXPORT bool dvz_drp2_recording_write_stream(
    const char* path, const DvzDrp2CommandStream* stream, const DvzDrp2RecordingInfo* info);


/**
 * Read a linear DRP2 recording directory.
 *
 * @param path recording directory path
 * @return a reconstructed command stream, or NULL on error
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_drp2_recording_read_stream(const char* path);


EXTERN_C_OFF
