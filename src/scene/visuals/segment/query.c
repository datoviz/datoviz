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
#include "stroke/internal.h"
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
    if (!_stroke_quad_query_geometry(ctx->visual, &out_plan->scratch, &vertex_count, &index_count))
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
    ANN(ctx->build->visual);
    DvzSceneVisualFamily family = ctx->build->visual->type == DVZ_VISUAL_TYPE_VECTOR
                                      ? DVZ_SCENE_VISUAL_FAMILY_VECTOR
                                      : DVZ_SCENE_VISUAL_FAMILY_SEGMENT;
    return _dvz_scene_query_decode_item_id(ctx, family, out_result);
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
