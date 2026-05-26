/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual upload emission                                                                 */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
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
#include "datoviz/drp2/runtime.h"
#include "render_contract.h"


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define DVZ_PATH_VERTEX_SIDE_NEGATIVE 0x01u
#define DVZ_PATH_VERTEX_ENDPOINT_END  0x02u
#define DVZ_PATH_VERTEX_HAS_PREV      0x04u
#define DVZ_PATH_VERTEX_HAS_NEXT      0x08u
#define DVZ_PATH_VERTEX_SUBPATH_START 0x10u
#define DVZ_PATH_VERTEX_SUBPATH_END   0x20u


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
    bool point_like = visual->type == DVZ_VISUAL_TYPE_POINT ||
                      visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                      visual->type == DVZ_VISUAL_TYPE_MARKER;
    if (point_like)
    {
        return visual->material.depth_cue_enabled ||
               (visual->type == DVZ_VISUAL_TYPE_POINT && visual->material.point_style_enabled) ||
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
           role == DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH;
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
        _dvz_mul_u64_overflows(item_count, 6, &index_count) || vertex_count > UINT32_MAX)
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
    const DvzColor* color = (const DvzColor*)visual->attrs[_attr_index(visual, "color")].data;
    const float* line_width = (const float*)visual->attrs[_attr_index(visual, "line_width")].data;

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
static void _scene_emit_segment_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
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
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, uploads[i].name, resource_id, sizeof(resource_id)))
            continue;
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(cache->vertex_count, uploads[i].item_size, &byte_size))
            continue;
        if (!_scene_frame_plan_upload_style_bytes(
                figure, plan, resource_id, 0, byte_size, uploads[i].name, uploads[i].data,
                uploads[i].role))
            continue;
        _scene_attach_upload_metadata(
            plan, visual, visual_index, uploads[i].role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
            UINT32_MAX, cache->vertex_count);
    }

    char index_id[128];
    if (_scene_visual_attr_resource_key(
            figure, visual, visual_index, "index", index_id, sizeof(index_id)))
    {
        uint64_t byte_size = 0;
        if (!_dvz_mul_u64_overflows(cache->index_count, sizeof(uint32_t), &byte_size))
        {
            dvz_frame_plan_upload_bytes(plan, index_id, 0, byte_size, "index", cache->indices);
            _scene_attach_upload_metadata(
                plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
                DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX, cache->index_count);
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
 * Return the Euclidean distance between two path points.
 *
 * @param position flat vec3 position array
 * @param i0 first point index
 * @param i1 second point index
 * @return the point distance in visual coordinates
 */
static float _path_point_distance(const float* position, uint64_t i0, uint64_t i1)
{
    ANN(position);
    float dx = position[3 * i1 + 0] - position[3 * i0 + 0];
    float dy = position[3 * i1 + 1] - position[3 * i0 + 1];
    float dz = position[3 * i1 + 2] - position[3 * i0 + 2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}


/**
 * Return whether one subpath repeats its first point as a closed-ring sentinel.
 *
 * @param position flat vec3 position array
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @return whether the first and last points are equal
 */
static bool _path_subpath_is_closed(const float* position, uint64_t offset, uint32_t length)
{
    ANN(position);
    if (length < 3)
        return false;

    const uint64_t first = offset;
    const uint64_t last = offset + length - 1;
    return position[3 * first + 0] == position[3 * last + 0] &&
           position[3 * first + 1] == position[3 * last + 1] &&
           position[3 * first + 2] == position[3 * last + 2];
}


/**
 * Return the previous adjacency point for one path endpoint.
 *
 * @param point_idx endpoint point index
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @param closed whether the subpath repeats its first point at the end
 * @return previous adjacency point index
 */
static uint64_t _path_prev_index(
    uint64_t point_idx, uint64_t offset, uint32_t length, bool closed)
{
    if (closed && point_idx == offset)
        return offset + length - 2;
    if (point_idx > offset)
        return point_idx - 1;
    return point_idx;
}


/**
 * Return the next adjacency point for one path endpoint.
 *
 * @param point_idx endpoint point index
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @param closed whether the subpath repeats its first point at the end
 * @return next adjacency point index
 */
static uint64_t _path_next_index(
    uint64_t point_idx, uint64_t offset, uint32_t length, bool closed)
{
    const uint64_t end = offset + length;
    if (closed && point_idx + 1 == end)
        return offset + 1;
    if (point_idx + 1 < end)
        return point_idx + 1;
    return point_idx;
}


/**
 * Return packed path vertex flags for one derived stroke vertex.
 *
 * @param side_negative whether the vertex is on the negative normal side
 * @param endpoint_end whether the vertex belongs to the segment end endpoint
 * @param has_prev whether the endpoint has a previous path point
 * @param has_next whether the endpoint has a next path point
 * @param subpath_start whether the endpoint is the first point in an open subpath
 * @param subpath_end whether the endpoint is the last point in an open subpath
 * @return packed path vertex flags
 */
static uint32_t _path_vertex_flags(
    bool side_negative, bool endpoint_end, bool has_prev, bool has_next, bool subpath_start,
    bool subpath_end)
{
    uint32_t flags = 0;
    flags |= side_negative ? DVZ_PATH_VERTEX_SIDE_NEGATIVE : 0u;
    flags |= endpoint_end ? DVZ_PATH_VERTEX_ENDPOINT_END : 0u;
    flags |= has_prev ? DVZ_PATH_VERTEX_HAS_PREV : 0u;
    flags |= has_next ? DVZ_PATH_VERTEX_HAS_NEXT : 0u;
    flags |= subpath_start ? DVZ_PATH_VERTEX_SUBPATH_START : 0u;
    flags |= subpath_end ? DVZ_PATH_VERTEX_SUBPATH_END : 0u;
    return flags;
}


/**
 * Rebuild one path visual's derived adjacency-style upload cache.
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
        _dvz_mul_u64_overflows(segment_count, 6, &index_count) || vertex_count > UINT32_MAX)
    {
        log_error("path visual segment count is too large");
        return false;
    }

    DvzPathGpuCache* cache = &visual->path.gpu;
    if (!_segment_cache_resize((void**)&cache->position_prev, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->position_curr, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->position_next, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->color, vertex_count, sizeof(DvzColor)) ||
        !_segment_cache_resize((void**)&cache->line_width, vertex_count, sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->path_flags, vertex_count, sizeof(uint32_t)) ||
        !_segment_cache_resize((void**)&cache->path_distance, vertex_count, sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->indices, index_count, sizeof(uint32_t)))
    {
        log_error("failed to allocate path visual derived GPU cache");
        return false;
    }

    const float* position = (const float*)visual->attrs[_attr_index(visual, "position")].data;
    const DvzColor* color = (const DvzColor*)visual->attrs[_attr_index(visual, "color")].data;
    const float* line_width = (const float*)visual->attrs[_attr_index(visual, "line_width")].data;

    uint64_t segment = 0;
    uint64_t offset = 0;
    uint32_t subpath_count = visual->path.subpath_count > 0 ? visual->path.subpath_count : 1;
    for (uint32_t sp = 0; sp < subpath_count; sp++)
    {
        uint32_t length = visual->path.subpath_count > 0 ? visual->path.subpath_lengths[sp]
                                                         : (uint32_t)point_count;
        bool closed = _path_subpath_is_closed(position, offset, length);
        float cumulative = 0.0f;
        for (uint32_t i = 0; i + 1 < length; i++)
        {
            uint64_t i0 = offset + i;
            uint64_t i1 = i0 + 1;
            float edge_length = _path_point_distance(position, i0, i1);
            for (uint32_t j = 0; j < 4; j++)
            {
                bool endpoint_end = j >= 2;
                bool side_negative = j == 1 || j == 2;
                uint64_t point_idx = endpoint_end ? i1 : i0;
                uint64_t prev_idx = _path_prev_index(point_idx, offset, length, closed);
                uint64_t next_idx = _path_next_index(point_idx, offset, length, closed);
                bool has_prev = prev_idx != point_idx;
                bool has_next = next_idx != point_idx;
                bool subpath_start = !closed && point_idx == offset;
                bool subpath_end = !closed && point_idx + 1 == offset + length;
                uint64_t dst = 4 * segment + j;
                dvz_memcpy(
                    &cache->position_prev[3 * dst], 3 * sizeof(float), &position[3 * prev_idx],
                    3 * sizeof(float));
                dvz_memcpy(
                    &cache->position_curr[3 * dst], 3 * sizeof(float), &position[3 * point_idx],
                    3 * sizeof(float));
                dvz_memcpy(
                    &cache->position_next[3 * dst], 3 * sizeof(float), &position[3 * next_idx],
                    3 * sizeof(float));
                dvz_memcpy(
                    &cache->color[dst], sizeof(DvzColor), &color[point_idx], sizeof(DvzColor));
                cache->line_width[dst] = line_width[point_idx];
                cache->path_flags[dst] = _path_vertex_flags(
                    side_negative, endpoint_end, has_prev, has_next, subpath_start, subpath_end);
                cache->path_distance[dst] = endpoint_end ? cumulative + edge_length : cumulative;
            }
            cache->indices[6 * segment + 0] = (uint32_t)(4 * segment + 0);
            cache->indices[6 * segment + 1] = (uint32_t)(4 * segment + 1);
            cache->indices[6 * segment + 2] = (uint32_t)(4 * segment + 2);
            cache->indices[6 * segment + 3] = (uint32_t)(4 * segment + 0);
            cache->indices[6 * segment + 4] = (uint32_t)(4 * segment + 2);
            cache->indices[6 * segment + 5] = (uint32_t)(4 * segment + 3);
            segment++;
            cumulative += edge_length;
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
static void _scene_emit_path_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
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
        {"position_start", cache->position_prev, 3 * sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_START},
        {"position", cache->position_curr, 3 * sizeof(float), DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION},
        {"position_end", cache->position_next, 3 * sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION_END},
        {"color", cache->color, sizeof(DvzColor), DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR},
        {"line_width", cache->line_width, sizeof(float), DVZ_FRAME_PLAN_RESOURCE_ROLE_LINE_WIDTH},
        {"path_flags", cache->path_flags, sizeof(uint32_t), DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_FLAGS},
        {"path_distance", cache->path_distance, sizeof(float),
         DVZ_FRAME_PLAN_RESOURCE_ROLE_PATH_DISTANCE},
    };

    for (uint32_t i = 0; i < 7; i++)
    {
        char resource_id[128];
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, uploads[i].name, resource_id, sizeof(resource_id)))
            continue;
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(cache->vertex_count, uploads[i].item_size, &byte_size))
            continue;
        if (!_scene_frame_plan_upload_style_bytes(
                figure, plan, resource_id, 0, byte_size, uploads[i].name, uploads[i].data,
                uploads[i].role))
            continue;
        _scene_attach_upload_metadata(
            plan, visual, visual_index, uploads[i].role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
            UINT32_MAX, cache->vertex_count);
    }

    char index_id[128];
    if (_scene_visual_attr_resource_key(
            figure, visual, visual_index, "index", index_id, sizeof(index_id)))
    {
        uint64_t byte_size = 0;
        if (!_dvz_mul_u64_overflows(cache->index_count, sizeof(uint32_t), &byte_size))
        {
            dvz_frame_plan_upload_bytes(plan, index_id, 0, byte_size, "index", cache->indices);
            _scene_attach_upload_metadata(
                plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
                DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX, cache->index_count);
            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
            node->u.upload.buffer_usage =
                DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
            node->u.upload.item_stride = sizeof(uint32_t);
        }
    }
}


/**
 * Return whether an image visual uses per-item rectangles.
 *
 * @param visual the image visual
 * @return whether the visual has an extent attribute
 */
bool _scene_image_uses_generated_quads(const DvzVisual* visual)
{
    ANN(visual);
    return (visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_LABELS) &&
           _scene_visual_has_attr_data(visual, "extent");
}


/**
 * Rebuild one image visual's derived six-vertex rectangle upload cache.
 *
 * @param visual the image visual
 * @return whether the cache is ready for upload
 */
static bool _image_cache_rebuild(DvzVisual* visual)
{
    ANN(visual);
    if (!_scene_image_uses_generated_quads(visual) ||
        !_scene_visual_has_attr_data(visual, "position"))
    {
        log_error("image-like visual per-item rectangles require position and extent attributes");
        return false;
    }

    DvzVisualAttr* position_attr = &visual->attrs[_attr_index(visual, "position")];
    DvzVisualAttr* extent_attr = &visual->attrs[_attr_index(visual, "extent")];
    const uint64_t item_count = position_attr->item_count;
    if (item_count == 0 || extent_attr->item_count != item_count)
    {
        log_error("image visual position and extent item counts must match");
        return false;
    }

    uint64_t vertex_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 6, &vertex_count) || vertex_count > UINT32_MAX)
    {
        log_error("image visual item count is too large");
        return false;
    }

    DvzImageGpuCache* cache = &visual->image_gpu;
    if (!_segment_cache_resize((void**)&cache->position, vertex_count, 3 * sizeof(float)) ||
        !_segment_cache_resize((void**)&cache->texcoords, vertex_count, 2 * sizeof(float)))
    {
        log_error("failed to allocate image visual derived GPU cache");
        return false;
    }

    const float* position = (const float*)position_attr->data;
    const float* extent = (const float*)extent_attr->data;
    const int anchor_idx = _attr_index(visual, "anchor");
    const int tex_rect_idx = _attr_index(visual, "tex_rect");
    const float* anchor = anchor_idx >= 0 ? (const float*)visual->attrs[anchor_idx].data : NULL;
    const float* tex_rect =
        tex_rect_idx >= 0 ? (const float*)visual->attrs[tex_rect_idx].data : NULL;

    for (uint64_t i = 0; i < item_count; i++)
    {
        const float x = position[3 * i + 0];
        const float y = position[3 * i + 1];
        const float z = position[3 * i + 2];
        const float w = extent[2 * i + 0];
        const float h = extent[2 * i + 1];
        const float ax = anchor != NULL ? anchor[2 * i + 0] : 0.0f;
        const float ay = anchor != NULL ? anchor[2 * i + 1] : 0.0f;

        const float x0 = x - 0.5f * (ax + 1.0f) * w;
        const float x1 = x0 + w;
        const float y0 = y - 0.5f * (ay + 1.0f) * h;
        const float y1 = y0 + h;

        const float u0 = tex_rect != NULL ? tex_rect[4 * i + 0] : 0.0f;
        const float v0 = tex_rect != NULL ? tex_rect[4 * i + 1] : 0.0f;
        const float u1 = tex_rect != NULL ? tex_rect[4 * i + 2] : 1.0f;
        const float v1 = tex_rect != NULL ? tex_rect[4 * i + 3] : 1.0f;

        const float quad_pos[6][3] = {
            {x0, y0, z}, {x0, y1, z}, {x1, y0, z}, {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
        };
        /* Generated image quads use top-origin UV bounds, matching RGBA row upload order. */
        const float quad_uv[6][2] = {
            {u0, v1}, {u0, v0}, {u1, v1}, {u1, v1}, {u0, v0}, {u1, v0},
        };

        for (uint32_t j = 0; j < 6; j++)
        {
            uint64_t dst = 6 * i + j;
            dvz_memcpy(
                &cache->position[3 * dst], 3 * sizeof(float), quad_pos[j], 3 * sizeof(float));
            dvz_memcpy(
                &cache->texcoords[2 * dst], 2 * sizeof(float), quad_uv[j], 2 * sizeof(float));
        }
    }

    cache->item_count = item_count;
    cache->vertex_count = vertex_count;
    cache->dirty = false;
    return true;
}


/**
 * Emit derived GPU uploads for one per-item image visual.
 *
 * @param plan the destination frame plan
 * @param visual the image visual
 * @param visual_index the scene visual index
 */
static void _scene_emit_image_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    DvzImageGpuCache* cache = &visual->image_gpu;
    bool dirty = cache->dirty;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        dirty = dirty || visual->attrs[i].dirty_item_count > 0;
    if (!dirty)
        return;
    if (!_image_cache_rebuild(visual))
        return;

    const struct
    {
        const char* name;
        const void* data;
        uint32_t item_size;
        DvzFramePlanResourceRole role;
    } uploads[] = {
        {"position", cache->position, 3 * sizeof(float), DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION},
        {"texcoords", cache->texcoords, 2 * sizeof(float), DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXCOORDS},
    };

    for (uint32_t i = 0; i < 2; i++)
    {
        char resource_id[128];
        if (!_scene_visual_attr_resource_key(
                figure, visual, visual_index, uploads[i].name, resource_id, sizeof(resource_id)))
            continue;
        uint64_t byte_size = 0;
        if (_dvz_mul_u64_overflows(cache->vertex_count, uploads[i].item_size, &byte_size))
            continue;
        dvz_frame_plan_upload_bytes(
            plan, resource_id, 0, byte_size, uploads[i].name, uploads[i].data);
        _scene_attach_upload_metadata(
            plan, visual, visual_index, uploads[i].role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
            UINT32_MAX, cache->vertex_count);
        if (strcmp(uploads[i].name, "position") == 0)
            dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
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
 * Interpolate retained volume alpha stops at one normalized value.
 *
 * @param state volume state
 * @param t normalized transfer coordinate
 * @return alpha in [0, 1]
 */
static float _volume_alpha_at(const DvzVolumeState* state, double t)
{
    ANN(state);
    if (state->alpha_stop_count == 0)
        return (float)t;
    if (t <= state->alpha_stops[0].position)
        return state->alpha_stops[0].alpha;
    uint32_t last = state->alpha_stop_count - 1;
    if (t >= state->alpha_stops[last].position)
        return state->alpha_stops[last].alpha;
    for (uint32_t i = 1; i < state->alpha_stop_count; i++)
    {
        const DvzVolumeAlphaStop* lo = &state->alpha_stops[i - 1];
        const DvzVolumeAlphaStop* hi = &state->alpha_stops[i];
        if (t <= hi->position)
        {
            double denom = hi->position - lo->position;
            double u = denom > 0.0 ? (t - lo->position) / denom : 0.0;
            return (float)((1.0 - u) * lo->alpha + u * hi->alpha);
        }
    }
    return state->alpha_stops[last].alpha;
}


/**
 * Build the 256x1 RGBA transfer texture for a scalar volume.
 *
 * @param visual the volume visual
 * @param out_data transfer texture bytes
 * @return whether transfer bytes are available
 */
static bool _scene_prepare_volume_transfer_texture(DvzVisual* visual, const void** out_data)
{
    ANN(visual);
    ANN(out_data);
    *out_data = NULL;
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL ||
        !_field_format_is_scalar(visual->field->desc.format))
        return false;

    const uint64_t size = 256ull * 4ull;
    if (visual->texture.rgba == NULL || visual->texture.rgba_size != size)
    {
        if (visual->texture.rgba != NULL)
            dvz_free(visual->texture.rgba);
        visual->texture.rgba = dvz_calloc(size, 1);
        if (visual->texture.rgba == NULL)
        {
            visual->texture.rgba_size = 0;
            log_error("volume transfer texture allocation failed");
            return false;
        }
        visual->texture.rgba_size = size;
    }

    uint8_t* rgba = (uint8_t*)visual->texture.rgba;
    const DvzColormap* colormap =
        visual->scale != NULL && visual->scale->colormap != NULL ? visual->scale->colormap : NULL;
    for (uint32_t i = 0; i < 256; i++)
    {
        double t = (double)i / 255.0;
        if (colormap != NULL)
            _scene_color_from_colormap(colormap, t, &rgba[4 * i]);
        else
        {
            uint8_t v = (uint8_t)i;
            rgba[4 * i + 0] = v;
            rgba[4 * i + 1] = v;
            rgba[4 * i + 2] = v;
            rgba[4 * i + 3] = 255;
        }
        float alpha = _volume_alpha_at(&visual->volume, t);
        rgba[4 * i + 3] = (uint8_t)((float)rgba[4 * i + 3] * alpha + 0.5f);
    }
    *out_data = visual->texture.rgba;
    return true;
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
    *params = visual->material_params;
    if ((visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_MARKER) &&
        visual->material.point_style_enabled)
    {
        params->params[0] *= _scene_screen_scale(figure);
    }
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

    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
    {
        _scene_emit_segment_uploads(figure, plan, visual, visual_index);
        *out_finished_visual = true;
        return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
    }
    if (visual->type == DVZ_VISUAL_TYPE_PATH && _scene_visual_has_attr_data(visual, "line_width"))
    {
        _scene_emit_path_uploads(figure, plan, visual, visual_index);
        *out_finished_visual = true;
        return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
    }
    if (_scene_image_uses_generated_quads(visual))
    {
        _scene_emit_image_uploads(figure, plan, visual, visual_index);
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
         visual->type != DVZ_VISUAL_TYPE_LABELS) ||
        visual->field == NULL || (!visual->texture.dirty && !visual->field->dirty))
    {
        return;
    }

    if (visual->type == DVZ_VISUAL_TYPE_LABELS)
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
    DvzFieldRegion upload_region = {0};
    const void* upload_data = NULL;
    uint32_t texture_format = 0;
    uint32_t bytes_per_texel = 0;
    uint64_t bytes = 0;
    if (!_scene_prepare_volume_texture(
            visual, &upload_region, &upload_data, &texture_format, &bytes_per_texel) ||
        !_field_region_byte_size(
            texture_format == VK_FORMAT_R8G8B8A8_UNORM ? DVZ_FIELD_FORMAT_RGBA8_UNORM
                                                       : visual->field->desc.format,
            &upload_region, &bytes) ||
        !dvz_frame_plan_upload_bytes(plan, tex_resource_id, 0, bytes, "field", upload_data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_3D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .visual_index = UINT32_MAX,
                .buffer_index = UINT32_MAX,
            }) ||
        !dvz_frame_plan_upload_set_texture_format(plan, texture_format, bytes_per_texel) ||
        !dvz_frame_plan_upload_set_texture_3d_extent(
            plan, upload_region.width, upload_region.height, upload_region.depth) ||
        !dvz_frame_plan_upload_set_texture_3d_allocation_extent(
            plan, visual->field->desc.width, visual->field->desc.height,
            visual->field->desc.depth) ||
        !dvz_frame_plan_upload_set_texture_3d_region(
            plan, upload_region.x, upload_region.y, upload_region.z))
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
static void
_scene_emit_volume_transfer_texture_upload(DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
    ANN(plan);
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME || visual->field == NULL ||
        !_field_format_is_scalar(visual->field->desc.format))
    {
        return;
    }

    char transfer_resource_id[128];
    const void* transfer_data = NULL;
    if (!_scene_resource_key_volume_transfer(
            visual_index, transfer_resource_id, sizeof(transfer_resource_id)) ||
        !_scene_prepare_volume_transfer_texture(visual, &transfer_data) ||
        !dvz_frame_plan_upload_bytes(
            plan, transfer_resource_id, 0, 256ull * 4ull, "volume_transfer", transfer_data) ||
        !dvz_frame_plan_upload_metadata(
            plan,
            &(DvzFramePlanUploadMeta){
                .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
                .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                .visual_index = UINT32_MAX,
                .buffer_index = UINT32_MAX,
            }) ||
        !dvz_frame_plan_upload_set_texture_format(plan, VK_FORMAT_R8G8B8A8_UNORM, 4) ||
        !dvz_frame_plan_upload_set_texture_extent(plan, 256, 1) ||
        !dvz_frame_plan_upload_set_texture_allocation_extent(plan, 256, 1) ||
        !dvz_frame_plan_upload_set_texture_region(plan, 0, 0))
    {
        log_error("volume transfer texture upload failed");
        return;
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
