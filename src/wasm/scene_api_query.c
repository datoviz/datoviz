/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  WASM scene bridge query packets                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene_api_internal.h"



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
    out_result->scene_id = dvz_scene_id(figure->scene);
    out_result->figure_id = dvz_figure_id(figure);
    out_result->panel_id = _scene_panel_public_id(figure, pending->panel);
    out_result->panel_position[0] = pending->x;
    out_result->panel_position[1] = pending->y;
    (void)_dvz_scene_query_framebuffer_position(
        figure, pending->panel, pending->x, pending->y, out_result->framebuffer_position);
    out_result->raw_target = pending->request.target;
    out_result->resolved_target = pending->request.target;
    out_result->value_kind = DVZ_QUERY_VALUE_NONE;
}



static void _remove_pending_query_at(DvzScene* scene, uint32_t index)
{
    ANN(scene);
    ASSERT(index < scene->pending_query_count);
    for (uint32_t i = index + 1; i < scene->pending_query_count; i++)
        scene->pending_queries[i - 1] = scene->pending_queries[i];
    scene->pending_query_count--;
    dvz_memset(
        &scene->pending_queries[scene->pending_query_count], sizeof(DvzPendingQueryRequest), 0,
        sizeof(DvzPendingQueryRequest));
}



static bool _push_immediate_query_result(
    DvzWasmApiScene* scene, DvzPanel* panel, uint64_t freshness_serial,
    const DvzQueryResult* result)
{
    ANN(scene);
    ANN(scene->scene);
    ANN(result);
    return _dvz_scene_query_push_result(scene->scene, panel, freshness_serial, result);
}



static const char* _query_family_name(const DvzSceneQueryFamilyOps* ops)
{
    return ops != NULL && ops->name != NULL ? ops->name : "<none>";
}



static void _query_setup_diagnostic(
    DvzWasmApiScene* scene, const char* reason, const DvzSceneQueryFamilyOps* ops,
    DvzVisualType visual_type, uint64_t visual_id, DvzSceneTargetKind target,
    DvzQueryProfile profile)
{
    if (scene == NULL || reason == NULL)
        return;

    char diagnostic[DVZ_SCENE_DIAGNOSTIC_SIZE];
    int ret = snprintf(
        diagnostic, sizeof(diagnostic),
        "WASM query setup failed: family=%s visual_type=%u visual_id=%llu target=%u "
        "profile=%u reason=%s",
        _query_family_name(ops), (uint32_t)visual_type, (unsigned long long)visual_id,
        (uint32_t)target, (uint32_t)profile, reason);
    if (ret < 0 || (size_t)ret >= sizeof(diagnostic))
        (void)dvz_diagnostic_report_add(&scene->report, "WASM query setup failed");
    else
        (void)dvz_diagnostic_report_add(&scene->report, diagnostic);
}



