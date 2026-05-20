/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene request execution                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "../drp2/_stream.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzScenePickPayload DvzScenePickPayload;
typedef struct DvzSceneProbePayload DvzSceneProbePayload;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzScenePickPayload
{
    DvzSceneVisualFamily visual_family;
    DvzSceneTargetKind raw_target;
    uint64_t raw_id;
    DvzSceneTargetKind resolved_target;
    uint64_t resolved_id;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id;
};



struct DvzSceneProbePayload
{
    DvzProbeStatus status;
    DvzSceneVisualFamily visual_family;
    DvzSceneTargetKind target;
    uint64_t target_id;
    uint64_t item_id;
    uint64_t group_id;
    uint64_t auxiliary_id;
    DvzProbeValueKind value_kind;
    double vector[4];
    uint64_t category_id;
    char label[DVZ_SCENE_LABEL_SIZE];
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzSceneVisualFamily _scene_visual_family(uint32_t visual_type);

static bool _scene_pick_target_supported(DvzSceneTargetKind target);

static bool _scene_probe_target_supported(DvzSceneTargetKind target);

static DvzPickResult _scene_pick_miss_result(
    const DvzFigure* figure, const DvzPanel* panel, const DvzPendingPickRequest* pending,
    DvzPickStatus status);

static DvzProbeResult _scene_probe_miss_result(
    const DvzFigure* figure, const DvzPanel* panel, const DvzPendingProbeRequest* pending,
    DvzProbeStatus status);

static bool _scene_decode_point_like_pick_payload(
    const DvzVisual* visual, const uint8_t rgba[4], DvzScenePickPayload* out_payload);

static void _scene_apply_pick_payload(
    const DvzScene* scene, const DvzVisual* visual, const DvzScenePickPayload* payload,
    DvzPickResult* out_result);

static bool _scene_decode_image_probe_payload(
    const DvzVisual* visual, bool segment_probe, const uint8_t rgba[4],
    DvzSceneProbePayload* out_payload);

static void _scene_apply_probe_payload(
    const DvzScene* scene, const DvzVisual* visual, const DvzSceneProbePayload* payload,
    DvzProbeResult* out_result);

static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint8_t rgba[4],
    bool* out_executed);

static bool _scene_runtime_config_matches(
    const DvzDrp2RuntimeConfig* a, const DvzDrp2RuntimeConfig* b);

static bool _scene_request_executor_prepare(
    DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime);

static bool _scene_image_probe_static_versions(
    const DvzVisual* visual, uint64_t* out_position_version, uint64_t* out_texcoord_version,
    uint64_t* out_texture_version);

static bool _scene_image_probe_needs_static_upload(
    const DvzSceneRequestExecutor* executor, const DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version);

static void _scene_image_probe_mark_static_uploaded(
    DvzSceneRequestExecutor* executor, DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version);

static bool _scene_probe_request_has_image_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_probe_request_has_volume_slice_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_pick_request_has_point_like_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_point_like_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending);

static bool _scene_process_volume_slice_probe_request(
    DvzFigure* figure, const DvzPendingProbeRequest* pending);



/**
 * Execute queued pick/probe requests for one figure through dedicated DRP2 readback streams.
 *
 * @param figure the figure
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of consumed requests
 */
uint32_t dvz_figure_process_requests(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps)
{
    DvzSceneRequestExecutor executor = {0};
    _scene_request_executor_init(&executor);
    uint32_t processed =
        _dvz_figure_process_requests_with_executor(figure, runtime, &executor, caps);
    _scene_request_executor_destroy(&executor);
    return processed;
}



/**
 * Execute queued pick/probe requests with a caller-owned retained request executor.
 *
 * @param figure the figure
 * @param runtime the caller's main DRP2 runtime
 * @param executor the retained request executor
 * @param caps the capability snapshot, or NULL for defaults
 * @return the number of consumed requests
 */
uint32_t _dvz_figure_process_requests_with_executor(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(runtime);
    ANN(executor);

    DvzCapabilitySnapshot local_caps = {0};
    if (caps == NULL)
    {
        dvz_capability_snapshot_default(&local_caps);
        local_caps.shader_format_glsl = true;
        caps = &local_caps;
    }

    DvzScene* scene = figure->scene;
    uint32_t processed = 0;
    _scene_coalesce_pending_pick_requests(scene, figure);
    _scene_coalesce_pending_probe_requests(scene, figure);

    if (scene->pending_pick_count == 0 && scene->pending_probe_count == 0)
        return 0;

    for (uint32_t i = 0; i < scene->pending_pick_count;)
    {
        const DvzPendingPickRequest pending = scene->pending_picks[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }
        if (_scene_pick_request_has_point_like_candidate(figure, &pending))
            (void)_scene_request_executor_prepare(executor, runtime);
        (void)_scene_process_point_pick_request(figure, executor, caps, &pending);
        _scene_remove_pending_pick_at(scene, i);
        processed++;
    }

    for (uint32_t i = 0; i < scene->pending_probe_count;)
    {
        const DvzPendingProbeRequest pending = scene->pending_probes[i];
        if (pending.panel == NULL || pending.panel->figure != figure)
        {
            i++;
            continue;
        }
        if (_scene_probe_request_has_volume_slice_candidate(figure, &pending))
        {
            (void)_scene_process_volume_slice_probe_request(figure, &pending);
            _scene_remove_pending_probe_at(scene, i);
            processed++;
            continue;
        }
        if (_scene_probe_request_has_image_candidate(figure, &pending))
            (void)_scene_request_executor_prepare(executor, runtime);
        (void)_scene_process_image_probe_request(figure, executor, caps, &pending);
        _scene_remove_pending_probe_at(scene, i);
        processed++;
    }

    return processed;
}



/**
 * Initialize a retained scene request executor.
 *
 * @param executor the request executor
 */
