/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lowering upload support                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_overflow.h"
#include "_scene.h"
#include "scene_emit/internal.h"
#include "_scene_resource_key.h"
#include "frame_plan/frame_plan.h"
#include "_visual_internal.h"
#include "core/panel_layout_internal.h"
#include "domain/buffer_internal.h"
#include "registry/registry.h"
#include "scene_emit/visual_lowering.h"
#include "datoviz/scene/scale.h"
#include "datoviz/drp2/runtime.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the typed FramePlan role for a visual attribute name.
 *
 * @param attr_name the visual attribute name
 * @return the typed resource role
 */
DvzFramePlanResourceRole _scene_attr_frame_plan_role(const char* attr_name)
{
    if (attr_name == NULL)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE;
    if (strcmp(attr_name, "position") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    if (strcmp(attr_name, "position_start") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START;
    if (strcmp(attr_name, "position_end") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END;
    if (strcmp(attr_name, "position_next") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_NEXT;
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
    if (strcmp(attr_name, "tex_rect") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS;
    if (strcmp(attr_name, "texcoords") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS;
    if (strcmp(attr_name, "normal") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL;
    if (strcmp(attr_name, "item_state") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE;
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
uint32_t _scene_buffer_drp2_usage(uint32_t usage)
{
    uint32_t out = DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_VERTEX) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_VERTEX;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_INDEX) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_INDEX;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_UNIFORM) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_UNIFORM;
    if ((usage & DVZ_SCENE_BUFFER_USAGE_STORAGE) != 0)
        out |= DVZ_DRP2_BUFFER_USAGE_STORAGE;
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
bool _scene_attach_upload_metadata(
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
 * Append an upload, scaling float screen-space payloads into owned plan storage when needed.
 *
 * @param figure parent figure
 * @param visual retained visual owning the payload
 * @param plan destination frame plan
 * @param resource_id resource key
 * @param byte_offset upload byte offset
 * @param byte_size upload byte size
 * @param data_tag debug data tag
 * @param data source payload
 * @return whether the upload was appended
 */
bool _scene_frame_plan_upload_style_bytes(
    const DvzFigure* figure, const DvzVisual* visual, DvzFramePlan* plan, const char* resource_id,
    uint64_t byte_offset, uint64_t byte_size, const char* data_tag, const void* data)
{
    ANN(visual);
    ANN(plan);
    const void* upload_data = data;
    void* owned = NULL;
    float scale = _scene_screen_scale(figure);
    if (_visual_family_attr_is_screen_space(visual->type, data_tag) && data != NULL &&
        scale != 1.0f)
    {
        if (byte_size % sizeof(float) != 0 || byte_size > SIZE_MAX)
            return false;
        owned = dvz_malloc((size_t)byte_size);
        if (owned == NULL)
            return false;
        DvzScenePayloadFieldDesc field = {
            .name = data_tag,
            .offset = 0,
            .count = byte_size / sizeof(float),
            .authored_unit = DVZ_SCENE_PAYLOAD_UNIT_LOGICAL_PX,
            .runtime_unit = DVZ_SCENE_PAYLOAD_UNIT_PHYSICAL_PX,
        };
        if (!_scene_payload_lower_fields(figure, data, byte_size, &field, 1, owned))
        {
            dvz_free(owned);
            return false;
        }
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
 * Lower authored payload fields into runtime payload units.
 *
 * @param figure parent figure
 * @param src authored source payload
 * @param byte_size payload byte size
 * @param fields field unit descriptors
 * @param field_count field descriptor count
 * @param dst destination payload
 * @return whether the payload was lowered
 */
bool _scene_payload_lower_fields(
    const DvzFigure* figure, const void* src, uint64_t byte_size,
    const DvzScenePayloadFieldDesc* fields, uint32_t field_count, void* dst)
{
    ANN(src);
    ANN(dst);
    if (byte_size > SIZE_MAX)
        return false;
    dvz_memcpy(dst, (size_t)byte_size, src, (size_t)byte_size);

    const float screen_scale = _scene_screen_scale(figure);
    for (uint32_t i = 0; i < field_count; i++)
    {
        const DvzScenePayloadFieldDesc* field = &fields[i];
        if (field->authored_unit == field->runtime_unit)
            continue;
        if (field->authored_unit != DVZ_SCENE_PAYLOAD_UNIT_LOGICAL_PX ||
            field->runtime_unit != DVZ_SCENE_PAYLOAD_UNIT_PHYSICAL_PX)
        {
            return false;
        }
        if (field->count == 0 || field->offset > (size_t)byte_size)
            return false;
        if (field->count > (uint64_t)((SIZE_MAX - field->offset) / sizeof(float)))
            return false;
        const size_t field_size = (size_t)field->count * sizeof(float);
        if (field_size > (size_t)byte_size - field->offset)
            return false;

        uint8_t* values = (uint8_t*)dst + field->offset;
        for (uint64_t j = 0; j < field->count; j++)
        {
            float value = 0;
            uint8_t* value_ptr = values + j * sizeof(float);
            dvz_memcpy(&value, sizeof(value), value_ptr, sizeof(value));
            value *= screen_scale;
            dvz_memcpy(value_ptr, sizeof(value), &value, sizeof(value));
        }
    }
    return true;
}



/**
 * Return whether one visual has dirty source attributes.
 *
 * @param visual the visual
 * @return whether any source attribute has a pending dirty range
 */
bool _scene_visual_attrs_dirty(const DvzVisual* visual)
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
 * Return whether an attribute is a retained scalar color that needs CPU colorization.
 *
 * @param visual the visual
 * @param attr the retained attribute
 * @return whether the attribute needs derived RGBA upload
 */
static bool _scene_attr_needs_scalar_color_upload(
    const DvzVisual* visual, const DvzVisualAttr* attr)
{
    ANN(visual);
    ANN(attr);
    return strcmp(attr->name, "color") == 0 &&
           attr->format == DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32 &&
           visual->ops != NULL && visual->ops->supports_scalar_color_scale;
}



/**
 * Resolve the color scale domain for one scalar attribute.
 *
 * @param scale optional retained scale
 * @param scalars retained scalar payload
 * @param count scalar count
 * @param out_min output domain minimum
 * @param out_max output domain maximum
 */
static void _scene_scalar_color_domain(
    const DvzScale* scale, const float* scalars, uint64_t count, double* out_min, double* out_max)
{
    ANN(scalars);
    ANN(out_min);
    ANN(out_max);
    if (scale != NULL && scale->has_domain)
    {
        *out_min = scale->domain_min;
        *out_max = scale->domain_max;
        return;
    }

    double min_value = INFINITY;
    double max_value = -INFINITY;
    for (uint64_t i = 0; i < count; i++)
    {
        const double value = (double)scalars[i];
        if (!isfinite(value))
            continue;
        if (value < min_value)
            min_value = value;
        if (value > max_value)
            max_value = value;
    }
    if (!isfinite(min_value) || !isfinite(max_value))
    {
        min_value = 0.0;
        max_value = 1.0;
    }
    *out_min = min_value;
    *out_max = max_value;
}



/**
 * Emit a derived RGBA upload for one scalar color attribute.
 *
 * @param figure the figure
 * @param plan destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param attr scalar color attribute
 * @return whether an upload was emitted
 */
static bool _scene_emit_scalar_color_upload(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    const DvzVisualAttr* attr)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    ANN(attr);
    if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
        return false;

    char resource_id[128];
    if (!_scene_visual_attr_resource_key(
            figure, visual, visual_index, attr->name, resource_id, sizeof(resource_id)))
    {
        return false;
    }

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(attr->dirty_item_count, sizeof(DvzColor), &byte_size) ||
        byte_size > SIZE_MAX)
    {
        return false;
    }

    DvzColor* colors = (DvzColor*)dvz_malloc((DvzSize)byte_size);
    if (colors == NULL)
        return false;

    const DvzScale* scale = _visual_family_state(visual)->scale;
    const DvzColormap* colormap = scale != NULL ? scale->colormap : NULL;
    const float* scalars = (const float*)attr->data;
    double domain_min = 0.0;
    double domain_max = 1.0;
    _scene_scalar_color_domain(scale, scalars, attr->item_count, &domain_min, &domain_max);
    const double span = domain_max - domain_min;

    for (uint64_t i = 0; i < attr->dirty_item_count; i++)
    {
        const uint64_t item_idx = attr->dirty_first_item + i;
        const double value = (double)scalars[item_idx];
        if (!isfinite(value))
        {
            colors[i] = dvz_color_rgba(0, 0, 0, 0);
            continue;
        }
        const double t = span != 0.0 ? (value - domain_min) / span : 0.5;
        dvz_colormap_sample(colormap, t, &colors[i]);
    }

    const uint64_t byte_offset = attr->dirty_first_item * sizeof(DvzColor);
    if (!dvz_frame_plan_upload_bytes(
            plan, resource_id, byte_offset, byte_size, attr->name, colors))
    {
        dvz_free(colors);
        return false;
    }
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR,
        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX, attr->item_count);
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.owned_data = colors;
    node->u.upload.item_stride = sizeof(DvzColor);
    return true;
}



/**
 * Emit material-parameter upload for one visual when registry policy and dirty state require it.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param upload_material_params whether this family participates in generic material uploads
 * @return whether emission can continue for this visual
 */
bool _scene_emit_visual_material_upload_if_needed(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index,
    bool upload_material_params)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    if (!upload_material_params)
        return true;
    if (!_scene_visual_needs_material_params(visual) || !_visual_family_state(visual)->material_params_dirty)
        return true;
    return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
}



/**
 * Emit dirty dense attribute uploads for one retained visual.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param upload_position_topology whether position uploads carry the visual topology
 * @param emitted_buffers scene-buffer emission guards shared across the upload pass
 */
void _scene_emit_visual_dense_attr_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    bool upload_position_topology, bool* emitted_buffers)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    ANN(visual);
    ANN(emitted_buffers);
    for (uint32_t ai = 0; ai < visual->attr_count; ai++)
    {
        const DvzVisualAttr* attr = &visual->attrs[ai];
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
                    plan, buffer_resource_id, 0, attr->buffer->desc.byte_size, attr->name,
                    attr->buffer->data);
                _scene_attach_upload_metadata(
                    plan, visual, visual_index, _scene_attr_frame_plan_role(attr->name),
                    DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx, attr->item_count);
                DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                node->u.upload.external = !has_cpu_data;
                node->u.upload.buffer_usage = _scene_buffer_drp2_usage(attr->buffer->desc.usage);
                node->u.upload.item_stride = attr->buffer->desc.stride;
                if (upload_position_topology && strcmp(attr->name, "position") == 0)
                    node->u.upload.topology = (uint32_t)_visual_family_state(visual)->topology;
            }
            emitted_buffers[buffer_idx] = true;
            continue;
        }
        if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
            continue;
        if (_scene_attr_needs_scalar_color_upload(visual, attr))
        {
            _scene_emit_scalar_color_upload(figure, plan, visual, visual_index, attr);
            continue;
        }
        char resource_id[128];
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, attr->name, resource_id, sizeof(resource_id)))
        {
            continue;
        }
        uint64_t byte_offset = (uint64_t)attr->dirty_first_item * attr->item_size;
        uint64_t byte_size = (uint64_t)attr->dirty_item_count * attr->item_size;
        const void* data_ptr = (const uint8_t*)attr->data + byte_offset;
        DvzFramePlanResourceRole role = _scene_attr_frame_plan_role(attr->name);
        if (!_scene_frame_plan_upload_style_bytes(
                figure, visual, plan, resource_id, byte_offset, byte_size, attr->name, data_ptr))
        {
            continue;
        }
        _scene_attach_upload_metadata(
            plan, visual, visual_index, role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX,
            attr->item_count);
        if (upload_position_topology && strcmp(attr->name, "position") == 0)
        {
            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
            node->u.upload.topology = (uint32_t)_visual_family_state(visual)->topology;
        }
    }
}



