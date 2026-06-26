/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Visual point-like query helpers                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "query_point_like.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether one retained point-like attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
static bool _query_point_like_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    return _dvz_scene_query_dense_attr(visual, attr_name, item_size, out_attr);
}



static bool _query_point_like_upload_bytes(
    DvzFramePlan* plan, const char* resource_id, uint64_t byte_size, const char* data_tag,
    const void* data, DvzFramePlanResourceRole role, DvzVisualType visual_type,
    uint64_t item_count, uint32_t buffer_index)
{
    ANN(plan);
    DvzFramePlanUploadMeta meta = {
        .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
        .role = role,
        .visual_type = (uint32_t)visual_type,
        .buffer_index = buffer_index,
        .logical_item_count = item_count,
    };
    return dvz_frame_plan_upload_bytes(plan, resource_id, 0, byte_size, data_tag, data) &&
           dvz_frame_plan_upload_metadata(plan, &meta);
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
static void _query_point_like_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const DvzVisual* visual,
    const vec2 request_ndc, uint32_t target_width, uint32_t target_height)
{
    _dvz_scene_query_apply_render_state(plan, panel, visual, request_ndc, target_width, target_height);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Build a point-like r32uint item query plan.
 *
 * @param ctx build context
 * @param desc point-like query descriptor
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
bool _scene_query_point_like_build_ex(
    const DvzSceneQueryBuildContext* ctx, const char* label, const char* plan_id,
    DvzVisualType metadata_visual_type, DvzSceneVisualDescKind desc_kind,
    DvzScenePointLikeKind point_like_kind, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(label);
    ANN(plan_id);
    ANN(out_plan);
    DvzFigure* figure = ctx->figure;
    DvzPanel* panel = ctx->panel;
    DvzVisual* visual = ctx->visual;
    const DvzPendingQueryRequest* pending = ctx->pending;
    vec2 request_ndc = {ctx->request_ndc[0], ctx->request_ndc[1]};

    const DvzVisualAttr* pos_attr = NULL;
    const DvzVisualAttr* color_attr = NULL;
    const DvzVisualAttr* size_attr = NULL;
    if (!_query_point_like_attr(visual, "position", sizeof(vec3), &pos_attr) ||
        !_query_point_like_attr(visual, "color", sizeof(DvzColor), &color_attr) ||
        !_query_point_like_attr(visual, "size", sizeof(float), &size_attr))
    {
        return false;
    }
    if (
        color_attr->item_count != pos_attr->item_count ||
        size_attr->item_count != pos_attr->item_count)
    {
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t size_bytes = 0;
    if (_dvz_mul_u64_overflows(pos_attr->item_count, pos_attr->item_size, &position_bytes) ||
        _dvz_mul_u64_overflows(color_attr->item_count, color_attr->item_size, &color_bytes) ||
        _dvz_mul_u64_overflows(size_attr->item_count, size_attr->item_size, &size_bytes))
    {
        log_error("%s query request buffer size overflow", label);
        return false;
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_dvz_scene_query_target_extent(figure, panel, &target_width, &target_height))
        return false;

    DvzFramePlan* plan = dvz_frame_plan(plan_id, pending->request.request_id);
    bool ok = plan != NULL;
    ok = ok &&
         _query_point_like_upload_bytes(
             plan, "query0_position", position_bytes, "position", pos_attr->data,
             DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION, metadata_visual_type, pos_attr->item_count, 0) &&
         _query_point_like_upload_bytes(
             plan, "query0_color", color_bytes, "color", color_attr->data,
             DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR, metadata_visual_type, color_attr->item_count, 1) &&
         _query_point_like_upload_bytes(
             plan, "query0_size", size_bytes, "size", size_attr->data,
             DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE, metadata_visual_type, size_attr->item_count, 2);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)metadata_visual_type;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_POINT_LIKE;
    metadata.desc_kind = (uint32_t)desc_kind;
    metadata.point_like_kind = (uint32_t)point_like_kind;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    metadata.depth_compare_op = visual->depth_compare_op;
    metadata.vertex_count =
        pos_attr->item_count <= UINT32_MAX ? (uint32_t)pos_attr->item_count : 0;
    metadata.has_item_range = visual->has_item_range;
    metadata.item_range_first = visual->item_range_first;
    metadata.item_range_count = visual->item_range_count;
    dvz_strlcpy(metadata.position_id, "query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "query0_color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "query0_size", sizeof(metadata.size_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query", "target.query", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "query0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
    {
        _query_point_like_apply_render_state(
            plan, panel, visual, request_ndc, target_width, target_height);
    }

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
    copy.request_id = pending->request.request_id;
    ok = ok && dvz_frame_plan_copy_ex(plan, &copy) &&
         dvz_frame_plan_readback(plan, "buf.query", "request.query");
    if (!ok)
    {
        log_error(
            "%s query request %" PRIu64 " failed to assemble the GPU readback plan",
            label, pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        return false;
    }

    out_plan->scratch.plan = plan;
    out_plan->target_width = target_width;
    out_plan->target_height = target_height;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    out_plan->schema.fields = DVZ_SCENE_QUERY_SCHEMA_FIELD_ITEM_ID;
    out_plan->schema.value_kind = DVZ_QUERY_VALUE_NONE;
    out_plan->schema.profile = ctx->profile;
    out_plan->schema.format = VK_FORMAT_R32_UINT;
    out_plan->schema.byte_size = sizeof(uint32_t);
    return true;
}



bool _scene_query_point_like_build(
    const DvzSceneQueryBuildContext* ctx, const DvzScenePointLikeQueryDesc* desc,
    DvzSceneQueryPlan* out_plan)
{
    ANN(desc);
    return _scene_query_point_like_build_ex(
        ctx, desc->label, desc->plan_id, desc->metadata_visual_type, desc->desc_kind,
        desc->point_like_kind, out_plan);
}



/**
 * Complete point-like readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
bool _scene_query_point_like_readout(
    const DvzSceneQueryReadoutContext* ctx, DvzQueryResult* result)
{
    ANN(ctx);
    ANN(result);
    result->value_kind = DVZ_QUERY_VALUE_NONE;
    return true;
}