static int _emit_current_query_packets(DvzWasmApiScene* scene, DvzDrp2CommandStream* stream)
{
    ANN(scene);
    ANN(stream);
    scene->frame_index++;
    scene->resource_version++;
    scene->frame_artifact =
        _scene_frame_artifact(stream, scene->resource_version, scene->frame_index);
    if (scene->frame_artifact == NULL)
    {
        (void)_fail(scene, "WASM query frame artifact creation failed");
        scene->packet_status = -2;
        return -1;
    }
    if (dvz_scene_frame_artifact_status(scene->frame_artifact) !=
        DVZ_SCENE_FRAME_ARTIFACT_STATUS_OK)
    {
        (void)_fail(scene, "WASM query DRP2 packet encoding failed");
        dvz_scene_frame_artifact_destroy(scene->frame_artifact);
        scene->frame_artifact = NULL;
        scene->packet_status = -2;
        return -1;
    }
    scene->packet_status = 0;
    return 0;
}
static void _mark_query_static_upload(DvzWasmApiScene* scene)
{
    ANN(scene);
    DvzSceneRequestExecutor* executor = &scene->scene->query_executor;
    const DvzSceneQueryPlan* plan = &scene->query_plan;
    if (!plan->mark_static_cache_uploaded || plan->static_cache_visual == NULL ||
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


static bool _query_emit_result(
    DvzWasmApiScene* scene, DvzFigure* figure, const DvzPendingQueryRequest* pending,
    DvzQueryResult* out_result)
{
    ANN(scene);
    ANN(figure);
    ANN(pending);
    ANN(out_result);
    _query_result_init(figure, pending, out_result);

    vec2 request_ndc = {0};
    if (!_scene_query_request_ndc(figure, pending->panel, pending->x, pending->y, request_ndc))
    {
        out_result->status = DVZ_QUERY_STATUS_OUTSIDE_PANEL;
        return false;
    }

    uint32_t capability = _dvz_scene_query_target_capability(pending->request.target);
    if (capability == 0)
    {
        out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_TARGET;
        return false;
    }

    out_result->profile = _dvz_scene_query_select_profile(&pending->request, &scene->caps);
    if (out_result->profile == DVZ_QUERY_PROFILE_UNSUPPORTED)
    {
        out_result->status = scene->caps.supports_readback
                                 ? DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE
                                 : DVZ_QUERY_STATUS_READBACK_FAILED;
        return false;
    }

    bool attempted = false;
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

        attempted = true;
        uint64_t visual_id = _scene_visual_public_id(figure->scene, visual);
        const DvzSceneQueryFamilyOps* ops =
            _dvz_scene_query_family_ops_for_visual(pending->panel, visual, &pending->request);
        if (ops == NULL || ops->build == NULL || ops->decode == NULL)
        {
            out_result->visual_id = visual_id;
            out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
            _query_setup_diagnostic(
                scene, "missing visual-family query operations", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }
        DvzSceneVisualFamily family = ops->family;

        DvzSceneQueryBuildContext build = {
            .figure = figure,
            .panel = pending->panel,
            .visual = visual,
            .executor = &scene->scene->query_executor,
            .pending = pending,
            .caps = &scene->caps,
            .profile = out_result->profile,
        };
        build.request_ndc[0] = request_ndc[0];
        build.request_ndc[1] = request_ndc[1];
        bool supports_profile = ops->supports_profile != NULL
                                    ? ops->supports_profile(&build, out_result->profile)
                                    : out_result->profile == DVZ_QUERY_PROFILE_U32_R32;
        if (!supports_profile)
        {
            out_result->visual_id = visual_id;
            out_result->visual_family = family;
            out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE;
            _query_setup_diagnostic(
                scene, "unsupported query profile for visual family", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }

        DvzSceneQueryPlan plan = {0};
        bool built = ops->build(&build, &plan);
        if (!built)
        {
            _scene_query_scratch_destroy(&plan.scratch);
            out_result->visual_id = visual_id;
            out_result->visual_family = family;
            out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
            _query_setup_diagnostic(
                scene, "visual-family query plan build failed", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }
        if (!dvz_frame_plan_render_metadata_complete(plan.scratch.plan))
        {
            out_result->visual_id = visual_id;
            out_result->visual_family = family;
            out_result->status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
            _scene_query_scratch_destroy(&plan.scratch);
            _query_setup_diagnostic(
                scene, "query frame plan render metadata incomplete", ops, visual->type, visual_id,
                pending->request.target, out_result->profile);
            return false;
        }

        out_result->visual_id = visual_id;
        out_result->visual_family = family;
        scene->query_pending = *pending;
        scene->query_build = build;
        scene->query_build.pending = &scene->query_pending;
        scene->query_plan = plan;
        scene->query_result = *out_result;
        scene->query_ops = ops;
        scene->query_visual_type = visual->type;
        scene->query_family = family;
        scene->query_panel = pending->panel;
        scene->query_active = true;
        return true;
    }

    DvzVisual* visual = _dvz_scene_query_candidate_visual(pending->panel, capability);
    if (visual == NULL)
    {
        out_result->status = DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL;
        return false;
    }

    out_result->visual_id = _scene_visual_public_id(figure->scene, visual);
    if (attempted)
        out_result->status = DVZ_QUERY_STATUS_MISS;
    else
    {
        const DvzSceneQueryFamilyOps* fallback_ops =
            _dvz_scene_query_registry_find_visual_type(visual->type);
        DvzQueryStatus unsupported_status = DVZ_QUERY_STATUS_UNKNOWN;
        if (fallback_ops != NULL && fallback_ops->reject_unsupported != NULL &&
            fallback_ops->reject_unsupported(visual, &pending->request, &unsupported_status))
        {
            out_result->status = unsupported_status;
        }
        else
        {
            out_result->status = DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY;
        }
    }
    return false;
}
EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_query_pending_count(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->scene != NULL ? scene->scene->pending_query_count : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_query_active(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->query_active ? 1 : 0;
}



EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_api_query_readback_size(uint32_t scene_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    return scene != NULL && scene->query_active ? scene->query_plan.byte_size : 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_emit_query_packets(uint32_t scene_handle, uint32_t figure_handle)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    DvzWasmApiFigure* figure = _figure(figure_handle);
    if (scene == NULL || figure == NULL || figure->owner != scene || figure->figure == NULL)
    {
        int ret = _fail(scene, "invalid WASM query packet emit request");
        if (scene != NULL)
            scene->packet_status = -1;
        return ret;
    }

    _clear_query(scene);
    _clear_payload(scene);
    DvzScene* owner = scene->scene;
    DvzFigure* target = figure->figure;
    if (owner == NULL)
        return _fail(scene, "invalid WASM query scene");
    if (owner->pending_query_count == 0)
        return 0;
    if (!_scene_figure_resolve_layouts(target))
        return _fail(scene, "WASM query layout resolution failed");

    uint32_t pending_index = UINT32_MAX;
    DvzPendingQueryRequest pending = {0};
    for (uint32_t i = 0; i < owner->pending_query_count; i++)
    {
        if (owner->pending_queries[i].panel != NULL &&
            owner->pending_queries[i].panel->figure == target)
        {
            pending_index = i;
            pending = owner->pending_queries[i];
            break;
        }
    }
    if (pending_index == UINT32_MAX)
        return 0;

    DvzQueryResult immediate = {0};
    bool needs_gpu = _query_emit_result(scene, target, &pending, &immediate);
    if (!needs_gpu)
    {
        _remove_pending_query_at(owner, pending_index);
        if (!_push_immediate_query_result(
                scene, pending.panel, pending.freshness_serial, &immediate))
            return _fail(scene, "WASM query result queue push failed");
        return 0;
    }

    dvz_diagnostic_report_init(&scene->report);
    DvzCapabilitySnapshot query_caps = _wasm_capability_snapshot();
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.target_width =
        scene->query_plan.target_width > 0 ? scene->query_plan.target_width : 1;
    emit_cfg.target_height =
        scene->query_plan.target_height > 0 ? scene->query_plan.target_height : 1;
    emit_cfg.color_target_format = scene->query_plan.format;

    DvzSceneRequestExecutor* executor = &owner->query_executor;
    if (executor->emitter == NULL)
    {
        if (!_ensure_query_emitter(owner))
            return _fail(scene, "WASM query emitter creation failed");
    }

    DvzDrp2CommandStream* query_stream = dvz_frame_plan_emitter_emit_drp2(
        executor->emitter, scene->query_plan.scratch.plan, &query_caps, &scene->report, &emit_cfg);
    if (query_stream == NULL)
    {
        DvzQueryResult result = scene->query_result;
        result.status = DVZ_QUERY_STATUS_GPU_EXEC_FAILED;
        if (dvz_diagnostic_report_count(&scene->report) == 0)
        {
            _query_setup_diagnostic(
                scene, "DRP2 query stream snapshot emission failed", scene->query_ops,
                scene->query_visual_type, result.visual_id, pending.request.target,
                result.profile);
        }
        _remove_pending_query_at(owner, pending_index);
        _clear_query(scene);
        if (!_push_immediate_query_result(scene, pending.panel, pending.freshness_serial, &result))
            return _fail(scene, "WASM query emission failure push failed");
        return 0;
    }

    if (_emit_current_query_packets(scene, query_stream) != 0)
        return -1;
    _remove_pending_query_at(owner, pending_index);
    return 0;
}



EMSCRIPTEN_KEEPALIVE
int dvz_wasm_api_query_resolve(uint32_t scene_handle, const uint8_t* bytes, uint32_t byte_size)
{
    DvzWasmApiScene* scene = _scene(scene_handle);
    if (scene == NULL || scene->scene == NULL || !scene->query_active)
        return _fail(scene, "WASM query resolve requested without an active query");
    if (bytes == NULL || byte_size < scene->query_plan.byte_size)
        return _fail(scene, "WASM query readback payload is too small");

    DvzQueryResult result = scene->query_result;
    DvzSceneQueryDecodeContext decode = {
        .build = &scene->query_build,
        .plan = &scene->query_plan,
        .bytes = bytes,
        .byte_size = scene->query_plan.byte_size,
    };
    bool decoded = false;
    if (scene->query_visual_type == DVZ_VISUAL_TYPE_POINT)
        decoded = _point_query_decode(&decode, &result);
    else if (scene->query_ops != NULL && scene->query_ops->decode != NULL)
        decoded = scene->query_ops->decode(&decode, &result);
    if (!decoded)
        result.status = DVZ_QUERY_STATUS_MISS;

    if (scene->query_ops != NULL && scene->query_ops->readout != NULL)
    {
        DvzSceneQueryReadoutContext readout = {
            .build = &scene->query_build,
            .plan = &scene->query_plan,
        };
        if (!scene->query_ops->readout(&readout, &result))
            result.status = DVZ_QUERY_STATUS_DECODE_FAILED;
    }

    _mark_query_static_upload(scene);
    DvzPanel* panel = scene->query_panel;
    const uint64_t freshness_serial = scene->query_pending.freshness_serial;
    _clear_query(scene);
    if (!_push_immediate_query_result(scene, panel, freshness_serial, &result))
        return _fail(scene, "WASM query result queue push failed");
    return 0;
}
