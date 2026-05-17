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
#include "_overflow.h"
#include "_scene.h"
#include "_scene_emit.h"
#include "_scene_resource_key.h"
#include "_technique.h"
#include "_visual_pipeline.h"
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
    if (strcmp(attr_name, "position_start") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START;
    if (strcmp(attr_name, "position_end") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END;
    if (strcmp(attr_name, "color") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    if (strcmp(attr_name, "size") == 0)
        return DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
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
 * Return whether one visual has CPU-side data for an attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return whether the attribute exists and has data
 */
static bool _scene_visual_has_attr_data(const DvzVisual* visual, const char* attr_name)
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
static bool _scene_visual_needs_material_params(const DvzVisual* visual)
{
    ANN(visual);
    bool point_like =
        visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
        visual->type == DVZ_VISUAL_TYPE_MARKER;
    if (point_like)
    {
        return visual->material.depth_cue_enabled ||
               (visual->type == DVZ_VISUAL_TYPE_POINT &&
                visual->material.point_style_enabled) ||
               visual->type == DVZ_VISUAL_TYPE_MARKER;
    }
    if (visual->type == DVZ_VISUAL_TYPE_SPHERE)
        return true;
    if (visual->type == DVZ_VISUAL_TYPE_PATH)
        return _scene_visual_has_attr_data(visual, "line_width");
    if (visual->type == DVZ_VISUAL_TYPE_PRIMITIVE || visual->type == DVZ_VISUAL_TYPE_MESH)
        return _scene_visual_has_attr_data(visual, "normal");
    return false;
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
 * Resolve the resource key used by one panel's EDL uniform.
 *
 * @param panel_id the panel id
 * @param out_key output resource key
 * @param out_size output resource key capacity
 * @return whether the key was resolved
 */
static bool _scene_edl_params_resource_key(
    const char* panel_id, char* out_key, size_t out_size)
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
static bool _scene_ssao_params_resource_key(
    const char* panel_id, char* out_key, size_t out_size)
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
 * Return whether one segment visual has all dense attributes required for rendering.
 *
 * @param visual the segment visual
 * @param out_count output segment count
 * @return whether all segment attributes are present
 */
static bool _segment_required_attrs(const DvzVisual* visual, uint64_t* out_count)
{
    ANN(visual);
    ANN(out_count);
    *out_count = 0;
    const char* names[] = {"position_start", "position_end", "color", "line_width"};
    uint64_t count = 0;
    for (uint32_t i = 0; i < 4; i++)
    {
        int idx = _attr_index(visual, names[i]);
        if (idx < 0 || visual->attrs[idx].data == NULL || visual->attrs[idx].item_count == 0)
            return false;
        if (i == 0)
            count = visual->attrs[idx].item_count;
        else if (visual->attrs[idx].item_count != count)
            return false;
    }
    *out_count = count;
    return true;
}


/**
 * Resize a segment cache array.
 *
 * @param ptr input/output array pointer
 * @param count item count
 * @param item_size byte size of one item
 * @return whether the allocation succeeded
 */
static bool _segment_cache_resize(void** ptr, uint64_t count, uint64_t item_size)
{
    ANN(ptr);
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, item_size, &bytes) || bytes > SIZE_MAX)
        return false;
    void* grown = dvz_realloc(*ptr, (size_t)bytes);
    if (grown == NULL && bytes > 0)
        return false;
    *ptr = grown;
    return true;
}


/**
 * Rebuild one segment visual's derived four-vertex/six-index upload cache.
 *
 * @param visual the segment visual
 * @return whether the cache is ready for upload
 */
static bool _segment_cache_rebuild(DvzVisual* visual)
{
    ANN(visual);
    uint64_t item_count = 0;
    if (!_segment_required_attrs(visual, &item_count))
        return false;
    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 4, &vertex_count) ||
        _dvz_mul_u64_overflows(item_count, 6, &index_count) ||
        vertex_count > UINT32_MAX)
    {
        log_error("segment visual item count is too large");
        return false;
    }

    DvzSegmentGpuCache* cache = &visual->segment.gpu;
    if (!_segment_cache_resize((void**)&cache->position_start, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->position_end, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->color, vertex_count, sizeof(DvzColor)) ||
        !_segment_cache_resize((void**)&cache->line_width, vertex_count, sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->indices, index_count, sizeof(uint32_t)))
    {
        log_error("failed to allocate segment visual derived GPU cache");
        return false;
    }

    const float* position_start =
        (const float*)visual->attrs[_attr_index(visual, "position_start")].data;
    const float* position_end =
        (const float*)visual->attrs[_attr_index(visual, "position_end")].data;
    const DvzColor* color =
        (const DvzColor*)visual->attrs[_attr_index(visual, "color")].data;
    const float* line_width =
        (const float*)visual->attrs[_attr_index(visual, "line_width")].data;

    for (uint64_t i = 0; i < item_count; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            uint64_t dst = 4 * i + j;
            dvz_memcpy(
                &cache->position_start[3 * dst], 3 * sizeof(float), &position_start[3 * i],
                3 * sizeof(float));
            dvz_memcpy(
                &cache->position_end[3 * dst], 3 * sizeof(float), &position_end[3 * i],
                3 * sizeof(float));
            dvz_memcpy(&cache->color[dst], sizeof(DvzColor), &color[i], sizeof(DvzColor));
            cache->line_width[dst] = line_width[i];
        }
        cache->indices[6 * i + 0] = (uint32_t)(4 * i + 0);
        cache->indices[6 * i + 1] = (uint32_t)(4 * i + 1);
        cache->indices[6 * i + 2] = (uint32_t)(4 * i + 2);
        cache->indices[6 * i + 3] = (uint32_t)(4 * i + 0);
        cache->indices[6 * i + 4] = (uint32_t)(4 * i + 2);
        cache->indices[6 * i + 5] = (uint32_t)(4 * i + 3);
    }
    cache->item_count = item_count;
    cache->vertex_count = vertex_count;
    cache->index_count = index_count;
    cache->dirty = false;
    return true;
}


