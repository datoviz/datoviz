/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene path-stroke cache builders                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_visual_internal.h"
#include "stroke/internal.h"


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
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one path visual has dense attributes for stroked lowering.
 *
 * @param visual the path visual
 * @param out_count output point count
 * @return whether all stroked path attributes are present
 */
static bool _path_stroke_required_attrs(const DvzVisual* visual, uint64_t* out_count)
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
static float _path_stroke_point_distance(const float* position, uint64_t i0, uint64_t i1)
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
static bool _path_stroke_subpath_is_closed(const float* position, uint64_t offset, uint32_t length)
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
static uint64_t _path_stroke_prev_index(
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
static uint64_t _path_stroke_next_index(
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
static uint32_t _path_stroke_vertex_flags(
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
 * Resize a path-stroke cache array.
 *
 * @param ptr input/output array pointer
 * @param count item count
 * @param item_size byte size of one item
 * @return whether the allocation succeeded
 */
static bool _path_stroke_cache_resize(void** ptr, uint64_t count, uint64_t item_size)
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Rebuild one path visual's derived adjacency-style upload cache.
 *
 * @param visual the path visual
 * @return whether the cache is ready for upload
 */
bool _path_stroke_cache_rebuild(DvzVisual* visual)
{
    ANN(visual);
    uint64_t point_count = 0;
    if (!_path_stroke_required_attrs(visual, &point_count) || point_count < 2)
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

    DvzPathGpuCache* cache =
        visual->type == DVZ_VISUAL_TYPE_VECTOR ? &visual->vector.path_gpu : &visual->path.gpu;
    if (!_path_stroke_cache_resize(
            (void**)&cache->position_prev, vertex_count, 3 * sizeof(float)) ||
        !_path_stroke_cache_resize(
            (void**)&cache->position_curr, vertex_count, 3 * sizeof(float)) ||
        !_path_stroke_cache_resize(
            (void**)&cache->position_next, vertex_count, 3 * sizeof(float)) ||
        !_path_stroke_cache_resize((void**)&cache->color, vertex_count, sizeof(DvzColor)) ||
        !_path_stroke_cache_resize((void**)&cache->line_width, vertex_count, sizeof(float)) ||
        !_path_stroke_cache_resize((void**)&cache->path_flags, vertex_count, sizeof(uint32_t)) ||
        !_path_stroke_cache_resize((void**)&cache->path_distance, vertex_count, sizeof(float)) ||
        !_path_stroke_cache_resize((void**)&cache->indices, index_count, sizeof(uint32_t)))
    {
        log_error("failed to allocate path visual derived GPU cache");
        return false;
    }

    const float* position = (const float*)visual->attrs[_attr_index(visual, "position")].data;
    const DvzColor* color = (const DvzColor*)visual->attrs[_attr_index(visual, "color")].data;
    const float* line_width = (const float*)visual->attrs[_attr_index(visual, "line_width")].data;

    uint64_t segment = 0;
    uint64_t offset = 0;
    uint32_t effective_subpath_count = subpath_count > 0 ? subpath_count : 1;
    for (uint32_t sp = 0; sp < effective_subpath_count; sp++)
    {
        uint32_t length = subpath_count > 0 ? subpath_lengths[sp] : (uint32_t)point_count;
        bool closed = _path_stroke_subpath_is_closed(position, offset, length);
        float cumulative = 0.0f;
        for (uint32_t i = 0; i + 1 < length; i++)
        {
            uint64_t i0 = offset + i;
            uint64_t i1 = i0 + 1;
            float edge_length = _path_stroke_point_distance(position, i0, i1);
            for (uint32_t j = 0; j < 4; j++)
            {
                bool endpoint_end = j >= 2;
                bool side_negative = j == 1 || j == 2;
                uint64_t point_idx = endpoint_end ? i1 : i0;
                uint64_t prev_idx = _path_stroke_prev_index(point_idx, offset, length, closed);
                uint64_t next_idx = _path_stroke_next_index(point_idx, offset, length, closed);
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
                cache->path_flags[dst] = _path_stroke_vertex_flags(
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