void _scene_request_executor_init(DvzSceneRequestExecutor* executor)
{
    ANN(executor);
    dvz_memset(executor, sizeof(DvzSceneRequestExecutor), 0, sizeof(DvzSceneRequestExecutor));
}



/**
 * Destroy a retained scene request executor.
 *
 * @param executor the request executor
 */
void _scene_request_executor_destroy(DvzSceneRequestExecutor* executor)
{
    if (executor == NULL)
        return;
    if (executor->runtime != NULL)
        dvz_drp2_runtime_destroy(executor->runtime);
    if (executor->emitter != NULL)
        dvz_frame_plan_emitter_destroy(executor->emitter);
    dvz_memset(executor, sizeof(DvzSceneRequestExecutor), 0, sizeof(DvzSceneRequestExecutor));
}


/**
 * Return the public visual family corresponding to an internal visual type.
 *
 * @param visual_type internal visual type value
 * @return public scene visual family
 */
static DvzSceneVisualFamily _scene_visual_family(uint32_t visual_type)
{
    switch (visual_type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        return DVZ_SCENE_VISUAL_FAMILY_POINT;
    case DVZ_VISUAL_TYPE_PIXEL:
        return DVZ_SCENE_VISUAL_FAMILY_PIXEL;
    case DVZ_VISUAL_TYPE_MARKER:
        return DVZ_SCENE_VISUAL_FAMILY_MARKER;
    case DVZ_VISUAL_TYPE_SEGMENT:
        return DVZ_SCENE_VISUAL_FAMILY_SEGMENT;
    case DVZ_VISUAL_TYPE_PATH:
        return DVZ_SCENE_VISUAL_FAMILY_PATH;
    case DVZ_VISUAL_TYPE_IMAGE:
        return DVZ_SCENE_VISUAL_FAMILY_IMAGE;
    case DVZ_VISUAL_TYPE_MESH:
        return DVZ_SCENE_VISUAL_FAMILY_MESH;
    case DVZ_VISUAL_TYPE_VOLUME:
        return DVZ_SCENE_VISUAL_FAMILY_VOLUME;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        return DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE;
    case DVZ_VISUAL_TYPE_SPHERE:
        return DVZ_SCENE_VISUAL_FAMILY_SPHERE;
    case DVZ_VISUAL_TYPE_GLYPH:
        return DVZ_SCENE_VISUAL_FAMILY_GLYPH;
    case DVZ_VISUAL_TYPE_TEXT:
        return DVZ_SCENE_VISUAL_FAMILY_TEXT;
    default:
        return DVZ_SCENE_VISUAL_FAMILY_NONE;
    }
}


/**
 * Return whether the first request slice supports a pick target kind.
 *
 * @param target requested scene target
 * @return true when the target can be resolved by the current pick executor
 */
static bool _scene_pick_target_supported(DvzSceneTargetKind target)
{
    return target == DVZ_SCENE_TARGET_NONE || target == DVZ_SCENE_TARGET_ITEM;
}


/**
 * Return whether the first request slice supports a probe target kind.
 *
 * @param target requested scene target
 * @return true when the target can be resolved or explicitly missed by the current probe executor
 */
static bool _scene_probe_target_supported(DvzSceneTargetKind target)
{
    return target == DVZ_SCENE_TARGET_NONE || target == DVZ_SCENE_TARGET_PIXEL ||
           target == DVZ_SCENE_TARGET_SAMPLE || target == DVZ_SCENE_TARGET_SEGMENT;
}


/**
 * Build a pick miss/error result with the common request metadata.
 *
 * @param figure owning figure
 * @param panel requesting panel
 * @param pending pending pick request
 * @param status miss or error status
 * @return initialized pick result
 */
static DvzPickResult _scene_pick_miss_result(
    const DvzFigure* figure, const DvzPanel* panel, const DvzPendingPickRequest* pending,
    DvzPickStatus status)
{
    ANN(figure);
    ANN(panel);
    ANN(pending);
    DvzPickResult result = {
        .request_id = pending->request.request_id,
        .status = status,
        .hit = false,
        .panel_id = _scene_panel_public_id(figure, panel),
        .panel_position = {pending->x, pending->y},
    };
    return result;
}


/**
 * Build a probe miss/error result with the common request metadata.
 *
 * @param figure owning figure
 * @param panel requesting panel
 * @param pending pending probe request
 * @param status miss or error status
 * @return initialized probe result
 */
static DvzProbeResult _scene_probe_miss_result(
    const DvzFigure* figure, const DvzPanel* panel, const DvzPendingProbeRequest* pending,
    DvzProbeStatus status)
{
    ANN(figure);
    ANN(panel);
    ANN(pending);
    DvzProbeResult result = {
        .request_id = pending->request.request_id,
        .status = status,
        .hit = false,
        .panel_id = _scene_panel_public_id(figure, panel),
        .panel_position = {pending->x, pending->y},
        .source_request_id = pending->request.request_id,
    };
    return result;
}


/**
 * Decode the GPU RGBA payload for a point-like visual into scene identity.
 *
 * @param visual visual that produced the payload
 * @param rgba encoded pick pixel
 * @param out_payload decoded scene payload
 * @return true when the pixel contains a non-zero item id
 */
static bool _scene_decode_point_like_pick_payload(
    const DvzVisual* visual, const uint8_t rgba[4], DvzScenePickPayload* out_payload)
{
    ANN(visual);
    ANN(rgba);
    ANN(out_payload);
    dvz_memset(out_payload, sizeof(DvzScenePickPayload), 0, sizeof(DvzScenePickPayload));
    uint32_t encoded =
        (uint32_t)rgba[0] | ((uint32_t)rgba[1] << 8) | ((uint32_t)rgba[2] << 16);
    if (encoded == 0)
        return false;

    uint64_t item_id = (uint64_t)encoded - 1;
    out_payload->visual_family = _scene_visual_family((uint32_t)visual->type);
    out_payload->raw_target = DVZ_SCENE_TARGET_ITEM;
    out_payload->raw_id = item_id;
    out_payload->resolved_target = DVZ_SCENE_TARGET_ITEM;
    out_payload->resolved_id = item_id;
    out_payload->item_id = item_id;
    return true;
}


