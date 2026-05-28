/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lowering upload emission                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_emit.h"
#include "_scene_emit_internal.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_internal.h"
#include "_visual_lowering.h"
#include "image/internal.h"
#include "stroke/internal.h"
#include "volume/internal.h"
#include "colorizer.h"
#include "datoviz/drp2/runtime.h"
#include "render_contract/render_contract.h"


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
    DvzFieldRegion upload_region = {0};
    const void* upload_data = NULL;
    if (!_scene_prepare_field_texture(field, &upload_region, &upload_data))
        return false;

    uint64_t bytes = 0;
    uint32_t bytes_per_texel = 0;
    uint32_t texture_format = 0;
    if (!_field_region_byte_size(field->desc.format, &upload_region, &bytes) ||
        !_field_format_bytes_per_texel(field->desc.format, &bytes_per_texel) ||
        !_field_format_texture_format(field->desc.format, &texture_format))
    {
        log_error("sampled field texture upload size or format conversion failed");
        return false;
    }

    if (!dvz_frame_plan_upload_bytes(plan, resource_id, 0, bytes, "field", upload_data))
        return false;

    DvzFramePlanUploadMeta metadata = {0};
    metadata.kind = field->desc.dim == DVZ_FIELD_DIM_3D ? DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D
                                                        : DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D;
    metadata.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE;
    metadata.visual_index = UINT32_MAX;
    metadata.buffer_index = UINT32_MAX;
    if (!dvz_frame_plan_upload_metadata(plan, &metadata) ||
        !dvz_frame_plan_upload_set_texture_format(plan, texture_format, bytes_per_texel))
        return false;

    if (field->desc.dim == DVZ_FIELD_DIM_3D)
    {
        return dvz_frame_plan_upload_set_texture_3d_extent(
                   plan, upload_region.width, upload_region.height, upload_region.depth) &&
               dvz_frame_plan_upload_set_texture_3d_allocation_extent(
                   plan, field->desc.width, field->desc.height, field->desc.depth) &&
               dvz_frame_plan_upload_set_texture_3d_region(
                   plan, upload_region.x, upload_region.y, upload_region.z);
    }

    return dvz_frame_plan_upload_set_texture_extent(
               plan, upload_region.width, upload_region.height) &&
           dvz_frame_plan_upload_set_texture_allocation_extent(
               plan, field->desc.width, field->desc.height) &&
           dvz_frame_plan_upload_set_texture_region(plan, upload_region.x, upload_region.y);
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
 * Emit material parameter uploads for one visual.
 *
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @return whether emission can continue for this visual
 */
static bool _scene_emit_visual_material_upload(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    if (!visual->material_params_dirty)
        return true;

    char material_resource_id[128];
    if (!_scene_visual_attr_resource_key(
            figure, visual, visual_index, "material_params", material_resource_id,
            sizeof(material_resource_id)))
    {
        return false;
    }
    DvzSceneMaterialParams* params =
        (DvzSceneMaterialParams*)dvz_malloc(sizeof(DvzSceneMaterialParams));
    if (params == NULL)
        return false;
    DvzVisualLowering lowering = {0};
    bool has_lowering = _scene_visual_lowering_resolve(visual, &lowering);
    bool point_style_scaled =
        has_lowering && lowering.point_style_enabled &&
        (lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_POINT ||
         lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER);
    _material_params_upload_payload(
        visual, point_style_scaled, _scene_screen_scale(figure), params);
    if (!dvz_frame_plan_upload_bytes(
            plan, material_resource_id, 0, sizeof(DvzSceneMaterialParams), "material_params",
            params))
    {
        dvz_free(params);
        return false;
    }
    plan->nodes[plan->count - 1].u.upload.owned_data = params;
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX, 1);
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    return true;
}



/**
 * Emit family-owned derived geometry uploads before the generic dense-attribute path.
 *
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param out_skip_dense_attrs whether generic dense attr uploads should be skipped
 * @param out_finished_visual whether no later generic upload path should run for this visual
 * @return whether emission can continue for this visual
 */
