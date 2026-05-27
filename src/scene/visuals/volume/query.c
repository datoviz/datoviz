/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Volume query policy                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/math/_cglm.h"
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
static bool _volume_query_attr(
    const DvzVisual* visual, const char* attr_name, uint32_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    ANN(out_attr);
    int attr_idx = _attr_index(visual, attr_name);
    if (attr_idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[attr_idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    *out_attr = attr;
    return true;
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
static bool _volume_query_target_extent(
    const DvzFigure* figure, const DvzPanel* panel, uint32_t* out_target_width,
    uint32_t* out_target_height)
{
    ANN(figure);
    ANN(panel);
    ANN(out_target_width);
    ANN(out_target_height);
    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;
    uint32_t target_width = (uint32_t)(panel_width + 0.5);
    uint32_t target_height = (uint32_t)(panel_height + 0.5);
    *out_target_width = target_width == 0 ? 1 : target_width;
    *out_target_height = target_height == 0 ? 1 : target_height;
    return true;
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
static void _volume_query_apply_render_state(
    DvzFramePlan* plan, const DvzPanel* panel, const vec2 request_ndc, uint32_t target_width,
    uint32_t target_height)
{
    ANN(plan);
    ANN(panel);
    ANN(request_ndc);
    DvzFramePlanNode* render = dvz_frame_plan_last_render_node(plan);
    if (render == NULL)
        return;

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    vec2 target_ndc = {
        -1.0f + 1.0f / (float)target_width,
        1.0f - 1.0f / (float)target_height,
    };
    vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
    mvp.proj[3][0] -= delta[0];
    mvp.proj[3][1] -= delta[1];
    render->u.render.has_mvp = true;
    render->u.render.apply_mvp = mvp;
    render->u.render.has_viewport = true;
    render->u.render.viewport =
        (DvzSceneViewportUniform){0.0f, 0.0f, (float)target_width, (float)target_height};
    render->u.render.controller_modes[0] = DVZ_CONTROLLER_APPLY;
}



/**
 * Return whether a volume visual can answer one identity query request.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request query request
 * @return true when the family should try the request
 */
static bool _volume_query_eligible(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return false;
    if (request->target != DVZ_SCENE_TARGET_NONE && request->target != DVZ_SCENE_TARGET_ITEM &&
        request->target != DVZ_SCENE_TARGET_OBJECT)
    {
        return false;
    }
    return (visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) != 0;
}



/**
 * Build a volume-family r32uint item query plan.
 *
 * @param ctx build context
 * @param out_plan output query plan
 * @return true when the plan was assembled
 */
static bool _volume_query_build(
    const DvzSceneQueryBuildContext* ctx, DvzSceneQueryPlan* out_plan)
{
    ANN(ctx);
    ANN(ctx->figure);
    ANN(ctx->panel);
    ANN(ctx->visual);
    ANN(ctx->pending);
    ANN(out_plan);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_volume_query_attr(ctx->visual, "position", sizeof(vec3), &pos_attr) ||
        pos_attr->item_count > UINT32_MAX)
    {
        return false;
    }
    uint64_t vertex_count = pos_attr->item_count;

    if ((out_plan->scratch.pick_ids = (uint32_t*)dvz_calloc(vertex_count, sizeof(uint32_t))) == NULL)
        return false;
    for (uint64_t i = 0; i < vertex_count; i++)
        out_plan->scratch.pick_ids[i] = 1u;

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_volume_query_target_extent(ctx->figure, ctx->panel, &target_width, &target_height))
    {
        _scene_probe_plan_destroy(&out_plan->scratch);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t id_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &id_bytes))
    {
        log_error("volume query request buffer size overflow");
        _scene_probe_plan_destroy(&out_plan->scratch);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.query.volume", ctx->pending->request.request_id);
    out_plan->scratch.plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_position", 0, position_bytes, "position", pos_attr->data);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "query0_id", 0, id_bytes, "query_id", out_plan->scratch.pick_ids);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_PRIMITIVE;
    metadata.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
        _volume_query_apply_render_state(
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
            "volume query request %" PRIu64 " failed to assemble the GPU readback plan",
            ctx->pending->request.request_id);
        _scene_probe_plan_destroy(&out_plan->scratch);
        return false;
    }

    out_plan->target_width = target_width;
    out_plan->target_height = target_height;
    out_plan->format = VK_FORMAT_R32_UINT;
    out_plan->byte_size = sizeof(uint32_t);
    return true;
}



/**
 * Decode a volume-family r32uint item query payload.
 *
 * @param ctx decode context
 * @param out_result output query result
 * @return true when a terminal result was produced
 */
static bool _volume_query_decode(
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
    out_result->visual_family = DVZ_SCENE_VISUAL_FAMILY_VOLUME;
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
 * Complete volume-family readout fields after decode.
 *
 * @param ctx readout context
 * @param result query result
 * @return true when readout succeeded
 */
static bool _volume_query_readout(
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
 * Return volume visual query operations.
 *
 * @return query operation table
 */
const DvzSceneQueryFamilyOps* _dvz_scene_query_volume_ops(void)
{
    static const DvzSceneQueryFamilyOps ops = {
        .name = "volume",
        .family = DVZ_SCENE_VISUAL_FAMILY_VOLUME,
        .pick_capabilities = DVZ_PICK_CAPABILITY_OBJECT | DVZ_PICK_CAPABILITY_ITEM |
                             DVZ_PICK_CAPABILITY_SAMPLE,
        .eligible = _volume_query_eligible,
        .build = _volume_query_build,
        .decode = _volume_query_decode,
        .readout = _volume_query_readout,
    };
    return &ops;
}
