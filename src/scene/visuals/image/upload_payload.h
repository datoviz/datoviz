/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual upload payloads                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "upload.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzImageTextureUploadPayload
{
    DvzFieldRegion region;
    const void* data;
    uint64_t byte_size;
    uint32_t allocation_width;
    uint32_t allocation_height;
} DvzImageTextureUploadPayload;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _image_generated_quad_derived_upload_payloads(
    const DvzFigure* figure, DvzVisual* visual, bool attrs_dirty,
    DvzVisualUploadPayload* out_payloads, uint32_t* out_count, bool* out_handled);

bool _image_texture_upload_payload(DvzVisual* visual, DvzImageTextureUploadPayload* out);

bool _image_texture_upload_payload_if_dirty(
    DvzVisual* visual, DvzImageTextureUploadPayload* out, bool* out_handled);
