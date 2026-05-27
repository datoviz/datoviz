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

#include "../_scene.h"
#include "_assertions.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the capability bit required by one query target.
 *
 * @param target the requested scene target
 * @return required capability bit, or zero for unsupported targets
 */
static uint32_t _query_target_capability(DvzSceneTargetKind target)
{
    switch (target)
    {
    case DVZ_SCENE_TARGET_NONE:
        return DVZ_QUERY_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_OBJECT:
        return DVZ_QUERY_CAPABILITY_OBJECT;
    case DVZ_SCENE_TARGET_ITEM:
        return DVZ_QUERY_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_VERTEX:
        return DVZ_QUERY_CAPABILITY_VERTEX;
    case DVZ_SCENE_TARGET_FACE:
    case DVZ_SCENE_TARGET_TRIANGLE:
        return DVZ_QUERY_CAPABILITY_FACE;
    case DVZ_SCENE_TARGET_PIXEL:
        return DVZ_QUERY_CAPABILITY_PIXEL;
    case DVZ_SCENE_TARGET_SAMPLE:
        return DVZ_QUERY_CAPABILITY_SAMPLE;
    case DVZ_SCENE_TARGET_STRIP:
        return DVZ_QUERY_CAPABILITY_GROUP;
    case DVZ_SCENE_TARGET_SEGMENT:
        return DVZ_QUERY_CAPABILITY_ITEM;
    case DVZ_SCENE_TARGET_TEXT:
    case DVZ_SCENE_TARGET_ANNOTATION:
        return DVZ_QUERY_CAPABILITY_OBJECT;
    default:
        return 0;
    }
}



/**
 * Return whether a query profile is supported by the capability snapshot.
 *
 * @param profile the query profile
 * @param caps the capability snapshot
 * @return true when supported
 */
static bool _query_profile_supported(DvzQueryProfile profile, const DvzCapabilitySnapshot* caps)
{
    ANN(caps);
    if (!caps->supports_readback)
        return false;

    switch (profile)
    {
    case DVZ_QUERY_PROFILE_U32_R32:
        return caps->query_profile_u32_r32;
    case DVZ_QUERY_PROFILE_U64_RG32:
        return caps->query_profile_u64_rg32;
    case DVZ_QUERY_PROFILE_U64_2XR32:
        return caps->query_profile_u64_2xr32;
    case DVZ_QUERY_PROFILE_UNSUPPORTED:
    default:
        return false;
    }
}



/**
 * Mark retained image-query static uploads after a successful command execution.
 *
 * @param executor retained query executor
 * @param plan query plan carrying image-query cache versions
 */
static void _query_mark_image_static_upload(
    DvzSceneRequestExecutor* executor, const DvzSceneQueryPlan* plan)
{
    ANN(executor);
    ANN(plan);
    if (!plan->mark_image_query_static_uploaded || plan->image_query_visual == NULL)
        return;
    executor->image_query_visual = plan->image_query_visual;
    executor->image_query_position_version = plan->image_query_position_version;
    executor->image_query_texcoord_version = plan->image_query_texcoord_version;
    executor->image_query_texture_version = plan->image_query_texture_version;
    executor->image_query_static_upload_count++;
}



/**
 * Resolve the effective profile for one query request.
 *
 * @param request the query request
 * @param caps the capability snapshot
 * @return a supported profile, or unsupported
 */
static DvzQueryProfile
_query_select_profile(const DvzQueryRequest* request, const DvzCapabilitySnapshot* caps)
{
    ANN(request);
    ANN(caps);
    if (request->profile != DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        if (_query_profile_supported(request->profile, caps))
            return request->profile;
        return DVZ_QUERY_PROFILE_UNSUPPORTED;
    }
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U32_R32, caps))
        return DVZ_QUERY_PROFILE_U32_R32;
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U64_RG32, caps))
        return DVZ_QUERY_PROFILE_U64_RG32;
    if (_query_profile_supported(DVZ_QUERY_PROFILE_U64_2XR32, caps))
        return DVZ_QUERY_PROFILE_U64_2XR32;
    return DVZ_QUERY_PROFILE_UNSUPPORTED;
}



