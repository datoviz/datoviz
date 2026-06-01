/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stroke visual internals                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_visual_pipeline.h"
#include "stroke/cache.h"
#include "stroke/derived_upload.h"
#include "stroke/state.h"
#include "upload.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

typedef struct DvzSceneQueryBuildContext DvzSceneQueryBuildContext;
typedef struct DvzSceneQueryPlan DvzSceneQueryPlan;
typedef struct DvzSceneQueryReadoutContext DvzSceneQueryReadoutContext;

typedef bool (*DvzStrokeQueryGeometryFn)(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count,
    uint64_t* out_index_count);

typedef struct
{
    const char* label;
    const char* plan_id;
    DvzVisualType metadata_visual_type;
    uint32_t renderable_kind;
    DvzSceneVisualDescKind desc_kind;
    bool path_stroke;
    DvzStrokeQueryGeometryFn geometry;
} DvzStrokeQueryDesc;

bool _stroke_cap_valid(DvzSegmentCap cap);
bool _stroke_join_valid(DvzPathJoin join);

bool _stroke_quad_segment_cache_rebuild(DvzVisual* visual);
bool _stroke_quad_vector_cache_rebuild(DvzVisual* visual);
bool _path_stroke_cache_rebuild(DvzVisual* visual);
bool _stroke_quad_segment_upload_payloads(
    DvzVisual* visual, DvzVisualUploadPayload* out_payloads, uint32_t* out_count);
bool _stroke_quad_vector_upload_payloads(
    DvzVisual* visual, DvzVisualUploadPayload* out_payloads, uint32_t* out_count);
bool _path_stroke_upload_payloads(
    DvzVisual* visual, DvzVisualUploadPayload* out_payloads, uint32_t* out_count);
bool _stroke_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr);
bool _stroke_query_alloc(
    const char* label, void** out_ptr, uint64_t count, uint64_t item_size);
bool _stroke_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height);
void _stroke_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const DvzVisual* visual,
    const float* request_ndc, uint32_t target_width, uint32_t target_height);
void _stroke_query_mark_last_upload_index(DvzFramePlan* plan, uint32_t stride);
void _stroke_query_mark_last_upload_uniform(DvzFramePlan* plan);
bool _stroke_quad_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count,
    uint64_t* out_index_count);
bool _path_stroke_query_geometry(
    const DvzVisual* visual, DvzSceneQueryScratch* scratch, uint64_t* out_vertex_count,
    uint64_t* out_index_count);
bool _stroke_query_build(
    const DvzSceneQueryBuildContext* ctx, const DvzStrokeQueryDesc* desc,
    DvzSceneQueryPlan* out_plan);
bool _stroke_query_readout(const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result);

int _stroke_set_path_subpaths(
    DvzVisual* visual, uint32_t subpath_count, const uint32_t* lengths, const char* label,
    uint32_t** out_lengths, uint32_t* out_count, DvzPathGpuCache* cache);
