/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual upload payload descriptors                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_VISUAL_UPLOAD_PAYLOAD_MAX 9



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVisualUploadPayload
{
    const char* name;
    const void* data;
    uint32_t item_size;
    uint64_t item_count;
    bool index;
} DvzVisualUploadPayload;
