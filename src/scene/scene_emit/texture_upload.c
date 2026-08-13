/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual texture upload emission                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "domain/buffer_internal.h"
#include "domain/field_internal.h"
#include "scene_emit/internal.h"
#include "_scene_resource_key.h"
#include "frame_plan/frame_plan.h"
#include "colorizer.h"
#include "image/upload_payload.h"
#include "registry/registry.h"
#include "volume/upload_payload.h"
#include "datoviz/drp2/runtime.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static bool _scene_emit_texture_upload_ex(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_size, const char* data_tag,
    const void* data, DvzFramePlanUploadMeta metadata, DvzFormat texture_format,
    uint32_t bytes_per_texel, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t alloc_width, uint32_t alloc_height, uint32_t alloc_depth, uint32_t origin_x,
    uint32_t origin_y, uint32_t origin_z)
{
    ANN(plan);
    ANN(resource_id);

    DvzFramePlanUploadDesc upload = dvz_frame_plan_upload_desc();
    upload.resource_id = resource_id;
    upload.byte_size = byte_size;
    upload.data_tag = data_tag;
    upload.data = data;
    upload.texture_format = texture_format;
    upload.texture_bytes_per_texel = bytes_per_texel;
    upload.texture_width = width;
    upload.texture_height = height;
    upload.texture_depth = depth;
    upload.texture_alloc_width = alloc_width;
    upload.texture_alloc_height = alloc_height;
    upload.texture_alloc_depth = alloc_depth;
    upload.texture_origin_x = origin_x;
    upload.texture_origin_y = origin_y;
    upload.texture_origin_z = origin_z;

    return dvz_frame_plan_upload_ex(plan, &upload) &&
           dvz_frame_plan_upload_metadata(plan, &metadata);
}


/**
 * Emit an external scene-buffer to sampled-texture copy.
 *
 * @param plan the destination frame plan
 * @param resource_id the canonical destination texture resource id
 * @param field the externally backed field
 * @return whether the source registration and copy were emitted
 */
static bool _scene_emit_external_sampled_field_texture_copy(
    DvzFramePlan* plan, const char* resource_id, DvzSampledField* field)
{
    ANN(plan);
    ANN(resource_id);
    ANN(field);
    DvzSceneBuffer* buffer = field->buffer;
    if (buffer == NULL || buffer->scene != field->scene)
        return false;

    uint32_t buffer_index = _scene_buffer_index(field->scene, buffer);
    char buffer_resource_id[128];
    uint32_t texture_format = 0;
    if (buffer_index == UINT32_MAX ||
        !_scene_resource_key_buffer(buffer->id, buffer_resource_id, sizeof(buffer_resource_id)) ||
        !_field_format_texture_format(field->desc.format, &texture_format))
    {
        return false;
    }

    if (!dvz_frame_plan_upload_bytes(
            plan, buffer_resource_id, 0, buffer->desc.byte_size, "field.external", NULL))
    {
        return false;
    }
    DvzFramePlanNode* upload = &plan->nodes[plan->count - 1];
    upload->u.upload.external = true;
    upload->u.upload.buffer_usage = _scene_buffer_drp2_usage(buffer->desc.usage);
    upload->u.upload.item_stride = buffer->desc.stride;
    upload->u.upload.metadata.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
    upload->u.upload.metadata.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE;
    upload->u.upload.metadata.buffer_index = buffer_index;

    DvzFramePlanCopyDesc copy = dvz_frame_plan_copy_desc();
    copy.direction = DVZ_FRAME_PLAN_COPY_BUFFER_TO_TEXTURE;
    copy.src_resource_id = buffer_resource_id;
    copy.dst_resource_id = resource_id;
    copy.extent[0] = field->desc.width;
    copy.extent[1] = field->desc.height;
    copy.extent[2] = 1;
    copy.format = (DvzFormat)texture_format;
    copy.bytes_per_texel = 4;
    copy.bytes_per_row = (uint64_t)field->desc.width * 4;
    copy.rows_per_image = field->desc.height;
    copy.byte_size = buffer->desc.byte_size;
    return dvz_frame_plan_copy_ex(plan, &copy);
}


/**
 * Emit one sampled field as a texture upload node.
 *
 * @param plan the destination frame plan
 * @param resource_id the texture resource id
 * @param field the sampled field
 * @return whether the upload node was emitted
 */
bool _scene_emit_sampled_field_texture_upload(
    DvzFramePlan* plan, const char* resource_id, DvzSampledField* field)
{
    ANN(plan);
    ANN(resource_id);
    ANN(field);

    if (field->buffer != NULL)
        return _scene_emit_external_sampled_field_texture_copy(plan, resource_id, field);

    DvzSampledFieldTextureUploadPayload payload = {0};
    if (!_scene_sampled_field_texture_upload_payload(field, &payload))
        return false;

    DvzFramePlanUploadMeta metadata = {0};
    metadata.kind = payload.texture_3d ? DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D
                                       : DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D;
    metadata.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE;
    metadata.color_role = payload.color_role;
    metadata.visual_index = UINT32_MAX;
    metadata.buffer_index = UINT32_MAX;

    const uint32_t depth = payload.texture_3d ? payload.region.depth : 1;
    const uint32_t alloc_depth = payload.texture_3d ? payload.allocation_depth : 1;
    return _scene_emit_texture_upload_ex(
        plan, resource_id, payload.byte_size, "field", payload.data, metadata,
        payload.texture_format, payload.bytes_per_texel, payload.region.width,
        payload.region.height, depth, payload.allocation_width, payload.allocation_height,
        alloc_depth, payload.region.x, payload.region.y, payload.texture_3d ? payload.region.z : 0);
}