/**
 * Return one currently drawable query candidate for a panel request.
 *
 * @param panel the panel
 * @param capability required query capability
 * @return the topmost matching visual, or NULL
 */
static DvzVisual* _query_candidate_visual(const DvzPanel* panel, uint32_t capability)
{
    ANN(panel);
    if (capability == 0)
        return NULL;

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        const DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        if ((visual->query_capabilities & capability) != 0)
            return visual;
    }
    return NULL;
}



/**
 * Return the family query operation table eligible for one visual.
 *
 * @param panel the panel
 * @param visual the visual
 * @param request the query request
 * @return operation table, or NULL when no native family path is eligible
 */
static const DvzSceneQueryFamilyOps* _query_family_ops_for_visual(
    const DvzPanel* panel, const DvzVisual* visual, const DvzQueryRequest* request)
{
    ANN(panel);
    ANN(visual);
    ANN(request);
    for (uint32_t i = 0; i < _dvz_scene_query_registry_count(); i++)
    {
        const DvzSceneQueryFamilyOps* ops = _dvz_scene_query_registry_get(i);
        if (ops == NULL || ops->eligible == NULL)
            continue;
        if (ops->eligible(panel, visual, request))
            return ops;
    }
    return NULL;
}



/**
 * Resolve one panel-local query coordinate to a figure framebuffer coordinate.
 *
 * @param figure the figure
 * @param panel the panel
 * @param x panel-local logical x coordinate
 * @param y panel-local logical y coordinate
 * @param out_position output framebuffer coordinate
 * @return true when the framebuffer coordinate was written
 */
static bool _query_framebuffer_position(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y,
    uint32_t out_position[2])
{
    ANN(figure);
    ANN(panel);
    ANN(out_position);
    if (figure->width == 0 || figure->height == 0 || x < 0.0 || y < 0.0)
        return false;

    double fb_x = (double)figure->width * (double)panel->desc.x + x;
    double fb_y = (double)figure->height * (double)panel->desc.y + y;
    if (fb_x < 0.0 || fb_y < 0.0)
        return false;

    uint32_t max_x = figure->width > 0 ? figure->width - 1 : 0;
    uint32_t max_y = figure->height > 0 ? figure->height - 1 : 0;
    out_position[0] = fb_x >= (double)figure->width ? max_x : (uint32_t)fb_x;
    out_position[1] = fb_y >= (double)figure->height ? max_y : (uint32_t)fb_y;
    return true;
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
    (void)_query_framebuffer_position(
        figure, pending->panel, pending->x, pending->y, out_result->framebuffer_position);
    out_result->raw_target = pending->request.target;
    out_result->resolved_target = pending->request.target;
    out_result->value_kind = DVZ_QUERY_VALUE_NONE;
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

    uint32_t capability = _query_target_capability(pending->request.target);
    if (capability == 0)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
        return true;
    }

    out_result->profile = _query_select_profile(&pending->request, caps);
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
            _query_family_ops_for_visual(pending->panel, visual, &pending->request);
        if (ops == NULL || ops->build == NULL || ops->decode == NULL)
            continue;
        native_attempted = true;
        if (_dvz_scene_query_execute_family(
                figure, runtime, executor, caps, pending, request_ndc, out_result->profile,
                visual, ops, out_result))
        {
            out_result->freshness_serial = pending->freshness_serial;
            out_result->profile = _query_select_profile(&pending->request, caps);
            return true;
        }
    }

    DvzVisual* visual = _query_candidate_visual(pending->panel, capability);
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
    if (!_scene_request_executor_prepare(executor, runtime))
    {
        out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        return true;
    }

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
        _query_mark_image_static_upload(executor, &plan);
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
