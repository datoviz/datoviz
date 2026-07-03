/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan runtime upload helpers                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_upload.h"
#include "_overflow.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static DvzDrp2ColorRole _drp2_color_role(DvzColorRole role)
{
    switch (role)
    {
    case DVZ_COLOR_ROLE_SRGB_COLOR:
        return DVZ_DRP2_COLOR_ROLE_SRGB_COLOR;
    case DVZ_COLOR_ROLE_LINEAR_COLOR:
        return DVZ_DRP2_COLOR_ROLE_LINEAR_COLOR;
    case DVZ_COLOR_ROLE_DATA:
        return DVZ_DRP2_COLOR_ROLE_DATA;
    case DVZ_COLOR_ROLE_NONE:
    default:
        return DVZ_DRP2_COLOR_ROLE_NONE;
    }
}


/**
 * Emit runtime-mode upload commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param node the upload node
 * @param out_id the emitted resource id
 * @return whether the commands were emitted
 */
bool _emitter_emit_upload(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* node,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(node);
    ANN(out_id);

    /* Texture upload: routed when texture_width > 0. */
    if (node->u.upload.texture_width > 0 && node->u.upload.texture_height > 0)
    {
        bool is_new = false;
        ResourceId* resource =
            _resource_entry(&emitter->resources, node->u.upload.resource_id, &is_new);
        if (resource == NULL)
            return false;
        dvz_strlcpy(resource->data_tag, node->u.upload.data_tag, sizeof(resource->data_tag));
        if (node->u.upload.metadata.has_metadata)
        {
            resource->kind = node->u.upload.metadata.kind;
            resource->role = node->u.upload.metadata.role;
            resource->color_role = node->u.upload.metadata.color_role;
            resource->logical_item_count = node->u.upload.metadata.logical_item_count;
        }
        resource->byte_size = node->u.upload.byte_size;
        uint32_t w = node->u.upload.texture_width;
        uint32_t h = node->u.upload.texture_height;
        uint32_t d = node->u.upload.texture_depth > 0 ? node->u.upload.texture_depth : 1;
        uint32_t bytes_per_texel =
            node->u.upload.texture_bytes_per_texel > 0 ? node->u.upload.texture_bytes_per_texel : 4;
        uint32_t format = node->u.upload.texture_format;
        uint64_t bpr64 = 0;
        if (_dvz_mul_u64_overflows(w, bytes_per_texel, &bpr64) || bpr64 > UINT32_MAX)
            return false;
        uint32_t bpr = (uint32_t)bpr64;
        uint32_t texture_w =
            node->u.upload.texture_alloc_width > 0 ? node->u.upload.texture_alloc_width : w;
        uint32_t texture_h =
            node->u.upload.texture_alloc_height > 0 ? node->u.upload.texture_alloc_height : h;
        uint32_t texture_d =
            node->u.upload.texture_alloc_depth > 0 ? node->u.upload.texture_alloc_depth : d;
        if (texture_w == 0 || texture_h == 0 || texture_d == 0)
            return false;
        if (node->u.upload.texture_origin_x != 0 || node->u.upload.texture_origin_y != 0 ||
            node->u.upload.texture_origin_z != 0)
        {
            if (node->u.upload.texture_alloc_width == 0 ||
                node->u.upload.texture_alloc_height == 0 ||
                node->u.upload.texture_alloc_depth == 0)
            {
                if (resource->texture_width > 0 && resource->texture_height > 0 &&
                    resource->texture_depth > 0)
                {
                    texture_w = resource->texture_width;
                    texture_h = resource->texture_height;
                    texture_d = resource->texture_depth;
                }
                else
                {
                    uint64_t alloc_w = 0;
                    uint64_t alloc_h = 0;
                    uint64_t alloc_d = 0;
                    if (_dvz_add_u64_overflows(node->u.upload.texture_origin_x, w, &alloc_w) ||
                        _dvz_add_u64_overflows(node->u.upload.texture_origin_y, h, &alloc_h) ||
                        _dvz_add_u64_overflows(node->u.upload.texture_origin_z, d, &alloc_d) ||
                        alloc_w > UINT32_MAX || alloc_h > UINT32_MAX ||
                        alloc_d > UINT32_MAX)
                        return false;
                    texture_w = (uint32_t)alloc_w;
                    texture_h = (uint32_t)alloc_h;
                    texture_d = (uint32_t)alloc_d;
                }
            }
        }
        uint64_t end_x = 0;
        uint64_t end_y = 0;
        uint64_t end_z = 0;
        if (_dvz_add_u64_overflows(node->u.upload.texture_origin_x, w, &end_x) ||
            _dvz_add_u64_overflows(node->u.upload.texture_origin_y, h, &end_y) ||
            _dvz_add_u64_overflows(node->u.upload.texture_origin_z, d, &end_z))
            return false;
        if (end_x > texture_w || end_y > texture_h || end_z > texture_d)
            return false;
        if (!_resource_ensure_texture(
                &emitter->resources, resource, texture_w, texture_h, texture_d, format,
                &is_new))
            return false;
        uint64_t id = resource->id;
        if (is_new)
        {
            uint32_t usage =
                DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
            DvzDrp2ColorRole color_role = _drp2_color_role(resource->color_role);
            if (texture_d > 1)
            {
                DvzDrp2TextureDesc desc = dvz_drp2_texture_desc();
                desc.id = id;
                desc.width = texture_w;
                desc.height = texture_h;
                desc.depth = texture_d;
                desc.format = format;
                desc.usage = usage;
                desc.color_role = color_role;
                if (!dvz_drp2_stream_create_texture(stream, &desc))
                    return false;
            }
            else
            {
                DvzDrp2TextureDesc desc = dvz_drp2_texture_desc();
                desc.id = id;
                desc.width = texture_w;
                desc.height = texture_h;
                desc.depth = 1;
                desc.format = format;
                desc.usage = usage;
                desc.color_role = color_role;
                if (!dvz_drp2_stream_create_texture(stream, &desc))
                    return false;
            }
        }
        if (emitter->resources.first_texture_id == 0)
            emitter->resources.first_texture_id = id;
        *out_id = id;
        if (node->u.upload.data == NULL)
        {
            char* zero_data = _zero_base64_alloc(node->u.upload.byte_size);
            if (zero_data == NULL)
                return false;
            bool ok = false;
            if (texture_d > 1 || d > 1 || node->u.upload.texture_origin_z != 0)
            {
                ok = dvz_drp2_stream_write_texture_3d(
                    stream, id, 0, node->u.upload.texture_origin_x,
                    node->u.upload.texture_origin_y, node->u.upload.texture_origin_z, w, h, d,
                    bpr, h, zero_data);
            }
            else if (node->u.upload.texture_origin_x == 0 && node->u.upload.texture_origin_y == 0)
            {
                ok = dvz_drp2_stream_write_texture_2d(stream, id, 0, w, h, bpr, h, zero_data);
            }
            else
            {
                ok = dvz_drp2_stream_write_texture_2d_region(
                    stream, id, 0, node->u.upload.texture_origin_x,
                    node->u.upload.texture_origin_y, w, h, bpr, h, zero_data);
            }
            dvz_free(zero_data);
            return ok;
        }
        if (texture_d > 1 || d > 1 || node->u.upload.texture_origin_z != 0)
        {
            return dvz_drp2_stream_write_texture_3d_bytes(
                stream, id, 0, node->u.upload.texture_origin_x, node->u.upload.texture_origin_y,
                node->u.upload.texture_origin_z, w, h, d, bpr, h, node->u.upload.data);
        }
        if (node->u.upload.texture_origin_x == 0 && node->u.upload.texture_origin_y == 0)
            return dvz_drp2_stream_write_texture_2d_bytes(
                stream, id, 0, w, h, bpr, h, node->u.upload.data);
        return dvz_drp2_stream_write_texture_2d_region_bytes(
            stream, id, 0, node->u.upload.texture_origin_x, node->u.upload.texture_origin_y, w, h,
            bpr, h, node->u.upload.data);
    }

    uint64_t buffer_size = 0;
    if (_dvz_add_u64_overflows(node->u.upload.byte_offset, node->u.upload.byte_size, &buffer_size))
        return false;

    bool is_new = false;
    ResourceId* resource =
        _resource_entry(&emitter->resources, node->u.upload.resource_id, &is_new);
    if (resource == NULL)
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, resource, buffer_size, &is_new))
        return false;

    dvz_strlcpy(resource->data_tag, node->u.upload.data_tag, sizeof(resource->data_tag));
    if (node->u.upload.metadata.has_metadata)
    {
        resource->kind = node->u.upload.metadata.kind;
        resource->role = node->u.upload.metadata.role;
        resource->color_role = node->u.upload.metadata.color_role;
        if (node->u.upload.metadata.logical_item_count > 0)
            resource->logical_item_count = node->u.upload.metadata.logical_item_count;
    }
    resource->usage = node->u.upload.buffer_usage;
    resource->item_stride = node->u.upload.item_stride;
    if (node->u.upload.topology != UINT32_MAX)
        resource->topology = node->u.upload.topology;
    uint64_t id = resource->id;
    uint32_t usage = node->u.upload.buffer_usage != 0
                         ? node->u.upload.buffer_usage
                         : (DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    if (emitter->resources.first_vertex_buffer_id == 0)
        emitter->resources.first_vertex_buffer_id = id;
    *out_id = id;

    if (node->u.upload.external)
        return true;

    if (is_new && !dvz_drp2_stream_create_buffer(stream, id, buffer_size, usage))
        return false;

    if (node->u.upload.data != NULL)
    {
        /* Real vertex data provided — encode directly into the stream. */
        return dvz_drp2_stream_write_buffer_bytes(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size,
            node->u.upload.data);
    }
    else
    {
        /* No data: write zeros (placeholder / test path). */
        char* zero_data = _zero_base64_alloc(node->u.upload.byte_size);
        if (zero_data == NULL)
            return false;
        bool ok = dvz_drp2_stream_write_buffer(
            stream, id, node->u.upload.byte_offset, node->u.upload.byte_size, zero_data);
        dvz_free(zero_data);
        return ok;
    }
}