/**
 * Emit derived GPU uploads for one segment visual.
 *
 * @param plan the destination frame plan
 * @param visual the segment visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_segment_uploads(DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(plan);
    ANN(visual);
    DvzSegmentGpuCache* cache = &visual->segment.gpu;
    bool dirty = cache->dirty;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        dirty = dirty || visual->attrs[i].dirty_item_count > 0;
    if (!dirty)
        return;
    if (!_segment_cache_rebuild(visual))
        return;

    const struct
    {
        const char* name;
        const void* data;
        uint32_t item_size;
        DvzFramePlanResourceRole role;
    } uploads[] = {
        {"position_start", cache->position_start, 3 * sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START},
        {"position_end", cache->position_end, 3 * sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END},
        {"color", cache->color, sizeof(DvzColor), DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR},
        {"line_width", cache->line_width, sizeof(float), DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH},
    };

    for (uint32_t i = 0; i < 4; i++)
    {
        char resource_id[128];
        if (!_scene_resource_key_visual_attr(
                visual_index, uploads[i].name, resource_id, sizeof(resource_id)))
            continue;
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(cache->vertex_count, uploads[i].item_size, &byte_size))
            continue;
        dvz_frame_plan_upload_bytes(
            plan, resource_id, 0, byte_size, uploads[i].name, uploads[i].data);
        _scene_attach_upload_metadata(
            plan, visual, visual_index, uploads[i].role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
            UINT32_MAX);
    }

    char index_id[128];
    if (_scene_resource_key_visual_attr(visual_index, "index", index_id, sizeof(index_id)))
    {
        uint64_t byte_size = 0;
        if (!_dvz_mul_u64_overflows(cache->index_count, sizeof(uint32_t), &byte_size))
        {
            dvz_frame_plan_upload_bytes(plan, index_id, 0, byte_size, "index", cache->indices);
            _scene_attach_upload_metadata(
                plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
                DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX);
            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
            node->u.upload.buffer_usage =
                DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
            node->u.upload.item_stride = sizeof(uint32_t);
        }
    }
}


/**
 * Return whether one path visual has dense attributes for stroked lowering.
 *
 * @param visual the path visual
 * @param out_count output point count
 * @return whether all stroked path attributes are present
 */
static bool _path_required_attrs(const DvzVisual* visual, uint64_t* out_count)
{
    ANN(visual);
    ANN(out_count);
    *out_count = 0;
    const char* names[] = {"position", "color", "line_width"};
    uint64_t count = 0;
    for (uint32_t i = 0; i < 3; i++)
    {
        int idx = _attr_index(visual, names[i]);
        if (idx < 0 || visual->attrs[idx].data == NULL || visual->attrs[idx].item_count == 0)
            return false;
        if (i == 0)
            count = visual->attrs[idx].item_count;
        else if (visual->attrs[idx].item_count != count)
            return false;
    }
    *out_count = count;
    return true;
}


/**
 * Rebuild one path visual's derived segment upload cache.
 *
 * @param visual the path visual
 * @return whether the cache is ready for upload
 */
