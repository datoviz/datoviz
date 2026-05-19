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
#include <stdarg.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_emit.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "render_contract.h"
#include "datoviz/drp2/runtime.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Report a panel graph-emission failure to logs and the optional diagnostic report.
 *
 * @param report optional diagnostic report
 * @param fmt printf-style diagnostic format
 */
static void _scene_emit_graph_report(DvzDiagnosticReport* report, const char* fmt, ...)
{
    ANN(fmt);

    char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    int written = dvz_vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (written < 0)
        return;

    log_error("%s", message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}

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
 * Return whether an image visual uses per-item rectangles.
 *
 * @param visual the image visual
 * @return whether the visual has an extent attribute
 */
static bool _scene_image_uses_generated_quads(const DvzVisual* visual)
{
    ANN(visual);
    return visual->type == DVZ_VISUAL_TYPE_IMAGE && _scene_visual_has_attr_data(visual, "extent");
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
        log_error("image visual per-item rectangles require position and extent attributes");
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
            {x0, y0, z}, {x0, y1, z}, {x1, y0, z},
            {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
        };
        const float quad_uv[6][2] = {
            {u0, v0}, {u0, v1}, {u1, v0}, {u1, v0}, {u0, v1}, {u1, v1},
        };

        for (uint32_t j = 0; j < 6; j++)
        {
            uint64_t dst = 6 * i + j;
            dvz_memcpy(
                &cache->position[3 * dst], 3 * sizeof(float), quad_pos[j],
                3 * sizeof(float));
            dvz_memcpy(
                &cache->texcoords[2 * dst], 2 * sizeof(float), quad_uv[j],
                2 * sizeof(float));
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
static void _scene_emit_image_uploads(DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index)
{
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
static bool _scene_resource_key_volume_transfer(uint32_t visual_index, char* out, size_t out_size)
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
    _scene_prepare_axis_visuals(figure);
    _scene_prepare_text_visuals(figure);
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
            bool image_generated = _scene_image_uses_generated_quads(visual);
            if (image_generated)
                _scene_emit_image_uploads(plan, visual, vidx);
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                DvzVisualAttr* attr = &visual->attrs[ai];
                if (image_generated)
                    continue;
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
                             visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                             visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
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
                     visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                     visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
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
            if ((visual->type == DVZ_VISUAL_TYPE_IMAGE ||
                 visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
                visual->field != NULL &&
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
            if (visual->type == DVZ_VISUAL_TYPE_VOLUME && visual->field != NULL &&
                _field_format_is_scalar(visual->field->desc.format))
            {
                char transfer_resource_id[128];
                const void* transfer_data = NULL;
                if (!_scene_resource_key_volume_transfer(
                        vidx, transfer_resource_id, sizeof(transfer_resource_id)) ||
                    !_scene_prepare_volume_transfer_texture(visual, &transfer_data) ||
                    !dvz_frame_plan_upload_bytes(
                        plan, transfer_resource_id, 0, 256ull * 4ull, "volume_transfer",
                        transfer_data) ||
                    !dvz_frame_plan_upload_metadata(
                        plan,
                        &(DvzFramePlanUploadMeta){
                            .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_TEXTURE_2D,
                            .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_TEXTURE,
                            .visual_index = UINT32_MAX,
                            .buffer_index = UINT32_MAX,
                        }) ||
                    !dvz_frame_plan_upload_set_texture_format(
                        plan, VK_FORMAT_R8G8B8A8_UNORM, 4) ||
                    !dvz_frame_plan_upload_set_texture_extent(plan, 256, 1) ||
                    !dvz_frame_plan_upload_set_texture_allocation_extent(plan, 256, 1) ||
                    !dvz_frame_plan_upload_set_texture_region(plan, 0, 0))
                {
                    log_error("volume transfer texture upload failed");
                    continue;
                }
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
    metadata->instance_count = 1;
    metadata->alpha_mode = visual->alpha_mode;
    metadata->depth_test_enabled = visual->depth_test_enabled;
    metadata->depth_cue_enabled = visual->material.depth_cue_enabled;
    metadata->point_style_enabled =
        visual->type == DVZ_VISUAL_TYPE_POINT && visual->material.point_style_enabled;
    metadata->glyph_atlas_encoding = (uint32_t)visual->glyph_atlas_encoding;
    metadata->scale_index = _scene_scale_index(figure->scene, visual->scale);
    metadata->scene_occluder = visual->scene_occluder;
    metadata->scene_occluded = visual->scene_occluded;

    const char* vertex_count_attr =
        visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position";
    int vertex_count_idx = _attr_index(visual, vertex_count_attr);
    if (vertex_count_idx >= 0)
    {
        if (visual->attrs[vertex_count_idx].item_count > UINT32_MAX)
            return false;
        metadata->vertex_count = (uint32_t)visual->attrs[vertex_count_idx].item_count;
    }

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
    if (!_scene_attr_resource_key(
            figure, visual, visual_index, "instance_transform", metadata->instance_transform_id,
            sizeof(metadata->instance_transform_id)))
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
        if (visual->field != NULL)
        {
            metadata->field_format = (uint32_t)visual->field->desc.format;
            metadata->field_width = visual->field->desc.width;
            metadata->field_height = visual->field->desc.height;
            metadata->field_depth = visual->field->desc.depth;
        }
        dvz_strlcpy(
            metadata->volume_texture_id, metadata->texture_id, sizeof(metadata->volume_texture_id));
        if (!_scene_resource_key_volume_transfer(
                visual_index, metadata->volume_transfer_texture_id,
                sizeof(metadata->volume_transfer_texture_id)))
            return false;
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
        if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
        {
            if (visual->segment.gpu.vertex_count > UINT32_MAX ||
                visual->segment.gpu.index_count > UINT32_MAX)
                return false;
            metadata->vertex_count = (uint32_t)visual->segment.gpu.vertex_count;
            metadata->index_count = (uint32_t)visual->segment.gpu.index_count;
        }
    }
    if (visual->type == DVZ_VISUAL_TYPE_IMAGE && _scene_image_uses_generated_quads(visual))
    {
        if (visual->image_gpu.vertex_count > UINT32_MAX)
            return false;
        if (visual->image_gpu.vertex_count > 0)
            metadata->vertex_count = (uint32_t)visual->image_gpu.vertex_count;
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
 * Return a mutable FramePlan node by index.
 *
 * @param plan the destination frame plan
 * @param node_index the node index
 * @return the mutable node, or NULL when the index is invalid
 */
static DvzFramePlanNode* _scene_frame_plan_node_mut(DvzFramePlan* plan, uint32_t node_index)
{
    ANN(plan);
    if (node_index >= plan->count)
        return NULL;
    return &plan->nodes[node_index];
}



/**
 * Build a stable pass-contract id for one render node.
 *
 * @param panel_id the panel id
 * @param pass_role the render-pass role
 * @param out output contract id
 * @param out_size output buffer size
 * @return whether the id was written without truncation
 */
static bool _scene_pass_contract_id(
    const char* panel_id, DvzFramePlanRenderPassRole pass_role, char* out, size_t out_size)
{
    ANN(panel_id);
    ANN(out);
    int ret = dvz_snprintf(out, out_size, "%s.pass.%u", panel_id, (uint32_t)pass_role);
    return ret >= 0 && (size_t)ret < out_size;
}



/**
 * Build a stable draw-contract id for one visual in one render pass.
 *
 * @param pass_contract_id the owning pass-contract id
 * @param visual_index the visual index within the figure
 * @param out output contract id
 * @param out_size output buffer size
 * @return whether the id was written without truncation
 */
static bool _scene_draw_contract_id(
    const char* pass_contract_id, uint32_t visual_index, char* out, size_t out_size)
{
    ANN(pass_contract_id);
    ANN(out);
    int ret = dvz_snprintf(out, out_size, "%s.draw.%u", pass_contract_id, visual_index);
    return ret >= 0 && (size_t)ret < out_size;
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
 * @param out_index output node index
 * @return whether the render node was appended
 */
static bool _scene_begin_panel_render_pass(
    DvzFramePlan* plan, const char* panel_id, const char* render_target_id, DvzPanelDesc desc,
    DvzFramePlanRenderPassRole pass_role, const DvzMVP* panel_apply_mvp,
    const DvzSceneViewportUniform* panel_viewport, uint32_t* out_index)
{
    ANN(plan);
    ANN(panel_id);
    ANN(render_target_id);
    ANN(out_index);
    uint32_t node_index = plan->count;
    if (!dvz_frame_plan_render_panel_role(plan, panel_id, render_target_id, false, desc, pass_role))
        return false;
    DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, node_index);
    if (node != NULL)
    {
        _scene_configure_panel_render_node(node, panel_apply_mvp, panel_viewport);
        if (!_scene_pass_contract_id(
                panel_id, pass_role, node->u.render.pass_contract_id,
                sizeof(node->u.render.pass_contract_id)))
            return false;
        node->u.render.has_pass_contract = true;
    }
    *out_index = node_index;
    return node != NULL;
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
    if (!_scene_resource_key_panel_graph(
            panel_id, "volume_occlusion.depth", depth_id, sizeof(depth_id)))
        return false;

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
    if (!_scene_resource_key_panel_graph(
            panel_id, "scene_occlusion.depth", depth_id, sizeof(depth_id)))
        return false;

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
        if (visual == NULL || !visual->visible || visual->type == DVZ_VISUAL_TYPE_TEXT ||
            !visual->volume_occluded ||
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
    if (visual->type == DVZ_VISUAL_TYPE_TEXT)
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
 * Return whether a draw contract requires any depth resource.
 *
 * @param contract the resolved draw contract
 * @return whether the draw uses fixed-function or sampled depth
 */
static bool _scene_draw_contract_needs_depth(const DvzSceneDrawContract* contract)
{
    ANN(contract);
    return (
        contract->depth_policy &
        (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE |
         DVZ_SCENE_DEPTH_POLICY_SAMPLE)) != 0;
}



/**
 * Resolve whether a transparent draw contract requires a pass depth attachment.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param pass_role the transparent render-pass role
 * @param out whether the draw requires depth
 * @return whether the draw contract was resolved
 */
static bool _scene_transparent_contract_needs_depth(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzFramePlanRenderPassRole pass_role,
    bool* out)
{
    ANN(out);
    *out = false;
    DvzSceneDrawContract contract = {0};
    if (!_scene_draw_contract_from_visual(visual, attach, pass_role, &contract))
        return false;
    *out = _scene_draw_contract_needs_depth(&contract);
    return true;
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

    DvzFramePlanVisualMeta metadata = {0};
    bool has_metadata = _scene_visual_frame_plan_metadata(figure, visual, visual_index, &metadata);
    if (has_metadata)
    {
        if (metadata.has_volume && volume_occlusion == NULL)
        {
            metadata.volume_occluded = false;
            metadata.has_volume_occlusion = false;
        }
        if (metadata.has_volume && volume_occlusion != NULL)
        {
            metadata.has_volume_occlusion = true;
            metadata.volume_occlusion = *volume_occlusion;
        }
        if (scene_occlusion == NULL)
            metadata.scene_occluded = false;
        if (metadata.scene_occluded && scene_occlusion != NULL)
        {
            metadata.has_scene_occlusion = true;
            metadata.scene_occlusion = *scene_occlusion;
        }
        DvzSceneDrawContract draw_contract = {0};
        if (!_scene_draw_contract_from_visual(
                visual, attach, node->u.render.pass_role, &draw_contract))
            return false;
        if (scene_occlusion == NULL)
        {
            draw_contract.samples_scene_occlusion = false;
            draw_contract.needs_scene_occlusion_set = false;
            draw_contract.shader_feature_mask &=
                ~((uint32_t)DVZ_SCENE_SHADER_FEATURE_SAMPLE_SCENE_OCCLUSION);
            draw_contract.bind_group_layout_mask &=
                ~((uint32_t)DVZ_SCENE_BIND_GROUP_REQUIREMENT_SCENE_OCCLUSION);
        }
        if (volume_occlusion == NULL)
        {
            draw_contract.samples_volume_occlusion = false;
            draw_contract.shader_feature_mask &=
                ~((uint32_t)DVZ_SCENE_SHADER_FEATURE_SAMPLE_VOLUME_OCCLUSION);
        }
        if (!_scene_draw_contract_id(
                node->u.render.pass_contract_id, visual_index, metadata.draw_contract_id,
                sizeof(metadata.draw_contract_id)))
            return false;
        metadata.has_draw_contract = true;
        metadata.draw_depth_policy = draw_contract.depth_policy;
        metadata.draw_blend_policy = (uint32_t)draw_contract.blend_policy;
        metadata.draw_shader_feature_mask = draw_contract.shader_feature_mask;
        metadata.draw_bind_group_layout_mask = draw_contract.bind_group_layout_mask;
        if (draw_contract.samples_volume_occlusion)
        {
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "volume_occlusion.depth",
                    metadata.draw_volume_occlusion_resource_id,
                    sizeof(metadata.draw_volume_occlusion_resource_id)))
                return false;
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "volume_occlusion",
                    metadata.draw_volume_occlusion_producer_pass_id,
                    sizeof(metadata.draw_volume_occlusion_producer_pass_id)))
                return false;
            metadata.draw_volume_occlusion_bind_set = DVZ_SCENE_SHADER_SET_VISUAL;
            metadata.draw_volume_occlusion_bind_binding = 3;
        }
        if (draw_contract.samples_scene_occlusion)
        {
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "scene_occlusion.depth",
                    metadata.draw_scene_occlusion_resource_id,
                    sizeof(metadata.draw_scene_occlusion_resource_id)))
                return false;
            if (!_scene_resource_key_panel_graph(
                    node->u.render.panel_id, "scene_occlusion",
                    metadata.draw_scene_occlusion_producer_pass_id,
                    sizeof(metadata.draw_scene_occlusion_producer_pass_id)))
                return false;
            bool scene_occlusion_uses_set2 =
                draw_contract.needs_image_set || draw_contract.needs_volume_set ||
                draw_contract.needs_material_set;
            metadata.draw_scene_occlusion_bind_set =
                scene_occlusion_uses_set2 ? DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION :
                                            DVZ_SCENE_SHADER_SET_VISUAL;
            metadata.draw_scene_occlusion_bind_binding = 0;
        }
    }

    uint32_t slot = node->u.render.visual_count++;
    dvz_strlcpy(node->u.render.visuals[slot], visual_id, sizeof(node->u.render.visuals[slot]));
    if (has_metadata)
    {
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
 * @param report optional diagnostic report
 * @return whether the panel render graph was emitted
 */
bool _scene_emit_panel_render_ex(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(figure_id);
    _scene_prepare_text_visuals(figure);
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
        if (visual->type == DVZ_VISUAL_TYPE_TEXT)
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
        return true;
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS];
    _scene_panel_visual_order(panel, order);

    DvzMVP panel_apply_mvp;
    _scene_panel_apply_mvp(panel, &panel_apply_mvp);
    DvzSceneViewportUniform panel_viewport = {0};
    _scene_panel_pixel_rect(
        panel, &panel_viewport.x, &panel_viewport.y, &panel_viewport.width,
        &panel_viewport.height);

    const uint32_t invalid_node = UINT32_MAX;
    uint32_t scene_occlusion_node = invalid_node;
    bool scene_occlusion_enabled = _scene_panel_has_visible_scene_occlusion_target(panel);
    if (scene_occlusion_enabled)
    {
        if (_scene_begin_panel_render_pass(
            plan, panel_id, "rt.scene_occlusion.depth", panel->desc,
            DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION, &panel_apply_mvp, &panel_viewport,
            &scene_occlusion_node))
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
                DvzFramePlanNode* node =
                    _scene_frame_plan_node_mut(plan, scene_occlusion_node);
                if (node == NULL)
                    continue;
                (void)_scene_append_visual_to_render_pass(
                    figure, plan, node, visual, attach, vidx, &panel->scene_occlusion,
                    volume_occlusion);
            }
        }
    }

    uint32_t volume_occlusion_node = invalid_node;
    bool volume_occlusion_enabled = _scene_panel_has_visible_volume_occlusion_target(panel);
    if (volume_occlusion_enabled)
    {
        uint32_t occluder_index = 0;
        if (_figure_visual_index(figure, panel->volume_occluder_visual, &occluder_index))
        {
            if (_scene_begin_panel_render_pass(
                plan, panel_id, "rt.volume_occlusion.depth", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION, &panel_apply_mvp,
                &panel_viewport, &volume_occlusion_node))
            {
                DvzPanelAttach attach = {
                    .visual = panel->volume_occluder_visual,
                    .z_layer = 0,
                    .controller_mode = DVZ_CONTROLLER_APPLY,
                };
                DvzFramePlanNode* node =
                    _scene_frame_plan_node_mut(plan, volume_occlusion_node);
                if (node != NULL)
                {
                    (void)_scene_append_visual_to_render_pass(
                        figure, plan, node, panel->volume_occluder_visual, &attach,
                        occluder_index, NULL, &panel->volume_occlusion);
                }
            }
        }
    }

    uint32_t opaque_node = invalid_node;
    uint32_t gbuffer_node = invalid_node;
    uint32_t transparent_node = invalid_node;
    uint32_t depth_peel_init_node = invalid_node;
    uint32_t depth_peel_iter_node = invalid_node;
    uint32_t depth_peel_composite_node = invalid_node;
    uint32_t blended_nodes[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    bool blended_needs_depth[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    bool blended_writes_depth[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t blended_count = 0;
    uint32_t edl_node = invalid_node;
    uint32_t ssao_node = invalid_node;
    uint32_t ssao_blur_node = invalid_node;
    uint32_t ssao_composite_node = invalid_node;
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
    bool graph_ok = true;
    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_TEXT)
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
            DvzFramePlanRenderPassRole pass_role =
                DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
            if (caps.draws_in_transparent_blend_pass)
            {
                pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND;
            }
            else if (caps.draws_in_wboit_pass)
            {
                pass_role = DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
            }
            else if (caps.draws_in_depth_peel_pass)
            {
                pass_role = DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT;
            }
            bool contract_needs_depth = false;
            if (_scene_transparent_contract_needs_depth(
                    visual, attach, pass_role, &contract_needs_depth))
            {
                transparent_needs_depth = transparent_needs_depth || contract_needs_depth;
            }
            continue;
        }

        if (gbuffer_required && _scene_technique_gbuffer_plan_add_visual(&gbuffer, visual, attach))
        {
            if (gbuffer_node == invalid_node)
            {
                if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.gbuffer.normal", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER, &panel_apply_mvp, &panel_viewport,
                    &gbuffer_node))
                    continue;
            }
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, gbuffer_node);
            if (node == NULL)
                continue;
            (void)_scene_append_visual_to_render_pass(
                figure, plan, node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        }

        if (opaque_node == invalid_node)
        {
            if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
                &panel_apply_mvp, &panel_viewport, &opaque_node))
                continue;
        }
        DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, opaque_node);
        if (node == NULL)
            continue;
        (void)_scene_append_visual_to_render_pass(
            figure, plan, node, visual, attach, vidx,
            scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
            volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        bool edl_depth_visual = edl_enabled && caps.eligible_for_depth_postprocess;
        opaque_needs_depth = opaque_needs_depth || caps.writes_depth || edl_depth_visual;
        edl_has_depth_producer = edl_has_depth_producer || edl_depth_visual;
    }

    if (opaque_node == invalid_node && has_transparent)
    {
        (void)_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE, &panel_apply_mvp,
            &panel_viewport, &opaque_node);
    }

    for (uint32_t k = 0; k < panel->visual_count; k++)
    {
        uint32_t vi = order[k];
        DvzPanelAttach* attach = &panel->visuals[vi];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type == DVZ_VISUAL_TYPE_TEXT)
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
            DvzSceneDrawContract draw_contract = {0};
            if (!_scene_draw_contract_from_visual(
                    visual, attach, DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND,
                    &draw_contract))
                continue;
            bool draw_needs_depth = _scene_draw_contract_needs_depth(&draw_contract);
            bool draw_writes_depth =
                (draw_contract.depth_policy & DVZ_SCENE_DEPTH_POLICY_WRITE) != 0;

            bool start_blended_pass = blended_count == 0;
            if (!start_blended_pass)
            {
                uint32_t prev = blended_count - 1;
                start_blended_pass = blended_writes_depth[prev] != draw_writes_depth;
            }
            if (start_blended_pass)
            {
                if (blended_count >= DVZ_SCENE_MAX_RENDER_VISUALS)
                    continue;
                uint32_t node_index = invalid_node;
                if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND, &panel_apply_mvp,
                    &panel_viewport, &node_index))
                    continue;
                blended_nodes[blended_count] = node_index;
                blended_count++;
            }
            uint32_t blend_idx = blended_count - 1;
            DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, blended_nodes[blend_idx]);
            if (node == NULL)
                continue;
            (void)_scene_append_visual_to_render_pass(
                figure, plan, node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            blended_needs_depth[blend_idx] = blended_needs_depth[blend_idx] || draw_needs_depth;
            blended_writes_depth[blend_idx] = blended_writes_depth[blend_idx] || draw_writes_depth;
            transparent_needs_depth = transparent_needs_depth || draw_needs_depth;
            continue;
        }

        if (caps.draws_in_depth_peel_pass)
        {
            if (depth_peel_init_node == invalid_node)
            {
                if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.depth_peel_init", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT, &panel_apply_mvp,
                    &panel_viewport, &depth_peel_init_node))
                    continue;
                if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt.depth_peel_iter", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER, &panel_apply_mvp,
                    &panel_viewport, &depth_peel_iter_node))
                    continue;
                if (!_scene_begin_panel_render_pass(
                    plan, panel_id, "rt", panel->desc,
                    DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE, &panel_apply_mvp,
                    &panel_viewport, &depth_peel_composite_node))
                    continue;
            }
            DvzFramePlanNode* init_node =
                _scene_frame_plan_node_mut(plan, depth_peel_init_node);
            DvzFramePlanNode* iter_node =
                _scene_frame_plan_node_mut(plan, depth_peel_iter_node);
            if (init_node == NULL || iter_node == NULL)
                continue;
            (void)_scene_append_visual_to_render_pass(
                figure, plan, init_node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            (void)_scene_append_visual_to_render_pass(
                figure, plan, iter_node, visual, attach, vidx,
                scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
                volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
            transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
            continue;
        }

        if (transparent_node == invalid_node)
        {
            if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt.wboit_accum", panel->desc,
                DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION, &panel_apply_mvp,
                &panel_viewport, &transparent_node))
                continue;
        }
        DvzFramePlanNode* node = _scene_frame_plan_node_mut(plan, transparent_node);
        if (node == NULL)
            continue;
        (void)_scene_append_visual_to_render_pass(
            figure, plan, node, visual, attach, vidx,
            scene_occlusion_enabled ? &panel->scene_occlusion : NULL,
            volume_occlusion_enabled ? &panel->volume_occlusion : NULL);
        transparent_needs_depth = transparent_needs_depth || caps.needs_depth_attachment;
    }

    if (scene_occlusion_node != invalid_node &&
        !_scene_technique_emit_scene_occlusion_frame_graph(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to emit scene occlusion FramePlan graph for panel %s", panel_id);
        graph_ok = false;
    }

    if (transparent_node != invalid_node)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        uint32_t resolve_node = invalid_node;
        (void)_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE,
            &panel_apply_mvp, &panel_viewport, &resolve_node);
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (!_scene_technique_emit_wboit_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit WBOIT FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (blended_count > 0 &&
            !_scene_technique_emit_blended_frame_graph(
                plan, panel_id, false, opaque_needs_depth,
                opaque_needs_depth || transparent_needs_depth, blended_count, blended_needs_depth,
                blended_writes_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (depth_peel_init_node != invalid_node)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (!_scene_technique_emit_depth_peel_frame_graph(
                plan, panel_id, opaque_needs_depth, transparent_needs_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit depth-peeling FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (blended_count > 0)
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        bool blended_depth_producer = opaque_needs_depth || transparent_needs_depth;
        if (!_scene_technique_emit_blended_frame_graph(
                plan, panel_id, true, blended_depth_producer, blended_depth_producer,
                blended_count, blended_needs_depth, blended_writes_depth))
        {
            _scene_emit_graph_report(
                report, "failed to emit blended FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    else if (
        opaque_node != invalid_node &&
        (opaque_needs_depth || volume_occlusion_node != invalid_node ||
         scene_occlusion_node != invalid_node))
    {
        if (volume_occlusion_node != invalid_node &&
            !_scene_technique_emit_volume_occlusion_frame_graph(plan, panel_id))
        {
            _scene_emit_graph_report(
                report, "failed to emit volume occlusion FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
        if (gbuffer_required && gbuffer_node != invalid_node &&
            !_scene_technique_emit_gbuffer_frame_graph(plan, panel_id, &gbuffer))
        {
            _scene_emit_graph_report(
                report, "failed to emit G-buffer FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
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
            if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE,
                &panel_apply_mvp, &panel_viewport, &edl_node) ||
                !_scene_technique_emit_edl_frame_graph(plan, panel_id))
            {
                _scene_emit_graph_report(
                    report, "failed to emit EDL FramePlan graph for panel %s", panel_id);
                graph_ok = false;
            }
        }
        else if ((gbuffer_node != invalid_node || volume_occlusion_node != invalid_node ||
                  scene_occlusion_node != invalid_node ||
                  (!ssao_enabled && msaa_state != NULL)) &&
                 !_scene_technique_emit_opaque_frame_graph(
                     plan, panel_id, opaque_needs_depth, msaa_state))
        {
            _scene_emit_graph_report(
                report, "failed to emit opaque FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    if (ssao_enabled && gbuffer_node != invalid_node && gbuffer.producer_count > 0)
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
        if (!_scene_begin_panel_render_pass(
            plan, panel_id, "rt.ssao.occlusion", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
            &panel_apply_mvp, &panel_viewport, &ssao_node))
            ssao_node = invalid_node;
        if (ssao_state->blur_enabled)
        {
            if (!_scene_begin_panel_render_pass(
                plan, panel_id, "rt.ssao.blur", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR,
                &panel_apply_mvp, &panel_viewport, &ssao_blur_node))
                ssao_blur_node = invalid_node;
        }
        if (!_scene_begin_panel_render_pass(
            plan, panel_id, "rt", panel->desc, DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
            &panel_apply_mvp, &panel_viewport, &ssao_composite_node))
            ssao_composite_node = invalid_node;
        if (ssao_node == invalid_node ||
            (ssao_state->blur_enabled && ssao_blur_node == invalid_node) ||
            ssao_composite_node == invalid_node ||
            !_scene_technique_emit_ssao_frame_graph(plan, panel_id, &gbuffer, ssao_state))
        {
            _scene_emit_graph_report(
                report, "failed to emit SSAO FramePlan graph for panel %s", panel_id);
            graph_ok = false;
        }
    }
    if (volume_occlusion_node != invalid_node &&
        !_scene_add_volume_occlusion_reads(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to add volume occlusion FramePlan reads for panel %s", panel_id);
        graph_ok = false;
    }
    if (scene_occlusion_node != invalid_node &&
        !_scene_add_scene_occlusion_reads(plan, panel_id))
    {
        _scene_emit_graph_report(
            report, "failed to add scene occlusion FramePlan reads for panel %s", panel_id);
        graph_ok = false;
    }
    return graph_ok;
}



/**
 * Emit one panel render node into a frame plan.
 *
 * @param figure the parent figure
 * @param panel_index the panel index within the figure
 * @param plan the destination frame plan
 * @param figure_id the stable figure identifier
 * @return whether the panel render graph was emitted
 */
bool _scene_emit_panel_render(
    DvzFigure* figure, uint32_t panel_index, DvzFramePlan* plan, const char* figure_id)
{
    return _scene_emit_panel_render_ex(figure, panel_index, plan, figure_id, NULL);
}
