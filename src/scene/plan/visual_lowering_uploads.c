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
#include "render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the typed FramePlan role for a visual attribute name.
 *
 * @param attr_name the visual attribute name
 * @return the typed resource role
 */
static DvzFramePlanResourceRole _scene_attr_frame_plan_role(const char* attr_name)
{
    if (attr_name == NULL)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE;
    if (strcmp(attr_name, "position") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    if (strcmp(attr_name, "position_start") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START;
    if (strcmp(attr_name, "position_end") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END;
    if (strcmp(attr_name, "color") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    if (strcmp(attr_name, "size") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
    if (strcmp(attr_name, "sigma") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA;
    if (strcmp(attr_name, "angle") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_ANGLE;
    if (strcmp(attr_name, "shape") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_SHAPE;
    if (strcmp(attr_name, "line_width") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH;
    if (strcmp(attr_name, "texcoords") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS;
    if (strcmp(attr_name, "normal") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL;
    if (strcmp(attr_name, "selection") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_SELECTION;
    if (strcmp(attr_name, "path_flags") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_FLAGS;
    if (strcmp(attr_name, "path_distance") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_DISTANCE;
    return DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE;
}



/**
 * Return the scene-buffer index backing one visual attribute, when present.
 *
 * @param figure the parent figure
 * @param visual the visual
 * @param attr_name the attribute name
 * @return the scene-buffer index, or UINT32_MAX when absent
 */
static uint32_t
_scene_attr_buffer_index(const DvzFigure* figure, const DvzVisual* visual, const char* attr_name)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(visual);
    ANN(attr_name);
    int attr_idx = _attr_index(visual, attr_name);
    if (attr_idx < 0 || visual->attrs[attr_idx].buffer == NULL)
        return UINT32_MAX;
    return _scene_buffer_index(figure->scene, visual->attrs[attr_idx].buffer);
}



/**
 * Return whether one visual has CPU-side data for an attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return whether the attribute exists and has data
 */
bool _scene_visual_has_attr_data(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int attr_idx = _attr_index(visual, attr_name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}



/**
 * Return whether one visual should expose material params to the renderer.
 *
 * @param visual the visual
 * @return whether render metadata should include the material params resource
 */
bool _scene_visual_needs_material_params(const DvzVisual* visual)
{
    ANN(visual);
    DvzVisualLowering lowering = {0};
    return _scene_visual_lowering_resolve(visual, &lowering) && lowering.needs_material_params;
}



/**
 * Resolve the resource key used by one visual attribute.
 *
 * @param figure the parent figure
 * @param visual the visual
 * @param visual_index the visual index
 * @param attr_name the attribute name
 * @param out_key output resource key
 * @param out_size output resource key capacity
 * @return whether the key was resolved
 */
bool _scene_attr_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index, const char* attr_name,
    char* out_key, size_t out_size)
{
    ANN(figure);
    ANN(visual);
    ANN(attr_name);
    ANN(out_key);
    uint32_t buffer_idx = _scene_attr_buffer_index(figure, visual, attr_name);
    if (buffer_idx != UINT32_MAX)
        return _scene_resource_key_buffer(buffer_idx, out_key, out_size);
    return _scene_visual_attr_resource_key(
        figure, visual, visual_index, attr_name, out_key, out_size);
}



/**
 * Resolve the resource key used by one panel's EDL uniform.
 *
 * @param panel_id the panel id
 * @param out_key output resource key
 * @param out_size output resource key capacity
 * @return whether the key was resolved
 */
bool _scene_edl_params_resource_key(const char* panel_id, char* out_key, size_t out_size)
{
    ANN(panel_id);
    ANN(out_key);
    if (out_size == 0)
        return false;
    dvz_snprintf(out_key, out_size, "%s.edl.params", panel_id);
    return true;
}



/**
 * Resolve the resource key used by one panel's SSAO uniform.
 *
 * @param panel_id the panel id
 * @param out_key output resource key
 * @param out_size output resource key capacity
 * @return whether the key was resolved
 */
bool _scene_ssao_params_resource_key(const char* panel_id, char* out_key, size_t out_size)
{
    ANN(panel_id);
    ANN(out_key);
    if (out_size == 0)
        return false;
    dvz_snprintf(out_key, out_size, "%s.ssao.params", panel_id);
    return true;
}



/**
 * Convert scene buffer usage flags to DRP2 buffer usage flags.
 *
 * @param usage the scene buffer usage flags
 * @return DRP2 buffer usage flags
 */
static uint32_t _scene_buffer_drp2_usage(uint32_t usage)
{
    uint32_t out = DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_VERTEX) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_VERTEX;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_INDEX) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_INDEX;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_UNIFORM) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_UNIFORM;
    return out;
}



/**
 * Attach typed metadata to the most recently emitted upload node.
 *
 * @param plan the destination frame plan
 * @param visual the retained visual
 * @param visual_index the visual index within the figure
 * @param role the typed resource role
 * @param kind the typed resource kind
 * @param buffer_index the optional scene-buffer index, or UINT32_MAX
 * @return whether metadata was attached
 */
static bool _scene_attach_upload_metadata(
    DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    DvzFramePlanResourceRole role, DvzFramePlanResourceKind kind, uint32_t buffer_index,
    uint64_t logical_item_count)
{
    ANN(plan);
    ANN(visual);
    DvzFramePlanUploadMeta metadata = {0};
    metadata.kind = kind;
    metadata.role = role;
    metadata.visual_type = (uint32_t)visual->type;
    metadata.visual_index = visual_index;
    metadata.buffer_index = buffer_index;
    metadata.logical_item_count = logical_item_count;
    return dvz_frame_plan_upload_metadata(plan, &metadata);
}


/**
 * Return whether one upload role stores logical screen-space pixels.
 *
 * @param role typed resource role
 * @return whether the upload should be lowered to physical style pixels
 */
static bool _scene_upload_role_screen_space(DvzFramePlanResourceRole role)
{
    return role == DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE ||
           role == DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH ||
           role == DVZ_FRAME_PLAN_RESOURCE_ROLE_SIGMA;
}



/**
 * Append an upload, scaling float screen-space payloads into owned plan storage when needed.
 *
 * @param figure parent figure
 * @param plan destination frame plan
 * @param resource_id resource key
 * @param byte_offset upload byte offset
 * @param byte_size upload byte size
 * @param data_tag debug data tag
 * @param data source payload
 * @param role typed resource role
 * @return whether the upload was appended
 */
static bool _scene_frame_plan_upload_style_bytes(
    const DvzFigure* figure, DvzFramePlan* plan, const char* resource_id, uint64_t byte_offset,
    uint64_t byte_size, const char* data_tag, const void* data, DvzFramePlanResourceRole role)
{
    ANN(plan);
    const void* upload_data = data;
    void* owned = NULL;
    float scale = _scene_screen_scale(figure);
    if (_scene_upload_role_screen_space(role) && data != NULL && scale != 1.0f)
    {
        if (byte_size % sizeof(float) != 0 || byte_size > SIZE_MAX)
            return false;
        owned = dvz_malloc((size_t)byte_size);
        if (owned == NULL)
            return false;
        size_t count = (size_t)(byte_size / sizeof(float));
        const float* src = (const float*)data;
        float* dst = (float*)owned;
        for (size_t i = 0; i < count; i++)
            dst[i] = src[i] * scale;
        upload_data = owned;
    }

    bool ok =
        dvz_frame_plan_upload_bytes(plan, resource_id, byte_offset, byte_size, data_tag, upload_data);
    if (!ok)
    {
        dvz_free(owned);
        return false;
    }
    if (owned != NULL)
        plan->nodes[plan->count - 1].u.upload.owned_data = owned;
    return true;
}



/**
 * Return whether one visual has dirty source attributes.
 *
 * @param visual the visual
 * @return whether any source attribute has a pending dirty range
 */
static bool _scene_visual_attrs_dirty(const DvzVisual* visual)
{
    ANN(visual);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (visual->attrs[i].dirty_item_count > 0)
            return true;
    }
    return false;
}



/**
 * Emit derived buffer payloads prepared by one visual family helper.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param payloads prepared payload descriptors
 * @param payload_count number of prepared payload descriptors
 * @param position_topology topology to set on position payloads, or zero to leave unset
 */
static void _scene_emit_visual_buffer_payloads(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    const DvzVisualUploadPayload* payloads, uint32_t payload_count, uint32_t position_topology)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    ANN(payloads);
    for (uint32_t i = 0; i < payload_count; i++)
    {
        const DvzVisualUploadPayload* payload = &payloads[i];
        char resource_id[128];
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, payload->name, resource_id, sizeof(resource_id)))
        {
            continue;
        }
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(payload->item_count, payload->item_size, &byte_size))
            continue;

        DvzFramePlanResourceRole role = payload->index
                                            ? DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX
                                            : _scene_attr_frame_plan_role(payload->name);
        if (payload->index)
        {
            if (!dvz_frame_plan_upload_bytes(
                    plan, resource_id, 0, byte_size, payload->name, payload->data))
            {
                continue;
            }
        }
        else if (!_scene_frame_plan_upload_style_bytes(
                     figure, plan, resource_id, 0, byte_size, payload->name, payload->data, role))
        {
            continue;
        }

        _scene_attach_upload_metadata(
            plan, visual, visual_index, role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX,
            payload->item_count);
        if (payload->index)
        {
            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
            node->u.upload.buffer_usage =
                DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
            node->u.upload.item_stride = payload->item_size;
        }
        else if (position_topology != 0 && strcmp(payload->name, "position") == 0)
        {
            dvz_frame_plan_upload_set_topology(plan, position_topology);
        }
    }
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

    DvzFieldRegion upload_region = {0};
    const void* upload_data = NULL;
    if (!_scene_prepare_image_texture(visual, &upload_region, &upload_data))
        return;
    char tex_resource_id[128];
    if (!_scene_visual_texture_resource_key(
            figure, visual, visual_index, tex_resource_id, sizeof(tex_resource_id)))
        return;
    uint64_t bytes = 0;
    if (!_field_region_byte_size(DVZ_FIELD_FORMAT_RGBA8_UNORM, &upload_region, &bytes))
    {
        log_error("image visual texture upload size overflow");
        return;
    }

    dvz_frame_plan_upload_bytes(plan, tex_resource_id, 0, bytes, "texture", upload_data);
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
        DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D, UINT32_MAX, 0);
    dvz_frame_plan_upload_set_texture_extent(plan, upload_region.width, upload_region.height);
    dvz_frame_plan_upload_set_texture_allocation_extent(
        plan, visual->texture.width, visual->texture.height);
    dvz_frame_plan_upload_set_texture_region(plan, upload_region.x, upload_region.y);
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
