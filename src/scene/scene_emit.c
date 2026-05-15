/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan lowering                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "_scene_emit.h"
#include "_scene_resource_key.h"
#include "datoviz/drp2/runtime.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
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
    if (strcmp(attr_name, "color") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    if (strcmp(attr_name, "size") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
    if (strcmp(attr_name, "texcoords") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS;
    if (strcmp(attr_name, "normal") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_NORMAL;
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
static uint32_t _scene_attr_buffer_index(
    const DvzFigure* figure, const DvzVisual* visual, const char* attr_name)
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
static bool _scene_attr_resource_key(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    const char* attr_name, char* out_key, size_t out_size)
{
    ANN(figure);
    ANN(visual);
    ANN(attr_name);
    ANN(out_key);
    uint32_t buffer_idx = _scene_attr_buffer_index(figure, visual, attr_name);
    if (buffer_idx != UINT32_MAX)
        return _scene_resource_key_buffer(buffer_idx, out_key, out_size);
    return _scene_resource_key_visual_attr(visual_index, attr_name, out_key, out_size);
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
    DvzFramePlanResourceRole role, DvzFramePlanResourceKind kind, uint32_t buffer_index)
{
    ANN(plan);
    ANN(visual);
    DvzFramePlanUploadMeta metadata = {0};
    metadata.kind = kind;
    metadata.role = role;
    metadata.visual_type = (uint32_t)visual->type;
    metadata.visual_index = visual_index;
    metadata.buffer_index = buffer_index;
    return dvz_frame_plan_upload_metadata(plan, &metadata);
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
 * Emit dirty uploads for all panel-visible visuals in one figure.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 */
void _scene_emit_visual_uploads(DvzFigure* figure, DvzFramePlan* plan)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    bool emitted_buffers[DVZ_SCENE_MAX_BUFFERS] = {0};
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
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
                            plan, buffer_resource_id, 0, attr->buffer->desc.byte_size, attr->name,
                            attr->buffer->data);
                        _scene_attach_upload_metadata(
                            plan, visual, vidx, _scene_attr_frame_plan_role(attr->name),
                            DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx);
                        DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                        node->u.upload.external = !has_cpu_data;
                        node->u.upload.buffer_usage =
                            _scene_buffer_drp2_usage(attr->buffer->desc.usage);
                        node->u.upload.item_stride = attr->buffer->desc.stride;
                        if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                             visual->type == DVZ_VISUAL_TYPE_MESH ||
                             visual->type == DVZ_VISUAL_TYPE_PATH) &&
                            strcmp(attr->name, "position") == 0)
                        {
                            dvz_frame_plan_upload_set_topology(plan, (uint32_t)visual->topology);
                        }
                    }
                    emitted_buffers[buffer_idx] = true;
                    continue;
                }
                if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
                    continue;
                char resource_id[128];
                if (!_scene_resource_key_visual_attr(
                        vidx, attr->name, resource_id, sizeof(resource_id)))
                    continue;
                uint64_t byte_offset = (uint64_t)attr->dirty_first_item * attr->item_size;
                uint64_t byte_size = (uint64_t)attr->dirty_item_count * attr->item_size;
                const void* data_ptr = (const uint8_t*)attr->data + byte_offset;
                dvz_frame_plan_upload_bytes(
                    plan, resource_id, byte_offset, byte_size, attr->name, data_ptr);
                _scene_attach_upload_metadata(
                    plan, visual, vidx, _scene_attr_frame_plan_role(attr->name),
                    DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX);
                if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                     visual->type == DVZ_VISUAL_TYPE_MESH ||
                     visual->type == DVZ_VISUAL_TYPE_PATH) &&
                    strcmp(attr->name, "position") == 0)
                {
                    dvz_frame_plan_upload_set_topology(plan, (uint32_t)visual->topology);
                }
            }
            if (visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH)
            {
                int normal_idx = _attr_index(visual, "normal");
                bool has_normals =
                    normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                    visual->attrs[normal_idx].item_count > 0;
                if (has_normals && visual->primitive_shading_dirty)
                {
                    char shading_resource_id[128];
                    if (!_scene_resource_key_visual_attr(
                            vidx, "primitive_shading", shading_resource_id,
                            sizeof(shading_resource_id)))
                        continue;
                    dvz_frame_plan_upload_bytes(
                        plan, shading_resource_id, 0, sizeof(DvzPrimitiveShadingState),
                        "primitive_shading", &visual->primitive_shading);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_PRIMITIVE_SHADING,
                        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX);
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
                }
            }
            if (visual->buffer != NULL && visual->buffer->data != NULL)
            {
                uint32_t buffer_idx = _scene_buffer_index(figure->scene, visual->buffer);
                if (visual->buffer->dirty && buffer_idx != UINT32_MAX && !emitted_buffers[buffer_idx])
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
                        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx);
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage =
                        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
                    node->u.upload.item_stride = visual->buffer->desc.stride;
                    emitted_buffers[buffer_idx] = true;
                }
            }
            if (visual->type == DVZ_VISUAL_TYPE_IMAGE && visual->field != NULL &&
                (visual->texture.dirty || visual->field->dirty))
            {
                DvzFieldRegion upload_region = {0};
                const void* upload_data = NULL;
                if (!_scene_prepare_image_texture(visual, &upload_region, &upload_data))
                    continue;
                char tex_resource_id[128];
                if (!_scene_resource_key_visual_texture(
                        vidx, tex_resource_id, sizeof(tex_resource_id)))
                    continue;
                uint64_t bytes = 0;
                if (_field_region_byte_size(DVZ_FIELD_FORMAT_RGBA8_UNORM, &upload_region, &bytes))
                {
                    dvz_frame_plan_upload_bytes(
                        plan, tex_resource_id, 0, bytes, "texture", upload_data);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                        DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D, UINT32_MAX);
                    dvz_frame_plan_upload_set_texture_extent(
                        plan, upload_region.width, upload_region.height);
                    dvz_frame_plan_upload_set_texture_allocation_extent(
                        plan, visual->texture.width, visual->texture.height);
                    dvz_frame_plan_upload_set_texture_region(
                        plan, upload_region.x, upload_region.y);
                }
                else
                {
                    log_error("image visual texture upload size overflow");
                    continue;
                }
            }
            if (visual->type == DVZ_VISUAL_TYPE_VOLUME && visual->field != NULL &&
                (visual->texture.dirty || visual->field->dirty))
            {
                char tex_resource_id[128];
                if (!_scene_resource_key_visual_texture(
                        vidx, tex_resource_id, sizeof(tex_resource_id)))
                    continue;
                if (!_scene_emit_sampled_field_texture_upload(plan, tex_resource_id, visual->field))
                {
                    log_error("volume visual texture upload failed");
                    continue;
                }
                _scene_attach_upload_metadata(
                    plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                    DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D, UINT32_MAX);
            }
        }
    }
}



