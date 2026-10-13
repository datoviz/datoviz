/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_time_utils.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct QueryContribution
{
    DvzSceneQueryBuildContext context;
    const DvzSceneQueryFamilyOps* ops;
    DvzSceneQueryPlan retained;
    DvzSceneQueryAppend draw;
    uint32_t count;
} QueryContribution;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Namespace one family-owned buffer into a bounded composed-query visual slot.
 *
 * @param id resource identifier to rewrite
 * @param slot visual slot
 */
static void _query_namespace_id(char* id, uint32_t slot)
{
    if (id[0] == '\0')
        return;
    char previous[DVZ_SCENE_LABEL_SIZE] = {0};
    dvz_strlcpy(previous, id, sizeof(previous));
    dvz_snprintf(id, DVZ_SCENE_LABEL_SIZE, "q%u_%s", slot, previous);
}



/**
 * Import a family's buffer uploads and normalize its one identity draw for composition.
 *
 * @param plan destination plan
 * @param slot bounded resource slot
 * @param source retained family plan
 * @param out normalized identity draw
 * @return whether the source satisfies the append contract
 */
bool _dvz_scene_query_append_identity_uploads(
    DvzFramePlan* plan, uint32_t slot, const DvzSceneQueryPlan* source,
    DvzSceneQueryAppend* out)
{
    if (source->format != DVZ_FORMAT_R32_UINT || source->scratch.plan == NULL)
        return false;
    const DvzFramePlanNode* render = NULL;
    for (uint32_t i = 0; i < source->scratch.plan->count; i++)
    {
        const DvzFramePlanNode* node = &source->scratch.plan->nodes[i];
        if (node->type == DVZ_FRAME_PLAN_NODE_RENDER)
        {
            if (render != NULL || node->u.render.visual_count != 1)
                return false;
            render = node;
        }
        else if (node->type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            if (node->u.upload.external || node->u.upload.texture_width != 0)
                return false;
            char id[DVZ_SCENE_LABEL_SIZE] = {0};
            dvz_strlcpy(id, node->u.upload.resource_id, sizeof(id));
            _query_namespace_id(id, slot);
            DvzFramePlanUploadDesc upload = dvz_frame_plan_upload_desc();
            upload.resource_id = id;
            upload.byte_offset = node->u.upload.byte_offset;
            upload.byte_size = node->u.upload.byte_size;
            upload.data_tag = node->u.upload.data_tag;
            upload.data = node->u.upload.data;
            upload.topology = node->u.upload.topology;
            if (!dvz_frame_plan_upload_ex(plan, &upload) ||
                !dvz_frame_plan_upload_metadata(plan, &node->u.upload.metadata))
                return false;
        }
        else if (node->type != DVZ_FRAME_PLAN_NODE_COPY &&
                 node->type != DVZ_FRAME_PLAN_NODE_READBACK)
            return false;
    }
    if (render == NULL || !render->u.render.has_mvp || !render->u.render.has_viewport)
        return false;
    out->metadata = render->u.render.visual_metadata[0];
    _query_namespace_id(out->metadata.position_id, slot);
    _query_namespace_id(out->metadata.color_id, slot);
    _query_namespace_id(out->metadata.size_id, slot);
    // The GPU selects the nearest surviving identity in one shared depth attachment.
    out->metadata.depth_test_enabled = true;
    out->metadata.depth_compare_op = DVZ_COMPARE_OP_LESS_OR_EQUAL;
    out->mvp = render->u.render.apply_mvp;
    out->viewport = render->u.render.viewport;
    return true;
}



/**
 * Append one R32 identity readback to a composed plan.
 *
 * @param plan composed plan
 * @param request_id readback correlation identity
 * @return whether the readback was appended
 */
