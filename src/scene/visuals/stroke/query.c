/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stroke visual query helpers                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "datoviz/math/_cglm.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_visual_internal.h"
#include "stroke/internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define PATH_QUERY_VERTEX_SIDE_NEGATIVE 0x01u
#define PATH_QUERY_VERTEX_ENDPOINT_END  0x02u
#define PATH_QUERY_VERTEX_HAS_PREV      0x04u
#define PATH_QUERY_VERTEX_HAS_NEXT      0x08u
#define PATH_QUERY_VERTEX_SUBPATH_START 0x10u
#define PATH_QUERY_VERTEX_SUBPATH_END   0x20u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return packed path vertex flags for one temporary query vertex.
 *
 * @param side_negative whether the vertex is on the negative normal side
 * @param endpoint_end whether the vertex belongs to the segment end endpoint
 * @param has_prev whether the endpoint has a previous path point
 * @param has_next whether the endpoint has a next path point
 * @param subpath_start whether the endpoint is the first point in an open subpath
 * @param subpath_end whether the endpoint is the last point in an open subpath
 * @return packed path vertex flags
 */
static uint32_t _path_query_vertex_flags(
    bool side_negative, bool endpoint_end, bool has_prev, bool has_next, bool subpath_start,
    bool subpath_end)
{
    uint32_t flags = 0;
    flags |= side_negative ? PATH_QUERY_VERTEX_SIDE_NEGATIVE : 0u;
    flags |= endpoint_end ? PATH_QUERY_VERTEX_ENDPOINT_END : 0u;
    flags |= has_prev ? PATH_QUERY_VERTEX_HAS_PREV : 0u;
    flags |= has_next ? PATH_QUERY_VERTEX_HAS_NEXT : 0u;
    flags |= subpath_start ? PATH_QUERY_VERTEX_SUBPATH_START : 0u;
    flags |= subpath_end ? PATH_QUERY_VERTEX_SUBPATH_END : 0u;
    return flags;
}



/**
 * Return the Euclidean distance between two path points.
 *
 * @param position flat vec3 position array
 * @param i0 first point index
 * @param i1 second point index
 * @return point distance in visual coordinates
 */
static float _path_query_point_distance(const float* position, uint64_t i0, uint64_t i1)
{
    ANN(position);
    float dx = position[3 * i1 + 0] - position[3 * i0 + 0];
    float dy = position[3 * i1 + 1] - position[3 * i0 + 1];
    float dz = position[3 * i1 + 2] - position[3 * i0 + 2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}



/**
 * Return whether one query subpath repeats its first point as a closed-ring sentinel.
 *
 * @param position flat vec3 position array
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @return whether the first and last points are equal
 */
static bool _path_query_subpath_is_closed(
    const float* position, uint64_t offset, uint32_t length)
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
 * Return the previous adjacency point for one temporary query endpoint.
 *
 * @param point_idx endpoint point index
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @param closed whether the subpath repeats its first point at the end
 * @return previous adjacency point index
 */
static uint64_t _path_query_prev_index(
    uint64_t point_idx, uint64_t offset, uint32_t length, bool closed)
{
    if (closed && point_idx == offset)
        return offset + length - 2;
    if (point_idx > offset)
        return point_idx - 1;
    return point_idx;
}



/**
 * Return the next adjacency point for one temporary query endpoint.
 *
 * @param point_idx endpoint point index
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @param closed whether the subpath repeats its first point at the end
 * @return next adjacency point index
 */
static uint64_t _path_query_next_index(
    uint64_t point_idx, uint64_t offset, uint32_t length, bool closed)
{
    const uint64_t end = offset + length;
    if (closed && point_idx + 1 == end)
        return offset + 1;
    if (point_idx + 1 < end)
        return point_idx + 1;
    return point_idx;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one retained stroke attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
bool _stroke_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    ANN(out_attr);
    int attr_idx = _attr_index(visual, attr_name);
    if (attr_idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[attr_idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    *out_attr = attr;
    return true;
}



/**
 * Allocate one temporary stroke query buffer with checked size arithmetic.
 *
 * @param label diagnostic family label
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @return true when allocation succeeds
 */
bool _stroke_query_alloc(
    const char* label, void** out_ptr, uint64_t count, uint64_t item_size)
{
    ANN(label);
    ANN(out_ptr);
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, item_size, &bytes) || bytes > SIZE_MAX)
    {
        log_error("%s query request buffer size overflow", label);
        return false;
    }
    void* ptr = dvz_calloc((size_t)count, (size_t)item_size);
    if (ptr == NULL && bytes > 0)
    {
        log_error("%s query request buffer allocation failed", label);
        return false;
    }
    *out_ptr = ptr;
    return true;
}



/**
 * Return the offscreen stroke query target extent for one panel.
 *
 * @param figure parent figure
 * @param panel panel receiving the query
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
bool _stroke_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height)
{
    ANN(figure);
    ANN(panel);
    ANN(out_target_width);
    ANN(out_target_height);
    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;
    uint32_t target_width = (uint32_t)(panel_width + 0.5);
    uint32_t target_height = (uint32_t)(panel_height + 0.5);
    *out_target_width = target_width == 0 ? 1 : target_width;
    *out_target_height = target_height == 0 ? 1 : target_height;
    return true;
}



/**
 * Apply the request-centered MVP and viewport to a stroke query render node.
 *
 * @param plan frame plan
 * @param panel panel receiving the query
 * @param request_ndc request coordinate in panel-local NDC
 * @param target_width offscreen target width
 * @param target_height offscreen target height
 */
void _stroke_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const float* request_ndc, uint32_t target_width,
    uint32_t target_height)
{
    ANN(plan);
    ANN(panel);
    ANN(request_ndc);
    DvzFramePlanNode* render = dvz_frame_plan_last_render_node(plan);
    if (render == NULL)
        return;

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    vec2 target_ndc = {
        -1.0f + 1.0f / (float)target_width,
        1.0f - 1.0f / (float)target_height,
    };
    vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
    mvp.proj[3][0] -= delta[0];
    mvp.proj[3][1] -= delta[1];
    render->u.render.has_mvp = true;
    render->u.render.apply_mvp = mvp;
    render->u.render.has_viewport = true;
    render->u.render.viewport =
        (DvzSceneViewportUniform){0.0f, 0.0f, (float)target_width, (float)target_height};
    render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
}



