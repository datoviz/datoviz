/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Stroke visual upload payloads                                                                */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "stroke/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Fill segment derived upload payload descriptors.
 *
 * @param visual the segment visual
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @return whether payload descriptors were written
 */
bool _stroke_quad_segment_upload_payloads(
    DvzVisual* visual, DvzVisualUploadPayload* out_payloads, uint32_t* out_count)
{
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    *out_count = 0;
    DvzSegmentGpuCache* cache = &_visual_family_state(visual)->segment.gpu;
    out_payloads[0] = (DvzVisualUploadPayload){
        .name = "position_start",
        .data = cache->position_start,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[1] = (DvzVisualUploadPayload){
        .name = "position_end",
        .data = cache->position_end,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[2] = (DvzVisualUploadPayload){
        .name = "color",
        .data = cache->color,
        .item_size = sizeof(DvzColor),
        .item_count = cache->vertex_count,
    };
    out_payloads[3] = (DvzVisualUploadPayload){
        .name = "line_width",
        .data = cache->line_width,
        .item_size = sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[4] = (DvzVisualUploadPayload){
        .name = "index",
        .data = cache->indices,
        .item_size = sizeof(uint32_t),
        .item_count = cache->index_count,
        .index = true,
    };
    *out_count = 5;
    return true;
}



/**
 * Fill straight-vector derived upload payload descriptors.
 *
 * @param visual the vector visual
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @return whether payload descriptors were written
 */
bool _stroke_quad_vector_upload_payloads(
    DvzVisual* visual, DvzVisualUploadPayload* out_payloads, uint32_t* out_count)
{
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    *out_count = 0;
    DvzSegmentGpuCache* cache = &_visual_family_state(visual)->vector.stroke_gpu;
    out_payloads[0] = (DvzVisualUploadPayload){
        .name = "position_start",
        .data = cache->position_start,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[1] = (DvzVisualUploadPayload){
        .name = "position_end",
        .data = cache->position_end,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[2] = (DvzVisualUploadPayload){
        .name = "color",
        .data = cache->color,
        .item_size = sizeof(DvzColor),
        .item_count = cache->vertex_count,
    };
    out_payloads[3] = (DvzVisualUploadPayload){
        .name = "line_width",
        .data = cache->line_width,
        .item_size = sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[4] = (DvzVisualUploadPayload){
        .name = "index",
        .data = cache->indices,
        .item_size = sizeof(uint32_t),
        .item_count = cache->index_count,
        .index = true,
    };
    *out_count = 5;
    return true;
}



/**
 * Fill stroked-path derived upload payload descriptors.
 *
 * @param visual the path or vector visual
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @return whether payload descriptors were written
 */
bool _path_stroke_upload_payloads(
    DvzVisual* visual, DvzVisualUploadPayload* out_payloads, uint32_t* out_count)
{
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    *out_count = 0;
    DvzPathGpuCache* cache =
        visual->type == DVZ_VISUAL_TYPE_VECTOR ? &_visual_family_state(visual)->vector.path_gpu : &_visual_family_state(visual)->path.gpu;

    out_payloads[0] = (DvzVisualUploadPayload){
        .name = "position_start",
        .data = cache->position_prev,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[1] = (DvzVisualUploadPayload){
        .name = "position",
        .data = cache->position_curr,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[2] = (DvzVisualUploadPayload){
        .name = "position_end",
        .data = cache->position_next,
        .item_size = 3 * sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[3] = (DvzVisualUploadPayload){
        .name = "color",
        .data = cache->color,
        .item_size = sizeof(DvzColor),
        .item_count = cache->vertex_count,
    };
    out_payloads[4] = (DvzVisualUploadPayload){
        .name = "line_width",
        .data = cache->line_width,
        .item_size = sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[5] = (DvzVisualUploadPayload){
        .name = "path_flags",
        .data = cache->path_flags,
        .item_size = sizeof(uint32_t),
        .item_count = cache->vertex_count,
    };
    out_payloads[6] = (DvzVisualUploadPayload){
        .name = "path_distance",
        .data = cache->path_distance,
        .item_size = sizeof(float),
        .item_count = cache->vertex_count,
    };
    out_payloads[7] = (DvzVisualUploadPayload){
        .name = "index",
        .data = cache->indices,
        .item_size = sizeof(uint32_t),
        .item_count = cache->index_count,
        .index = true,
    };
    *out_count = 8;
    return true;
}



/**
 * Resolve dirty stroke-quad upload payloads for a segment-like visual.
 *
 * @param visual the segment or vector visual
 * @param vector_params_sync whether vector-owned stroke params must be synchronized first
 * @param attrs_dirty whether retained visual attributes have a pending dirty range
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @return whether the payload decision succeeded
 */
bool _stroke_quad_segment_derived_upload_payloads(
    DvzVisual* visual, bool vector_params_sync, bool attrs_dirty,
    DvzVisualUploadPayload* out_payloads, uint32_t* out_count)
{
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    *out_count = 0;

    bool dirty = attrs_dirty;
    if (vector_params_sync)
    {
        _vector_sync_params(visual);
        dirty = dirty || _visual_family_state(visual)->vector.stroke_gpu.dirty;
        if (!dirty)
            return true;
        return _stroke_quad_vector_cache_rebuild(visual) &&
               _stroke_quad_vector_upload_payloads(visual, out_payloads, out_count);
    }

    dirty = dirty || _visual_family_state(visual)->segment.gpu.dirty;
    if (!dirty)
        return true;
    return _stroke_quad_segment_cache_rebuild(visual) &&
           _stroke_quad_segment_upload_payloads(visual, out_payloads, out_count);
}



/**
 * Resolve dirty path-stroke upload payloads for a path-like visual.
 *
 * @param visual the path or vector visual
 * @param vector_params_sync whether vector-owned stroke params must be synchronized first
 * @param attrs_dirty whether retained visual attributes have a pending dirty range
 * @param out_payloads output payload descriptors
 * @param out_count output payload count
 * @return whether the payload decision succeeded
 */
bool _path_stroke_derived_upload_payloads(
    DvzVisual* visual, bool vector_params_sync, bool attrs_dirty,
    DvzVisualUploadPayload* out_payloads, uint32_t* out_count)
{
    ANN(visual);
    ANN(out_payloads);
    ANN(out_count);
    *out_count = 0;

    if (vector_params_sync)
        _vector_sync_params(visual);
    DvzPathGpuCache* cache =
        visual->type == DVZ_VISUAL_TYPE_VECTOR ? &_visual_family_state(visual)->vector.path_gpu : &_visual_family_state(visual)->path.gpu;
    bool dirty = cache->dirty || attrs_dirty;
    if (!dirty)
        return true;

    return _path_stroke_cache_rebuild(visual) &&
           _path_stroke_upload_payloads(visual, out_payloads, out_count);
}