/**
 * Format the per-volume transfer texture resource id.
 *
 * @param visual_index figure-local visual index
 * @param out output buffer
 * @param out_size output buffer size
 * @return whether the key was written
 */
bool _scene_resource_key_volume_transfer(uint32_t visual_index, char* out, size_t out_size)
{
    return dvz_snprintf(out, out_size, "visual.%u.volume_transfer", visual_index) > 0;
}


/**
 * Format the per-volume sparse label lookup resource id.
 *
 * @param visual_index figure-local visual index
 * @param out output buffer
 * @param out_size output buffer size
 * @return whether the key was written
 */
bool _scene_resource_key_volume_label_lookup(uint32_t visual_index, char* out, size_t out_size)
{
    return dvz_snprintf(out, out_size, "visual.%u.volume_label_lookup", visual_index) > 0;
}


/**
 * Return the color role for a marker symbol atlas texture.
 *
 * @param kind symbol source kind
 * @return texture color role
 */
static DvzColorRole _marker_symbol_atlas_color_role(DvzSymbolSourceKind kind)
{
    switch (kind)
    {
    case DVZ_SYMBOL_SOURCE_BITMAP:
        return DVZ_COLOR_ROLE_SRGB_COLOR;
    case DVZ_SYMBOL_SOURCE_SDF:
    case DVZ_SYMBOL_SOURCE_MSDF:
        return DVZ_COLOR_ROLE_DATA;
    default:
        return DVZ_COLOR_ROLE_NONE;
    }
}