/**
 * Build typed FramePlan metadata for one retained visual.
 *
 * @param figure the parent figure
 * @param visual the retained visual
 * @param visual_index the visual index within the figure
 * @param metadata the output metadata
 * @return whether metadata was built
 */
static bool _scene_visual_frame_plan_metadata(
    const DvzFigure* figure, const DvzVisual* visual, uint32_t visual_index,
    DvzFramePlanVisualMeta* metadata)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(visual);
    ANN(metadata);

    dvz_memset(metadata, sizeof(DvzFramePlanVisualMeta), 0, sizeof(DvzFramePlanVisualMeta));
    metadata->has_metadata = true;
    metadata->visual_type = (uint32_t)visual->type;
    metadata->visual_index = visual_index;
    metadata->buffer_index = UINT32_MAX;
    metadata->topology = (uint32_t)visual->topology;
    metadata->alpha_mode = visual->alpha_mode;

    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position", metadata->position_id,
            sizeof(metadata->position_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "color", metadata->color_id, sizeof(metadata->color_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "size", metadata->size_id, sizeof(metadata->size_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "texcoords", metadata->texcoords_id,
            sizeof(metadata->texcoords_id)))
        return false;
    if (!_scene_resource_key_visual_texture(
            visual_index, metadata->texture_id, sizeof(metadata->texture_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "normal", metadata->normal_id,
            sizeof(metadata->normal_id)))
        return false;
    if (!_scene_resource_key_visual_attr(
            visual_index, "primitive_shading", metadata->shading_id, sizeof(metadata->shading_id)))
        return false;

    uint32_t buffer_index = _scene_buffer_index(figure->scene, visual->buffer);
    if (buffer_index != UINT32_MAX)
    {
        metadata->buffer_index = buffer_index;
        if (!_scene_resource_key_buffer(
                buffer_index, metadata->index_id, sizeof(metadata->index_id)))
            return false;
    }
    return true;
}



/**
 * Return whether an alpha mode belongs in the transparent WBOIT accumulation pass.
 *
 * @param mode the visual alpha mode
 * @return whether the visual should be planned through WBOIT
 */
static bool _scene_alpha_mode_is_wboit(DvzAlphaMode mode)
{
    return mode == DVZ_ALPHA_WBOIT;
}



/**
 * Return whether one retained visual should participate in depth-tested scene passes.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @return whether the visual requests scene depth
 */
static bool _scene_visual_requests_depth(const DvzVisual* visual, const DvzPanelAttach* attach)
{
    ANN(visual);
    ANN(attach);
    if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
        return false;
    return visual->type == DVZ_VISUAL_TYPE_PRIMITIVE || visual->type == DVZ_VISUAL_TYPE_MESH ||
           visual->type == DVZ_VISUAL_TYPE_PATH;
}



/**
 * Return whether a graph resource id is already declared.
 *
 * @param plan the frame plan
 * @param resource_id the resource id
 * @return whether the graph resource exists
 */
static bool _scene_frame_graph_has_resource(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, resource_id) == 0)
            return true;
    }
    return false;
}