static bool _scene_emit_visual_family_derived_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index,
    bool* out_skip_dense_attrs, bool* out_finished_visual)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    ANN(out_skip_dense_attrs);
    ANN(out_finished_visual);
    *out_skip_dense_attrs = false;
    *out_finished_visual = false;

    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return true;

    if (lowering.renderable_kind == DVZ_RENDERABLE_STROKE_QUAD &&
        lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_SEGMENT)
    {
        DvzVisualUploadPayload payloads[DVZ_VISUAL_UPLOAD_PAYLOAD_MAX] = {0};
        uint32_t payload_count = 0;
        bool dirty = _scene_visual_attrs_dirty(visual);
        if (lowering.needs_vector_params_sync)
        {
            _vector_sync_params(visual);
            dirty = dirty || visual->vector.stroke_gpu.dirty;
            if (dirty && _stroke_quad_vector_cache_rebuild(visual) &&
                _stroke_quad_vector_upload_payloads(visual, payloads, &payload_count))
            {
                _scene_emit_visual_buffer_payloads(
                    figure, plan, visual, visual_index, payloads, payload_count, 0);
            }
        }
        else
        {
            dirty = dirty || visual->segment.gpu.dirty;
            if (dirty && _stroke_quad_segment_cache_rebuild(visual) &&
                _stroke_quad_segment_upload_payloads(visual, payloads, &payload_count))
            {
                _scene_emit_visual_buffer_payloads(
                    figure, plan, visual, visual_index, payloads, payload_count, 0);
            }
        }
        *out_finished_visual = true;
        return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
    }
    if (lowering.renderable_kind == DVZ_RENDERABLE_PATH_STROKE)
    {
        if (lowering.needs_vector_params_sync)
            _vector_sync_params(visual);
        DvzVisualUploadPayload payloads[DVZ_VISUAL_UPLOAD_PAYLOAD_MAX] = {0};
        uint32_t payload_count = 0;
        DvzPathGpuCache* cache =
            visual->type == DVZ_VISUAL_TYPE_VECTOR ? &visual->vector.path_gpu : &visual->path.gpu;
        bool dirty = cache->dirty || _scene_visual_attrs_dirty(visual);
        if (dirty && _path_stroke_cache_rebuild(visual) &&
            _path_stroke_upload_payloads(visual, payloads, &payload_count))
        {
            _scene_emit_visual_buffer_payloads(
                figure, plan, visual, visual_index, payloads, payload_count, 0);
        }
        *out_finished_visual = true;
        return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
    }
    if (_image_uses_generated_quads(visual))
    {
        DvzVisualUploadPayload payloads[DVZ_VISUAL_UPLOAD_PAYLOAD_MAX] = {0};
        uint32_t payload_count = 0;
        bool dirty = visual->image_gpu.dirty || _scene_visual_attrs_dirty(visual);
        if (dirty && _image_generated_quad_cache_rebuild(figure, visual) &&
            _image_generated_quad_upload_payloads(figure, visual, payloads, &payload_count))
        {
            _scene_emit_visual_buffer_payloads(
                figure, plan, visual, visual_index, payloads, payload_count,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        }
        *out_skip_dense_attrs = true;
    }
    return true;
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
    if ((visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH &&
         visual->type != DVZ_VISUAL_TYPE_LABELS && visual->type != DVZ_VISUAL_TYPE_MESH) ||
        visual->field == NULL || (!visual->texture.dirty && !visual->field->dirty))
    {
        return;
    }

    if (visual->type == DVZ_VISUAL_TYPE_LABELS || visual->type == DVZ_VISUAL_TYPE_MESH)
    {
        char tex_resource_id[128];
        if (!_scene_visual_texture_resource_key(
                figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
            return;
        if (_scene_emit_sampled_field_texture_upload(plan, tex_resource_id, visual->field))
        {
            visual->texture.width = visual->field->desc.width;
            visual->texture.height = visual->field->desc.height;
        }
        return;
    }

    char tex_resource_id[128];
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        return;
    DvzImageTextureUploadPayload payload = {0};
    if (!_image_texture_upload_payload(visual, &payload))
        return;

    dvz_frame_plan_upload_bytes(
        plan, tex_resource_id, 0, payload.byte_size, "texture", payload.data);
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D, UINT32_MAX, 0);
    dvz_frame_plan_upload_set_texture_extent(plan, payload.region.width, payload.region.height);
    dvz_frame_plan_upload_set_texture_allocation_extent(
        plan, payload.allocation_width, payload.allocation_height);
    dvz_frame_plan_upload_set_texture_region(plan, payload.region.x, payload.region.y);
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
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL ||
        (!visual->texture.dirty && !visual->field->dirty))
    {
        return;
    }

    char tex_resource_id[128];
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        return;
    DvzVolumeTextureUploadPayload payload = {0};
    if (!_volume_source_texture_payload(visual, &payload) ||
        !dvz_frame_plan_upload_bytes(plan, tex_resource_id, 0, payload.byte_size, "field",
                                     payload.data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .visual_index = UINT32_MAX,
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
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D, UINT32_MAX, 0);
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
    if (!_volume_uses_color_texture(visual))
    {
        return;
    }

    char transfer_resource_id[128];
    DvzVolumeTransferTexturePayload payload = {0};
    if (!_scene_resource_key_volume_transfer(
            visual_index, transfer_resource_id, sizeof(transfer_resource_id)) ||
        !_volume_transfer_texture_payload(visual, &payload) ||
        !dvz_frame_plan_upload_bytes(
            plan, transfer_resource_id, 0, payload.byte_size, "volume_transfer", payload.data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .visual_index = UINT32_MAX,
                .buffer_index = UINT32_MAX,
            }) ||
        !dvz_frame_plan_upload_set_texture_format(plan, VK_FORMAT_R8G8B8A8_UNORM, 4) ||
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
    if (!_volume_uses_label_lookup(visual, NULL))
    {
        return;
    }

    char lookup_resource_id[128];
    const void* lookup_data = NULL;
    uint64_t lookup_size = 0;
    if (!_scene_resource_key_volume_label_lookup(
            visual_index, lookup_resource_id, sizeof(lookup_resource_id)) ||
        !_volume_label_lookup_payload(visual, &lookup_data, &lookup_size) ||
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
static void _scene_emit_visual_family_texture_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    _scene_emit_image_like_texture_upload(figure, plan, visual, visual_index);
    _scene_emit_volume_source_texture_upload(figure, plan, visual, visual_index);
    _scene_emit_volume_transfer_texture_upload(plan, visual, visual_index);
    _scene_emit_volume_label_lookup_upload(plan, visual, visual_index);
}



/**
 * Emit dirty uploads for all panel-visible visuals in one figure.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param report optional diagnostic report
 */
void _scene_emit_visual_uploads(
    DvzFigure* figure, DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    _scene_prepare_composite_visuals(figure);
    _scene_prepare_axis_visuals(figure);
    _scene_prepare_colorbar_visuals(figure, report);
    _scene_prepare_legend_visuals(figure, report);
    _scene_prepare_text_visuals(figure);
    _scene_prepare_bounds_visuals(figure);
    bool emitted_buffers[DVZ_SCENE_MAX_BUFFERS] = {0};
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            if (visual->type == DVZ_VISUAL_TYPE_TEXT)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
            bool skip_dense_attrs = false;
            bool finished_visual = false;
            if (!_scene_emit_visual_family_derived_uploads(
                    figure, plan, visual, vidx, &skip_dense_attrs, &finished_visual))
            {
                continue;
            }
            if (finished_visual)
                continue;

            if (!skip_dense_attrs)
            {
                for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                {
                    DvzVisualAttr* attr = &visual->attrs[ai];
                    if (attr->buffer != NULL)
                    {
                        uint32_t buffer_idx = _scene_buffer_index(figure->scene, attr->buffer);
                        if (buffer_idx == UINT32_MAX || emitted_buffers[buffer_idx])
                            continue;
                        char buffer_resource_id[128];
                        if (!_scene_resource_key_buffer(
                                buffer_idx, buffer_resource_id, sizeof(buffer_resource_id)))
                            continue;
                        bool has_cpu_data = attr->buffer->data != NULL;
                        if ((has_cpu_data && attr->buffer->dirty) || !has_cpu_data)
                        {
                            dvz_frame_plan_upload_bytes(
                                plan, buffer_resource_id, 0, attr->buffer->desc.byte_size,
                                attr->name, attr->buffer->data);
                            _scene_attach_upload_metadata(
                                plan, visual, vidx, _scene_attr_frame_plan_role(attr->name),
                                DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx,
                                attr->item_count);
                            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                            node->u.upload.external = !has_cpu_data;
                            node->u.upload.buffer_usage =
                                _scene_buffer_drp2_usage(attr->buffer->desc.usage);
                            node->u.upload.item_stride = attr->buffer->desc.stride;
                            if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                                 visual->type == DVZ_VISUAL_TYPE_MESH ||
                                 visual->type == DVZ_VISUAL_TYPE_PATH ||
                                 visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                                 visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
                                strcmp(attr->name, "position") == 0)
                            {
                                dvz_frame_plan_upload_set_topology(
                                    plan, (uint32_t)visual->topology);
                            }
                        }
                        emitted_buffers[buffer_idx] = true;
                        continue;
                    }
                    if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
                        continue;
                    char resource_id[128];
                    if (!_scene_visual_attr_resource_key(
                            figure, visual, vidx, attr->name, resource_id, sizeof(resource_id)))
                    {
                        continue;
                    }
                    uint64_t byte_offset = (uint64_t)attr->dirty_first_item * attr->item_size;
                    uint64_t byte_size = (uint64_t)attr->dirty_item_count * attr->item_size;
                    const void* data_ptr = (const uint8_t*)attr->data + byte_offset;
                    DvzFramePlanResourceRole role = _scene_attr_frame_plan_role(attr->name);
                    if (!_scene_frame_plan_upload_style_bytes(
                            figure, plan, resource_id, byte_offset, byte_size, attr->name,
                            data_ptr, role))
                    {
                        continue;
                    }
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
                        UINT32_MAX, attr->item_count);
                    if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                         visual->type == DVZ_VISUAL_TYPE_MESH ||
                         visual->type == DVZ_VISUAL_TYPE_PATH ||
                         visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                         visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
                        strcmp(attr->name, "position") == 0)
                    {
                        dvz_frame_plan_upload_set_topology(plan, (uint32_t)visual->topology);
                    }
                }
            }
            if (visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                visual->type == DVZ_VISUAL_TYPE_MARKER ||
                visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH || visual->type == DVZ_VISUAL_TYPE_SPHERE)
            {
                if (_scene_visual_needs_material_params(visual) && visual->material_params_dirty)
                {
                    if (!_scene_emit_visual_material_upload(figure, plan, visual, vidx))
                        continue;
                }
            }
            if (visual->buffer != NULL && visual->buffer->data != NULL)
            {
                uint32_t buffer_idx = _scene_buffer_index(figure->scene, visual->buffer);
                if (visual->buffer->dirty && buffer_idx != UINT32_MAX &&
                    !emitted_buffers[buffer_idx])
                {
                    char buffer_resource_id[128];
                    if (!_scene_resource_key_buffer(
                            buffer_idx, buffer_resource_id, sizeof(buffer_resource_id)))
                        continue;
                    dvz_frame_plan_upload_bytes(
                        plan, buffer_resource_id, 0, visual->buffer->desc.byte_size, "index",
                        visual->buffer->data);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
                        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx,
                        visual->buffer->desc.stride > 0
                            ? visual->buffer->desc.byte_size / visual->buffer->desc.stride
                            : 0);
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage =
                        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
                    node->u.upload.item_stride = visual->buffer->desc.stride;
                    emitted_buffers[buffer_idx] = true;
                }
            }
            _scene_emit_visual_family_texture_uploads(figure, plan, visual, vidx);
        }
    }
}
