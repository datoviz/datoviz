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
#include "domain/field_internal.h"
#include "scene_emit/internal.h"
#include "_scene_resource_key.h"
#include "colorizer.h"
#include "image/upload_payload.h"
#include "registry/registry.h"
#include "volume/upload_payload.h"
#include "datoviz/drp2/runtime.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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

    DvzSampledFieldTextureUploadPayload payload = {0};
    if (!_scene_sampled_field_texture_upload_payload(field, &payload))
        return false;

    if (!dvz_frame_plan_upload_bytes(plan, resource_id, 0, payload.byte_size, "field", payload.data))
        return false;

    DvzFramePlanUploadMeta metadata = {0};
    metadata.kind = payload.texture_3d ? DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D
                                       : DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D;
    metadata.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE;
    metadata.color_role = payload.color_role;
    metadata.visual_index = UINT32_MAX;
    metadata.buffer_index = UINT32_MAX;
    if (!dvz_frame_plan_upload_metadata(plan, &metadata) ||
        !dvz_frame_plan_upload_set_texture_format(
            plan, payload.texture_format, payload.bytes_per_texel))
        return false;

    if (payload.texture_3d)
    {
        return dvz_frame_plan_upload_set_texture_3d_extent(
                   plan, payload.region.width, payload.region.height, payload.region.depth) &&
               dvz_frame_plan_upload_set_texture_3d_allocation_extent(
                   plan, payload.allocation_width, payload.allocation_height,
                   payload.allocation_depth) &&
               dvz_frame_plan_upload_set_texture_3d_region(
                   plan, payload.region.x, payload.region.y, payload.region.z);
    }

    return dvz_frame_plan_upload_set_texture_extent(
               plan, payload.region.width, payload.region.height) &&
           dvz_frame_plan_upload_set_texture_allocation_extent(
               plan, payload.allocation_width, payload.allocation_height) &&
           dvz_frame_plan_upload_set_texture_region(plan, payload.region.x, payload.region.y);
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

    dvz_frame_plan_upload_bytes(
        plan, tex_resource_id, 0, payload.byte_size, "texture", payload.data);
    dvz_frame_plan_upload_metadata(
        plan,
        &(DvzFramePlanUploadMeta){
            .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
            .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
            .color_role = payload.color_role,
            .visual_type = (uint32_t)visual->type,
            .visual_index = visual_index,
            .buffer_index = UINT32_MAX,
        });
    dvz_frame_plan_upload_set_texture_extent(plan, payload.region.width, payload.region.height);
    dvz_frame_plan_upload_set_texture_allocation_extent(
        plan, payload.allocation_width, payload.allocation_height);
    dvz_frame_plan_upload_set_texture_region(plan, payload.region.x, payload.region.y);
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
    if (!dvz_frame_plan_upload_bytes(
            plan, tex_resource_id, 0, page->byte_size, "marker_symbol_atlas", page->data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .color_role = _marker_symbol_atlas_color_role(symbol_source_kind),
                .visual_index = visual_index,
                .buffer_index = UINT32_MAX,
            }) ||
        !dvz_frame_plan_upload_set_texture_format(plan, texture_format, bytes_per_texel) ||
        !dvz_frame_plan_upload_set_texture_extent(plan, page->width, page->height) ||
        !dvz_frame_plan_upload_set_texture_allocation_extent(plan, page->width, page->height) ||
        !dvz_frame_plan_upload_set_texture_region(plan, 0, 0))
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
    if (!dvz_frame_plan_upload_bytes(plan, tex_resource_id, 0, payload.byte_size, "field",
                                     payload.data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .color_role = payload.color_role,
                .visual_type = (uint32_t)visual->type,
                .visual_index = visual_index,
                .buffer_index = UINT32_MAX,
            }) ||
        !dvz_frame_plan_upload_set_texture_format(
            plan, payload.texture_format, payload.bytes_per_texel) ||
        !dvz_frame_plan_upload_set_texture_3d_extent(
            plan, payload.region.width, payload.region.height, payload.region.depth) ||
        !dvz_frame_plan_upload_set_texture_3d_allocation_extent(
            plan, payload.allocation_width, payload.allocation_height, payload.allocation_depth) ||
        !dvz_frame_plan_upload_set_texture_3d_region(
            plan, payload.region.x, payload.region.y, payload.region.z))
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
    if (!_scene_resource_key_volume_transfer(
            visual_index, transfer_resource_id, sizeof(transfer_resource_id)) ||
        !dvz_frame_plan_upload_bytes(
            plan, transfer_resource_id, 0, payload.byte_size, "volume_transfer", payload.data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .color_role = DVZ_COLOR_ROLE_SRGB_COLOR,
                .visual_index = UINT32_MAX,
                .buffer_index = UINT32_MAX,
            }) ||
        !dvz_frame_plan_upload_set_texture_format(plan, DVZ_FORMAT_R8G8B8A8_UNORM, 4) ||
        !dvz_frame_plan_upload_set_texture_extent(plan, payload.width, 1) ||
        !dvz_frame_plan_upload_set_texture_allocation_extent(plan, payload.width, 1) ||
        !dvz_frame_plan_upload_set_texture_region(plan, 0, 0))
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
