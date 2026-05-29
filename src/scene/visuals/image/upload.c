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
#include "domain/field_internal.h"
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
    out->allocation_width = _visual_family_state(visual)->texture.width;
    out->allocation_height = _visual_family_state(visual)->texture.height;
    return true;
}



/**
 * Prepare a dirty RGBA texture upload payload for an image or glyph visual.
 *
 * @param visual the image-like visual
 * @param out output texture upload payload
 * @param out_handled whether the visual is handled by image texture upload logic
 * @return whether the payload decision succeeded
 */
bool _image_texture_upload_payload_if_dirty(
    DvzVisual* visual, DvzImageTextureUploadPayload* out, bool* out_handled)
{
    ANN(visual);
    ANN(out);
    ANN(out_handled);
    dvz_memset(out, sizeof(DvzImageTextureUploadPayload), 0, sizeof(DvzImageTextureUploadPayload));
    *out_handled = visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_GLYPH;
    if (!*out_handled)
        return true;
    if (_visual_family_state(visual)->field == NULL || (!_visual_family_state(visual)->texture.dirty && !_visual_family_state(visual)->field->dirty))
        return true;
    return _image_texture_upload_payload(visual, out);
}