/**
 * Add a graph resource unless it already exists.
 *
 * @param plan the frame plan
 * @param resource the graph resource descriptor
 * @return whether the resource exists or was added
 */
static bool
_scene_frame_graph_resource_once(DvzFramePlan* plan, const DvzFrameGraphResource* resource)
{
    ANN(plan);
    ANN(resource);
    if (_scene_frame_graph_has_resource(plan, resource->id))
        return true;
    return dvz_frame_plan_graph_resource(plan, resource);
}



/**
 * Fill a color attachment descriptor.
 *
 * @param attachment the graph attachment descriptor
 * @param resource_id the resource id
 * @param load_op the attachment load op
 * @param clear whether the attachment starts with a clear value
 */
static void _scene_frame_graph_color_attachment(
    DvzFrameGraphAttachment* attachment, const char* resource_id,
    DvzFrameGraphAttachmentLoadOp load_op, bool clear)
{
    ANN(attachment);
    ANN(resource_id);
    dvz_memset(attachment, sizeof(DvzFrameGraphAttachment), 0, sizeof(DvzFrameGraphAttachment));
    dvz_strlcpy(attachment->resource_id, resource_id, sizeof(attachment->resource_id));
    attachment->load_op = load_op;
    attachment->store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment->access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    attachment->clear_color[3] = clear ? 0.0f : 1.0f;
}



/**
 * Fill a depth attachment descriptor.
 *
 * @param attachment the graph attachment descriptor
 * @param resource_id the resource id
 * @param load_op the attachment load op
 * @param access the attachment access
 */