/**
 * Mark the most recent stroke query upload node as an index buffer.
 *
 * @param plan the frame plan
 * @param stride index item stride in bytes
 */
void _stroke_query_mark_last_upload_index(DvzFramePlan* plan, uint32_t stride)
{
    ANN(plan);
    DvzFramePlanNode* node = plan->count > 0 ? &plan->nodes[plan->count - 1] : NULL;
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
    node->u.upload.item_stride = stride;
}



/**
 * Mark the most recent stroke query upload node as a material uniform buffer.
 *
 * @param plan the frame plan
 */
void _stroke_query_mark_last_upload_uniform(DvzFramePlan* plan)
{
    ANN(plan);
    DvzFramePlanNode* node = plan->count > 0 ? &plan->nodes[plan->count - 1] : NULL;
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_UNIFORM;
    node->u.upload.item_stride = sizeof(DvzSceneMaterialParams);
}



/**
 * Build temporary GPU stroke buffers for one segment or straight-vector visual.
 *
 * @param visual segment or vector visual
 * @param scratch output scratch plan storage
 * @param out_vertex_count output derived vertex count
 * @param out_index_count output derived index count
 * @return true when derived buffers were created
 */
bool _stroke_quad_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count,
    uint64_t* out_index_count)
{
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);
    ANN(out_index_count);

    const DvzVisualAttr* start_attr = NULL;
    const DvzVisualAttr* end_attr = NULL;
    const DvzVisualAttr* position_attr = NULL;
    const DvzVisualAttr* vector_attr = NULL;
    const DvzVisualAttr* width_attr = NULL;
    bool vector_mode = visual->type == DVZ_VISUAL_TYPE_VECTOR;
    if (vector_mode)
    {
        if (!_stroke_query_attr(visual, "position", sizeof(vec3), &position_attr) ||
            !_stroke_query_attr(visual, "vector", sizeof(vec3), &vector_attr) ||
            !_stroke_query_attr(visual, "line_width", sizeof(float), &width_attr))
        {
            return false;
        }
    }
    else
    {
        if (!_stroke_query_attr(visual, "position_start", sizeof(vec3), &start_attr) ||
            !_stroke_query_attr(visual, "position_end", sizeof(vec3), &end_attr) ||
            !_stroke_query_attr(visual, "line_width", sizeof(float), &width_attr))
        {
            return false;
        }
    }
    uint64_t item_count = vector_mode ? position_attr->item_count : start_attr->item_count;
    if (vector_mode)
    {
        if (vector_attr->item_count != item_count || width_attr->item_count != item_count)
            return false;
    }
    else if (end_attr->item_count != item_count || width_attr->item_count != item_count)
    {
        return false;
    }

    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 4, &vertex_count) ||
        _dvz_mul_u64_overflows(item_count, 6, &index_count) || vertex_count > UINT32_MAX)
    {
        log_error("segment query request buffer size overflow");
        return false;
    }

    if (!_stroke_query_alloc(
            "segment",
            (void**)&scratch->query_position_start, vertex_count, 3 * sizeof(float)) ||
        !_stroke_query_alloc(
            "segment",
            (void**)&scratch->query_position_end, vertex_count, 3 * sizeof(float)) ||
        !_stroke_query_alloc(
            "segment", (void**)&scratch->query_line_width, vertex_count, sizeof(float)) ||
        !_stroke_query_alloc(
            "segment", (void**)&scratch->query_ids, vertex_count, sizeof(uint32_t)) ||
        !_stroke_query_alloc(
            "segment", (void**)&scratch->query_indices, index_count, sizeof(uint32_t)))
    {
        return false;
    }

    const float* position_start = vector_mode ? NULL : (const float*)start_attr->data;
    const float* position_end = vector_mode ? NULL : (const float*)end_attr->data;
    const float* position = vector_mode ? (const float*)position_attr->data : NULL;
    const float* vector = vector_mode ? (const float*)vector_attr->data : NULL;
    const float* line_width = (const float*)width_attr->data;
    for (uint64_t i = 0; i < item_count; i++)
    {
        float vector_start[3] = {0};
        float vector_end[3] = {0};
        if (vector_mode)
        {
            float scale = visual->vector.scale;
            float head_factor = 1.0f;
            float tail_factor = 0.0f;
            if (visual->vector.anchor == DVZ_VECTOR_ANCHOR_CENTER)
            {
                tail_factor = -0.5f;
                head_factor = 0.5f;
            }
            else if (visual->vector.anchor == DVZ_VECTOR_ANCHOR_HEAD)
            {
                tail_factor = -1.0f;
                head_factor = 0.0f;
            }
            for (uint32_t k = 0; k < 3; k++)
            {
                float delta = vector[3 * i + k] * scale;
                vector_start[k] = position[3 * i + k] + tail_factor * delta;
                vector_end[k] = position[3 * i + k] + head_factor * delta;
            }
        }
        for (uint32_t j = 0; j < 4; j++)
        {
            uint64_t dst = 4 * i + j;
            const float* start = vector_mode ? vector_start : &position_start[3 * i];
            const float* end = vector_mode ? vector_end : &position_end[3 * i];
            dvz_memcpy(
                &scratch->query_position_start[3 * dst], 3 * sizeof(float), start,
                3 * sizeof(float));
            dvz_memcpy(
                &scratch->query_position_end[3 * dst], 3 * sizeof(float), end,
                3 * sizeof(float));
            scratch->query_line_width[dst] = line_width[i];
            scratch->query_ids[dst] = (uint32_t)i + 1u;
        }
        scratch->query_indices[6 * i + 0] = (uint32_t)(4 * i + 0);
        scratch->query_indices[6 * i + 1] = (uint32_t)(4 * i + 1);
        scratch->query_indices[6 * i + 2] = (uint32_t)(4 * i + 2);
        scratch->query_indices[6 * i + 3] = (uint32_t)(4 * i + 0);
        scratch->query_indices[6 * i + 4] = (uint32_t)(4 * i + 2);
        scratch->query_indices[6 * i + 5] = (uint32_t)(4 * i + 3);
    }

    *out_vertex_count = vertex_count;
    *out_index_count = index_count;
    return true;
}



