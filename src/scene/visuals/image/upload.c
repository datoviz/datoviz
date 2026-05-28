/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual upload payloads                                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "image/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Prepare the RGBA texture upload payload for an image-like visual.
 *
 * @param visual the image-like visual
 * @param out output texture upload payload
 * @return whether the payload is available
 */
bool _image_texture_upload_payload(DvzVisual* visual, DvzImageTextureUploadPayload* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzImageTextureUploadPayload), 0, sizeof(DvzImageTextureUploadPayload));
    if (!_scene_prepare_image_texture(visual, &out->region, &out->data))
        return false;
    if (!_field_region_byte_size(DVZ_FIELD_FORMAT_RGBA8_UNORM, &out->region, &out->byte_size))
    {
        log_error("image visual texture upload size overflow");
        return false;
    }
    out->allocation_width = visual->texture.width;
    out->allocation_height = visual->texture.height;
    return true;
}