/**
 * Copy a decoded pick payload into a public pick result.
 *
 * @param scene owning scene
 * @param visual visual that produced the payload
 * @param payload decoded scene payload
 * @param out_result result to populate
 */
static void _scene_apply_pick_payload(
    const DvzScene* scene, const DvzVisual* visual, const DvzScenePickPayload* payload,
    DvzPickResult* out_result)
{
    ANN(scene);
    ANN(visual);
    ANN(payload);
    ANN(out_result);
    out_result->status = DVZ_PICK_STATUS_HIT;
    out_result->hit = true;
    out_result->visual_id = _scene_visual_public_id(scene, visual);
    out_result->visual_family = payload->visual_family;
    out_result->raw_target = payload->raw_target;
    out_result->raw_id = payload->raw_id;
    out_result->resolved_target = payload->resolved_target;
    out_result->resolved_id = payload->resolved_id;
    out_result->item_id = payload->item_id;
    out_result->group_id = payload->group_id;
    out_result->auxiliary_id = payload->auxiliary_id;
    if (visual->link_keys != NULL && payload->item_id < visual->link_key_count)
        out_result->link_key = visual->link_keys[payload->item_id];
}


/**
 * Decode the GPU RGBA payload for an image or hidden label image probe.
 *
 * @param visual visual that produced the payload
 * @param segment_probe whether the request targets segment labels
 * @param rgba sampled GPU pixel
 * @param out_payload decoded scene payload
 * @return true when the pixel contains a supported hit payload
 */
static bool _scene_decode_image_probe_payload(
    const DvzVisual* visual, bool segment_probe, const uint8_t rgba[4],
    DvzSceneProbePayload* out_payload)
{
    ANN(visual);
    ANN(rgba);
    ANN(out_payload);
    dvz_memset(out_payload, sizeof(DvzSceneProbePayload), 0, sizeof(DvzSceneProbePayload));
    out_payload->status = DVZ_PROBE_STATUS_MISS;
    out_payload->visual_family = _scene_visual_family((uint32_t)visual->type);

    if (rgba[3] == 0)
        return false;

    out_payload->status = DVZ_PROBE_STATUS_HIT;
    if (segment_probe)
    {
        uint64_t label_id = (uint64_t)rgba[0] | ((uint64_t)rgba[1] << 8) |
                            ((uint64_t)rgba[2] << 16);
        if (label_id == 0)
        {
            out_payload->status = DVZ_PROBE_STATUS_MISS;
            return false;
        }
        out_payload->target = DVZ_SCENE_TARGET_SEGMENT;
        out_payload->target_id = label_id;
        out_payload->group_id = label_id;
        out_payload->value_kind = DVZ_PROBE_VALUE_LABEL;
        out_payload->category_id = label_id;
        dvz_snprintf(out_payload->label, sizeof(out_payload->label), "label %" PRIu64, label_id);
        return true;
    }

    out_payload->target = DVZ_SCENE_TARGET_PIXEL;
    out_payload->value_kind = DVZ_PROBE_VALUE_VEC4;
    out_payload->vector[0] = rgba[0] / 255.0;
    out_payload->vector[1] = rgba[1] / 255.0;
    out_payload->vector[2] = rgba[2] / 255.0;
    out_payload->vector[3] = rgba[3] / 255.0;
    dvz_strlcpy(out_payload->label, "rgba", sizeof(out_payload->label));
    return true;
}


/**
 * Copy a decoded probe payload into a public probe result.
 *
 * @param scene owning scene
 * @param visual visual that produced the payload
 * @param payload decoded scene payload
 * @param out_result result to populate
 */
static void _scene_apply_probe_payload(
    const DvzScene* scene, const DvzVisual* visual, const DvzSceneProbePayload* payload,
    DvzProbeResult* out_result)
{
    ANN(scene);
    ANN(visual);
    ANN(payload);
    ANN(out_result);
    out_result->status = payload->status;
    out_result->hit = payload->status == DVZ_PROBE_STATUS_HIT;
    out_result->visual_id = _scene_visual_public_id(scene, visual);
    out_result->visual_family = payload->visual_family;
    out_result->target = payload->target;
    out_result->target_id = payload->target_id;
    out_result->item_id = payload->item_id;
    out_result->group_id = payload->group_id;
    out_result->auxiliary_id = payload->auxiliary_id;
    out_result->value_kind = payload->value_kind;
    for (uint32_t i = 0; i < 4; i++)
        out_result->vector[i] = payload->vector[i];
    out_result->category_id = payload->category_id;
    dvz_strlcpy(out_result->label, payload->label, sizeof(out_result->label));
}



/**
 * Emit, execute, and download one 4-byte readback request.
 *
 * @param scene the owning scene, used for instance-scoped test controls
 * @param runtime the DRP2 runtime
 * @param caps the capability snapshot
 * @param plan the prepared frame plan
 * @param rgba the destination 4-byte readback buffer
 * @param out_executed whether the stream executed successfully before download
 * @return true on successful execution and download
 */
