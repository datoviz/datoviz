/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query execution                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_assertions.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Mark retained static uploads after a successful command execution.
 *
 * @param executor retained query executor
 * @param plan query plan carrying static cache versions
 */
static void _query_mark_static_upload(
    DvzSceneRequestExecutor* executor, const DvzSceneQueryPlan* plan)
{
    ANN(executor);
    ANN(plan);
    if (
        !plan->mark_static_cache_uploaded || plan->static_cache_visual == NULL ||
        plan->static_cache_key_count > DVZ_SCENE_QUERY_STATIC_CACHE_KEY_COUNT)
    {
        return;
    }
    executor->query_static_cache_family = plan->static_cache_family;
    executor->query_static_cache_visual = plan->static_cache_visual;
    executor->query_static_cache_key_count = plan->static_cache_key_count;
    for (uint32_t i = 0; i < plan->static_cache_key_count; i++)
        executor->query_static_cache_keys[i] = plan->static_cache_keys[i];
    executor->query_static_cache_upload_count++;
}



/**
 * Initialize a query result from one pending request.
 *
 * @param figure the figure
 * @param pending the pending query
 * @param out_result output result
 */
static void _query_result_init(
    const DvzFigure* figure, const DvzPendingQueryRequest* pending, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(pending);
    ANN(out_result);
    *out_result = (DvzQueryResult){0};
    out_result->request_id = pending->request.request_id;
    out_result->freshness_serial = pending->freshness_serial;
    out_result->status = DVZ_QUERY_STATUS_UNKNOWN;
    out_result->panel_id = _scene_panel_public_id(figure, pending->panel);
    out_result->panel_position[0] = pending->x;
    out_result->panel_position[1] = pending->y;
    (void)_dvz_scene_query_framebuffer_position(
        figure, pending->panel, pending->x, pending->y, out_result->framebuffer_position);
    out_result->raw_target = pending->request.target;
    out_result->resolved_target = pending->request.target;
    out_result->value_kind = DVZ_QUERY_VALUE_NONE;
}



/**
 * Reset retained query resources when the next plan uses a different resource schema.
 *
 * @param executor retained query executor
 * @param family query visual family
 * @param target query target
 */