static void _scene_frame_graph_depth_attachment(
    DvzFrameGraphAttachment* attachment, const char* resource_id,
    DvzFrameGraphAttachmentLoadOp load_op, DvzFrameGraphAttachmentAccess access)
{
    ANN(attachment);
    ANN(resource_id);
    dvz_memset(attachment, sizeof(DvzFrameGraphAttachment), 0, sizeof(DvzFrameGraphAttachment));
    dvz_strlcpy(attachment->resource_id, resource_id, sizeof(attachment->resource_id));
    attachment->load_op = load_op;
    attachment->store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
    attachment->access = access;
    attachment->clear_depth = 1.0f;
}



/**
 * Emit graph descriptors for one WBOIT panel plan.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @param opaque_needs_depth whether the opaque pass writes depth
 * @param transparent_needs_depth whether the accumulation pass reads depth
 * @return whether graph descriptors were emitted
 */
static bool _scene_emit_wboit_frame_graph(
    DvzFramePlan* plan, const char* panel_id, bool opaque_needs_depth,
    bool transparent_needs_depth)
{
    ANN(plan);
    ANN(panel_id);

    char accum_id[DVZ_SCENE_LABEL_SIZE];
    char weight_id[DVZ_SCENE_LABEL_SIZE];
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    char opaque_pass_id[DVZ_SCENE_LABEL_SIZE];
    char accum_pass_id[DVZ_SCENE_LABEL_SIZE];
    char resolve_pass_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(accum_id, sizeof(accum_id), "%s.wboit.accum", panel_id);
    dvz_snprintf(weight_id, sizeof(weight_id), "%s.wboit.weight", panel_id);
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.depth", panel_id);
    dvz_snprintf(opaque_pass_id, sizeof(opaque_pass_id), "%s.opaque", panel_id);
    dvz_snprintf(accum_pass_id, sizeof(accum_pass_id), "%s.wboit.accum", panel_id);
    dvz_snprintf(resolve_pass_id, sizeof(resolve_pass_id), "%s.wboit.resolve", panel_id);

    DvzFrameGraphResource rt = {0};
    dvz_strlcpy(rt.id, "rt", sizeof(rt.id));
    rt.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
    rt.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    rt.usage_flags =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
    rt.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    if (!_scene_frame_graph_resource_once(plan, &rt))
        return false;

    DvzFrameGraphResource accum = {0};
    dvz_strlcpy(accum.id, accum_id, sizeof(accum.id));
    accum.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    accum.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    accum.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    accum.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                        DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    accum.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &accum))
        return false;

    DvzFrameGraphResource weight = {0};
    dvz_strlcpy(weight.id, weight_id, sizeof(weight.id));
    weight.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    weight.format = VK_FORMAT_R16_SFLOAT;
    weight.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    weight.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT |
                         DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    weight.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (!_scene_frame_graph_resource_once(plan, &weight))
        return false;

    bool shared_depth = opaque_needs_depth && transparent_needs_depth;
    if (shared_depth)
    {
        DvzFrameGraphResource depth = {0};
        dvz_strlcpy(depth.id, depth_id, sizeof(depth.id));
        depth.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        depth.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT |
                            DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        depth.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        if (!_scene_frame_graph_resource_once(plan, &depth))
            return false;
    }

    DvzFrameGraphAttachment color = {0};
    DvzFrameGraphAttachment depth = {0};
    DvzFrameGraphPass opaque = {0};
    dvz_strlcpy(opaque.id, opaque_pass_id, sizeof(opaque.id));
    dvz_strlcpy(opaque.panel_id, panel_id, sizeof(opaque.panel_id));
    dvz_strlcpy(opaque.work_label, "opaque", sizeof(opaque.work_label));
    opaque.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&opaque, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE);
        if (!dvz_frame_graph_pass_depth_attachment(&opaque, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &opaque))
        return false;

    DvzFrameGraphPass accum_pass = {0};
    dvz_strlcpy(accum_pass.id, accum_pass_id, sizeof(accum_pass.id));
    dvz_strlcpy(accum_pass.panel_id, panel_id, sizeof(accum_pass.panel_id));
    dvz_strlcpy(accum_pass.work_label, "wboit_accum", sizeof(accum_pass.work_label));
    accum_pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    _scene_frame_graph_color_attachment(
        &color, accum_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&accum_pass, &color))
        return false;
    _scene_frame_graph_color_attachment(
        &color, weight_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR, true);
    if (!dvz_frame_graph_pass_color_attachment(&accum_pass, &color))
        return false;
    if (shared_depth)
    {
        _scene_frame_graph_depth_attachment(
            &depth, depth_id, DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD,
            DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ);
        if (!dvz_frame_graph_pass_depth_attachment(&accum_pass, &depth))
            return false;
    }
    if (!dvz_frame_plan_graph_pass(plan, &accum_pass))
        return false;

    DvzFrameGraphPass resolve = {0};
    dvz_strlcpy(resolve.id, resolve_pass_id, sizeof(resolve.id));
    dvz_strlcpy(resolve.panel_id, panel_id, sizeof(resolve.panel_id));
    dvz_strlcpy(resolve.work_label, "wboit_resolve", sizeof(resolve.work_label));
    resolve.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    if (!dvz_frame_graph_pass_read(&resolve, accum_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED) ||
        !dvz_frame_graph_pass_read(&resolve, weight_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
        return false;
    _scene_frame_graph_color_attachment(
        &color, "rt", DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD, false);
    if (!dvz_frame_graph_pass_color_attachment(&resolve, &color))
        return false;
    return dvz_frame_plan_graph_pass(plan, &resolve);
}



/**
 * Configure common panel transform metadata on a render node.
 *
 * @param node the render node
 * @param panel_apply_mvp the panel APPLY MVP
 * @param panel_viewport the panel pixel viewport
 */
static void _scene_configure_panel_render_node(
    DvzFramePlanNode* node, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport)
{
    ANN(node);
    ANN(panel_apply_mvp);
    ANN(panel_viewport);
    node->u.render.has_mvp = true;
    node->u.render.apply_mvp = *panel_apply_mvp;
    node->u.render.has_viewport = true;
    node->u.render.viewport = *panel_viewport;
}



/**
 * Append a panel render pass with common panel transform metadata.
 *
 * @param plan the destination frame plan
 * @param panel_id the panel id
 * @param render_target_id the render target id
 * @param desc the normalized panel rectangle
 * @param pass_role the render pass role
 * @param panel_apply_mvp the panel APPLY MVP
 * @param panel_viewport the panel pixel viewport
 * @return the appended render node, or NULL on failure
 */
static DvzFramePlanNode* _scene_begin_panel_render_pass(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc,
    DvzFramePlanRenderPassRole pass_role, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport)
{
    ANN(plan);
    ANN(panel_id);
    ANN(render_target_id);
    if (!dvz_frame_plan_render_panel_role(plan, panel_id, render_target_id, false, desc, pass_role))
        return NULL;
    DvzFramePlanNode* node = dvz_frame_plan_last_render_node(plan);
    if (node != NULL)
        _scene_configure_panel_render_node(node, panel_apply_mvp, panel_viewport);
    return node;
}



/**
 * Append one visual to the active render pass.
 *
 * @param figure the parent figure
 * @param plan the destination frame plan
 * @param node the active render node
 * @param visual the visual
 * @param attach the panel attachment
 * @param visual_index the visual index within the figure
 * @return whether the visual was appended
 */
static bool _scene_append_visual_to_render_pass(
    const DvzFigure* figure, DvzFramePlan* plan, DvzFramePlanNode* node, const DvzVisual* visual,
    const DvzPanelAttach* attach, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(node);
    ANN(visual);
    ANN(attach);

    char visual_id[64];
    uint32_t buffer_idx = _scene_buffer_index(figure->scene, visual->buffer);
    if (buffer_idx != UINT32_MAX)
    {
        if (!_scene_resource_key_visual_indexed(
                visual_index, buffer_idx, visual_id, sizeof(visual_id)))
            return false;
    }
    else
    {
        if (!_scene_resource_key_visual(visual_index, visual_id, sizeof(visual_id)))
            return false;
    }
    if (!dvz_frame_plan_render_visual(plan, visual_id))
        return false;

    DvzFramePlanVisualMeta metadata = {0};
    if (_scene_visual_frame_plan_metadata(figure, visual, visual_index, &metadata))
        dvz_frame_plan_render_visual_metadata(plan, &metadata);
    node->u.render.controller_modes[node->u.render.visual_count - 1] = attach->controller_mode;
    return true;
}



/**
 * Emit one panel render node into a frame plan.
 *
 * @param figure the parent figure
 * @param panel_index the panel index within the figure
 * @param plan the destination frame plan
 * @param figure_id the stable figure identifier
 */
void _scene_emit_panel_render(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id)
{
    ANN(figure);
    ANN(plan);
    ANN(figure_id);
    ASSERT(panel_index < figure->panel_count);
    DvzPanel* panel = &figure->panels[panel_index];

    char panel_id[64];
    dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, panel_index);
    uint32_t drawable_count = 0;
    for (uint32_t vi = 0; vi < panel->visual_count; vi++)
    {
        DvzVisual* visual = panel->visuals[vi].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_VOLUME)
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        int pos_idx = _attr_index(visual, "position");
        if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
            drawable_count++;
        else
            log_warn(
                "%s visual (index %u) has no 'position' data — it will render nothing",
                _visual_type_name(visual->type), vidx);
    }

    if (drawable_count == 0)
    {
        dvz_frame_plan_clear_panel(plan, panel_id, "rt", panel->desc);
        return;
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS];
    _scene_panel_visual_order(panel, order);

    DvzMVP panel_apply_mvp;
    _scene_panel_apply_mvp(panel, &panel_apply_mvp);
    DvzSceneViewportUniform panel_viewport = {0};
    _scene_panel_pixel_rect(
        panel, &panel_viewport.x, &panel_viewport.y, &panel_viewport.width,
        &panel_viewport.height);

    DvzFramePlanNode* opaque_node = NULL;
    DvzFramePlanNode* transparent_node = NULL;
    bool has_transparent = false;
    bool opaque_needs_depth = false;
    bool transparent_needs_depth = false;
    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        int pos_idx = _attr_index(visual, "position");
        if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
            continue;

        bool transparent = _scene_alpha_mode_is_wboit(visual->alpha_mode);
        if (transparent)
        {
            has_transparent = true;
            transparent_needs_depth =
                transparent_needs_depth || _scene_visual_requests_depth(visual, attach);
            continue;
        }

        if (opaque_node == NULL)
        {
            opaque_node = _scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
                &panel_apply_mvp, &panel_viewport);
            if (opaque_node == NULL)
                continue;
        }
        (void)_scene_append_visual_to_render_pass(
            figure, plan, opaque_node, visual, attach, vidx);
        opaque_needs_depth = opaque_needs_depth || _scene_visual_requests_depth(visual, attach);
    }

    if (opaque_node == NULL && has_transparent)
    {
        opaque_node = _scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &panel_apply_mvp,
            &panel_viewport);
    }

    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (!_scene_alpha_mode_is_wboit(visual->alpha_mode))
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        int pos_idx = _attr_index(visual, "position");
        if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
            continue;

        if (transparent_node == NULL)
        {
            transparent_node = _scene_begin_panel_render_pass(
                plan, panel_id, "rt.wboit_accum", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &panel_apply_mvp,
                &panel_viewport);
            if (transparent_node == NULL)
                continue;
        }
        (void)_scene_append_visual_to_render_pass(
            figure, plan, transparent_node, visual, attach, vidx);
        transparent_needs_depth =
            transparent_needs_depth || _scene_visual_requests_depth(visual, attach);
    }

    if (transparent_node != NULL)
    {
        (void)_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
            &panel_apply_mvp, &panel_viewport);
        if (!_scene_emit_wboit_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
            log_error("failed to emit WBOIT FramePlan graph for panel %s", panel_id);
    }
}