static bool _path_cache_rebuild(DvzVisual* visual)
{
    ANN(visual);
    uint64_t point_count = 0;
    if (!_path_required_attrs(visual, &point_count) || point_count < 2)
        return false;

    uint64_t segment_count = 0;
    uint64_t consumed = 0;
    if (visual->path.subpath_count > 0)
    {
        for (uint32_t i = 0; i < visual->path.subpath_count; i++)
        {
            uint32_t length = visual->path.subpath_lengths[i];
            consumed += length;
            if (length >= 2)
                segment_count += length - 1;
        }
        if (consumed != point_count)
        {
            log_error("path subpath lengths must sum to the path point count");
            return false;
        }
    }
    else
    {
        segment_count = point_count - 1;
    }

    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(segment_count, 4, &vertex_count) ||
        _dvz_mul_u64_overflows(segment_count, 6, &index_count) ||
        vertex_count > UINT32_MAX)
    {
        log_error("path visual segment count is too large");
        return false;
    }

    DvzPathGpuCache* cache = &visual->path.gpu;
    if (!_segment_cache_resize((void**)&cache->position_start, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->position_end, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->color, vertex_count, sizeof(DvzColor)) ||
        !_segment_cache_resize((void**)&cache->line_width, vertex_count, sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->indices, index_count, sizeof(uint32_t)))
    {
        log_error("failed to allocate path visual derived GPU cache");
        return false;
    }

    const float* position = (const float*)visual->attrs[_attr_index(visual, "position")].data;
    const DvzColor* color = (const DvzColor*)visual->attrs[_attr_index(visual, "color")].data;
    const float* line_width =
        (const float*)visual->attrs[_attr_index(visual, "line_width")].data;

    uint64_t segment = 0;
    uint64_t offset = 0;
    uint32_t subpath_count = visual->path.subpath_count > 0 ? visual->path.subpath_count : 1;
    for (uint32_t sp = 0; sp < subpath_count; sp++)
    {
        uint32_t length = visual->path.subpath_count > 0 ?
                              visual->path.subpath_lengths[sp] :
                              (uint32_t)point_count;
        for (uint32_t i = 0; i + 1 < length; i++)
        {
            uint64_t i0 = offset + i;
            uint64_t i1 = i0 + 1;
            for (uint32_t j = 0; j < 4; j++)
            {
                uint64_t dst = 4 * segment + j;
                dvz_memcpy(
                    &cache->position_start[3 * dst], 3 * sizeof(float), &position[3 * i0],
                    3 * sizeof(float));
                dvz_memcpy(
                    &cache->position_end[3 * dst], 3 * sizeof(float), &position[3 * i1],
                    3 * sizeof(float));
                dvz_memcpy(&cache->color[dst], sizeof(DvzColor), &color[i0], sizeof(DvzColor));
                cache->line_width[dst] = 0.5f * (line_width[i0] + line_width[i1]);
            }
            cache->indices[6 * segment + 0] = (uint32_t)(4 * segment + 0);
            cache->indices[6 * segment + 1] = (uint32_t)(4 * segment + 1);
            cache->indices[6 * segment + 2] = (uint32_t)(4 * segment + 2);
            cache->indices[6 * segment + 3] = (uint32_t)(4 * segment + 0);
            cache->indices[6 * segment + 4] = (uint32_t)(4 * segment + 2);
            cache->indices[6 * segment + 5] = (uint32_t)(4 * segment + 3);
            segment++;
        }
        offset += length;
    }
    cache->point_count = point_count;
    cache->segment_count = segment_count;
    cache->vertex_count = vertex_count;
    cache->index_count = index_count;
    cache->dirty = false;
    return true;
}