static bool _scene_execute_readback_plan(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint8_t rgba[4],
    bool* out_executed)
{
    ANN(executor);
    ANN(caps);
    ANN(rgba);
    ANN(out_executed);
    *out_executed = false;
    if (plan == NULL || executor->runtime == NULL || executor->emitter == NULL)
    {
        log_error("scene readback requires a prepared frame plan and emitter");
        return false;
    }

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = target_width > 0 ? target_width : 1;
    cfg.target_height = target_height > 0 ? target_height : 1;
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(executor->emitter, plan, caps, &report, &cfg);
    if (stream == NULL)
    {
        log_error("scene readback DRP2 emission failed");
        return false;
    }
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(executor->emitter, "_rb");
    bool ok = false;
    if (rb_id == 0)
    {
        log_error("scene readback plan did not emit the _rb buffer");
    }
    else
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(executor->runtime, stream);
        if (!result.ok)
        {
            log_error(
                "scene readback runtime execution failed (code=%d command=%u)",
                (int)result.code, result.command_index);
        }
        else
        {
            *out_executed = true;
            if (scene != NULL && scene->test.force_readback_download_failure)
            {
                log_error("scene readback buffer download forced to fail");
            }
            else
            {
                ok = dvz_drp2_runtime_download_buffer(executor->runtime, rb_id, 0, 4, rgba);
                if (!ok)
                    log_error("scene readback buffer download failed");
            }
        }
    }
    dvz_drp2_stream_destroy(stream);
    return ok;
}


/**
 * Return whether two DRP2 runtime configurations borrow the same backend.
 *
 * @param a first runtime configuration
 * @param b second runtime configuration
 * @return true when the configurations match
 */
static bool _scene_runtime_config_matches(
    const DvzDrp2RuntimeConfig* a, const DvzDrp2RuntimeConfig* b)
{
    ANN(a);
    ANN(b);
    return a->device == b->device && a->allocator == b->allocator &&
           a->semantic_only == b->semantic_only;
}



/**
 * Ensure a retained request executor is ready for the caller runtime's backend.
 *
 * @param executor the retained request executor
 * @param source_runtime the caller's main DRP2 runtime
 * @return true when the executor is ready
 */
static bool _scene_request_executor_prepare(
    DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime)
{
    ANN(executor);
    ANN(source_runtime);

    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_config(source_runtime);
    if (executor->runtime != NULL && executor->emitter != NULL &&
        _scene_runtime_config_matches(&executor->runtime_cfg, &runtime_cfg))
    {
        return true;
    }

    _scene_request_executor_destroy(executor);
    executor->emitter = dvz_frame_plan_emitter();
    if (executor->emitter == NULL)
    {
        log_error("scene request emitter creation failed");
        return false;
    }
    executor->emitter_create_count++;

    executor->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (executor->runtime == NULL)
    {
        log_error("scene request runtime creation failed");
        _scene_request_executor_destroy(executor);
        return false;
    }
    executor->runtime_cfg = runtime_cfg;
    executor->runtime_create_count++;
    return true;
}



/**
 * Return image-probe static resource versions for one visual.
 *
 * @param visual the image visual
 * @param out_position_version position attribute version
 * @param out_texcoord_version texcoord attribute version
 * @param out_texture_version texture payload version
 * @return true when required static resources exist
 */
static bool _scene_image_probe_static_versions(
    const DvzVisual* visual, uint64_t* out_position_version, uint64_t* out_texcoord_version,
    uint64_t* out_texture_version)
{
    ANN(visual);
    ANN(out_position_version);
    ANN(out_texcoord_version);
    ANN(out_texture_version);

    int pos_idx = _attr_index(visual, "position");
    int uv_idx = _attr_index(visual, "texcoords");
    if (pos_idx < 0 || uv_idx < 0)
        return false;
    const DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    const DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
    if (pos_attr->data == NULL || uv_attr->data == NULL || pos_attr->item_count == 0 ||
        uv_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        uv_attr->item_size != sizeof(vec2))
    {
        return false;
    }

    *out_position_version = pos_attr->version;
    *out_texcoord_version = uv_attr->version;
    *out_texture_version = visual->texture.version;
    return true;
}



/**
 * Return whether the retained image-probe static resources must be refreshed.
 *
 * @param executor the retained request executor
 * @param visual the image visual
 * @param position_version position attribute version
 * @param texcoord_version texcoord attribute version
 * @param texture_version texture payload version
 * @return true when static uploads are required
 */
static bool _scene_image_probe_needs_static_upload(
    const DvzSceneRequestExecutor* executor, const DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version)
{
    ANN(executor);
    ANN(visual);
    return executor->image_probe_visual != visual ||
           executor->image_probe_position_version != position_version ||
           executor->image_probe_texcoord_version != texcoord_version ||
           executor->image_probe_texture_version != texture_version;
}



/**
 * Mark the retained image-probe static resources as current.
 *
 * @param executor the retained request executor
 * @param visual the image visual
 * @param position_version position attribute version
 * @param texcoord_version texcoord attribute version
 * @param texture_version texture payload version
 */
static void _scene_image_probe_mark_static_uploaded(
    DvzSceneRequestExecutor* executor, DvzVisual* visual, uint64_t position_version,
    uint64_t texcoord_version, uint64_t texture_version)
{
    ANN(executor);
    ANN(visual);
    executor->image_probe_visual = visual;
    executor->image_probe_position_version = position_version;
    executor->image_probe_texcoord_version = texcoord_version;
    executor->image_probe_texture_version = texture_version;
}


/**
 * Return one axis component from a volume coordinate.
 *
 * @param axis axis index
 * @param value coordinate
 * @return selected component
 */
static double _volume_axis_value(uint32_t axis, const double value[3])
{
    return axis == 0 ? value[0] : (axis == 1 ? value[1] : value[2]);
}


/**
 * Intersect a ray with an axis-aligned box.
 *
 * @param ro ray origin
 * @param rd ray direction
 * @param box_min minimum box coordinate
 * @param box_max maximum box coordinate
 * @param out_t0 near ray parameter
 * @param out_t1 far ray parameter
 * @return whether the ray intersects the box
 */
