/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Path query policy                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
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



/**
 * Build temporary GPU stroke buffers for one stroked path visual.
 *
 * @param visual path visual
 * @param scratch output scratch plan storage
 * @param out_vertex_count output derived vertex count
 * @param out_index_count output derived index count
 * @return true when derived buffers were created
 */
static bool _path_query_geometry(
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
                uint64_t prev_idx =
                    _path_query_prev_index(point_idx, offset, length, closed);
                uint64_t next_idx =
                    _path_query_next_index(point_idx, offset, length, closed);
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



/**
 * Return whether a path visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _path_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_PATH)
    {
        if (visual->type != DVZ_VISUAL_TYPE_VECTOR)
            return false;
        int vector_idx = _attr_index(visual, "vector");
        if (vector_idx >= 0 && visual->attrs[vector_idx].data != NULL &&
            visual->attrs[vector_idx].item_count > 0)
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
    int width_idx = _attr_index(visual, "line_width");
    if (width_idx < 0)
        return false;
    const DvzVisualAttr* width = &visual->attrs[width_idx];
    if (width->data == NULL || width->item_count == 0 || width->item_size != sizeof(float))
        return false;
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a path-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _path_query_build(const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
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
    if (!_path_query_geometry(ctx->visual, &out_plan->scratch, &vertex_count, &index_count))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t id_bytes = 0;
    uint64_t width_bytes = 0;
    uint64_t flags_bytes = 0;
    uint64_t distance_bytes = 0;
    uint64_t index_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, 3 * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &id_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(float), &width_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &flags_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(float), &distance_bytes) ||
        _dvz_mul_u64_overflows(index_count, sizeof(uint32_t), &index_bytes))
    {
        log_error("path query request buffer size overflow");
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.path", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position_start", 0, position_bytes, "position_start",
                   out_plan->scratch.query_position_start) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_position", 0, position_bytes, "position",
             out_plan->scratch.query_position_curr) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_position_end", 0, position_bytes, "position_end",
             out_plan->scratch.query_position_end) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.query_ids) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_line_width", 0, width_bytes, "line_width",
             out_plan->scratch.query_line_width) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_path_flags", 0, flags_bytes, "path_flags",
             out_plan->scratch.query_path_flags) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_path_distance", 0, distance_bytes, "path_distance",
             out_plan->scratch.query_path_distance) &&
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
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_PATH;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_PATH_STROKE;
    metadata.desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_PATH;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    metadata.index_count = (uint32_t)index_count;
    dvz_strlcpy(
        metadata.position_start_id, "query0_position_start", sizeof(metadata.position_start_id));
    dvz_strlcpy(metadata.position_id, "query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.position_end_id, "query0_position_end", sizeof(metadata.position_end_id));
    dvz_strlcpy(metadata.color_id, "query0_id", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.line_width_id, "query0_line_width", sizeof(metadata.line_width_id));
    dvz_strlcpy(metadata.index_id, "query0_index", sizeof(metadata.index_id));
    dvz_strlcpy(metadata.material_id, "query0_material", sizeof(metadata.material_id));
    dvz_strlcpy(metadata.path_flags_id, "query0_path_flags", sizeof(metadata.path_flags_id));
    dvz_strlcpy(
        metadata.path_distance_id, "query0_path_distance",
        sizeof(metadata.path_distance_id));

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
            "path query request %" PRIu64 " failed to assemble the GPU readback plan",
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
 * Decode a path-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _path_query_decode(const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
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
                                    : DVZ_SCENE_VISUAL_FAMILY_PATH;
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
 * Complete path-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _path_query_readout(const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
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
 * Return path visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_path_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "path",
        .family = DVZ_SCENE_VISUAL_FAMILY_PATH,
        .eligible = _path_query_eligible,
        .build = _path_query_build,
        .decode = _path_query_decode,
        .readout = _path_query_readout,
    };
    return &ops;
}
