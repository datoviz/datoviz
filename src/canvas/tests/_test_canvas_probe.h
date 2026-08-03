/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas test probe shared declarations                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct CanvasRefreshProbeState
{
    bool awaiting_refresh;
    bool saw_update_since_refresh;
    uint32_t start_count;
    uint32_t update_count;
    uint32_t submit_count;
    uint32_t stale_submit_count;
    uint32_t wait_value_count;
    uint32_t wait_value_non_monotonic;
    int start_rc;
    int update_rc;
    uint64_t last_wait_value;
    VkExtent2D latest_extent;
    int latest_memory_fd;
    int latest_wait_semaphore_fd;
    bool latest_handles_dirty;
} CanvasRefreshProbeState;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

extern const DvzStreamSinkBackend CANVAS_REFRESH_PROBE_BACKEND;
