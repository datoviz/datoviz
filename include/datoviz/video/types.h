/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video public value types                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Video capture strategy used by app capture and video sinks.
typedef enum DvzVideoCaptureMode
{
    DVZ_VIDEO_CAPTURE_AUTO = 0,           // choose the best available path at runtime
    DVZ_VIDEO_CAPTURE_EXTERNAL = 1,       // use external memory/semaphore interop
    DVZ_VIDEO_CAPTURE_CPU_READBACK = 2,   // read back RGBA to CPU before encoding
} DvzVideoCaptureMode;