static bool _volume_ray_box(
    const double ro[3], const double rd[3], const double box_min[3], const double box_max[3],
    double* out_t0, double* out_t1)
{
    ANN(ro);
    ANN(rd);
    ANN(box_min);
    ANN(box_max);
    ANN(out_t0);
    ANN(out_t1);
    double t0 = -INFINITY;
    double t1 = +INFINITY;
    for (uint32_t i = 0; i < 3; i++)
    {
        if (fabs(rd[i]) < 1e-12)
        {
            if (ro[i] < box_min[i] || ro[i] > box_max[i])
                return false;
            continue;
        }
        double a = (box_min[i] - ro[i]) / rd[i];
        double b = (box_max[i] - ro[i]) / rd[i];
        if (a > b)
        {
            double tmp = a;
            a = b;
            b = tmp;
        }
        if (a > t0)
            t0 = a;
        if (b < t1)
            t1 = b;
    }
    *out_t0 = t0;
    *out_t1 = t1;
    return t1 >= fmax(t0, 0.0);
}


/**
 * Return whether a normalized coordinate survives the arbitrary clipping plane.
 *
 * @param state volume state
 * @param uvw normalized volume coordinate
 * @return whether the coordinate is inside the plane half-space
 */
static bool _volume_inside_clip_plane(const DvzVolumeState* state, const double uvw[3])
{
    ANN(state);
    ANN(uvw);
    if (!state->clip_plane_enabled)
        return true;
    double side = 0.0;
    for (uint32_t i = 0; i < 3; i++)
        side += state->clip_plane_normal[i] * (uvw[i] - state->clip_plane_point[i]);
    return state->clip_plane_keep_positive ? side >= -1e-12 : side <= 1e-12;
}


/**
 * Map normalized volume coordinates to texture coordinates.
 *
 * @param state volume state
 * @param uvw normalized volume coordinate
 * @param out_texture_uvw texture coordinate after axis mapping
 */
static void _volume_texture_uvw(
    const DvzVolumeState* state, const double uvw[3], double out_texture_uvw[3])
{
    ANN(state);
    ANN(uvw);
    ANN(out_texture_uvw);
    for (uint32_t i = 0; i < 3; i++)
    {
        double v = uvw[state->axis_order[i]];
        out_texture_uvw[i] = state->axis_flip[i] ? 1.0 - v : v;
        if (out_texture_uvw[i] < 0.0)
            out_texture_uvw[i] = 0.0;
        if (out_texture_uvw[i] > 1.0)
            out_texture_uvw[i] = 1.0;
    }
}


/**
 * Read one scalar sample for CPU-side volume probing.
 *
 * @param field retained sampled field
 * @param sample_index flat sample index
 * @param out_value scalar value
 * @return whether the scalar format is supported
 */
