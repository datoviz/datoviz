/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Segment query policy                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "../../query/internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build temporary GPU stroke buffers for one segment visual.
 *
 * @param visual segment visual
 * @param scratch output scratch plan storage
 * @param out_vertex_count output derived vertex count
 * @param out_index_count output derived index count
 * @return true when derived buffers were created
 */
static bool _segment_query_geometry(
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
 * Return whether a segment visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _segment_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_SEGMENT)
    {
        if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
            return false;
        int vector_idx = _attr_index(visual, "vector");
        if (vector_idx < 0 || visual->attrs[vector_idx].data == NULL ||
            visual->attrs[vector_idx].item_count == 0)
        {
            return false;
        }
    }
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_ITEM &&
        request->target != DVZ_SCENE_TARGET_OBJECT &&
        request->target != DVZ_SCENE_TARGET_SEGMENT)
    {
        return false;
    }
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a segment-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _segment_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_stroke_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
        return false;

    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (!_segment_query_geometry(ctx->visual, &out_plan->scratch, &vertex_count, &index_count))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t id_bytes = 0;
    uint64_t width_bytes = 0;
    uint64_t index_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, 3 * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &id_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(float), &width_bytes) ||
        _dvz_mul_u64_overflows(index_count, sizeof(uint32_t), &index_bytes))
    {
        log_error("segment query request buffer size overflow");
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.segment", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position_start", 0, position_bytes, "position_start",
                   out_plan->scratch.query_position_start) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_position_end", 0, position_bytes, "position_end",
             out_plan->scratch.query_position_end) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.query_ids) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_line_width", 0, width_bytes, "line_width",
             out_plan->scratch.query_line_width) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_index", 0, index_bytes, "index", out_plan->scratch.query_indices);
    if (ok)
        _stroke_query_mark_last_upload_index(plan, sizeof(uint32_t));
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_material", 0, sizeof(DvzSceneMaterialParams), "material_params",
                   &ctx->visual->material_params);
    if (ok)
        _stroke_query_mark_last_upload_uniform(plan);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_SEGMENT;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_STROKE_QUAD;
    metadata.desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_SEGMENT;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    metadata.index_count = (uint32_t)index_count;
    dvz_strlcpy(
        metadata.position_start_id, "query0_position_start", sizeof(metadata.position_start_id));
    dvz_strlcpy(metadata.position_end_id, "query0_position_end", sizeof(metadata.position_end_id));
    dvz_strlcpy(metadata.color_id, "query0_id", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.line_width_id, "query0_line_width", sizeof(metadata.line_width_id));
    dvz_strlcpy(metadata.index_id, "query0_index", sizeof(metadata.index_id));
    dvz_strlcpy(metadata.material_id, "query0_material", sizeof(metadata.material_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query", "target.query", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "query0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _stroke_query_apply_render_state(
            plan, ctx->panel, ctx->request_ndc, target_width, target_height);

    DvzFramePlanCopyDesc copy = {
        .src_resource_id = "target.query",
        .dst_resource_id = "buf.query",
        .extent = {1, 1, 1},
        .format = VK_FORMAT_R32_UINT,
        .bytes_per_texel = sizeof(uint32_t),
        .bytes_per_row = sizeof(uint32_t),
        .rows_per_image = 1,
        .byte_size = sizeof(uint32_t),
        .request_id = ctx->pending->request.request_id,
    };
    ok = ok && dvz_frame_plan_copy_ex(plan, &copy) &&
         dvz_frame_plan_readback(plan, "buf.query", "request.query");
    if (!ok)
    {
        log_error(
            "segment query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    out_plan->target_width = target_width;
    out_plan->target_height = target_height;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    return true;
}



/**
 * Decode a segment-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _segment_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    ANN(ctx);
    ANN(ctx->build);
    ANN(ctx->build->figure);
    ANN(ctx->build->visual);
    ANN(ctx->bytes);
    ANN(out_result);
    if (ctx->byte_size < sizeof(uint32_t))
    {
        out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
        return true;
    }

    uint32_t encoded = 0;
    dvz_memcpy(&encoded, sizeof(encoded), ctx->bytes, sizeof(encoded));
    if (encoded == 0)
        return false;

    uint64_t item_id = (uint64_t)encoded - 1u;
    out_result->status = DVZ_QUERY_STATUS_HIT;
    out_result->hit = true;
    out_result->visual_id = _scene_visual_public_id(ctx->build->figure->scene, ctx->build->visual);
    out_result->visual_family = ctx->build->visual->type == DVZ_VISUAL_TYPE_VECTOR
                                    ? DVZ_SCENE_VISUAL_FAMILY_VECTOR
                                    : DVZ_SCENE_VISUAL_FAMILY_SEGMENT;
    out_result->payload_version = 1;
    out_result->raw_target = DVZ_SCENE_TARGET_ITEM;
    out_result->raw_id = item_id;
    out_result->resolved_target = DVZ_SCENE_TARGET_ITEM;
    out_result->resolved_id = item_id;
    out_result->item_id = item_id;
    if (ctx->build->visual->link_keys != NULL && item_id < ctx->build->visual->link_key_count)
        out_result->link_key = ctx->build->visual->link_keys[item_id];
    return true;
}



/**
 * Complete segment-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _segment_query_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(result);
    result->value_kind = DVZ_QUERY_VALUE_NONE;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return segment visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_segment_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "segment",
        .family = DVZ_SCENE_VISUAL_FAMILY_SEGMENT,
        .eligible = _segment_query_eligible,
        .build = _segment_query_build,
        .decode = _segment_query_decode,
        .readout = _segment_query_readout,
    };
    return &ops;
}
