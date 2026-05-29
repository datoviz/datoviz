/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sphere query policy                                                                          */
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
 * Return whether one retained attribute has valid dense data.
 *
 * @param visual the visual
 * @param attr_name retained attribute name
 * @param item_size expected item size
 * @param out_attr output attribute
 * @return true when the attribute is present and dense
 */
static bool _sphere_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    return _dvz_scene_query_dense_attr(visual, attr_name, item_size, out_attr);
}



/**
 * Return the offscreen query target extent for one panel.
 *
 * @param figure parent figure
 * @param panel panel receiving the query
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
static bool _sphere_query_target_extent(
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
static void _sphere_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const vec2 request_ndc, uint32_t target_width,
    uint32_t target_height)
{
    _dvz_scene_query_apply_render_state(plan, panel, request_ndc, target_width, target_height);
}



/**
 * Return whether a sphere visual can answer one query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _sphere_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_SPHERE)
        return false;
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_ITEM &&
        request->target != DVZ_SCENE_TARGET_OBJECT)
    {
        return false;
    }
    return (visual->query_capabilities & DVZ_QUERY_CAPABILITY_ITEM) != 0;
}



/**
 * Build a sphere-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _sphere_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    const DvzVisualAttr* pos_attr = NULL;
    const DvzVisualAttr* color_attr = NULL;
    const DvzVisualAttr* size_attr = NULL;
    if (!_sphere_query_attr(ctx->visual, "position", sizeof(vec3), &pos_attr) ||
        !_sphere_query_attr(ctx->visual, "color", sizeof(DvzColor), &color_attr) ||
        !_sphere_query_attr(ctx->visual, "size", sizeof(float), &size_attr))
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
        log_error("sphere query request buffer size overflow");
        return false;
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_sphere_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
        return false;

    DvzFramePlan* plan = dvz_frame_plan("figure.query.sphere", ctx->pending->request.request_id);
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position", 0, position_bytes, "position", pos_attr->data) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_color", 0, color_bytes, "color", color_attr->data) &&
         dvz_frame_plan_upload_bytes(
             plan, "query0_size", 0, size_bytes, "size", size_attr->data);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_SPHERE;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_POINT_LIKE;
    metadata.desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_SPHERE;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = ctx->visual->depth_test_enabled;
    metadata.depth_compare_op = ctx->visual->depth_compare_op;
    dvz_strlcpy(metadata.position_id, "query0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "query0_color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "query0_size", sizeof(metadata.size_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.query", "target.query", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "query0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _sphere_query_apply_render_state(
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
            "sphere query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        return false;
    }

    out_plan->scratch.plan = plan;
    out_plan->target_width = target_width;
    out_plan->target_height = target_height;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    return true;
}



/**
 * Decode a sphere-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _sphere_query_decode(
    const DvzSceneQueryDecodeContext* ctx, DvzQueryResult* out_result)
{
    return _dvz_scene_query_decode_item_id(ctx, DVZ_SCENE_VISUAL_FAMILY_SPHERE, out_result);
}



/**
 * Complete sphere-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _sphere_query_readout(
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
 * Return sphere visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_sphere_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "sphere",
        .family = DVZ_SCENE_VISUAL_FAMILY_SPHERE,
        .eligible = _sphere_query_eligible,
        .build = _sphere_query_build,
        .decode = _sphere_query_decode,
        .readout = _sphere_query_readout,
    };
    return &ops;
}