/**
 * Emit dirty 2D texture uploads for image-like visuals.
 *
 * @param plan the destination frame plan
 * @param visual the image or glyph visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_image_like_texture_upload(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);

    DvzSampledField* field = _visual_family_state(visual)->field;
    if (field != NULL && field->buffer != NULL)
    {
        if (!_visual_family_state(visual)->texture.dirty && !field->dirty)
            return;
        char tex_resource_id[128];
        if (!_scene_visual_texture_resource_key(
                figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        {
            return;
        }
        if (_scene_emit_sampled_field_texture_upload(plan, tex_resource_id, field))
        {
            _visual_family_state(visual)->texture.width = field->desc.width;
            _visual_family_state(visual)->texture.height = field->desc.height;
        }
        return;
    }

    if (visual->ops != NULL && visual->ops->sampled_field_texture_upload)
    {
        if (_visual_family_state(visual)->field == NULL || (!_visual_family_state(visual)->texture.dirty && !_visual_family_state(visual)->field->dirty))
            return;
        char tex_resource_id[128];
        if (!_scene_visual_texture_resource_key(
                figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
            return;
        if (_scene_emit_sampled_field_texture_upload(plan, tex_resource_id, _visual_family_state(visual)->field))
        {
            _visual_family_state(visual)->texture.width = _visual_family_state(visual)->field->desc.width;
            _visual_family_state(visual)->texture.height = _visual_family_state(visual)->field->desc.height;
        }
        return;
    }

    bool handled = false;
    DvzImageTextureUploadPayload payload = {0};
    if (!_image_texture_upload_payload_if_dirty(visual, &payload, &handled) || !handled ||
        payload.byte_size == 0)
    {
        return;
    }

    char tex_resource_id[128];
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        return;

    DvzFramePlanUploadMeta metadata = {
        .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
        .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        .color_role = payload.color_role,
        .visual_type = (uint32_t)visual->type,
        .visual_index = visual_index,
        .buffer_index = UINT32_MAX,
    };
    _scene_emit_texture_upload_ex(
        plan, tex_resource_id, payload.byte_size, "texture", payload.data, metadata,
        DVZ_FORMAT_NONE, 0, payload.region.width, payload.region.height, 1,
        payload.allocation_width, payload.allocation_height, 1, payload.region.x,
        payload.region.y, 0);
}


/**
 * Emit a marker bitmap-symbol atlas texture upload.
 *
 * @param figure the parent figure
 * @param plan the destination frame plan
 * @param visual the marker visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_marker_symbol_texture_upload(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);

    const DvzSymbolAtlasPage* page = NULL;
    DvzSymbolSourceKind symbol_source_kind = DVZ_SYMBOL_SOURCE_NONE;
    if (!_scene_marker_symbol_atlas_page(visual, &page, &symbol_source_kind))
        return;

    uint32_t texture_format = 0;
    uint32_t bytes_per_texel = page->channels;
    if (page->channels == 1)
    {
        texture_format = DVZ_FORMAT_R8_UNORM;
    }
    else if (page->channels == 4)
    {
        texture_format = DVZ_FORMAT_R8G8B8A8_UNORM;
    }
    else
    {
        log_error("unsupported marker symbol atlas channel count");
        return;
    }

    char tex_resource_id[128];
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        return;
    DvzFramePlanUploadMeta metadata = {
        .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
        .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        .color_role = _marker_symbol_atlas_color_role(symbol_source_kind),
        .visual_index = visual_index,
        .buffer_index = UINT32_MAX,
    };
    if (!_scene_emit_texture_upload_ex(
            plan, tex_resource_id, page->byte_size, "marker_symbol_atlas", page->data, metadata,
            texture_format, bytes_per_texel, page->width, page->height, 1, page->width,
            page->height, 1, 0, 0, 0))
    {
        log_error("marker symbol atlas texture upload failed");
        return;
    }
}


/**
 * Emit dirty 3D source texture uploads for a volume visual.
 *
 * @param plan the destination frame plan
 * @param visual the volume visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_volume_source_texture_upload(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);

    bool handled = false;
    DvzVolumeTextureUploadPayload payload = {0};
    if (!_volume_source_texture_payload_if_dirty(visual, &payload, &handled) || !handled ||
        payload.byte_size == 0)
        return;

    char tex_resource_id[128];
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        return;
    DvzFramePlanUploadMeta metadata = {
        .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
        .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        .color_role = payload.color_role,
        .visual_type = (uint32_t)visual->type,
        .visual_index = visual_index,
        .buffer_index = UINT32_MAX,
    };
    if (!_scene_emit_texture_upload_ex(
            plan, tex_resource_id, payload.byte_size, "field", payload.data, metadata,
            payload.texture_format, payload.bytes_per_texel, payload.region.width,
            payload.region.height, payload.region.depth, payload.allocation_width,
            payload.allocation_height, payload.allocation_depth, payload.region.x,
            payload.region.y, payload.region.z))
    {
        log_error("volume visual texture upload failed");
        return;
    }
}


/**
 * Emit the scalar transfer texture upload for a volume visual.
 *
 * @param plan the destination frame plan
 * @param visual the volume visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_volume_transfer_texture_upload(
    DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(plan);
    ANN(visual);

    bool handled = false;
    DvzVolumeTransferTexturePayload payload = {0};
    if (!_volume_transfer_texture_payload_if_needed(visual, &payload, &handled))
    {
        log_error("volume transfer texture upload failed");
        return;
    }
    if (!handled)
        return;

    char transfer_resource_id[128];
    DvzFramePlanUploadMeta metadata = {
        .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
        .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        .color_role = DVZ_COLOR_ROLE_SRGB_COLOR,
        .visual_index = UINT32_MAX,
        .buffer_index = UINT32_MAX,
    };
    if (!_scene_resource_key_volume_transfer(
            visual_index, transfer_resource_id, sizeof(transfer_resource_id)) ||
        !_scene_emit_texture_upload_ex(
            plan, transfer_resource_id, payload.byte_size, "volume_transfer", payload.data,
            metadata, DVZ_FORMAT_R8G8B8A8_UNORM, 4, payload.width, 1, 1, payload.width, 1, 1, 0,
            0, 0))
    {
        log_error("volume transfer texture upload failed");
        return;
    }
}


/**
 * Emit the sparse label lookup storage-buffer upload for a volume visual.
 *
 * @param plan the destination frame plan
 * @param visual the volume visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_volume_label_lookup_upload(
    DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(plan);
    ANN(visual);

    bool handled = false;
    const void* lookup_data = NULL;
    uint64_t lookup_size = 0;
    if (!_volume_label_lookup_payload_if_needed(
            visual, &lookup_data, &lookup_size, &handled))
    {
        log_error("volume label lookup upload failed");
        return;
    }
    if (!handled)
        return;

    char lookup_resource_id[128];
    if (!_scene_resource_key_volume_label_lookup(
            visual_index, lookup_resource_id, sizeof(lookup_resource_id)) ||
        !dvz_frame_plan_upload_bytes(
            plan, lookup_resource_id, 0, lookup_size, "volume_label_lookup", lookup_data))
    {
        log_error("volume label lookup upload failed");
        return;
    }
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.buffer_usage =
        DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    node->u.upload.item_stride = sizeof(DvzSceneLabelLookupEntry);
    if (!dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE,
                .visual_index = visual_index,
                .buffer_index = UINT32_MAX,
            }))
    {
        log_error("volume label lookup metadata upload failed");
    }
}


/**
 * Emit family-owned texture uploads after shared buffer uploads.
 *
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 */
void _scene_emit_visual_family_texture_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    _scene_emit_image_like_texture_upload(figure, plan, visual, visual_index);
    _scene_emit_marker_symbol_texture_upload(figure, plan, visual, visual_index);
    _scene_emit_volume_source_texture_upload(figure, plan, visual, visual_index);
    _scene_emit_volume_transfer_texture_upload(plan, visual, visual_index);
    _scene_emit_volume_label_lookup_upload(plan, visual, visual_index);
}
