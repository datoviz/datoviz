/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Window host                                                                                  */
/*************************************************************************************************/

#pragma once


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/window.h"
#include "datoviz/window/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_WINDOW_BACKEND_INIT_CAP  4
#define DVZ_WINDOW_INSTANCE_INIT_CAP 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWindowBackendSlot DvzWindowBackendSlot;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWindowBackendSlot
{
    DvzWindowBackend backend;
    bool available;
    bool probed;
};



struct DvzWindow
{
    DvzWindowHost* host;
    DvzWindowBackendSlot* backend_slot;
    void* backend_handle;
    void* backend_payload;
    DvzInputRouter* router;
    DvzWindowConfig config;
    DvzWindowSurface surface;
    char title[DVZ_WINDOW_TITLE_MAX];
    bool frame_pending;
    void* user_data;
};



struct DvzWindowHost
{
    DvzWindow** windows;
    uint32_t window_count;
    uint32_t window_capacity;

    DvzWindowBackendSlot* backends;
    uint32_t backend_count;
    uint32_t backend_capacity;
};
