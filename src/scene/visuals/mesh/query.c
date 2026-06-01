/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mesh query policy                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
#include "_visual_pipeline.h"
#include "query_geometry.h"
#include "registry/registry.h"
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
 * Return the offscreen query target extent for one panel.
 *
 * @param figure parent figure
 * @param panel panel receiving the query
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
static bool _mesh_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height)
{
    return _dvz_scene_query_target_extent(figure, panel, out_target_width, out_target_height);
}



/**
 * Apply the request-centered MVP and viewport to a query render node.
 *
 * @param plan frame plan
 * @param panel panel receiving the query
 * @param request_ndc request coordinate in panel-local NDC
 * @param target_width offscreen target width
 * @param target_height offscreen target height
 */
static void _mesh_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const DvzVisual* visual,
    const vec2 request_ndc, uint32_t target_width, uint32_t target_height)
{
    _dvz_scene_query_apply_render_state(plan, panel, visual, request_ndc, target_width, target_height);
}



/**
 * Return whether a mesh visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _mesh_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    return _dvz_scene_query_item_target_eligible(visual, request, DVZ_VISUAL_TYPE_MESH);
}



/**
 * Build a mesh-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _mesh_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    uint64_t vertex_count = 0;
    uint32_t topology = 0;
    if (!_scene_query_indexed_primitive_geometry(
            "mesh", ctx->visual, &out_plan->scratch, &vertex_count, &topology))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_mesh_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
    {
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t id_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &id_bytes))
    {
        log_error("mesh query request buffer size overflow");
        _scene_query_scratch_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.mesh", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position", 0, position_bytes, "position",
                   out_plan->scratch.query_positions);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, topology);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.query_ids);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_MESH;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_INDEXED_MESH;
    metadata.desc_kind = (uint32_t)_scene_visual_family_desc_kind(DVZ_VISUAL_TYPE_PRIMITIVE);
    metadata.topology = topology;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    dvz_strlcpy(metadata.position_id, "query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "query0_id", sizeof(metadata.color_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query", "target.query", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "query0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _mesh_query_apply_render_state(
            plan, ctx->panel, ctx->visual, ctx->request_ndc, target_width, target_height);

    DvzFramePlanCopyDesc copy = dvz_frame_plan_copy_desc();
    copy.src_resource_id = "target.query";
    copy.dst_resource_id = "buf.query";
    copy.extent[0] = 1;
    copy.extent[1] = 1;
    copy.extent[2] = 1;
    copy.format = VK_FORMAT_R32_UINT;
    copy.bytes_per_texel = sizeof(uint32_t);
    copy.bytes_per_row = sizeof(uint32_t);
    copy.rows_per_image = 1;
    copy.byte_size = sizeof(uint32_t);
    copy.request_id = ctx->pending->request.request_id;
    ok = ok && dvz_frame_plan_copy_ex(plan, &copy) &&
         dvz_frame_plan_readback(plan, "buf.query", "request.query");
    if (!ok)
    {
        log_error(
            "mesh query request %" PRIu64 " failed to assemble the GPU readback plan",
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
 * Decode a mesh-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _mesh_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_MESH, out_result);
}



/**
 * Complete mesh-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _mesh_query_readout(
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
 * Return mesh visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_mesh_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "mesh",
        .family = DVZ_SCENE_VISUAL_FAMILY_MESH,
        .eligible = _mesh_query_eligible,
        .build = _mesh_query_build,
        .decode = _mesh_query_decode,
        .readout = _mesh_query_readout,
    };
    return &ops;
}