/**
 * Emit runtime-mode compute input/output buffer commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param upload the upload node backing the compute input
 * @param compute the compute node naming the output resource
 * @return whether the commands were emitted
 */
bool _emitter_emit_compute_buffers(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* upload,
    const DvzFramePlanNode* compute)
{
    ANN(emitter);
    ANN(stream);
    ANN(upload);
    ANN(compute);
    if (compute->u.compute.write_count == 0)
        return false;

    uint64_t input_size = 0;
    if (_dvz_add_u64_overflows(
            upload->u.upload.byte_offset, upload->u.upload.byte_size, &input_size))
        return false;

    bool input_create = false;
    bool output_create = false;
    ResourceId* input =
        _resource_entry(&emitter->resources, upload->u.upload.resource_id, &input_create);
    ResourceId* output =
        _resource_entry(&emitter->resources, compute->u.compute.writes[0], &output_create);
    if (input == NULL || output == NULL)
        return false;
    input = _resource_find(&emitter->resources, upload->u.upload.resource_id);
    output = _resource_find(&emitter->resources, compute->u.compute.writes[0]);
    if (input == NULL || output == NULL)
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, input, input_size, &input_create))
        return false;
    if (!_resource_ensure_byte_size(&emitter->resources, output, input_size, &output_create))
        return false;

    uint64_t input_id = input->id;
    uint64_t output_id = output->id;
    emitter->resources.first_compute_input_id = input_id;
    emitter->resources.first_compute_output_id = output_id;
    emitter->resources.first_vertex_buffer_id = output_id;
    emitter->resources.compute_buffer_size = input_size;

    char* data = _zero_base64_alloc(upload->u.upload.byte_size);
    if (data == NULL)
        return false;

    bool ok = true;
    if (input_create)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, input_id, input_size,
                       DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE);
    }
    ok = ok && dvz_drp2_stream_write_buffer(
                   stream, input_id, upload->u.upload.byte_offset, upload->u.upload.byte_size,
                   data);
    if (output_create)
    {
        ok = ok && dvz_drp2_stream_create_buffer(
                       stream, output_id, input_size,
                       DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_VERTEX);
    }
    dvz_free(data);
    return ok;
}