static bool _query_append_readback(DvzFramePlan* plan, uint64_t request_id)
{
    DvzFramePlanCopyDesc copy = dvz_frame_plan_copy_desc();
    copy.src_resource_id = "target.query";
    copy.dst_resource_id = "buf.query";
    copy.extent[0] = copy.extent[1] = copy.extent[2] = 1;
    copy.format = DVZ_FORMAT_R32_UINT;
    copy.bytes_per_texel = copy.bytes_per_row = copy.byte_size = sizeof(uint32_t);
    copy.rows_per_image = 1;
    copy.request_id = request_id;
    return dvz_frame_plan_copy_ex(plan, &copy) &&
           dvz_frame_plan_readback(plan, "buf.query", "request.query");
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve multiple family identity draws through one GPU depth and identity pass.
 *
 * @param figure parent figure
 * @param runtime caller runtime
 * @param executor retained query executor
 * @param caps backend capabilities
 * @param pending query request
 * @param request_ndc panel-local query coordinate
 * @param result initialized result
 * @return true for a terminal composed result, false for the ordinary single-family route
 */
bool _dvz_scene_query_process_composed(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, DvzQueryResult* result)
{
    DvzSceneTargetKind target = pending->request.target;
    if (target != DVZ_SCENE_TARGET_NONE && target != DVZ_SCENE_TARGET_OBJECT &&
        target != DVZ_SCENE_TARGET_ITEM && target != DVZ_SCENE_TARGET_FACE)
        return false;
    DvzPanel* panel = pending->panel;
    uint32_t capability = _dvz_scene_query_target_capability(target);
    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    uint32_t candidate_count = 0;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[order[i]];
        const DvzVisual* visual = attach->visual;
        if (visual != NULL && visual->visible && attach->controller_mode != DVZ_CONTROLLER_FIXED &&
            (visual->query_capabilities & capability) != 0)
            candidate_count++;
    }
    if (candidate_count < 2)
        return false;
    if (pending->request.hit_policy != DVZ_QUERY_HIT_FRONTMOST)
    {
        result->status = DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
        return true;
    }
    if (result->profile != DVZ_QUERY_PROFILE_U32_R32)
    {
        result->status = DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE;
        return true;
    }
    if (runtime == NULL)
    {
        result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        return true;
    }

    uint64_t build_start = executor->timing_enabled ? dvz_time_monotonic_ns() : 0;
    QueryContribution* contributions =
        (QueryContribution*)dvz_calloc(candidate_count, sizeof(QueryContribution));
    DvzFramePlan* plan = dvz_frame_plan("figure.query.composed", pending->request.request_id);
    uint32_t count = 0;
    uint32_t identity_end = 0;
    bool ok = contributions != NULL && plan != NULL;
    result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
    for (uint32_t i = 0; ok && i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[order[i]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || attach->controller_mode == DVZ_CONTROLLER_FIXED ||
            (visual->query_capabilities & capability) == 0)
            continue;
        const DvzSceneQueryFamilyOps* ops =
            _dvz_scene_query_family_ops_for_visual(panel, visual, &pending->request);
        if (ops == NULL || ops->identity_count == NULL || ops->append_identity == NULL)
        {
            ok = false;
            break;
        }
        result->visual_id = _scene_visual_public_id(figure->scene, visual);
        result->visual_family = ops->family;
        QueryContribution* contribution = &contributions[count++];
        contribution->ops = ops;
        contribution->context = (DvzSceneQueryBuildContext){
            .figure = figure, .panel = panel, .visual = visual, .pending = pending,
            .caps = caps, .profile = result->profile, .identity_base = identity_end,
        };
        contribution->context.request_ndc[0] = request_ndc[0];
        contribution->context.request_ndc[1] = request_ndc[1];
        ok = ops->identity_count(&contribution->context, &contribution->count);
        if (!ok)
            break;
        if (contribution->count == 0)
            continue;
        if (contribution->count > UINT32_MAX - identity_end)
        {
            result->status = DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE;
            ok = false;
            break;
        }
        identity_end += contribution->count;
        ok = ops->append_identity(
            &contribution->context, plan, count - 1, &contribution->retained, &contribution->draw);
    }
    uint32_t width = 0, height = 0;
    if (ok && identity_end == 0)
    {
        result->status = DVZ_QUERY_STATUS_MISS;
        goto cleanup;
    }
    if (ok)
        ok = _dvz_scene_query_target_extent(figure, panel, &width, &height) &&
             dvz_frame_plan_render_panel(
                 plan, "panel.query", "target.query", true,
                 (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1});
    for (uint32_t i = 0; ok && i < count; i++)
    {
        QueryContribution* contribution = &contributions[i];
        if (contribution->count == 0)
            continue;
        char id[DVZ_SCENE_LABEL_SIZE] = {0};
        dvz_snprintf(id, sizeof(id), "q%u", i);
        ok = dvz_frame_plan_render_visual(plan, id) &&
             dvz_frame_plan_render_visual_metadata(plan, &contribution->draw.metadata);
        if (ok)
        {
            DvzFramePlanNode* render = dvz_frame_plan_last_render_node(plan);
            uint32_t index = render->u.render.visual_count - 1;
            render->u.render.visual_mvp[index] = contribution->draw.mvp;
            render->u.render.visual_has_mvp[index] = true;
            render->u.render.controller_modes[index] = DVZ_CONTROLLER_APPLY;
            render->u.render.has_viewport = true;
            render->u.render.viewport = contribution->draw.viewport;
        }
    }
    if (ok)
        ok = _query_append_readback(plan, pending->request.request_id);
    if (!ok)
        goto cleanup;

    if (!executor->active_query_composed)
        _scene_request_executor_destroy(executor);
    if (!_scene_request_executor_prepare(executor, runtime))
    {
        result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        goto cleanup;
    }
    executor->active_query_composed = true;
    if (executor->timing_enabled)
        executor->last_timing.build_ns += dvz_time_monotonic_ns() - build_start;
    for (uint32_t i = 0; i < count; i++)
    {
        executor->last_timing.derived_vertex_count += contributions[i].retained.derived_vertex_count;
        executor->last_timing.static_upload_bytes += contributions[i].retained.static_upload_bytes;
        executor->last_timing.retained_resource_bytes += contributions[i].retained.retained_resource_bytes;
        if (contributions[i].count > 0)
            executor->last_timing.static_upload_count++;
    }
    uint32_t token = 0;
    bool executed = false;
    ok = _dvz_scene_query_execute_readback(
        figure->scene, executor, caps, plan, width, height, DVZ_FORMAT_R32_UINT,
        (uint8_t*)&token, sizeof(token), &executed);
    if (!ok)
    {
        result->status = executed ? DVZ_QUERY_STATUS_READBACK_FAILED : DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        goto cleanup;
    }
    result->status = token == 0 ? DVZ_QUERY_STATUS_MISS : DVZ_QUERY_STATUS_DECODE_FAILED;
    for (uint32_t i = 0; token != 0 && i < count; i++)
    {
        QueryContribution* contribution = &contributions[i];
        uint32_t base = contribution->context.identity_base;
        if (token <= base || token - base > contribution->count)
            continue;
        uint32_t local_token = token - base;
        DvzSceneQueryDecodeContext decode = {
            .build = &contribution->context, .plan = &contribution->retained,
            .bytes = (const uint8_t*)&local_token, .byte_size = sizeof(local_token),
        };
        if (contribution->ops->decode(&decode, result) && contribution->ops->readout != NULL)
        {
            DvzSceneQueryReadoutContext readout = {
                .build = &contribution->context, .plan = &contribution->retained,
            };
            if (!contribution->ops->readout(&readout, result))
                result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
        }
        break;
    }
cleanup:
    for (uint32_t i = 0; i < count; i++)
        _scene_query_scratch_destroy(&contributions[i].retained.scratch);
    dvz_frame_plan_destroy(plan);
    dvz_free(contributions);
    return true;
}
