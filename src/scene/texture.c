/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/texture.h"
#include "../resources_utils.h"
#include "_map.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "fileio.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

DvzTexture* dvz_texture(DvzBatch* batch, DvzTexDims dims, int flags)
{
    ANN(batch);
    DvzTexture* texture = (DvzTexture*)calloc(1, sizeof(DvzTexture));
    texture->batch = batch;
    texture->dims = dims;
    texture->flags = flags;
    dvz_obj_init(&texture->obj);
    return texture;
}



void dvz_texture_shape(DvzTexture* texture, uint32_t width, uint32_t height, uint32_t depth)
{
    ANN(texture);
    texture->shape[0] = width;
    texture->shape[1] = height;
    texture->shape[2] = depth;
}



void dvz_texture_format(DvzTexture* texture, DvzFormat format)
{
    ANN(texture);
    texture->format = format;
}



void dvz_texture_filter(DvzTexture* texture, DvzFilter filter)
{
    ANN(texture);
    texture->filter = filter;
}



void dvz_texture_address_mode(DvzTexture* texture, DvzSamplerAddressMode address_mode)
{
    ANN(texture);
    texture->address_mode = address_mode;
}



void dvz_texture_data(
    DvzTexture* texture, uint32_t xoffset, uint32_t yoffset, uint32_t zoffset, //
    uint32_t width, uint32_t height, uint32_t depth, DvzSize size, void* data)
{
    ANN(texture);
    ANN(texture->batch);
    ANN(data);
    ASSERT(size > 0);
    ASSERT(width > 0);

    if (!dvz_obj_is_created(&texture->obj))
    {
        dvz_texture_create(texture);
        ASSERT(texture->tex != DVZ_ID_NONE);
    }

    uvec3 offset = {xoffset, yoffset, zoffset};
    uvec3 shape = {width, height, depth};
    dvz_upload_tex(texture->batch, texture->tex, offset, shape, size, data, 0);
}



void dvz_texture_create(DvzTexture* texture)
{
    ANN(texture);
    log_trace("creating texture");
    if (!dvz_obj_is_created(&texture->obj))
    {
        DvzRequest req = dvz_create_tex(
            texture->batch, texture->dims, texture->format, texture->shape, texture->flags);
        texture->tex = req.id;

        req = dvz_create_sampler(texture->batch, texture->filter, texture->address_mode);
        texture->sampler = req.id;

        dvz_obj_created(&texture->obj);
    }
    ASSERT(dvz_obj_is_created(&texture->obj));
}



DvzTexture* dvz_texture_image(
    DvzBatch* batch, DvzFormat format, DvzFilter filter, DvzSamplerAddressMode address_mode,
    uint32_t width, uint32_t height, void* data, int flags)
{
    ANN(batch);
    ASSERT(width > 0);
    ASSERT(height > 0);

    uvec3 shape = {width, height, 1};
    DvzSize size = width * height * _format_size(format);

    DvzTexture* texture = dvz_texture(batch, DVZ_TEX_2D, flags);
    dvz_texture_format(texture, format);
    dvz_texture_shape(texture, width, height, 1);
    dvz_texture_filter(texture, filter);
    dvz_texture_address_mode(texture, address_mode);

    if (data != NULL)
        dvz_texture_data(texture, 0, 0, 0, width, height, 1, size, data);

    return texture;
}



DvzTexture* dvz_texture_volume(
    DvzBatch* batch, DvzFormat format, DvzFilter filter, DvzSamplerAddressMode address_mode,
    uint32_t width, uint32_t height, uint32_t depth, void* data, int flags)
{
    ANN(batch);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);

    uvec3 shape = {width, height, depth};
    DvzSize size = width * height * depth * _format_size(format);

    DvzTexture* texture = dvz_texture(batch, DVZ_TEX_3D, flags);
    dvz_texture_format(texture, format);
    dvz_texture_shape(texture, width, height, depth);
    dvz_texture_filter(texture, filter);
    dvz_texture_address_mode(texture, address_mode);

    if (data != NULL)
        dvz_texture_data(texture, 0, 0, 0, width, height, depth, size, data);

    return texture;
}



void dvz_texture_destroy(DvzTexture* texture)
{
    ANN(texture);
    if (dvz_obj_is_created(&texture->obj))
    {
        ANN(texture->batch);
        ASSERT(texture->tex != DVZ_ID_NONE);
        log_trace("destroy texture");

        dvz_delete_tex(texture->batch, texture->tex);
        dvz_delete_sampler(texture->batch, texture->sampler);

        dvz_obj_destroyed(&texture->obj);
    }

    FREE(texture);
}