/**
 * Emit derived GPU uploads for one stroked path visual.
 *
 * @param plan the destination frame plan
 * @param visual the path visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_path_uploads(DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(plan);
    ANN(visual);
    DvzPathGpuCache* cache = &visual->path.gpu;
    bool dirty = cache->dirty;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        dirty = dirty || visual->attrs[i].dirty_item_count > 0;
    if (!dirty)
        return;
    if (!_path_cache_rebuild(visual))
        return;

    const struct
    {
        const char* name;
        const void* data;
        uint32_t item_size;
        DvzFramePlanResourceRole role;
    } uploads[] = {
        {"position_start", cache->position_start, 3 * sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START},
        {"position_end", cache->position_end, 3 * sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END},
        {"color", cache->color, sizeof(DvzColor), DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR},
        {"line_width", cache->line_width, sizeof(float), DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH},
    };

    for (uint32_t i = 0; i < 4; i++)
    {
        char resource_id[128];
        if (!_scene_resource_key_visual_attr(
                visual_index, uploads[i].name, resource_id, sizeof(resource_id)))
            continue;
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(cache->vertex_count, uploads[i].item_size, &byte_size))
            continue;
        dvz_frame_plan_upload_bytes(
            plan, resource_id, 0, byte_size, uploads[i].name, uploads[i].data);
        _scene_attach_upload_metadata(
            plan, visual, visual_index, uploads[i].role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
            UINT32_MAX);
    }

    char index_id[128];
    if (_scene_resource_key_visual_attr(visual_index, "index", index_id, sizeof(index_id)))
    {
        uint64_t byte_size = 0;
        if (!_dvz_mul_u64_overflows(cache->index_count, sizeof(uint32_t), &byte_size))
        {
            dvz_frame_plan_upload_bytes(plan, index_id, 0, byte_size, "index", cache->indices);
            _scene_attach_upload_metadata(
                plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
                DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX);
            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
            node->u.upload.buffer_usage =
                DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
            node->u.upload.item_stride = sizeof(uint32_t);
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
            if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
            {
                _scene_emit_segment_uploads(plan, visual, vidx);
                if (visual->material_params_dirty)
                {
                    char material_resource_id[128];
                    if (!_scene_resource_key_visual_attr(
                            vidx, "material_params", material_resource_id,
                            sizeof(material_resource_id)))
                        continue;
                    dvz_frame_plan_upload_bytes(
                        plan, material_resource_id, 0, sizeof(DvzSceneMaterialParams),
                        "material_params", &visual->material_params);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
                        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX);
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
                }
                continue;
            }
            if (visual->type == DVZ_VISUAL_TYPE_PATH &&
                _scene_visual_has_attr_data(visual, "line_width"))
            {
                _scene_emit_path_uploads(plan, visual, vidx);
                if (visual->material_params_dirty)
                {
                    char material_resource_id[128];
                    if (!_scene_resource_key_visual_attr(
                            vidx, "material_params", material_resource_id,
                            sizeof(material_resource_id)))
                        continue;
                    dvz_frame_plan_upload_bytes(
                        plan, material_resource_id, 0, sizeof(DvzSceneMaterialParams),
                        "material_params", &visual->material_params);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
                        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX);
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
                }
                continue;
            }
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
                             visual->type == DVZ_VISUAL_TYPE_PATH ||
                             visual->type == DVZ_VISUAL_TYPE_SPHERE) &&
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
                     visual->type == DVZ_VISUAL_TYPE_PATH ||
                     visual->type == DVZ_VISUAL_TYPE_SPHERE) &&
                    strcmp(attr->name, "position") == 0)
                {
                    dvz_frame_plan_upload_set_topology(plan, (uint32_t)visual->topology);
                }
            }
            if (
                visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                visual->type == DVZ_VISUAL_TYPE_MARKER ||
                visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH ||
                visual->type == DVZ_VISUAL_TYPE_SPHERE)
            {
                if (_scene_visual_needs_material_params(visual) && visual->material_params_dirty)
                {
                    char material_resource_id[128];
                    if (!_scene_resource_key_visual_attr(
                            vidx, "material_params", material_resource_id,
                            sizeof(material_resource_id)))
                        continue;
                    dvz_frame_plan_upload_bytes(
                        plan, material_resource_id, 0, sizeof(DvzSceneMaterialParams),
                        "material_params", &visual->material_params);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_MATERIAL_PARAMS,
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
                DvzFieldRegion upload_region = {0};
                const void* upload_data = NULL;
                uint32_t texture_format = 0;
                uint32_t bytes_per_texel = 0;
                uint64_t bytes = 0;
                if (!_scene_prepare_volume_texture(
                        visual, &upload_region, &upload_data, &texture_format,
                        &bytes_per_texel) ||
                    !_field_region_byte_size(
                        texture_format == VK_FORMAT_R8G8B8A8_UNORM ?
                            DVZ_FIELD_FORMAT_RGBA8_UNORM :
                            visual->field->desc.format,
                        &upload_region, &bytes) ||
                    !dvz_frame_plan_upload_bytes(
                        plan, tex_resource_id, 0, bytes, "field", upload_data) ||
                    !dvz_frame_plan_upload_metadata(
                        plan,
                        &(DvzFramePlanUploadMeta){
                            .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
                            .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                            .visual_index = UINT32_MAX,
                            .buffer_index = UINT32_MAX,
                        }) ||
                    !dvz_frame_plan_upload_set_texture_format(
                        plan, texture_format, bytes_per_texel) ||
                    !dvz_frame_plan_upload_set_texture_3d_extent(
                        plan, upload_region.width, upload_region.height, upload_region.depth) ||
                    !dvz_frame_plan_upload_set_texture_3d_allocation_extent(
                        plan, visual->field->desc.width, visual->field->desc.height,
                        visual->field->desc.depth) ||
                    !dvz_frame_plan_upload_set_texture_3d_region(
                        plan, upload_region.x, upload_region.y, upload_region.z))
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
bool _scene_visual_frame_plan_metadata(
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
    metadata->depth_test_enabled = visual->depth_test_enabled;
    metadata->depth_cue_enabled = visual->material.depth_cue_enabled;
    metadata->point_style_enabled =
        visual->type == DVZ_VISUAL_TYPE_POINT && visual->material.point_style_enabled;
    metadata->scale_index = _scene_scale_index(figure->scene, visual->scale);
    metadata->scene_occluder = visual->scene_occluder;
    metadata->scene_occluded = visual->scene_occluded;

    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position", metadata->position_id,
            sizeof(metadata->position_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position_start", metadata->position_start_id,
            sizeof(metadata->position_start_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "position_end", metadata->position_end_id,
            sizeof(metadata->position_end_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "color", metadata->color_id, sizeof(metadata->color_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "size", metadata->size_id, sizeof(metadata->size_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "angle", metadata->angle_id,
            sizeof(metadata->angle_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "shape", metadata->shape_id,
            sizeof(metadata->shape_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "line_width", metadata->line_width_id,
            sizeof(metadata->line_width_id)))
        return false;
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "texcoords", metadata->texcoords_id,
            sizeof(metadata->texcoords_id)))
        return false;
    if (!_scene_resource_key_visual_texture(
            visual_index, metadata->texture_id, sizeof(metadata->texture_id)))
        return false;
    if (visual->type == DVZ_VISUAL_TYPE_VOLUME)
    {
        metadata->has_volume = true;
        metadata->volume_state = visual->volume;
        metadata->volume_occluded = visual->volume_occluded;
        metadata->volume_transfer_rgba =
            visual->field != NULL && visual->field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM;
        metadata->volume_transfer_rgba =
            metadata->volume_transfer_rgba ||
            (visual->scale != NULL && visual->scale->colormap != NULL);
        if (visual->field != NULL)
        {
            metadata->field_format = (uint32_t)visual->field->desc.format;
            metadata->field_width = visual->field->desc.width;
            metadata->field_height = visual->field->desc.height;
            metadata->field_depth = visual->field->desc.depth;
        }
        dvz_strlcpy(
            metadata->volume_texture_id, metadata->texture_id, sizeof(metadata->volume_texture_id));
    }
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "normal", metadata->normal_id,
            sizeof(metadata->normal_id)))
        return false;
    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT || _scene_visual_needs_material_params(visual))
    {
        if (!_scene_resource_key_visual_attr(
                visual_index, "material_params", metadata->material_id,
                sizeof(metadata->material_id)))
            return false;
    }

    uint32_t buffer_index = _scene_buffer_index(figure->scene, visual->buffer);
    if (buffer_index != UINT32_MAX)
    {
        metadata->buffer_index = buffer_index;
        if (!_scene_resource_key_buffer(
                buffer_index, metadata->index_id, sizeof(metadata->index_id)))
            return false;
    }
    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
        (visual->type == DVZ_VISUAL_TYPE_PATH &&
         _scene_visual_has_attr_data(visual, "line_width")))
    {
        if (!_scene_resource_key_visual_attr(
                visual_index, "index", metadata->index_id, sizeof(metadata->index_id)))
            return false;
    }
    return true;
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
 * Return whether a graph pass should sample the panel volume-occlusion texture.
 *
 * @param pass the graph pass
 * @return whether the pass renders ordinary visual fragments
 */