static bool _volume_read_scalar_sample(
    const DvzSampledField* field, uint64_t sample_index, double* out_value)
{
    ANN(field);
    ANN(field->data);
    ANN(out_value);
    switch (field->desc.format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
        *out_value = (double)((const uint8_t*)field->data)[sample_index] / 255.0;
        return true;
    case DVZ_FIELD_FORMAT_R8_UINT:
        *out_value = (double)((const uint8_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R8_SINT:
        *out_value = (double)((const int8_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_UNORM:
        *out_value = (double)((const uint16_t*)field->data)[sample_index] / 65535.0;
        return true;
    case DVZ_FIELD_FORMAT_R16_UINT:
        *out_value = (double)((const uint16_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_SINT:
        *out_value = (double)((const int16_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_UINT:
        *out_value = (double)((const uint32_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_SINT:
        *out_value = (double)((const int32_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        *out_value = (double)((const float*)field->data)[sample_index];
        return true;
    default:
        return false;
    }
}


/**
 * Sample one retained field at nearest texture coordinates.
 *
 * @param field retained sampled field
 * @param texture_uvw normalized texture coordinate
 * @param out_index flat voxel index
 * @param out_probe probe payload to fill
 * @return whether the sample was read
 */
static bool _volume_probe_sample_nearest(
    const DvzSampledField* field, const double texture_uvw[3], uint64_t* out_index,
    DvzProbeResult* out_probe)
{
    ANN(field);
    ANN(texture_uvw);
    ANN(out_index);
    ANN(out_probe);
    if (field->data == NULL || field->desc.width == 0 || field->desc.height == 0 ||
        field->desc.depth == 0)
        return false;

    uint32_t x = (uint32_t)llround(texture_uvw[0] * (double)(field->desc.width - 1));
    uint32_t y = (uint32_t)llround(texture_uvw[1] * (double)(field->desc.height - 1));
    uint32_t z = (uint32_t)llround(texture_uvw[2] * (double)(field->desc.depth - 1));
    if (x >= field->desc.width)
        x = field->desc.width - 1;
    if (y >= field->desc.height)
        y = field->desc.height - 1;
    if (z >= field->desc.depth)
        z = field->desc.depth - 1;
    uint64_t index = ((uint64_t)z * field->desc.height + y) * field->desc.width + x;
    *out_index = index;

    if (_field_format_is_scalar(field->desc.format))
    {
        double value = 0.0;
        if (!_volume_read_scalar_sample(field, index, &value))
            return false;
        out_probe->value_kind = DVZ_PROBE_VALUE_SCALAR;
        out_probe->scalar = value;
        dvz_strlcpy(out_probe->label, "scalar", sizeof(out_probe->label));
        return true;
    }
    if (field->desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        const uint8_t* rgba = (const uint8_t*)field->data + 4 * index;
        out_probe->value_kind = DVZ_PROBE_VALUE_VEC4;
        for (uint32_t i = 0; i < 4; i++)
            out_probe->vector[i] = (double)rgba[i] / 255.0;
        dvz_strlcpy(out_probe->label, "rgba", sizeof(out_probe->label));
        return true;
    }
    return false;
}


/**
 * Return whether one pending probe has a visible slice-volume CPU candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending probe request
 * @return true when a matching slice volume visual exists
 */
static bool _scene_probe_request_has_volume_slice_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(pending);
    if (pending->panel == NULL || pending->panel->figure != figure)
        return false;
    if (pending->request.target != DVZ_SCENE_TARGET_NONE &&
        pending->request.target != DVZ_SCENE_TARGET_SAMPLE)
        return false;

    const DvzPanel* panel = pending->panel;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_VOLUME)
            continue;
        if (visual->volume.render_mode == DVZ_VOLUME_RENDER_SLICE && visual->field != NULL)
            return true;
    }
    return false;
}



/**
 * Return whether one pending probe has a visible image candidate that may need GPU readback.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending probe request
 * @return true when a matching image visual exists
 */
static bool _scene_probe_request_has_image_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(pending);
    if (pending->panel == NULL || pending->panel->figure != figure)
        return false;
    if (
        pending->request.target != DVZ_SCENE_TARGET_NONE &&
        pending->request.target != DVZ_SCENE_TARGET_PIXEL &&
        pending->request.target != DVZ_SCENE_TARGET_SEGMENT)
    {
        return false;
    }

    const DvzPanel* panel = pending->panel;
    bool segment_probe = pending->request.target == DVZ_SCENE_TARGET_SEGMENT;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if (segment_probe)
        {
            if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_GROUP) != 0)
                return true;
        }
        else if (visual->visible)
        {
            return true;
        }
    }
    return false;
}


/**
 * Return whether one pending pick has a visible point-like GPU candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when a matching point, pixel, or marker visual exists
 */
static bool _scene_pick_request_has_point_like_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending)
{
    ANN(figure);
    ANN(pending);
    if (pending->panel == NULL || pending->panel->figure != figure)
        return false;
    if (!_scene_pick_target_supported(pending->request.target))
        return false;

    const DvzPanel* panel = pending->panel;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (
            visual->type != DVZ_VISUAL_TYPE_POINT && visual->type != DVZ_VISUAL_TYPE_PIXEL &&
            visual->type != DVZ_VISUAL_TYPE_MARKER)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (panel->visuals[i].controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        return true;
    }
    return false;
}


/**
 * Build a synthetic GPU readback frame plan for one point-like pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the point or pixel visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_point_like_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height)
{
    ANN(figure);
    ANN(panel);
    ANN(visual);
    ANN(pending);
    ANN(request_ndc);
    ANN(out_plan);
    ANN(out_target_width);
    ANN(out_target_height);

    int pos_idx = _attr_index(visual, "position");
    int color_idx = _attr_index(visual, "color");
    int size_idx = _attr_index(visual, "size");
    if (pos_idx < 0 || color_idx < 0 || size_idx < 0)
        return false;

    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    DvzVisualAttr* color_attr = &visual->attrs[color_idx];
    DvzVisualAttr* size_attr = &visual->attrs[size_idx];
    if (pos_attr->data == NULL || color_attr->data == NULL || size_attr->data == NULL ||
        pos_attr->item_count == 0 || color_attr->item_count != pos_attr->item_count ||
        size_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        color_attr->item_size != sizeof(DvzColor) || size_attr->item_size != sizeof(float))
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
        log_error("point-like pick request buffer size overflow");
        return false;
    }

    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;
    uint32_t target_width = (uint32_t)(panel_width + 0.5);
    uint32_t target_height = (uint32_t)(panel_height + 0.5);
    if (target_width == 0)
        target_width = 1;
    if (target_height == 0)
        target_height = 1;

    DvzColor* pick_colors = (DvzColor*)dvz_calloc(pos_attr->item_count, sizeof(DvzColor));
    if (pick_colors == NULL)
        return false;
    for (uint64_t i = 0; i < pos_attr->item_count; i++)
    {
        uint32_t encoded = (uint32_t)i + 1u;
        pick_colors[i][0] = (uint8_t)(encoded & 0xFFu);
        pick_colors[i][1] = (uint8_t)((encoded >> 8u) & 0xFFu);
        pick_colors[i][2] = (uint8_t)((encoded >> 16u) & 0xFFu);
        pick_colors[i][3] = 255;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position", 0, position_bytes, "position", pos_attr->data) &&
         dvz_frame_plan_upload_bytes(
             plan, "pick0_color", 0, color_bytes, "color", pick_colors) &&
         dvz_frame_plan_upload_bytes(
             plan, "pick0_size", 0, size_bytes, "size", size_attr->data);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type =
        visual->type == DVZ_VISUAL_TYPE_MARKER ? (uint32_t)DVZ_VISUAL_TYPE_PIXEL :
                                                 (uint32_t)visual->type;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    dvz_strlcpy(metadata.position_id, "pick0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "pick0_color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.size_id, "pick0_size", sizeof(metadata.size_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.pick", "target.pick", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "pick0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);

    DvzFramePlanNode* render = plan != NULL ? dvz_frame_plan_last_render_node(plan) : NULL;
    if (render != NULL)
    {
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

    ok = ok && dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
         dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
    if (!ok)
    {
        log_error(
            "point-like pick request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        dvz_frame_plan_destroy(plan);
        dvz_free(pick_colors);
        return false;
    }

    out_plan->plan = plan;
    out_plan->pick_colors = pick_colors;
    *out_target_width = target_width;
    *out_target_height = target_height;
    return true;
}



static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending)
{
    ANN(figure);
    ANN(caps);
    ANN(pending);
    ANN(pending->panel);

    DvzScene* scene = figure->scene;
    DvzPanel* panel = pending->panel;
    DvzPickResult miss =
        _scene_pick_miss_result(figure, panel, pending, DVZ_PICK_STATUS_NO_CAPABLE_VISUAL);

    if (!_scene_pick_target_supported(pending->request.target))
    {
        miss.status = DVZ_PICK_STATUS_UNSUPPORTED_TARGET;
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PICK_STATUS_OUTSIDE_PANEL;
        _scene_pick_trace(
            "picker_request request=%llu x=%.3f y=%.3f panel=%p figure=%ux%u outside_panel=1\n",
            (unsigned long long)pending->request.request_id, pending->x, pending->y,
            (void*)panel, figure->width, figure->height);
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    _scene_pick_trace(
        "picker_request request=%llu x=%.3f y=%.3f ndc=%.6f,%.6f panel=%p "
        "figure=%ux%u panel_desc=%.3f,%.3f,%.3f,%.3f visual_count=%u\n",
        (unsigned long long)pending->request.request_id, pending->x, pending->y, request_ndc[0],
        request_ndc[1], (void*)panel, figure->width, figure->height, panel->desc.x,
        panel->desc.y, panel->desc.width, panel->desc.height, panel->visual_count);

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);

    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (
            visual->type != DVZ_VISUAL_TYPE_POINT && visual->type != DVZ_VISUAL_TYPE_PIXEL &&
            visual->type != DVZ_VISUAL_TYPE_MARKER)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (miss.status == DVZ_PICK_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PICK_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("point-like pick request requires a DRP2 runtime");
            miss.status = DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_point_like_pick_plan(
                figure, panel, visual, pending, request_ndc, &pick_plan, &target_width,
                &target_height))
        {
            _scene_pick_trace(
                "picker_visual_miss request=%llu visual=%p order_index=%d attach_slot=%u\n",
                (unsigned long long)pending->request.request_id, (void*)visual, oi, order[oi]);
            continue;
        }

        uint8_t rgba[4] = {0};
        bool executed = false;
        bool readback_ok = _scene_execute_readback_plan(
            scene, executor, caps, pick_plan.plan, target_width, target_height, rgba, &executed);
        _scene_probe_plan_destroy(&pick_plan);

        DvzScenePickPayload payload = {0};
        if (!readback_ok || !_scene_decode_point_like_pick_payload(visual, rgba, &payload))
        {
            if (!readback_ok)
                miss.status = executed ? DVZ_PICK_STATUS_READBACK_FAILED :
                                         DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            _scene_pick_trace(
                "picker_visual_miss request=%llu visual=%p order_index=%d attach_slot=%u "
                "readback_ok=%d executed=%d rgba=%u,%u,%u,%u\n",
                (unsigned long long)pending->request.request_id, (void*)visual, oi, order[oi],
                readback_ok ? 1 : 0, executed ? 1 : 0, rgba[0], rgba[1], rgba[2], rgba[3]);
            continue;
        }

        DvzPickResult resolved = miss;
        _scene_apply_pick_payload(scene, visual, &payload, &resolved);
        _scene_pick_trace(
            "picker_resolved request=%llu visual=%p visual_id=%llu item=%llu\n",
            (unsigned long long)pending->request.request_id, (void*)visual,
            (unsigned long long)resolved.visual_id, (unsigned long long)payload.item_id);
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &resolved);
    }

    _scene_pick_trace(
        "picker_request_miss request=%llu x=%.3f y=%.3f\n",
        (unsigned long long)pending->request.request_id, pending->x, pending->y);
    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
}


/**
 * Resolve one slice-volume probe directly from retained CPU field data.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending probe request
 * @return whether a result was queued
 */
static bool _scene_process_volume_slice_probe_request(
    DvzFigure* figure, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(pending);
    ANN(pending->panel);

    DvzScene* scene = figure->scene;
    DvzPanel* panel = pending->panel;
    DvzProbeResult miss =
        _scene_probe_miss_result(figure, panel, pending, DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL);

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PROBE_STATUS_OUTSIDE_PANEL;
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_VOLUME ||
            visual->field == NULL || visual->volume.render_mode != DVZ_VOLUME_RENDER_SLICE)
            continue;

        DvzMVP mvp = {0};
        _scene_panel_apply_mvp(panel, &mvp);
        mat4 mv = {0};
        mat4 mvp_mat = {0};
        mat4 inv_mvp = {0};
        glm_mat4_mul(mvp.view, mvp.model, mv);
        glm_mat4_mul(mvp.proj, mv, mvp_mat);
        glm_mat4_inv(mvp_mat, inv_mvp);

        vec4 near_clip = {request_ndc[0], request_ndc[1], -1.0f, 1.0f};
        vec4 far_clip = {request_ndc[0], request_ndc[1], +1.0f, 1.0f};
        vec4 near_obj = {0};
        vec4 far_obj = {0};
        glm_mat4_mulv(inv_mvp, near_clip, near_obj);
        glm_mat4_mulv(inv_mvp, far_clip, far_obj);
        if (fabsf(near_obj[3]) < 1e-12f || fabsf(far_obj[3]) < 1e-12f)
            continue;

        double ro_obj[3] = {
            (double)(near_obj[0] / near_obj[3]),
            (double)(near_obj[1] / near_obj[3]),
            (double)(near_obj[2] / near_obj[3]),
        };
        double far_point[3] = {
            (double)(far_obj[0] / far_obj[3]),
            (double)(far_obj[1] / far_obj[3]),
            (double)(far_obj[2] / far_obj[3]),
        };
        double rd_obj[3] = {
            far_point[0] - ro_obj[0],
            far_point[1] - ro_obj[1],
            far_point[2] - ro_obj[2],
        };
        double rd_len = sqrt(
            rd_obj[0] * rd_obj[0] + rd_obj[1] * rd_obj[1] + rd_obj[2] * rd_obj[2]);
        if (rd_len <= 0.0)
            continue;
        for (uint32_t i = 0; i < 3; i++)
            rd_obj[i] /= rd_len;

        const DvzVolumeState* state = &visual->volume;
        double extent[3] = {
            state->bounds_max[0] - state->bounds_min[0],
            state->bounds_max[1] - state->bounds_min[1],
            state->bounds_max[2] - state->bounds_min[2],
        };
        if (extent[0] <= 0.0 || extent[1] <= 0.0 || extent[2] <= 0.0)
            continue;
        double ro[3] = {
            (ro_obj[0] - state->bounds_min[0]) / extent[0],
            (ro_obj[1] - state->bounds_min[1]) / extent[1],
            (ro_obj[2] - state->bounds_min[2]) / extent[2],
        };
        double rd[3] = {rd_obj[0] / extent[0], rd_obj[1] / extent[1], rd_obj[2] / extent[2]};

        double proxy_min[3] = {0.0, 0.0, 0.0};
        double proxy_max[3] = {1.0, 1.0, 1.0};
        double proxy_t0 = 0.0;
        double proxy_t1 = 0.0;
        if (!_volume_ray_box(ro, rd, proxy_min, proxy_max, &proxy_t0, &proxy_t1))
            continue;

        double box_min[3] = {0.0, 0.0, 0.0};
        double box_max[3] = {1.0, 1.0, 1.0};
        if (state->clipping_enabled)
        {
            for (uint32_t i = 0; i < 3; i++)
            {
                box_min[i] = state->clip_min[i];
                box_max[i] = state->clip_max[i];
            }
        }

        uint32_t axis = (uint32_t)state->slice_axis;
        double slice_coord =
            box_min[axis] + state->slice_position * (box_max[axis] - box_min[axis]);
        double axis_rd = _volume_axis_value(axis, rd);
        if (fabs(axis_rd) < 1e-12)
            continue;
        double slice_t = (slice_coord - _volume_axis_value(axis, ro)) / axis_rd;
        if (slice_t < fmax(proxy_t0, 0.0) || slice_t > proxy_t1)
            continue;

        double uvw[3] = {
            ro[0] + rd[0] * slice_t,
            ro[1] + rd[1] * slice_t,
            ro[2] + rd[2] * slice_t,
        };
        if (
            uvw[0] < box_min[0] || uvw[0] > box_max[0] || uvw[1] < box_min[1] ||
            uvw[1] > box_max[1] || uvw[2] < box_min[2] || uvw[2] > box_max[2] ||
            !_volume_inside_clip_plane(state, uvw))
        {
            continue;
        }

        double texture_uvw[3] = {0};
        _volume_texture_uvw(state, uvw, texture_uvw);
        DvzProbeResult resolved = miss;
        uint64_t voxel_index = 0;
        if (!_volume_probe_sample_nearest(visual->field, texture_uvw, &voxel_index, &resolved))
            continue;
        resolved.hit = true;
        resolved.status = DVZ_PROBE_STATUS_HIT;
        resolved.visual_id = _scene_visual_public_id(scene, visual);
        resolved.visual_family = _scene_visual_family((uint32_t)visual->type);
        resolved.target = DVZ_SCENE_TARGET_SAMPLE;
        resolved.target_id = voxel_index;
        resolved.auxiliary_id = voxel_index;
        resolved.has_coordinate = true;
        resolved.has_uvw = true;
        for (uint32_t i = 0; i < 3; i++)
        {
            resolved.uvw[i] = uvw[i];
            resolved.coordinate[i] = state->bounds_min[i] + uvw[i] * extent[i];
        }
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &resolved);
    }

    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
}



static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(caps);
    ANN(pending);
    ANN(pending->panel);

    DvzScene* scene = figure->scene;
    DvzPanel* panel = pending->panel;
    DvzProbeResult miss =
        _scene_probe_miss_result(figure, panel, pending, DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL);

    if (!_scene_probe_target_supported(pending->request.target))
    {
        miss.status = DVZ_PROBE_STATUS_UNSUPPORTED_TARGET;
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
    }
    if (pending->request.target == DVZ_SCENE_TARGET_SAMPLE)
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PROBE_STATUS_OUTSIDE_PANEL;
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    bool segment_probe = pending->request.target == DVZ_SCENE_TARGET_SEGMENT;

    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if (segment_probe)
        {
            if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_GROUP) == 0)
                continue;
        }
        else if (!visual->visible)
        {
            continue;
        }

        if (miss.status == DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PROBE_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("image probe request requires a DRP2 runtime");
            miss.status = DVZ_PROBE_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        uint64_t position_version = 0;
        uint64_t texcoord_version = 0;
        uint64_t texture_version = 0;
        if (!_scene_image_probe_static_versions(
                visual, &position_version, &texcoord_version, &texture_version))
        {
            continue;
        }

        bool include_static_uploads = _scene_image_probe_needs_static_upload(
            executor, visual, position_version, texcoord_version, texture_version);

        DvzSceneProbePlan probe_plan = {0};
        if (!_scene_image_probe_plan(
                panel, visual, pending, request_ndc, include_static_uploads, &probe_plan))
            continue;

        uint8_t rgba[4] = {0};
        bool executed = false;
        bool readback_ok = _scene_execute_readback_plan(
            scene, executor, caps, probe_plan.plan, 1, 1, rgba, &executed);
        if (!readback_ok)
            miss.status = executed ? DVZ_PROBE_STATUS_READBACK_FAILED :
                                     DVZ_PROBE_STATUS_GPU_EXEC_FAILED;
        if (executed && include_static_uploads)
        {
            _scene_image_probe_mark_static_uploaded(
                executor, visual, position_version, texcoord_version, texture_version);
            executor->image_probe_static_upload_count++;
        }
        bool hit = readback_ok && rgba[3] > 0;
        if (!segment_probe && readback_ok && rgba[3] == 0)
        {
            log_error(
                "image probe request %" PRIu64 " returned a transparent GPU pixel",
                pending->request.request_id);
        }
        _scene_probe_plan_destroy(&probe_plan);

        if (!hit)
            continue;

        DvzSceneProbePayload payload = {0};
        if (!_scene_decode_image_probe_payload(visual, segment_probe, rgba, &payload))
            continue;

        DvzProbeResult resolved = miss;
        _scene_apply_probe_payload(scene, visual, &payload, &resolved);
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &resolved);
    }

    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
}