/**
 * Emit the retained index buffer upload for one visual when it is dirty.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param emitted_buffers scene-buffer emission guards shared across the upload pass
 */
void _scene_emit_visual_index_buffer_upload(
    const DvzFigure* figure, DvzFramePlan* plan, const DvzVisual* visual, uint32_t visual_index,
    bool* emitted_buffers)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    ANN(visual);
    ANN(emitted_buffers);
    if (_visual_family_state(visual)->buffer == NULL || _visual_family_state(visual)->buffer->data == NULL)
        return;

    uint32_t buffer_idx = _scene_buffer_index(figure->scene, _visual_family_state(visual)->buffer);
    if (!_visual_family_state(visual)->buffer->dirty || buffer_idx == UINT32_MAX || emitted_buffers[buffer_idx])
        return;

    char buffer_resource_id[128];
    if (!_scene_resource_key_buffer(buffer_idx, buffer_resource_id, sizeof(buffer_resource_id)))
        return;
    dvz_frame_plan_upload_bytes(
        plan, buffer_resource_id, 0, _visual_family_state(visual)->buffer->desc.byte_size, "index",
        _visual_family_state(visual)->buffer->data);
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx,
        _visual_family_state(visual)->buffer->desc.stride > 0
            ? _visual_family_state(visual)->buffer->desc.byte_size / _visual_family_state(visual)->buffer->desc.stride
            : 0);
    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
    node->u.upload.item_stride = _visual_family_state(visual)->buffer->desc.stride;
    emitted_buffers[buffer_idx] = true;
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
void _scene_emit_visual_buffer_payloads(
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
                     figure, visual, plan, resource_id, 0, byte_size, payload->name,
                     payload->data))
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
            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
            node->u.upload.topology = position_topology;
        }
    }
}