static void _query_executor_reset_for_schema(
    DvzSceneRequestExecutor* executor, DvzSceneVisualFamily family, DvzSceneTargetKind target)
{
    ANN(executor);
    if (
        executor->active_query_family != DVZ_SCENE_VISUAL_FAMILY_NONE &&
        (executor->active_query_family != family || executor->active_query_target != target))
    {
        _scene_request_executor_destroy(executor);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve one native query request through visual-family query operations.
 *
 * @param figure the figure
 * @param runtime the DRP2 runtime
 * @param executor retained request executor
 * @param caps capability snapshot
 * @param pending pending query request
 * @param out_result output result
 * @return true when a result was produced
 */
bool _dvz_scene_query_process_pending(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps,
    const DvzPendingQueryRequest* pending, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(executor);
    ANN(caps);
    ANN(pending);
    ANN(out_result);
    _query_result_init(figure, pending, out_result);

    vec2 request_ndc = {0};
    if (!_scene_query_request_ndc(figure, pending->panel, pending->x, pending->y, request_ndc))
    {
        out_result->status = DVZ_QUERY_STATUS_OUTSIDE_PANEL;
        return true;
    }

    uint32_t capability = _dvz_scene_query_target_capability(pending->request.target);
    if (capability == 0)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
        return true;
    }

    out_result->profile = _dvz_scene_query_select_profile(&pending->request, caps);
    if (out_result->profile == DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        out_result->status = caps->supports_readback ? DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE
                                                     : DVZ_QUERY_STATUS_READBACK_FAILED;
        return true;
    }

    bool native_attempted = false;
    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(pending->panel, order);
    for (int32_t oi = (int32_t)pending->panel->visual_count - 1; oi >= 0; oi--)
    {
        const DvzPanelAttach* attach = &pending->panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        if ((visual->query_capabilities & capability) == 0)
            continue;
        const DvzSceneQueryFamilyOps* ops =
            _dvz_scene_query_family_ops_for_visual(pending->panel, visual, &pending->request);
        if (ops == NULL || ops->build == NULL || ops->decode == NULL)
            continue;
        native_attempted = true;
        if (_dvz_scene_query_execute_family(
                figure, runtime, executor, caps, pending, request_ndc, out_result->profile,
                visual, ops, out_result))
        {
            out_result->freshness_serial = pending->freshness_serial;
            out_result->profile = _dvz_scene_query_select_profile(&pending->request, caps);
            return true;
        }
    }

    DvzVisual* visual = _dvz_scene_query_candidate_visual(pending->panel, capability);
    if (visual == NULL)
    {
        out_result->status = DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL;
        return true;
    }

    if (native_attempted)
    {
        out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
        out_result->status = DVZ_QUERY_STATUS_MISS;
        return true;
    }

    if (
        pending->request.target == DVZ_SCENE_TARGET_PIXEL ||
        pending->request.target == DVZ_SCENE_TARGET_SEGMENT)
    {
        out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
        return true;
    }

    out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
    out_result->status = runtime == NULL ? DVZ_QUERY_STATUS_GPU_EXEC_FAILED
                                         : DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
    return true;
}



/**
 * Execute one native query with a visual-family operation table.
 *
 * @param figure the figure
 * @param runtime the caller runtime
 * @param executor retained query executor
 * @param caps capability snapshot
 * @param pending pending query request
 * @param request_ndc panel-local NDC coordinate
 * @param profile selected query profile
 * @param visual candidate visual
 * @param ops family operation table
 * @param out_result output result
 * @return true when the family produced a terminal result
 */
bool _dvz_scene_query_execute_family(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingQueryRequest* pending,
    const vec2 request_ndc, DvzQueryProfile profile, DvzVisual* visual,
    const DvzSceneQueryFamilyOps* ops, DvzQueryResult* out_result)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(executor);
    ANN(caps);
    ANN(pending);
    ANN(request_ndc);
    ANN(visual);
    ANN(ops);
    ANN(out_result);
    out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
    out_result->visual_family = ops->family;
    if (ops->build == NULL || ops->decode == NULL)
        return false;
    bool volume_rg32_sample =
        profile == DVZ_QUERY_PROFILE_U64_RG32 && ops->family == DVZ_SCENE_VISUAL_FAMILY_VOLUME &&
        pending->request.target == DVZ_SCENE_TARGET_SAMPLE;
    if (profile != DVZ_QUERY_PROFILE_U32_R32 && !volume_rg32_sample)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE;
        return true;
    }
    if (runtime == NULL)
    {
        out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        return true;
    }
    _query_executor_reset_for_schema(executor, ops->family, pending->request.target);
    if (!_scene_request_executor_prepare(executor, runtime))
    {
        out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        return true;
    }
    executor->active_query_family = ops->family;
    executor->active_query_target = pending->request.target;

    DvzSceneQueryBuildContext build = {
        .figure = figure,
        .panel = pending->panel,
        .visual = visual,
        .executor = executor,
        .pending = pending,
        .caps = caps,
        .profile = profile,
    };
    build.request_ndc[0] = request_ndc[0];
    build.request_ndc[1] = request_ndc[1];

    DvzSceneQueryPlan plan = {0};
    if (!ops->build(&build, &plan))
    {
        _scene_query_scratch_destroy(&plan.scratch);
        return false;
    }

    uint8_t bytes[DVZ_SCENE_QUERY_PAYLOAD_WORDS * sizeof(uint32_t)] = {0};
    bool executed = false;
    bool ok = false;
    if (ops->execute != NULL)
    {
        ok = ops->execute(&build, executor, caps, &plan, bytes, plan.byte_size, &executed);
    }
    else
    {
        ok = _dvz_scene_query_execute_readback(
            figure->scene, executor, caps, plan.scratch.plan, plan.target_width,
            plan.target_height, plan.format, bytes, plan.byte_size, &executed);
    }
    if (executed)
        _query_mark_static_upload(executor, &plan);
    if (!ok)
    {
        out_result->status =
            executed ? DVZ_QUERY_STATUS_READBACK_FAILED : DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        _scene_query_scratch_destroy(&plan.scratch);
        return true;
    }

    DvzSceneQueryDecodeContext decode = {
        .build = &build,
        .plan = &plan,
        .bytes = bytes,
        .byte_size = plan.byte_size,
    };
    if (!ops->decode(&decode, out_result))
    {
        _scene_query_scratch_destroy(&plan.scratch);
        return false;
    }

    if (ops->readout != NULL)
    {
        DvzSceneQueryReadoutContext readout = {
            .build = &build,
            .plan = &plan,
        };
        if (!ops->readout(&readout, out_result))
            out_result->status = DVZ_QUERY_STATUS_DECODE_FAILED;
    }

    _scene_query_scratch_destroy(&plan.scratch);
    return true;
}
