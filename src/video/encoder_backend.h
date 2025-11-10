/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video encoder backends                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "encoder.h"



/*************************************************************************************************/
/*  Backend interface                                                                            */
/*************************************************************************************************/

struct DvzVideoBackend
{
    const char* name;
    bool (*probe)(const DvzVideoEncoderConfig* cfg);
    int (*init)(DvzVideoEncoder* enc);
    int (*start)(DvzVideoEncoder* enc);
    int (*submit)(DvzVideoEncoder* enc, uint64_t timeline_value);
    int (*stop)(DvzVideoEncoder* enc);
    void (*destroy)(DvzVideoEncoder* enc);
};

const DvzVideoBackend* dvz_video_backend_pick(const DvzVideoEncoderConfig* cfg);
