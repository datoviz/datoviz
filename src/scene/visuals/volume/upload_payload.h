/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Volume visual upload payloads                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVolumeTextureUploadPayload
{
    DvzFieldRegion region;
    const void* data;
    uint64_t byte_size;
    uint32_t texture_format;
    uint32_t bytes_per_texel;
    uint32_t allocation_width;
    uint32_t allocation_height;
    uint32_t allocation_depth;
} DvzVolumeTextureUploadPayload;


typedef struct DvzVolumeTransferTexturePayload
{
    const void* data;
    uint64_t byte_size;
    uint32_t width;
} DvzVolumeTransferTexturePayload;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _volume_source_texture_payload_if_dirty(
    DvzVisual* visual, DvzVolumeTextureUploadPayload* out, bool* out_handled);

bool _volume_transfer_texture_payload_if_needed(
    DvzVisual* visual, DvzVolumeTransferTexturePayload* out, bool* out_handled);

bool _volume_label_lookup_payload_if_needed(
    DvzVisual* visual, const void** out_data, uint64_t* out_size, bool* out_handled);
