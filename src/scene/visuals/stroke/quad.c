/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene stroke-quad cache builders                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "_visual_internal.h"
#include "stroke/internal.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one segment visual has all dense attributes required for rendering.
 *
 * @param visual the segment visual
 * @param out_count output segment count
 * @return whether all segment attributes are present
 */
static bool _stroke_quad_segment_required_attrs(const DvzVisual* visual, uint64_t* out_count)
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
 * Return whether one vector visual has dense straight-vector attributes.
 *
 * @param visual the vector visual
 * @param out_count output vector count
 * @return whether all straight-vector attributes are present
 */
static bool _stroke_quad_vector_required_attrs(const DvzVisual* visual, uint64_t* out_count)
{
    ANN(visual);
    ANN(out_count);
    *out_count = 0;
    const char* names[] = {"position", "vector", "color", "line_width"};
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
 * Derive vector endpoints from an anchor point and displacement.
 *
 * @param visual the vector visual
 * @param position anchor position
 * @param vector displacement vector
 * @param out_start output start endpoint
 * @param out_end output end endpoint
 */
static void _stroke_quad_vector_endpoints(
    const DvzVisual* visual, const float* position, const float* vector, float* out_start,
    float* out_end)
{
    ANN(visual);
    ANN(position);
    ANN(vector);
    ANN(out_start);
    ANN(out_end);
    float scale = _visual_family_state(visual)->vector.scale;
    float head_factor = 1.0f;
    float tail_factor = 0.0f;
    if (_visual_family_state(visual)->vector.anchor == DVZ_VECTOR_ANCHOR_CENTER)
    {
        tail_factor = -0.5f;
        head_factor = 0.5f;
    }
    else if (_visual_family_state(visual)->vector.anchor == DVZ_VECTOR_ANCHOR_HEAD)
    {
        tail_factor = -1.0f;
        head_factor = 0.0f;
    }

    for (uint32_t k = 0; k < 3; k++)
    {
        float delta = vector[k] * scale;
        out_start[k] = position[k] + tail_factor * delta;
        out_end[k] = position[k] + head_factor * delta;
    }
}



/**
 * Resize a stroke-quad cache array.
 *
 * @param ptr input/output array pointer
 * @param count item count
 * @param item_size byte size of one item
 * @return whether the allocation succeeded
 */
static bool _stroke_quad_cache_resize(void** ptr, uint64_t count, uint64_t item_size)
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
 * Rebuild one segment visual's derived four-vertex/six-index upload cache.
 *
 * @param visual the segment visual
 * @return whether the cache is ready for upload
 */
bool _stroke_quad_segment_cache_rebuild(DvzVisual* visual)
{
    ANN(visual);
    uint64_t item_count = 0;
    if (!_stroke_quad_segment_required_attrs(visual, &item_count))
        return false;
    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 4, &vertex_count) ||
        _dvz_mul_u64_overflows(item_count, 6, &index_count) || vertex_count > UINT32_MAX)
    {
        log_error("segment visual item count is too large");
        return false;
    }

    DvzSegmentGpuCache* cache = &_visual_family_state(visual)->segment.gpu;
    if (!_stroke_quad_cache_resize(
            (void**)&cache->position_start, vertex_count, 3 * sizeof(float)) ||
        !_stroke_quad_cache_resize(
            (void**)&cache->position_end, vertex_count, 3 * sizeof(float)) ||
        !_stroke_quad_cache_resize((void**)&cache->color, vertex_count, sizeof(DvzColor)) ||
        !_stroke_quad_cache_resize((void**)&cache->line_width, vertex_count, sizeof(float)) ||
        !_stroke_quad_cache_resize((void**)&cache->indices, index_count, sizeof(uint32_t)))
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
 * Rebuild one vector visual's straight-arrow upload cache.
 *
 * @param visual the vector visual
 * @return whether the cache is ready for upload
 */
bool _stroke_quad_vector_cache_rebuild(DvzVisual* visual)
{
    ANN(visual);
    uint64_t item_count = 0;
    if (!_stroke_quad_vector_required_attrs(visual, &item_count))
        return false;
    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 4, &vertex_count) ||
        _dvz_mul_u64_overflows(item_count, 6, &index_count) || vertex_count > UINT32_MAX)
    {
        log_error("vector visual item count is too large");
        return false;
    }

    DvzSegmentGpuCache* cache = &_visual_family_state(visual)->vector.stroke_gpu;
    if (!_stroke_quad_cache_resize(
            (void**)&cache->position_start, vertex_count, 3 * sizeof(float)) ||
        !_stroke_quad_cache_resize(
            (void**)&cache->position_end, vertex_count, 3 * sizeof(float)) ||
        !_stroke_quad_cache_resize((void**)&cache->color, vertex_count, sizeof(DvzColor)) ||
        !_stroke_quad_cache_resize((void**)&cache->line_width, vertex_count, sizeof(float)) ||
        !_stroke_quad_cache_resize((void**)&cache->indices, index_count, sizeof(uint32_t)))
    {
        log_error("failed to allocate vector visual derived GPU cache");
        return false;
    }

    const float* position = (const float*)visual->attrs[_attr_index(visual, "position")].data;
    const float* vector = (const float*)visual->attrs[_attr_index(visual, "vector")].data;
    const DvzColor* color = (const DvzColor*)visual->attrs[_attr_index(visual, "color")].data;
    const float* line_width = (const float*)visual->attrs[_attr_index(visual, "line_width")].data;

    for (uint64_t i = 0; i < item_count; i++)
    {
        float start[3] = {0};
        float end[3] = {0};
        _stroke_quad_vector_endpoints(visual, &position[3 * i], &vector[3 * i], start, end);
        for (uint32_t j = 0; j < 4; j++)
        {
            uint64_t dst = 4 * i + j;
            dvz_memcpy(
                &cache->position_start[3 * dst], 3 * sizeof(float), start, 3 * sizeof(float));
            dvz_memcpy(&cache->position_end[3 * dst], 3 * sizeof(float), end, 3 * sizeof(float));
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