/**
 * Build temporary GPU stroke buffers for one stroked path or curved-vector visual.
 *
 * @param visual path or vector visual
 * @param scratch output scratch plan storage
 * @param out_vertex_count output derived vertex count
 * @param out_index_count output derived index count
 * @return true when derived buffers were created
 */
bool _path_stroke_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count,
    uint64_t* out_index_count)
{
    ANN(visual);
    ANN(scratch);
    ANN(out_vertex_count);
    ANN(out_index_count);

    const DvzVisualAttr* pos_attr = NULL;
    const DvzVisualAttr* width_attr = NULL;
    if (!_stroke_query_attr(visual, "position", sizeof(vec3), &pos_attr) ||
        !_stroke_query_attr(visual, "line_width", sizeof(float), &width_attr))
    {
        return false;
    }
    uint64_t point_count = pos_attr->item_count;
    if (width_attr->item_count != point_count || point_count < 2)
        return false;

    const uint32_t* subpath_lengths = visual->type == DVZ_VISUAL_TYPE_VECTOR
                                          ? visual->vector.subpath_lengths
                                          : visual->path.subpath_lengths;
    uint32_t subpath_count = visual->type == DVZ_VISUAL_TYPE_VECTOR
                                 ? visual->vector.subpath_count
                                 : visual->path.subpath_count;

    uint64_t segment_count = 0;
    uint64_t consumed = 0;
    if (subpath_count > 0)
    {
        for (uint32_t i = 0; i < subpath_count; i++)
        {
            uint32_t length = subpath_lengths[i];
            consumed += length;
            if (length >= 2)
                segment_count += length - 1;
        }
        if (consumed != point_count)
        {
            log_error("path query request subpath lengths must sum to the path point count");
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
        log_error("path query request buffer size overflow");
        return false;
    }

    if (!_stroke_query_alloc(
            "path",
            (void**)&scratch->query_position_start, vertex_count, 3 * sizeof(float)) ||
        !_stroke_query_alloc(
            "path",
            (void**)&scratch->query_position_curr, vertex_count, 3 * sizeof(float)) ||
        !_stroke_query_alloc(
            "path",
            (void**)&scratch->query_position_end, vertex_count, 3 * sizeof(float)) ||
        !_stroke_query_alloc(
            "path", (void**)&scratch->query_ids, vertex_count, sizeof(uint32_t)) ||
        !_stroke_query_alloc(
            "path", (void**)&scratch->query_line_width, vertex_count, sizeof(float)) ||
        !_stroke_query_alloc(
            "path", (void**)&scratch->query_path_flags, vertex_count, sizeof(uint32_t)) ||
        !_stroke_query_alloc(
            "path", (void**)&scratch->query_path_distance, vertex_count, sizeof(float)) ||
        !_stroke_query_alloc(
            "path", (void**)&scratch->query_indices, index_count, sizeof(uint32_t)))
    {
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    const float* line_width = (const float*)width_attr->data;
    uint64_t segment = 0;
    uint64_t offset = 0;
    uint32_t effective_subpath_count = subpath_count > 0 ? subpath_count : 1;
    for (uint32_t sp = 0; sp < effective_subpath_count; sp++)
    {
        uint32_t length = subpath_count > 0 ? subpath_lengths[sp] : (uint32_t)point_count;
        bool closed = _path_query_subpath_is_closed(position, offset, length);
        float cumulative = 0.0f;
        for (uint32_t i = 0; i + 1 < length; i++)
        {
            uint64_t i0 = offset + i;
            uint64_t i1 = i0 + 1;
            float edge_length = _path_query_point_distance(position, i0, i1);
            for (uint32_t j = 0; j < 4; j++)
            {
                bool endpoint_end = j >= 2;
                bool side_negative = j == 1 || j == 2;
                uint64_t point_idx = endpoint_end ? i1 : i0;
                uint64_t prev_idx = _path_query_prev_index(point_idx, offset, length, closed);
                uint64_t next_idx = _path_query_next_index(point_idx, offset, length, closed);
                bool has_prev = prev_idx != point_idx;
                bool has_next = next_idx != point_idx;
                bool subpath_start = !closed && point_idx == offset;
                bool subpath_end = !closed && point_idx + 1 == offset + length;
                uint64_t dst = 4 * segment + j;
                dvz_memcpy(
                    &scratch->query_position_start[3 * dst], 3 * sizeof(float),
                    &position[3 * prev_idx], 3 * sizeof(float));
                dvz_memcpy(
                    &scratch->query_position_curr[3 * dst], 3 * sizeof(float),
                    &position[3 * point_idx], 3 * sizeof(float));
                dvz_memcpy(
                    &scratch->query_position_end[3 * dst], 3 * sizeof(float),
                    &position[3 * next_idx], 3 * sizeof(float));
                scratch->query_ids[dst] = (uint32_t)segment + 1u;
                scratch->query_line_width[dst] = line_width[point_idx];
                scratch->query_path_flags[dst] = _path_query_vertex_flags(
                    side_negative, endpoint_end, has_prev, has_next, subpath_start, subpath_end);
                scratch->query_path_distance[dst] =
                    endpoint_end ? cumulative + edge_length : cumulative;
            }
            scratch->query_indices[6 * segment + 0] = (uint32_t)(4 * segment + 0);
            scratch->query_indices[6 * segment + 1] = (uint32_t)(4 * segment + 1);
            scratch->query_indices[6 * segment + 2] = (uint32_t)(4 * segment + 2);
            scratch->query_indices[6 * segment + 3] = (uint32_t)(4 * segment + 0);
            scratch->query_indices[6 * segment + 4] = (uint32_t)(4 * segment + 2);
            scratch->query_indices[6 * segment + 5] = (uint32_t)(4 * segment + 3);
            segment++;
            cumulative += edge_length;
        }
        offset += length;
    }

    *out_vertex_count = vertex_count;
    *out_index_count = index_count;
    return true;
}