static bool _scene_graph_pass_can_sample_visual_occlusion(const DvzFrameGraphPass* pass)
{
    ANN(pass);
    return strcmp(pass->work_label, "opaque") == 0 ||
           strcmp(pass->work_label, "transparent_blend") == 0 ||
           strcmp(pass->work_label, "wboit_accum") == 0 ||
           strcmp(pass->work_label, "depth_peel_init") == 0 ||
           strcmp(pass->work_label, "depth_peel_iter") == 0;
}


/**
 * Add sampled volume-occlusion reads to panel visual render passes.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether all required reads were added
 */
static bool _scene_add_volume_occlusion_reads(DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.volume_occlusion.depth", panel_id);

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (strcmp(pass->panel_id, panel_id) != 0 ||
            !_scene_graph_pass_can_sample_visual_occlusion(pass))
            continue;
        bool already = false;
        for (uint32_t j = 0; j < pass->read_count; j++)
            already = already || strcmp(pass->reads[j].resource_id, depth_id) == 0;
        if (!already &&
            !dvz_frame_graph_pass_read(pass, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
    }
    return true;
}


/**
 * Add sampled scene-occlusion reads to panel visual render passes.
 *
 * @param plan the frame plan
 * @param panel_id the panel id
 * @return whether all required reads were added
 */
static bool _scene_add_scene_occlusion_reads(DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);
    char depth_id[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(depth_id, sizeof(depth_id), "%s.scene_occlusion.depth", panel_id);

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (strcmp(pass->panel_id, panel_id) != 0 ||
            !_scene_graph_pass_can_sample_visual_occlusion(pass))
            continue;
        bool already = false;
        for (uint32_t j = 0; j < pass->read_count; j++)
            already = already || strcmp(pass->reads[j].resource_id, depth_id) == 0;
        if (!already &&
            !dvz_frame_graph_pass_read(pass, depth_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return false;
    }
    return true;
}


/**
 * Return whether the panel has a visible target for volume occlusion.
 *
 * @param panel the panel
 * @return whether a volume occlusion prepass should be emitted
 */
static bool _scene_panel_has_visible_volume_occlusion_target(const DvzPanel* panel)
{
    ANN(panel);
    if (!panel->volume_occlusion_enabled || panel->volume_occluder_visual == NULL ||
        !panel->volume_occlusion.enabled || !panel->volume_occluder_visual->visible)
        return false;

    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || !visual->visible || !visual->volume_occluded ||
            visual == panel->volume_occluder_visual)
            continue;
        int pos_idx = _attr_index(
            visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
        if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
            return true;
    }
    return false;
}


/**
 * Return whether one panel visual is visible and drawable.
 *
 * @param visual the visual
 * @return whether the visual has position data
 */
static bool _scene_visual_is_visible_drawable(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible)
        return false;
    int pos_idx = _attr_index(
        visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
    return pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0;
}


/**
 * Return whether the panel has visible scene occluder and occluded targets.
 *
 * @param panel the panel
 * @return whether a scene occlusion prepass should be emitted
 */
static bool _scene_panel_has_visible_scene_occlusion_target(const DvzPanel* panel)
{
    ANN(panel);
    if (!panel->scene_occlusion_enabled || !panel->scene_occlusion.enabled)
        return false;

    bool has_occluder = false;
    bool has_occluded = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (!_scene_visual_is_visible_drawable(visual))
            continue;
        has_occluder = has_occluder || visual->scene_occluder;
        has_occluded = has_occluded || visual->scene_occluded;
    }
    return has_occluder && has_occluded;
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
    const DvzPanelAttach* attach, uint32_t visual_index,
    const DvzSceneOcclusionDesc* scene_occlusion, const DvzVolumeOcclusionDesc* volume_occlusion)
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
    (void)plan;
    if (node->u.render.visual_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
        return false;
    uint32_t slot = node->u.render.visual_count++;
    dvz_strlcpy(node->u.render.visuals[slot], visual_id, sizeof(node->u.render.visuals[slot]));

    DvzFramePlanVisualMeta metadata = {0};
    if (_scene_visual_frame_plan_metadata(figure, visual, visual_index, &metadata))
    {
        if (metadata.has_volume && volume_occlusion != NULL)
        {
            metadata.has_volume_occlusion = true;
            metadata.volume_occlusion = *volume_occlusion;
        }
        if (metadata.scene_occluded && scene_occlusion != NULL)
        {
            metadata.has_scene_occlusion = true;
            metadata.scene_occlusion = *scene_occlusion;
        }
        dvz_memcpy(
            &node->u.render.visual_metadata[slot], sizeof(DvzFramePlanVisualMeta), &metadata,
            sizeof(DvzFramePlanVisualMeta));
        node->u.render.visual_metadata[slot].has_metadata = true;
    }
    node->u.render.controller_modes[slot] = attach->controller_mode;
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
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        const char* position_attr =
            visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position";
        int pos_idx = _attr_index(visual, position_attr);
        if (pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0)
            drawable_count++;
        else
            log_warn(
                "%s visual (index %u) has no '%s' data — it will render nothing",
                _visual_type_name(visual->type), vidx, position_attr);
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

    DvzFramePlanNode* scene_occlusion_node = NULL;
    bool scene_occlusion_enabled = _scene_panel_has_visible_scene_occlusion_target(panel);
    if (scene_occlusion_enabled)
    {
        scene_occlusion_node = _scene_begin_panel_render_pass(
            plan, panel_id, "rt.scene_occlusion.depth", panel->desc,
            DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, &panel_apply_mvp, &panel_viewport);
        if (scene_occlusion_node != NULL)
        {
            for (uint32_t k = 0; k < panel->visual_count; k++)
            {
                uint32_t vi = order[k];
                DvzPanelAttach* attach = &panel->visuals[vi];
                DvzVisual* visual = attach->visual;
                if (!_scene_visual_is_visible_drawable(visual) || !visual->scene_occluder)
                    continue;
                uint32_t vidx = 0;
                if (!_figure_visual_index(figure, visual, &vidx))
                    continue;
                const DvzVolumeOcclusionDesc* volume_occlusion =
                    visual == panel->volume_occluder_visual && panel->volume_occlusion_enabled
                        ? &panel->volume_occlusion
                        : NULL;
                (void)_scene_append_visual_to_render_pass(
                    figure, plan, scene_occlusion_node, visual, attach, vidx,
                    &panel->scene_occlusion, volume_occlusion);
            }
        }
    }

    DvzFramePlanNode* volume_occlusion_node = NULL;
    bool volume_occlusion_enabled = _scene_panel_has_visible_volume_occlusion_target(panel);
    if (volume_occlusion_enabled)
    {
        uint32_t occluder_index = 0;
        if (_figure_visual_index(figure, panel->volume_occluder_visual, &occluder_index))
        {
            volume_occlusion_node = _scene_begin_panel_render_pass(
                plan, panel_id, "rt.volume_occlusion.depth", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION, &panel_apply_mvp,
                &panel_viewport);
            if (volume_occlusion_node != NULL)
            {
                DvzPanelAttach attach = {
                    .visual = panel->volume_occluder_visual,
                    .z_layer = 0,
                    .controller_mode = DVZ_CONTROLLER_APPLY,
                };
                (void)_scene_append_visual_to_render_pass(
                    figure, plan, volume_occlusion_node, panel->volume_occluder_visual, &attach,
                    occluder_index, NULL, &panel->volume_occlusion);
            }
        }
    }

    DvzFramePlanNode* opaque_node = NULL;
    DvzFramePlanNode* gbuffer_node = NULL;
    DvzFramePlanNode* transparent_node = NULL;
    DvzFramePlanNode* depth_peel_init_node = NULL;
    DvzFramePlanNode* depth_peel_iter_node = NULL;
    DvzFramePlanNode* depth_peel_composite_node = NULL;
    DvzFramePlanNode* blended_node = NULL;
    DvzFramePlanNode* edl_node = NULL;
    DvzFramePlanNode* ssao_node = NULL;
    DvzFramePlanNode* ssao_blur_node = NULL;
    DvzFramePlanNode* ssao_composite_node = NULL;
    DvzSceneGBufferPlan gbuffer = {0};
    _scene_technique_gbuffer_plan_init(&gbuffer);
    bool gbuffer_enabled = _scene_technique_gbuffer_enabled(figure->scene, panel);
    const DvzSceneSsaoTechniqueState* ssao_state =
        _scene_technique_ssao_state(figure->scene, panel);
    const DvzSceneMsaaTechniqueState* msaa_state =
        _scene_technique_msaa_state(figure->scene, panel);
    bool ssao_enabled = ssao_state != NULL && ssao_state->enabled;
    bool gbuffer_required = gbuffer_enabled || ssao_enabled;
    const DvzSceneEdlTechniqueState* edl_state =
        _scene_technique_edl_state(figure->scene, panel);
    bool edl_enabled = edl_state != NULL && edl_state->enabled;
    bool edl_has_depth_producer = false;
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
        int pos_idx = _attr_index(
            visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
        if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
            continue;

        DvzSceneVisualPassCaps caps = {0};
        if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
            continue;
        if (!caps.draws_in_opaque_pass)
        {
            has_transparent = true;
            transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
            continue;
        }

        if (gbuffer_required && _scene_technique_gbuffer_plan_add_visual(&gbuffer, visual, attach))
        {
            if (gbuffer_node == NULL)
            {
                gbuffer_node = _scene_begin_panel_render_pass(
                    plan, panel_id, "rt.gbuffer.normal", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER, &panel_apply_mvp, &panel_viewport);
                if (gbuffer_node == NULL)
                    continue;
            }
            (void)_scene_append_visual_to_render_pass(
                figure, plan, gbuffer_node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
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
            figure, plan, opaque_node, visual, attach, vidx,
            scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
            volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        bool edl_depth_visual = edl_enabled && caps.eligible_for_depth_postprocess;
        opaque_needs_depth = opaque_needs_depth || caps.writes_depth || edl_depth_visual;
        edl_has_depth_producer = edl_has_depth_producer || edl_depth_visual;
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
        DvzSceneVisualPassCaps caps = {0};
        if (!_scene_visual_pass_caps_from_visual(visual, attach, &caps))
            continue;
        if (caps.draws_in_opaque_pass)
            continue;
        uint32_t vidx = 0;
        if (!_figure_visual_index(figure, visual, &vidx))
            continue;
        int pos_idx = _attr_index(
            visual, visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position");
        if (pos_idx < 0 || visual->attrs[pos_idx].item_count == 0)
            continue;

        if (caps.draws_in_transparent_blend_pass)
        {
            if (blended_node == NULL)
            {
                blended_node = _scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &panel_apply_mvp,
                    &panel_viewport);
                if (blended_node == NULL)
                    continue;
            }
            (void)_scene_append_visual_to_render_pass(
                figure, plan, blended_node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
            continue;
        }

        if (caps.draws_in_depth_peel_pass)
        {
            if (depth_peel_init_node == NULL)
            {
                uint32_t first_depth_peel_node = plan->count;
                depth_peel_init_node = _scene_begin_panel_render_pass(
                    plan, panel_id, "rt.depth_peel_init", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, &panel_apply_mvp,
                    &panel_viewport);
                depth_peel_iter_node = _scene_begin_panel_render_pass(
                    plan, panel_id, "rt.depth_peel_iter", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, &panel_apply_mvp,
                    &panel_viewport);
                depth_peel_composite_node = _scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, &panel_apply_mvp,
                    &panel_viewport);
                if (depth_peel_init_node == NULL || depth_peel_iter_node == NULL ||
                    depth_peel_composite_node == NULL)
                    continue;
                depth_peel_init_node = &plan->nodes[first_depth_peel_node];
                depth_peel_iter_node = &plan->nodes[first_depth_peel_node + 1];
                depth_peel_composite_node = &plan->nodes[first_depth_peel_node + 2];
                if (depth_peel_init_node == NULL || depth_peel_iter_node == NULL ||
                    depth_peel_composite_node == NULL)
                    continue;
            }
            (void)_scene_append_visual_to_render_pass(
                figure, plan, depth_peel_init_node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            (void)_scene_append_visual_to_render_pass(
                figure, plan, depth_peel_iter_node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
            continue;
        }

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
            figure, plan, transparent_node, visual, attach, vidx,
            scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
            volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
    }

    if (scene_occlusion_node != NULL &&
        !_scene_technique_emit_scene_occlusion_frame_graph(plan, panel_id))
        log_error("failed to emit scene occlusion FramePlan graph for panel %s", panel_id);

    if (transparent_node != NULL)
    {
        if (volume_occlusion_node != NULL &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
            log_error("failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
        (void)_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
            &panel_apply_mvp, &panel_viewport);
        if (gbuffer_required && gbuffer_node != NULL &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
            log_error("failed to emit G-buffer FramePlan graph for panel %s", panel_id);
        if (!_scene_technique_emit_wboit_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
            log_error("failed to emit WBOIT FramePlan graph for panel %s", panel_id);
        if (blended_node != NULL)
        {
            char blend_pass_id[DVZ_SCENE_LABEL_SIZE];
            dvz_snprintf(
                blend_pass_id, sizeof(blend_pass_id), "%s.transparent_blend", panel_id);
            DvzFrameGraphAttachment color = {0};
            dvz_strlcpy(color.resource_id, "rt", sizeof(color.resource_id));
            color.load_op = DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
            color.store_op = DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE;
            color.access = DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
            color.clear_color[3] = 1.0f;
            DvzFrameGraphPass blend = {0};
            dvz_strlcpy(blend.id, blend_pass_id, sizeof(blend.id));
            dvz_strlcpy(blend.panel_id, panel_id, sizeof(blend.panel_id));
            dvz_strlcpy(blend.work_label, "transparent_blend", sizeof(blend.work_label));
            blend.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
            if (!dvz_frame_graph_pass_color_attachment(&blend, &color) ||
                !dvz_frame_plan_graph_pass(plan, &blend))
                log_error("failed to emit blended FramePlan graph for panel %s", panel_id);
        }
    }
    else if (depth_peel_init_node != NULL)
    {
        if (volume_occlusion_node != NULL &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
            log_error("failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
        if (gbuffer_required && gbuffer_node != NULL &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
            log_error("failed to emit G-buffer FramePlan graph for panel %s", panel_id);
        if (!_scene_technique_emit_depth_peel_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
            log_error("failed to emit depth-peeling FramePlan graph for panel %s", panel_id);
    }
    else if (blended_node != NULL)
    {
        if (volume_occlusion_node != NULL &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
            log_error("failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
        if (gbuffer_required && gbuffer_node != NULL &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
            log_error("failed to emit G-buffer FramePlan graph for panel %s", panel_id);
        if (!_scene_technique_emit_blended_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
            log_error("failed to emit blended FramePlan graph for panel %s", panel_id);
    }
    else if (
        opaque_node != NULL &&
        (opaque_needs_depth || volume_occlusion_node != NULL || scene_occlusion_node != NULL))
    {
        if (volume_occlusion_node != NULL &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
            log_error("failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
        if (gbuffer_required && gbuffer_node != NULL &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
            log_error("failed to emit G-buffer FramePlan graph for panel %s", panel_id);
        if (edl_enabled && edl_has_depth_producer)
        {
            char edl_params_key[DVZ_SCENE_LABEL_SIZE];
            if (_scene_edl_params_resource_key(panel_id, edl_params_key, sizeof(edl_params_key)))
            {
                _scene_technique_edl_uniform(edl_state, &panel->techniques.edl.uniform);
                if (dvz_frame_plan_upload_bytes(
                        plan, edl_params_key, 0, sizeof(DvzSceneEdlUniform), "edl_params",
                        &panel->techniques.edl.uniform))
                {
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                                  DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                                  DVZ_DRP2_BUFFER_USAGE_COPY_DST;
                }
            }
            edl_node = _scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
                &panel_apply_mvp, &panel_viewport);
            if (edl_node == NULL ||
                !_scene_technique_emit_edl_frame_graph(plan, panel_id))
                log_error("failed to emit EDL FramePlan graph for panel %s", panel_id);
        }
        else if ((gbuffer_node != NULL || volume_occlusion_node != NULL ||
                  scene_occlusion_node != NULL ||
                  (!ssao_enabled && msaa_state != NULL)) &&
                 !_scene_technique_emit_opaque_frame_graph(
                     plan, panel_id, opaque_needs_depth, msaa_state))
            log_error("failed to emit opaque FramePlan graph for panel %s", panel_id);
    }
    if (ssao_enabled && gbuffer_node != NULL && gbuffer.producer_count > 0)
    {
        char ssao_params_key[DVZ_SCENE_LABEL_SIZE];
        if (_scene_ssao_params_resource_key(
                panel_id, ssao_params_key, sizeof(ssao_params_key)))
        {
            _scene_technique_ssao_uniform(
                ssao_state, &panel_apply_mvp, &panel_viewport, &panel->techniques.ssao.uniform);
            if (dvz_frame_plan_upload_bytes(
                    plan, ssao_params_key, 0, sizeof(DvzSceneSsaoUniform), "ssao_params",
                    &panel->techniques.ssao.uniform))
            {
                DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                              DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                              DVZ_DRP2_BUFFER_USAGE_COPY_DST;
            }
        }
        ssao_node = _scene_begin_panel_render_pass(
            plan, panel_id, "rt.ssao.occlusion", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
            &panel_apply_mvp, &panel_viewport);
        if (ssao_state->blur_enabled)
        {
            ssao_blur_node = _scene_begin_panel_render_pass(
                plan, panel_id, "rt.ssao.blur", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR,
                &panel_apply_mvp, &panel_viewport);
        }
        ssao_composite_node = _scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
            &panel_apply_mvp, &panel_viewport);
        if (ssao_node == NULL || (ssao_state->blur_enabled && ssao_blur_node == NULL) ||
            ssao_composite_node == NULL ||
            !_scene_technique_emit_ssao_frame_graph(plan, panel_id, &gbuffer, ssao_state))
            log_error("failed to emit SSAO FramePlan graph for panel %s", panel_id);
    }
    if (volume_occlusion_node != NULL &&
        !_scene_add_volume_occlusion_reads(plan, panel_id))
        log_error("failed to add volume occlusion FramePlan reads for panel %s", panel_id);
    if (scene_occlusion_node != NULL &&
        !_scene_add_scene_occlusion_reads(plan, panel_id))
        log_error("failed to add scene occlusion FramePlan reads for panel %s", panel_id);
}
