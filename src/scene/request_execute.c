/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene request execution                                                                      */
/*************************************************************************************************/

/* TODO(v0.4-query): this file is a migration bridge while the GPU-only panel query system moves
 * generic orchestration to src/scene/query/ and visual-family policy to scene visual folders. */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

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
typedef struct DvzScenePickResolver DvzScenePickResolver;



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
    DvzCategoryId category_id;
    char label[DVZ_SCENE_LABEL_SIZE];
};


struct DvzScenePickResolver
{
    const char* name;
    bool needs_runtime;
    bool (*has_candidate)(const DvzFigure* figure, const DvzPendingPickRequest* pending);
    bool (*process)(
        DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
        const DvzPendingPickRequest* pending);
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

static void _scene_pick_item_payload(
    const DvzVisual* visual, DvzSceneTargetKind target, uint64_t item_id,
    DvzScenePickPayload* out_payload);

static bool _scene_pick_visual_has_attr_data(const DvzVisual* visual, const char* attr_name);

static void _scene_apply_pick_payload(
    const DvzScene* scene, const DvzVisual* visual, const DvzScenePickPayload* payload,
    DvzPickResult* out_result);

static bool _scene_decode_image_probe_payload(
    const DvzVisual* visual, const uint8_t rgba[4], DvzSceneProbePayload* out_payload);

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

static bool _scene_labels_probe_integer_format(
    DvzFieldFormat format, uint32_t* out_texture_format, uint32_t* out_bytes_per_texel);

static bool _scene_labels_probe_visual_uv(
    const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc, double out_uv[2]);

static bool _scene_labels_probe_readback(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzSampledField* field,
    uint32_t texel_x, uint32_t texel_y, uint8_t out_sample[4], bool* out_executed);

static bool _scene_labels_probe_decode_sample(
    DvzFieldFormat format, const uint8_t sample[4], DvzCategoryId* out_id);

static bool _scene_probe_request_has_labels_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_probe_request_has_image_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_probe_request_has_volume_slice_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_pick_request_has_point_like_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_pick_request_has_sphere_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_pick_request_has_stroke_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_pick_request_has_primitive_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_pick_request_has_image_candidate(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_pick_request_needs_runtime(
    const DvzFigure* figure, const DvzPendingPickRequest* pending);

static bool _scene_item_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_stroke_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_primitive_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_image_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_volume_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_visual_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height);

static bool _scene_process_point_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_sphere_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_stroke_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_primitive_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_image_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_pick_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingPickRequest* pending);

static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending);

static bool _scene_process_labels_probe_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending);

static bool _scene_process_volume_slice_probe_request(
    DvzFigure* figure, const DvzPendingProbeRequest* pending);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const DvzScenePickResolver PICK_RESOLVERS[] = {
    {
        .name = "point-like",
        .needs_runtime = true,
        .has_candidate = _scene_pick_request_has_point_like_candidate,
        .process = _scene_process_point_pick_request,
    },
    {
        .name = "sphere",
        .needs_runtime = true,
        .has_candidate = _scene_pick_request_has_sphere_candidate,
        .process = _scene_process_sphere_pick_request,
    },
    {
        .name = "stroke",
        .needs_runtime = true,
        .has_candidate = _scene_pick_request_has_stroke_candidate,
        .process = _scene_process_stroke_pick_request,
    },
    {
        .name = "primitive",
        .needs_runtime = true,
        .has_candidate = _scene_pick_request_has_primitive_candidate,
        .process = _scene_process_primitive_pick_request,
    },
    {
        .name = "image",
        .needs_runtime = true,
        .has_candidate = _scene_pick_request_has_image_candidate,
        .process = _scene_process_image_pick_request,
    },
};

static const uint32_t SCENE_PICK_PATH_VERTEX_SIDE_NEGATIVE = 0x01u;
static const uint32_t SCENE_PICK_PATH_VERTEX_ENDPOINT_END  = 0x02u;
static const uint32_t SCENE_PICK_PATH_VERTEX_HAS_PREV      = 0x04u;
static const uint32_t SCENE_PICK_PATH_VERTEX_HAS_NEXT      = 0x08u;
static const uint32_t SCENE_PICK_PATH_VERTEX_SUBPATH_START = 0x10u;
static const uint32_t SCENE_PICK_PATH_VERTEX_SUBPATH_END   = 0x20u;



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

    if (!_scene_figure_resolve_layouts(figure))
        return 0;

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
        if (_scene_pick_request_needs_runtime(figure, &pending))
            (void)_scene_request_executor_prepare(executor, runtime);
        (void)_scene_process_pick_request(figure, executor, caps, &pending);
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
        if (_scene_probe_request_has_labels_candidate(figure, &pending))
        {
            (void)_scene_request_executor_prepare(executor, runtime);
            (void)_scene_process_labels_probe_request(figure, executor, caps, &pending);
            _scene_remove_pending_probe_at(scene, i);
            processed++;
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
    case DVZ_VISUAL_TYPE_LABELS:
        return DVZ_SCENE_VISUAL_FAMILY_LABELS;
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
    _scene_pick_item_payload(visual, DVZ_SCENE_TARGET_ITEM, item_id, out_payload);
    return true;
}


/**
 * Fill a pick payload with one visual-local item identity.
 *
 * @param visual visual that produced the hit
 * @param target semantic target represented by the item id
 * @param item_id resolved item id
 * @param out_payload decoded scene payload
 */
static void _scene_pick_item_payload(
    const DvzVisual* visual, DvzSceneTargetKind target, uint64_t item_id,
    DvzScenePickPayload* out_payload)
{
    ANN(visual);
    ANN(out_payload);
    dvz_memset(out_payload, sizeof(DvzScenePickPayload), 0, sizeof(DvzScenePickPayload));
    out_payload->visual_family = _scene_visual_family((uint32_t)visual->type);
    out_payload->raw_target = target;
    out_payload->raw_id = item_id;
    out_payload->resolved_target = target;
    out_payload->resolved_id = item_id;
    out_payload->item_id = item_id;
    if (target == DVZ_SCENE_TARGET_SEGMENT)
        out_payload->group_id = item_id;
    if (target == DVZ_SCENE_TARGET_FACE || target == DVZ_SCENE_TARGET_TRIANGLE)
        out_payload->auxiliary_id = item_id;
}


/**
 * Return whether one retained visual attribute has a non-empty dense payload.
 *
 * @param visual the visual
 * @param attr_name the retained attribute name
 * @return true when the attribute has data
 */
static bool _scene_pick_visual_has_attr_data(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int attr_idx = _attr_index(visual, attr_name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
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
 * Decode the GPU RGBA payload for an image probe.
 *
 * @param visual visual that produced the payload
 * @param rgba sampled GPU pixel
 * @param out_payload decoded scene payload
 * @return true when the pixel contains a supported hit payload
 */
static bool _scene_decode_image_probe_payload(
    const DvzVisual* visual, const uint8_t rgba[4], DvzSceneProbePayload* out_payload)
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
            const DvzDrp2Command* failed = dvz_drp2_stream_get(stream, result.command_index);
            uint64_t failed_id = 0;
            if (failed != NULL)
            {
                if (failed->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
                    failed_id = failed->u.create_render_pipeline.id;
                else if (failed->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
                    failed_id = failed->u.create_shader_module.id;
                else if (failed->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
                    failed_id = failed->u.create_bind_group_layout.id;
                else if (failed->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
                    failed_id = failed->u.create_bind_group.id;
                else if (failed->type == DVZ_DRP2_COMMAND_DRAW)
                    failed_id = failed->u.draw.pass_id;
            }
            const char* failed_label =
                failed_id != 0 ? dvz_drp2_stream_label(stream, failed_id) : NULL;
            log_error(
                "scene readback runtime execution failed (code=%d command=%u type=%d id=%llu "
                "label=%s)",
                (int)result.code, result.command_index, failed != NULL ? (int)failed->type : -1,
                (unsigned long long)failed_id, failed_label != NULL ? failed_label : "");
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
    if (pos_idx < 0)
        return false;
    const DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    if (pos_attr->data == NULL || pos_attr->item_count == 0 || pos_attr->item_size != sizeof(vec3))
    {
        return false;
    }

    uint64_t texcoord_version = 0;
    int extent_idx = _attr_index(visual, "extent");
    int extent_px_idx = _attr_index(visual, "extent_px");
    bool has_extent = extent_idx >= 0 && visual->attrs[extent_idx].data != NULL;
    bool has_extent_px = extent_px_idx >= 0 && visual->attrs[extent_px_idx].data != NULL;
    if (has_extent || has_extent_px)
    {
        if (has_extent && has_extent_px)
            return false;
        const DvzVisualAttr* extent_attr =
            has_extent_px ? &visual->attrs[extent_px_idx] : &visual->attrs[extent_idx];
        if (
            extent_attr->item_count != pos_attr->item_count ||
            extent_attr->item_size != sizeof(vec2))
        {
            return false;
        }
        texcoord_version = extent_attr->version;

        int anchor_idx = _attr_index(visual, "anchor");
        if (anchor_idx >= 0 && visual->attrs[anchor_idx].data != NULL)
            texcoord_version ^= visual->attrs[anchor_idx].version;
        int tex_rect_idx = _attr_index(visual, "tex_rect");
        if (tex_rect_idx >= 0 && visual->attrs[tex_rect_idx].data != NULL)
            texcoord_version ^= visual->attrs[tex_rect_idx].version;
    }
    else
    {
        int uv_idx = _attr_index(visual, "texcoords");
        if (uv_idx < 0)
            return false;
        const DvzVisualAttr* uv_attr = &visual->attrs[uv_idx];
        if (
            uv_attr->data == NULL || uv_attr->item_count != pos_attr->item_count ||
            uv_attr->item_size != sizeof(vec2))
        {
            return false;
        }
        texcoord_version = uv_attr->version;
    }

    *out_position_version = pos_attr->version;
    *out_texcoord_version = texcoord_version;
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
 * Return the DRP2 texture format for a labels-probe integer field.
 *
 * @param format sampled-field format
 * @param out_texture_format output texture format, using VkFormat values
 * @param out_bytes_per_texel output texel byte size
 * @return true when the format can carry raw label IDs
 */
static bool _scene_labels_probe_integer_format(
    DvzFieldFormat format, uint32_t* out_texture_format, uint32_t* out_bytes_per_texel)
{
    ANN(out_texture_format);
    ANN(out_bytes_per_texel);
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UINT:
    case DVZ_FIELD_FORMAT_R8_SINT:
    case DVZ_FIELD_FORMAT_R16_UINT:
    case DVZ_FIELD_FORMAT_R16_SINT:
    case DVZ_FIELD_FORMAT_R32_UINT:
    case DVZ_FIELD_FORMAT_R32_SINT:
        return _field_format_texture_format(format, out_texture_format) &&
               _field_format_bytes_per_texel(format, out_bytes_per_texel);
    default:
        *out_texture_format = 0;
        *out_bytes_per_texel = 0;
        return false;
    }
}



/**
 * Project one visual-space vertex into panel-local NDC.
 *
 * @param mvp panel MVP transform
 * @param position visual-space position
 * @param out_ndc projected NDC coordinate
 * @return true when the projected position is finite
 */
static bool
_scene_labels_probe_project_vertex(DvzMVP* mvp, const float position[3], double out_ndc[2])
{
    ANN(mvp);
    ANN(position);
    ANN(out_ndc);

    vec4 p = {position[0], position[1], position[2], 1.0f};
    vec4 tmp0 = {0};
    vec4 tmp1 = {0};
    vec4 clip = {0};
    glm_mat4_mulv(mvp->model, p, tmp0);
    glm_mat4_mulv(mvp->view, tmp0, tmp1);
    glm_mat4_mulv(mvp->proj, tmp1, clip);
    if (fabsf(clip[3]) < 1e-12f)
        return false;
    out_ndc[0] = (double)(clip[0] / clip[3]);
    out_ndc[1] = (double)(clip[1] / clip[3]);
    return isfinite(out_ndc[0]) && isfinite(out_ndc[1]);
}



/**
 * Interpolate texture coordinates for a point inside one projected triangle.
 *
 * @param request_ndc requested panel-local NDC point
 * @param p0 first projected triangle vertex
 * @param p1 second projected triangle vertex
 * @param p2 third projected triangle vertex
 * @param uv0 first texture coordinate
 * @param uv1 second texture coordinate
 * @param uv2 third texture coordinate
 * @param out_uv interpolated texture coordinate
 * @return true when the request falls inside the triangle
 */
static bool _scene_labels_probe_triangle_uv(
    const vec2 request_ndc, const double p0[2], const double p1[2], const double p2[2],
    const float uv0[2], const float uv1[2], const float uv2[2], double out_uv[2])
{
    ANN(request_ndc);
    ANN(p0);
    ANN(p1);
    ANN(p2);
    ANN(uv0);
    ANN(uv1);
    ANN(uv2);
    ANN(out_uv);

    const double x = (double)request_ndc[0];
    const double y = (double)request_ndc[1];
    const double denom =
        (p1[1] - p2[1]) * (p0[0] - p2[0]) + (p2[0] - p1[0]) * (p0[1] - p2[1]);
    if (fabs(denom) < 1e-18)
        return false;

    const double w0 =
        ((p1[1] - p2[1]) * (x - p2[0]) + (p2[0] - p1[0]) * (y - p2[1])) / denom;
    const double w1 =
        ((p2[1] - p0[1]) * (x - p2[0]) + (p0[0] - p2[0]) * (y - p2[1])) / denom;
    const double w2 = 1.0 - w0 - w1;
    const double eps = 1e-7;
    if (w0 < -eps || w1 < -eps || w2 < -eps)
        return false;

    out_uv[0] = w0 * (double)uv0[0] + w1 * (double)uv1[0] + w2 * (double)uv2[0];
    out_uv[1] = w0 * (double)uv0[1] + w1 * (double)uv1[1] + w2 * (double)uv2[1];
    return isfinite(out_uv[0]) && isfinite(out_uv[1]);
}



/**
 * Test one labels triangle and return the interpolated texture coordinate.
 *
 * @param mvp panel MVP transform
 * @param request_ndc requested panel-local NDC point
 * @param positions three visual-space positions
 * @param texcoords three texture coordinates
 * @param out_uv interpolated texture coordinate
 * @return true when the triangle contains the request
 */
static bool _scene_labels_probe_projected_triangle_uv(
    DvzMVP* mvp, const vec3 positions[3], const vec2 texcoords[3], const vec2 request_ndc,
    double out_uv[2])
{
    ANN(mvp);
    ANN(positions);
    ANN(texcoords);
    ANN(request_ndc);
    ANN(out_uv);

    double p0[2] = {0};
    double p1[2] = {0};
    double p2[2] = {0};
    if (
        !_scene_labels_probe_project_vertex(mvp, positions[0], p0) ||
        !_scene_labels_probe_project_vertex(mvp, positions[1], p1) ||
        !_scene_labels_probe_project_vertex(mvp, positions[2], p2))
    {
        return false;
    }
    return _scene_labels_probe_triangle_uv(
        request_ndc, p0, p1, p2, texcoords[0], texcoords[1], texcoords[2], out_uv);
}



/**
 * Return whether a visual attribute has dense data of one item size.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param item_size expected item size
 * @param out_attr optional output attribute
 * @return true when matching data exists
 */
static bool _scene_labels_probe_attr(
    const DvzVisual* visual, const char* attr_name, uint64_t item_size,
    const DvzVisualAttr** out_attr)
{
    ANN(visual);
    ANN(attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return false;
    if (out_attr != NULL)
        *out_attr = attr;
    return true;
}



/**
 * Resolve the labels texture coordinate for one retained labels visual.
 *
 * @param panel requesting panel
 * @param visual labels visual
 * @param request_ndc requested panel-local NDC coordinate
 * @param out_uv output texture coordinate
 * @return true when the request falls on the labels visual
 */
static bool _scene_labels_probe_visual_uv(
    const DvzPanel* panel, const DvzVisual* visual, const vec2 request_ndc, double out_uv[2])
{
    ANN(panel);
    ANN(visual);
    ANN(request_ndc);
    ANN(out_uv);

    const DvzVisualAttr* pos_attr = NULL;
    if (!_scene_labels_probe_attr(visual, "position", sizeof(vec3), &pos_attr))
        return false;
    const float* position = (const float*)pos_attr->data;

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);

    const DvzVisualAttr* extent_attr = NULL;
    if (_scene_labels_probe_attr(visual, "extent", sizeof(vec2), &extent_attr))
    {
        if (extent_attr->item_count != pos_attr->item_count)
            return false;
        const float* extent = (const float*)extent_attr->data;
        const DvzVisualAttr* anchor_attr = NULL;
        const bool has_anchor =
            _scene_labels_probe_attr(visual, "anchor", sizeof(vec2), &anchor_attr) &&
            anchor_attr->item_count == pos_attr->item_count;
        const float* anchor = has_anchor ? (const float*)anchor_attr->data : NULL;
        const DvzVisualAttr* tex_rect_attr = NULL;
        const bool has_tex_rect =
            _scene_labels_probe_attr(visual, "tex_rect", 4 * sizeof(float), &tex_rect_attr) &&
            tex_rect_attr->item_count == pos_attr->item_count;
        const float* tex_rect = has_tex_rect ? (const float*)tex_rect_attr->data : NULL;

        for (uint64_t k = pos_attr->item_count; k > 0; k--)
        {
            uint64_t i = k - 1;
            const float x = position[3 * i + 0];
            const float y = position[3 * i + 1];
            const float z = position[3 * i + 2];
            const float w = extent[2 * i + 0];
            const float h = extent[2 * i + 1];
            const float ax = anchor != NULL ? anchor[2 * i + 0] : 0.0f;
            const float ay = anchor != NULL ? anchor[2 * i + 1] : 0.0f;
            const float x0 = x - 0.5f * (ax + 1.0f) * w;
            const float x1 = x0 + w;
            const float y0 = y - 0.5f * (ay + 1.0f) * h;
            const float y1 = y0 + h;
            const float u0 = tex_rect != NULL ? tex_rect[4 * i + 0] : 0.0f;
            const float v0 = tex_rect != NULL ? tex_rect[4 * i + 1] : 0.0f;
            const float u1 = tex_rect != NULL ? tex_rect[4 * i + 2] : 1.0f;
            const float v1 = tex_rect != NULL ? tex_rect[4 * i + 3] : 1.0f;
            const vec3 quad_pos[6] = {
                {x0, y0, z}, {x0, y1, z}, {x1, y0, z},
                {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
            };
            const vec2 quad_uv[6] = {
                {u0, v0}, {u0, v1}, {u1, v0}, {u1, v0}, {u0, v1}, {u1, v1},
            };
            for (uint32_t j = 0; j < 6; j += 3)
            {
                const vec3 tri_pos[3] = {
                    {quad_pos[j + 0][0], quad_pos[j + 0][1], quad_pos[j + 0][2]},
                    {quad_pos[j + 1][0], quad_pos[j + 1][1], quad_pos[j + 1][2]},
                    {quad_pos[j + 2][0], quad_pos[j + 2][1], quad_pos[j + 2][2]},
                };
                const vec2 tri_uv[3] = {
                    {quad_uv[j + 0][0], quad_uv[j + 0][1]},
                    {quad_uv[j + 1][0], quad_uv[j + 1][1]},
                    {quad_uv[j + 2][0], quad_uv[j + 2][1]},
                };
                if (_scene_labels_probe_projected_triangle_uv(
                        &mvp, tri_pos, tri_uv, request_ndc, out_uv))
                    return true;
            }
        }
        return false;
    }

    const DvzVisualAttr* uv_attr = NULL;
    if (!_scene_labels_probe_attr(visual, "texcoords", sizeof(vec2), &uv_attr))
        return false;
    if (uv_attr->item_count != pos_attr->item_count)
        return false;
    const float* texcoords = (const float*)uv_attr->data;
    if (pos_attr->item_count == 4)
    {
        const uint32_t order[6] = {0, 1, 2, 2, 1, 3};
        for (uint32_t j = 0; j < 6; j += 3)
        {
            const uint32_t i0 = order[j + 0];
            const uint32_t i1 = order[j + 1];
            const uint32_t i2 = order[j + 2];
            const vec3 tri_pos[3] = {
                {position[3 * i0 + 0], position[3 * i0 + 1], position[3 * i0 + 2]},
                {position[3 * i1 + 0], position[3 * i1 + 1], position[3 * i1 + 2]},
                {position[3 * i2 + 0], position[3 * i2 + 1], position[3 * i2 + 2]},
            };
            const vec2 tri_uv[3] = {
                {texcoords[2 * i0 + 0], texcoords[2 * i0 + 1]},
                {texcoords[2 * i1 + 0], texcoords[2 * i1 + 1]},
                {texcoords[2 * i2 + 0], texcoords[2 * i2 + 1]},
            };
            if (_scene_labels_probe_projected_triangle_uv(
                    &mvp, tri_pos, tri_uv, request_ndc, out_uv))
                return true;
        }
        return false;
    }
    if (pos_attr->item_count % 3 != 0)
        return false;
    for (uint64_t tri = pos_attr->item_count / 3; tri > 0; tri--)
    {
        uint64_t base = 3 * (tri - 1);
        const vec3 tri_pos[3] = {
            {position[3 * (base + 0) + 0], position[3 * (base + 0) + 1],
             position[3 * (base + 0) + 2]},
            {position[3 * (base + 1) + 0], position[3 * (base + 1) + 1],
             position[3 * (base + 1) + 2]},
            {position[3 * (base + 2) + 0], position[3 * (base + 2) + 1],
             position[3 * (base + 2) + 2]},
        };
        const vec2 tri_uv[3] = {
            {texcoords[2 * (base + 0) + 0], texcoords[2 * (base + 0) + 1]},
            {texcoords[2 * (base + 1) + 0], texcoords[2 * (base + 1) + 1]},
            {texcoords[2 * (base + 2) + 0], texcoords[2 * (base + 2) + 1]},
        };
        if (_scene_labels_probe_projected_triangle_uv(&mvp, tri_pos, tri_uv, request_ndc, out_uv))
            return true;
    }
    return false;
}



/**
 * Execute a direct integer labels texture readback.
 *
 * @param scene owning scene, used for instance-scoped test controls
 * @param executor retained request executor
 * @param field labels sampled field
 * @param texel_x x texel to decode after readback
 * @param texel_y y texel to decode after readback
 * @param out_sample raw texel bytes
 * @param out_executed whether the stream executed successfully before download
 * @return true when the selected sample was downloaded
 */
static bool _scene_labels_probe_readback(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzSampledField* field,
    uint32_t texel_x, uint32_t texel_y, uint8_t out_sample[4], bool* out_executed)
{
    ANN(executor);
    ANN(field);
    ANN(out_sample);
    ANN(out_executed);
    *out_executed = false;

    uint32_t texture_format = 0;
    uint32_t bytes_per_texel = 0;
    if (
        executor->runtime == NULL || !_scene_labels_probe_integer_format(
                                         field->desc.format, &texture_format, &bytes_per_texel))
    {
        return false;
    }

    uint64_t row_bytes = 0;
    uint64_t buffer_size = 0;
    if (
        _dvz_mul_u64_overflows(field->desc.width, bytes_per_texel, &row_bytes) ||
        _dvz_mul_u64_overflows(row_bytes, field->desc.height, &buffer_size) ||
        row_bytes > UINT32_MAX || buffer_size == 0)
    {
        log_error("labels probe readback buffer size overflow");
        return false;
    }
    uint64_t sample_offset = 0;
    uint64_t row_offset = 0;
    uint64_t texel_offset = 0;
    if (
        _dvz_mul_u64_overflows(texel_y, row_bytes, &row_offset) ||
        _dvz_mul_u64_overflows(texel_x, bytes_per_texel, &texel_offset) ||
        _dvz_add_u64_overflows(row_offset, texel_offset, &sample_offset))
    {
        log_error("labels probe readback sample offset overflow");
        return false;
    }

    dvz_drp2_runtime_reset(executor->runtime);
    executor->image_probe_visual = NULL;
    executor->image_probe_position_version = 0;
    executor->image_probe_texcoord_version = 0;
    executor->image_probe_texture_version = 0;

    const uint64_t texture_id = 7001;
    const uint64_t buffer_id = 7002;
    const uint64_t encoder_id = 7003;
    const uint64_t command_buffer_id = 7004;
    const uint64_t submission_id = 7005;
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    if (stream == NULL)
        return false;

    bool ok = dvz_drp2_stream_hello_renderer(stream, "scene-labels-probe") &&
              dvz_drp2_stream_renderer_hello_reply(stream, "datoviz") &&
              dvz_drp2_stream_create_texture_2d_format_usage(
                  stream, texture_id, field->desc.width, field->desc.height, texture_format,
                  DVZ_DRP2_TEXTURE_USAGE_COPY_DST | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) &&
              dvz_drp2_stream_write_texture_2d_bytes(
                  stream, texture_id, 0, field->desc.width, field->desc.height,
                  (uint32_t)row_bytes, field->desc.height, field->data) &&
              dvz_drp2_stream_create_buffer(
                  stream, buffer_id, buffer_size,
                  DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ) &&
              dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
              dvz_drp2_stream_copy_texture_to_buffer(
                  stream, encoder_id, texture_id, buffer_id, 0, field->desc.width,
                  field->desc.height, (uint32_t)row_bytes, field->desc.height) &&
              dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id) &&
              dvz_drp2_stream_queue_submit(stream, command_buffer_id, submission_id);
    if (!ok)
    {
        log_error("labels probe readback stream assembly failed");
        dvz_drp2_stream_destroy(stream);
        return false;
    }

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(executor->runtime, stream);
    if (!result.ok)
    {
        log_error(
            "labels probe readback runtime execution failed (code=%d command=%u)",
            (int)result.code, result.command_index);
        dvz_drp2_stream_destroy(stream);
        return false;
    }
    *out_executed = true;

    ok = false;
    if (scene != NULL && scene->test.force_readback_download_failure)
    {
        log_error("labels probe readback buffer download forced to fail");
    }
    else
    {
        ok = dvz_drp2_runtime_download_buffer(
            executor->runtime, buffer_id, sample_offset, bytes_per_texel, out_sample);
        if (!ok)
            log_error("labels probe readback buffer download failed");
    }
    dvz_drp2_stream_destroy(stream);
    return ok;
}



/**
 * Decode raw labels-probe texel bytes into a category ID.
 *
 * @param format sampled-field format
 * @param sample raw texel bytes
 * @param out_id decoded category ID
 * @return true when the format was decoded
 */
static bool _scene_labels_probe_decode_sample(
    DvzFieldFormat format, const uint8_t sample[4], DvzCategoryId* out_id)
{
    ANN(sample);
    ANN(out_id);
    switch (format)
    {
    case DVZ_FIELD_FORMAT_R8_UINT:
        *out_id = (DvzCategoryId)sample[0];
        return true;
    case DVZ_FIELD_FORMAT_R8_SINT:
    {
        int8_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R16_UINT:
    {
        uint16_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R16_SINT:
    {
        int16_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R32_UINT:
    {
        uint32_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    case DVZ_FIELD_FORMAT_R32_SINT:
    {
        int32_t v = 0;
        dvz_memcpy(&v, sizeof(v), sample, sizeof(v));
        *out_id = (DvzCategoryId)v;
        return true;
    }
    default:
        return false;
    }
}



/**
 * Return the display label for one labels-probe category.
 *
 * @param visual labels visual
 * @param id category ID
 * @param out_label output display label
 * @param label_size output label capacity
 */
static void _scene_labels_probe_category_label(
    const DvzVisual* visual, DvzCategoryId id, char* out_label, uint64_t label_size)
{
    ANN(visual);
    ANN(out_label);
    if (visual->scale != NULL)
    {
        for (uint32_t i = 0; i < visual->scale->category_count; i++)
        {
            const DvzScaleCategoryState* category = &visual->scale->categories[i];
            if (category->category_id == id && category->has_label)
            {
                dvz_strlcpy(out_label, category->label, label_size);
                return;
            }
        }
    }
    dvz_snprintf(out_label, label_size, "label %" PRIi64, id);
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
 * Return whether one pending probe has a labels visual candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending probe request
 * @return true when a matching labels visual exists
 */
static bool _scene_probe_request_has_labels_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(pending);
    if (pending->panel == NULL || pending->panel->figure != figure)
        return false;
    if (
        pending->request.target != DVZ_SCENE_TARGET_NONE &&
        pending->request.target != DVZ_SCENE_TARGET_SEGMENT)
    {
        return false;
    }

    const DvzPanel* panel = pending->panel;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        const DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_LABELS)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        if (visual->field == NULL || visual->field->data == NULL)
            continue;
        uint32_t texture_format = 0;
        uint32_t bytes_per_texel = 0;
        if (!_scene_labels_probe_integer_format(
                visual->field->desc.format, &texture_format, &bytes_per_texel))
            continue;
        if (
            visual->field->desc.dim != DVZ_FIELD_DIM_2D || visual->field->desc.width == 0 ||
            visual->field->desc.height == 0)
        {
            continue;
        }
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
        pending->request.target != DVZ_SCENE_TARGET_PIXEL)
    {
        return false;
    }

    const DvzPanel* panel = pending->panel;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        const DvzVisual* visual = attach->visual;
        if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if (visual->visible && attach->controller_mode != DVZ_CONTROLLER_FIXED)
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
 * Return whether one pending pick has a visible sphere candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when a matching sphere visual exists
 */
static bool _scene_pick_request_has_sphere_candidate(
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
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_SPHERE)
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
 * Return whether one pending pick has a visible stroke candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when a matching segment or stroked path visual exists
 */
static bool _scene_pick_request_has_stroke_candidate(
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
        if (visual->type != DVZ_VISUAL_TYPE_SEGMENT && visual->type != DVZ_VISUAL_TYPE_PATH)
            continue;
        if (
            visual->type == DVZ_VISUAL_TYPE_PATH &&
            !_scene_pick_visual_has_attr_data(visual, "line_width"))
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
 * Return whether one pending pick has a visible primitive-pipeline candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when a matching primitive, mesh, or non-stroked path visual exists
 */
static bool _scene_pick_request_has_primitive_candidate(
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
        bool primitive_like = visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                              visual->type == DVZ_VISUAL_TYPE_MESH ||
                              visual->type == DVZ_VISUAL_TYPE_PATH ||
                              visual->type == DVZ_VISUAL_TYPE_VOLUME;
        if (!primitive_like)
            continue;
        if (
            visual->type == DVZ_VISUAL_TYPE_PATH &&
            _scene_pick_visual_has_attr_data(visual, "line_width"))
        {
            continue;
        }
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (panel->visuals[i].controller_mode == DVZ_CONTROLLER_FIXED)
            continue;
        return true;
    }
    return false;
}


/**
 * Return whether one pending pick has a visible image candidate.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when a matching image visual exists
 */
static bool _scene_pick_request_has_image_candidate(
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
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_IMAGE)
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
 * Return whether one pending pick needs an auxiliary request runtime.
 *
 * @param figure figure whose request queue is being processed
 * @param pending pending pick request
 * @return true when at least one matching resolver requires the DRP2 runtime
 */
static bool _scene_pick_request_needs_runtime(
    const DvzFigure* figure, const DvzPendingPickRequest* pending)
{
    ANN(figure);
    ANN(pending);
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(PICK_RESOLVERS); i++)
    {
        const DvzScenePickResolver* resolver = &PICK_RESOLVERS[i];
        if (
            resolver->needs_runtime && resolver->has_candidate != NULL &&
            resolver->has_candidate(figure, pending))
        {
            return true;
        }
    }
    return false;
}


/**
 * Allocate one temporary pick-plan buffer with checked size arithmetic.
 *
 * @param out_ptr output pointer
 * @param count item count
 * @param item_size item byte size
 * @param label buffer label used in diagnostics
 * @return true when allocation succeeds
 */
static bool _scene_pick_alloc(
    void** out_ptr, uint64_t count, uint64_t item_size, const char* label)
{
    ANN(out_ptr);
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(count, item_size, &bytes) || bytes > SIZE_MAX)
    {
        log_error("%s pick request buffer size overflow", label != NULL ? label : "visual");
        return false;
    }
    void* ptr = dvz_calloc((size_t)count, (size_t)item_size);
    if (ptr == NULL && bytes > 0)
    {
        log_error("%s pick request buffer allocation failed", label != NULL ? label : "visual");
        return false;
    }
    *out_ptr = ptr;
    return true;
}


/**
 * Store one item id as an RGB pick payload.
 *
 * @param item_id zero-based visual-local item id
 * @return encoded color
 */
static DvzColor _scene_pick_encode_item(uint64_t item_id)
{
    uint32_t encoded = (uint32_t)item_id + 1u;
    return dvz_color_rgba(
        (uint8_t)(encoded & 0xFFu),         //
        (uint8_t)((encoded >> 8u) & 0xFFu), //
        (uint8_t)((encoded >> 16u) & 0xFFu),
        255);
}


/**
 * Return the offscreen target extent for one panel-local request.
 *
 * @param figure parent figure
 * @param panel panel receiving the request
 * @param out_target_width output target width
 * @param out_target_height output target height
 * @return true when the extent is valid
 */
static bool _scene_pick_target_extent(
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
 * Apply the request-centered MVP and viewport to the last render node in a pick plan.
 *
 * @param plan the frame plan
 * @param panel panel receiving the request
 * @param request_ndc request coordinate in panel-local NDC
 * @param target_width offscreen target width
 * @param target_height offscreen target height
 */
static void _scene_pick_plan_apply_render_state(
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
 * Mark the most recent upload node as an index buffer.
 *
 * @param plan the frame plan
 * @param stride index item stride in bytes
 */
static void _scene_pick_mark_last_upload_index(DvzFramePlan* plan, uint32_t stride)
{
    ANN(plan);
    DvzFramePlanNode* node = plan->count > 0 ? &plan->nodes[plan->count - 1] : NULL;
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
    node->u.upload.item_stride = stride;
}


/**
 * Mark the most recent upload node as a uniform buffer.
 *
 * @param plan the frame plan
 */
static void _scene_pick_mark_last_upload_uniform(DvzFramePlan* plan)
{
    ANN(plan);
    DvzFramePlanNode* node = plan->count > 0 ? &plan->nodes[plan->count - 1] : NULL;
    if (node == NULL || node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
        return;
    node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_UNIFORM;
    node->u.upload.item_stride = sizeof(DvzSceneMaterialParams);
}


/**
 * Return packed path vertex flags for one temporary pick vertex.
 *
 * @param side_negative whether the vertex is on the negative normal side
 * @param endpoint_end whether the vertex belongs to the segment end endpoint
 * @param has_prev whether the endpoint has a previous path point
 * @param has_next whether the endpoint has a next path point
 * @param subpath_start whether the endpoint is the first point in an open subpath
 * @param subpath_end whether the endpoint is the last point in an open subpath
 * @return packed path vertex flags
 */
static uint32_t _scene_pick_path_vertex_flags(
    bool side_negative, bool endpoint_end, bool has_prev, bool has_next, bool subpath_start,
    bool subpath_end)
{
    uint32_t flags = 0;
    flags |= side_negative ? SCENE_PICK_PATH_VERTEX_SIDE_NEGATIVE : 0u;
    flags |= endpoint_end ? SCENE_PICK_PATH_VERTEX_ENDPOINT_END : 0u;
    flags |= has_prev ? SCENE_PICK_PATH_VERTEX_HAS_PREV : 0u;
    flags |= has_next ? SCENE_PICK_PATH_VERTEX_HAS_NEXT : 0u;
    flags |= subpath_start ? SCENE_PICK_PATH_VERTEX_SUBPATH_START : 0u;
    flags |= subpath_end ? SCENE_PICK_PATH_VERTEX_SUBPATH_END : 0u;
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
static float _scene_pick_path_point_distance(const float* position, uint64_t i0, uint64_t i1)
{
    ANN(position);
    float dx = position[3 * i1 + 0] - position[3 * i0 + 0];
    float dy = position[3 * i1 + 1] - position[3 * i0 + 1];
    float dz = position[3 * i1 + 2] - position[3 * i0 + 2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}


/**
 * Return whether one pick subpath repeats its first point as a closed-ring sentinel.
 *
 * @param position flat vec3 position array
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @return whether the first and last points are equal
 */
static bool _scene_pick_path_subpath_is_closed(
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
 * Return the previous adjacency point for one temporary pick endpoint.
 *
 * @param point_idx endpoint point index
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @param closed whether the subpath repeats its first point at the end
 * @return previous adjacency point index
 */
static uint64_t _scene_pick_path_prev_index(
    uint64_t point_idx, uint64_t offset, uint32_t length, bool closed)
{
    if (closed && point_idx == offset)
        return offset + length - 2;
    if (point_idx > offset)
        return point_idx - 1;
    return point_idx;
}


/**
 * Return the next adjacency point for one temporary pick endpoint.
 *
 * @param point_idx endpoint point index
 * @param offset first point index of the subpath
 * @param length subpath point count
 * @param closed whether the subpath repeats its first point at the end
 * @return next adjacency point index
 */
static uint64_t _scene_pick_path_next_index(
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
 * Return one source vertex index from either direct or indexed visual geometry.
 *
 * @param visual visual carrying an optional index buffer binding
 * @param draw_index draw-order vertex index
 * @param vertex_count source vertex count
 * @param out_source_index output source vertex index
 * @return true when the source index is valid
 */
static bool _scene_pick_source_vertex_index(
    const DvzVisual* visual, uint64_t draw_index, uint64_t vertex_count,
    uint64_t* out_source_index)
{
    ANN(visual);
    ANN(out_source_index);
    uint64_t source_index = draw_index;
    if (visual->buffer != NULL && visual->buffer->data != NULL)
    {
        uint32_t stride = visual->buffer->desc.stride;
        uint64_t offset = 0;
        if (
            stride == 0 ||
            _dvz_mul_u64_overflows(draw_index, stride, &offset) ||
            offset + stride > visual->buffer->desc.byte_size)
        {
            return false;
        }
        const uint8_t* data = (const uint8_t*)visual->buffer->data + offset;
        if (stride == sizeof(uint16_t))
        {
            uint16_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else if (stride == sizeof(uint32_t))
        {
            uint32_t index = 0;
            dvz_memcpy(&index, sizeof(index), data, sizeof(index));
            source_index = (uint64_t)index;
        }
        else
            return false;
    }
    if (source_index >= vertex_count)
        return false;
    *out_source_index = source_index;
    return true;
}


/**
 * Build temporary GPU stroke buffers for one segment visual.
 *
 * @param visual segment visual
 * @param out_plan output pick-plan wrapper
 * @param out_vertex_count output derived vertex count
 * @param out_index_count output derived index count
 * @return true when derived buffers were created
 */
static bool _scene_segment_pick_geometry(
    const DvzVisual* visual, DvzSceneProbePlan* out_plan, uint64_t* out_vertex_count,
    uint64_t* out_index_count)
{
    ANN(visual);
    ANN(out_plan);
    ANN(out_vertex_count);
    ANN(out_index_count);

    int start_idx = _attr_index(visual, "position_start");
    int end_idx = _attr_index(visual, "position_end");
    int color_idx = _attr_index(visual, "color");
    int width_idx = _attr_index(visual, "line_width");
    if (start_idx < 0 || end_idx < 0 || color_idx < 0 || width_idx < 0)
        return false;

    const DvzVisualAttr* start_attr = &visual->attrs[start_idx];
    const DvzVisualAttr* end_attr = &visual->attrs[end_idx];
    const DvzVisualAttr* color_attr = &visual->attrs[color_idx];
    const DvzVisualAttr* width_attr = &visual->attrs[width_idx];
    uint64_t item_count = start_attr->item_count;
    if (
        start_attr->data == NULL || end_attr->data == NULL || color_attr->data == NULL ||
        width_attr->data == NULL || item_count == 0 || end_attr->item_count != item_count ||
        color_attr->item_count != item_count || width_attr->item_count != item_count ||
        start_attr->item_size != sizeof(vec3) || end_attr->item_size != sizeof(vec3) ||
        color_attr->item_size != sizeof(DvzColor) || width_attr->item_size != sizeof(float))
    {
        return false;
    }

    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(item_count, 4, &vertex_count) ||
        _dvz_mul_u64_overflows(item_count, 6, &index_count) || vertex_count > UINT32_MAX)
    {
        log_error("segment pick request buffer size overflow");
        return false;
    }

    if (!_scene_pick_alloc(
            (void**)&out_plan->pick_position_start, vertex_count, 3 * sizeof(float),
            "segment") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_position_end, vertex_count, 3 * sizeof(float), "segment") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_colors, vertex_count, sizeof(DvzColor), "segment") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_line_width, vertex_count, sizeof(float), "segment") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_indices, index_count, sizeof(uint32_t), "segment"))
    {
        return false;
    }

    const float* position_start = (const float*)start_attr->data;
    const float* position_end = (const float*)end_attr->data;
    const float* line_width = (const float*)width_attr->data;
    for (uint64_t i = 0; i < item_count; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            uint64_t dst = 4 * i + j;
            dvz_memcpy(
                &out_plan->pick_position_start[3 * dst], 3 * sizeof(float),
                &position_start[3 * i], 3 * sizeof(float));
            dvz_memcpy(
                &out_plan->pick_position_end[3 * dst], 3 * sizeof(float),
                &position_end[3 * i], 3 * sizeof(float));
            out_plan->pick_colors[dst] = _scene_pick_encode_item(i);
            out_plan->pick_line_width[dst] = line_width[i];
        }
        out_plan->pick_indices[6 * i + 0] = (uint32_t)(4 * i + 0);
        out_plan->pick_indices[6 * i + 1] = (uint32_t)(4 * i + 1);
        out_plan->pick_indices[6 * i + 2] = (uint32_t)(4 * i + 2);
        out_plan->pick_indices[6 * i + 3] = (uint32_t)(4 * i + 0);
        out_plan->pick_indices[6 * i + 4] = (uint32_t)(4 * i + 2);
        out_plan->pick_indices[6 * i + 5] = (uint32_t)(4 * i + 3);
    }

    *out_vertex_count = vertex_count;
    *out_index_count = index_count;
    return true;
}


/**
 * Build temporary GPU stroke buffers for one stroked path visual.
 *
 * @param visual path visual
 * @param out_plan output pick-plan wrapper
 * @param out_vertex_count output derived vertex count
 * @param out_index_count output derived index count
 * @return true when derived buffers were created
 */
static bool _scene_path_pick_geometry(
    const DvzVisual* visual, DvzSceneProbePlan* out_plan, uint64_t* out_vertex_count,
    uint64_t* out_index_count)
{
    ANN(visual);
    ANN(out_plan);
    ANN(out_vertex_count);
    ANN(out_index_count);

    int pos_idx = _attr_index(visual, "position");
    int color_idx = _attr_index(visual, "color");
    int width_idx = _attr_index(visual, "line_width");
    if (pos_idx < 0 || color_idx < 0 || width_idx < 0)
        return false;

    const DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    const DvzVisualAttr* color_attr = &visual->attrs[color_idx];
    const DvzVisualAttr* width_attr = &visual->attrs[width_idx];
    uint64_t point_count = pos_attr->item_count;
    if (
        pos_attr->data == NULL || color_attr->data == NULL || width_attr->data == NULL ||
        point_count < 2 || color_attr->item_count != point_count ||
        width_attr->item_count != point_count || pos_attr->item_size != sizeof(vec3) ||
        color_attr->item_size != sizeof(DvzColor) || width_attr->item_size != sizeof(float))
    {
        return false;
    }

    uint64_t segment_count = 0;
    uint64_t consumed = 0;
    if (visual->path.subpath_count > 0)
    {
        for (uint32_t i = 0; i < visual->path.subpath_count; i++)
        {
            uint32_t length = visual->path.subpath_lengths[i];
            consumed += length;
            if (length >= 2)
                segment_count += length - 1;
        }
        if (consumed != point_count)
        {
            log_error("path pick request subpath lengths must sum to the path point count");
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
        log_error("path pick request buffer size overflow");
        return false;
    }

    if (!_scene_pick_alloc(
            (void**)&out_plan->pick_position_start, vertex_count, 3 * sizeof(float), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_position_curr, vertex_count, 3 * sizeof(float), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_position_end, vertex_count, 3 * sizeof(float), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_colors, vertex_count, sizeof(DvzColor), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_line_width, vertex_count, sizeof(float), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_path_flags, vertex_count, sizeof(uint32_t), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_path_distance, vertex_count, sizeof(float), "path") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_indices, index_count, sizeof(uint32_t), "path"))
    {
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    const float* line_width = (const float*)width_attr->data;
    uint64_t segment = 0;
    uint64_t offset = 0;
    uint32_t subpath_count = visual->path.subpath_count > 0 ? visual->path.subpath_count : 1;
    for (uint32_t sp = 0; sp < subpath_count; sp++)
    {
        uint32_t length = visual->path.subpath_count > 0 ? visual->path.subpath_lengths[sp]
                                                         : (uint32_t)point_count;
        bool closed = _scene_pick_path_subpath_is_closed(position, offset, length);
        float cumulative = 0.0f;
        for (uint32_t i = 0; i + 1 < length; i++)
        {
            uint64_t i0 = offset + i;
            uint64_t i1 = i0 + 1;
            float edge_length = _scene_pick_path_point_distance(position, i0, i1);
            for (uint32_t j = 0; j < 4; j++)
            {
                bool endpoint_end = j >= 2;
                bool side_negative = j == 1 || j == 2;
                uint64_t point_idx = endpoint_end ? i1 : i0;
                uint64_t prev_idx =
                    _scene_pick_path_prev_index(point_idx, offset, length, closed);
                uint64_t next_idx =
                    _scene_pick_path_next_index(point_idx, offset, length, closed);
                bool has_prev = prev_idx != point_idx;
                bool has_next = next_idx != point_idx;
                bool subpath_start = !closed && point_idx == offset;
                bool subpath_end = !closed && point_idx + 1 == offset + length;
                uint64_t dst = 4 * segment + j;
                dvz_memcpy(
                    &out_plan->pick_position_start[3 * dst], 3 * sizeof(float),
                    &position[3 * prev_idx], 3 * sizeof(float));
                dvz_memcpy(
                    &out_plan->pick_position_curr[3 * dst], 3 * sizeof(float),
                    &position[3 * point_idx], 3 * sizeof(float));
                dvz_memcpy(
                    &out_plan->pick_position_end[3 * dst], 3 * sizeof(float),
                    &position[3 * next_idx], 3 * sizeof(float));
                out_plan->pick_colors[dst] = _scene_pick_encode_item(segment);
                out_plan->pick_line_width[dst] = line_width[point_idx];
                out_plan->pick_path_flags[dst] = _scene_pick_path_vertex_flags(
                    side_negative, endpoint_end, has_prev, has_next, subpath_start, subpath_end);
                out_plan->pick_path_distance[dst] =
                    endpoint_end ? cumulative + edge_length : cumulative;
            }
            out_plan->pick_indices[6 * segment + 0] = (uint32_t)(4 * segment + 0);
            out_plan->pick_indices[6 * segment + 1] = (uint32_t)(4 * segment + 1);
            out_plan->pick_indices[6 * segment + 2] = (uint32_t)(4 * segment + 2);
            out_plan->pick_indices[6 * segment + 3] = (uint32_t)(4 * segment + 0);
            out_plan->pick_indices[6 * segment + 4] = (uint32_t)(4 * segment + 2);
            out_plan->pick_indices[6 * segment + 5] = (uint32_t)(4 * segment + 3);
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
 * Build a synthetic GPU readback frame plan for one item-identity pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the point-like or sphere visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_item_pick_plan(
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
        log_error("item pick request buffer size overflow");
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
        pick_colors[i] = _scene_pick_encode_item(i);
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
    metadata.depth_compare_op = visual->depth_compare_op;
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
            "item pick request %" PRIu64 " failed to assemble the GPU readback plan",
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


/**
 * Build a synthetic GPU readback frame plan for one stroke item-identity pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the segment or stroked path visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_stroke_pick_plan(
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

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_scene_pick_target_extent(figure, panel, &target_width, &target_height))
        return false;

    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    bool geometry_ok = false;
    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
        geometry_ok = _scene_segment_pick_geometry(visual, out_plan, &vertex_count, &index_count);
    else if (visual->type == DVZ_VISUAL_TYPE_PATH)
        geometry_ok = _scene_path_pick_geometry(visual, out_plan, &vertex_count, &index_count);
    if (!geometry_ok)
    {
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t width_bytes = 0;
    uint64_t flags_bytes = 0;
    uint64_t distance_bytes = 0;
    uint64_t index_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, 3 * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(DvzColor), &color_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(float), &width_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(uint32_t), &flags_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(float), &distance_bytes) ||
        _dvz_mul_u64_overflows(index_count, sizeof(uint32_t), &index_bytes))
    {
        log_error("stroke pick request buffer size overflow");
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
    out_plan->plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position_start", 0, position_bytes, "position_start",
                   out_plan->pick_position_start);
    if (visual->type == DVZ_VISUAL_TYPE_PATH)
    {
        ok = ok && dvz_frame_plan_upload_bytes(
                       plan, "pick0_position", 0, position_bytes, "position",
                       out_plan->pick_position_curr);
    }
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position_end", 0, position_bytes, "position_end",
                   out_plan->pick_position_end) &&
         dvz_frame_plan_upload_bytes(
             plan, "pick0_color", 0, color_bytes, "color", out_plan->pick_colors) &&
         dvz_frame_plan_upload_bytes(
             plan, "pick0_line_width", 0, width_bytes, "line_width",
             out_plan->pick_line_width);
    if (visual->type == DVZ_VISUAL_TYPE_PATH)
    {
        ok = ok && dvz_frame_plan_upload_bytes(
                       plan, "pick0_path_flags", 0, flags_bytes, "path_flags",
                       out_plan->pick_path_flags) &&
             dvz_frame_plan_upload_bytes(
                 plan, "pick0_path_distance", 0, distance_bytes, "path_distance",
                 out_plan->pick_path_distance);
    }
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_index", 0, index_bytes, "index", out_plan->pick_indices);
    if (ok)
        _scene_pick_mark_last_upload_index(plan, sizeof(uint32_t));
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_material", 0, sizeof(DvzSceneMaterialParams), "material_params",
                   &visual->material_params);
    if (ok)
        _scene_pick_mark_last_upload_uniform(plan);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)visual->type;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    metadata.depth_compare_op = visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    metadata.index_count = (uint32_t)index_count;
    dvz_strlcpy(
        metadata.position_start_id, "pick0_position_start", sizeof(metadata.position_start_id));
    if (visual->type == DVZ_VISUAL_TYPE_PATH)
        dvz_strlcpy(metadata.position_id, "pick0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.position_end_id, "pick0_position_end", sizeof(metadata.position_end_id));
    dvz_strlcpy(metadata.color_id, "pick0_color", sizeof(metadata.color_id));
    dvz_strlcpy(metadata.line_width_id, "pick0_line_width", sizeof(metadata.line_width_id));
    dvz_strlcpy(metadata.index_id, "pick0_index", sizeof(metadata.index_id));
    dvz_strlcpy(metadata.material_id, "pick0_material", sizeof(metadata.material_id));
    if (visual->type == DVZ_VISUAL_TYPE_PATH)
    {
        dvz_strlcpy(metadata.path_flags_id, "pick0_path_flags", sizeof(metadata.path_flags_id));
        dvz_strlcpy(
            metadata.path_distance_id, "pick0_path_distance",
            sizeof(metadata.path_distance_id));
    }

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.pick", "target.pick", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "pick0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _scene_pick_plan_apply_render_state(plan, panel, request_ndc, target_width, target_height);
    ok = ok && dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
         dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
    if (!ok)
    {
        log_error(
            "stroke pick request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    *out_target_width = target_width;
    *out_target_height = target_height;
    return true;
}


/**
 * Build a synthetic GPU readback frame plan for one primitive-pipeline pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the primitive, mesh, or non-stroked path visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_primitive_pick_plan(
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
    if (pos_idx < 0)
        return false;
    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    if (pos_attr->data == NULL || pos_attr->item_count == 0 ||
        pos_attr->item_size != sizeof(vec3))
    {
        return false;
    }
    uint64_t vertex_count = pos_attr->item_count;

    uint64_t source_index_count = vertex_count;
    if (visual->buffer != NULL && visual->buffer->data != NULL &&
        visual->buffer->desc.byte_size > 0 && visual->buffer->desc.stride > 0)
    {
        uint32_t stride = visual->buffer->desc.stride;
        if (stride != sizeof(uint16_t) && stride != sizeof(uint32_t))
        {
            log_error("primitive pick request index stride must be 16-bit or 32-bit");
            return false;
        }
        if (visual->buffer->desc.byte_size % stride != 0)
        {
            log_error("primitive pick request index buffer size is not stride-aligned");
            return false;
        }
        source_index_count = visual->buffer->desc.byte_size / stride;
    }

    uint64_t primitive_count = 0;
    uint64_t draw_vertex_count = 0;
    uint32_t draw_topology = (uint32_t)visual->topology;
    switch (visual->topology)
    {
    case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
        primitive_count = source_index_count;
        draw_vertex_count = primitive_count;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
        primitive_count = source_index_count / 2;
        draw_vertex_count = primitive_count * 2;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        if (source_index_count < 2)
            return false;
        primitive_count = source_index_count - 1;
        draw_vertex_count = primitive_count * 2;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        primitive_count = source_index_count / 3;
        draw_vertex_count = primitive_count * 3;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        if (source_index_count < 3)
            return false;
        primitive_count = source_index_count - 2;
        draw_vertex_count = primitive_count * 3;
        draw_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    default:
        return false;
    }
    if (primitive_count == 0 || draw_vertex_count == 0 || draw_vertex_count > UINT32_MAX)
        return false;

    if (!_scene_pick_alloc(
            (void**)&out_plan->probe_positions, draw_vertex_count, sizeof(vec3), "primitive") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_colors, draw_vertex_count, sizeof(DvzColor), "primitive"))
    {
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    const float* position = (const float*)pos_attr->data;
    for (uint64_t prim = 0; prim < primitive_count; prim++)
    {
        uint64_t draw_indices[3] = {0, 0, 0};
        uint32_t prim_vertex_count = 1;
        switch (visual->topology)
        {
        case DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST:
            draw_indices[0] = prim;
            prim_vertex_count = 1;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST:
            draw_indices[0] = 2 * prim + 0;
            draw_indices[1] = 2 * prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            prim_vertex_count = 2;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            draw_indices[0] = 3 * prim + 0;
            draw_indices[1] = 3 * prim + 1;
            draw_indices[2] = 3 * prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            draw_indices[0] = prim;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        case DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            draw_indices[0] = 0;
            draw_indices[1] = prim + 1;
            draw_indices[2] = prim + 2;
            prim_vertex_count = 3;
            break;
        default:
            return false;
        }

        for (uint32_t j = 0; j < prim_vertex_count; j++)
        {
            uint64_t source_index = 0;
            if (!_scene_pick_source_vertex_index(
                    visual, draw_indices[j], vertex_count, &source_index))
            {
                _scene_probe_plan_destroy(out_plan);
                return false;
            }
            uint64_t dst = prim * prim_vertex_count + j;
            dvz_memcpy(
                out_plan->probe_positions[dst], sizeof(vec3), &position[3 * source_index],
                sizeof(vec3));
            out_plan->pick_colors[dst] = _scene_pick_encode_item(prim);
        }
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_scene_pick_target_extent(figure, panel, &target_width, &target_height))
    {
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t color_bytes = 0;
    if (
        _dvz_mul_u64_overflows(draw_vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(draw_vertex_count, sizeof(DvzColor), &color_bytes))
    {
        log_error("primitive pick request buffer size overflow");
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
    out_plan->plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position", 0, position_bytes, "position",
                   out_plan->probe_positions);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, draw_topology);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_color", 0, color_bytes, "color", out_plan->pick_colors);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)visual->type;
    metadata.topology = draw_topology;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    metadata.depth_compare_op = visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)draw_vertex_count;
    dvz_strlcpy(metadata.position_id, "pick0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "pick0_color", sizeof(metadata.color_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.pick", "target.pick", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "pick0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _scene_pick_plan_apply_render_state(plan, panel, request_ndc, target_width, target_height);
    ok = ok && dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
         dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
    if (!ok)
    {
        log_error(
            "primitive pick request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    *out_target_width = target_width;
    *out_target_height = target_height;
    return true;
}


/**
 * Build a synthetic GPU readback frame plan for one image item pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the image visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_image_pick_plan(
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
    if (pos_idx < 0)
        return false;
    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    if (pos_attr->data == NULL || pos_attr->item_count == 0 ||
        pos_attr->item_size != sizeof(vec3))
    {
        return false;
    }
    const float* position = (const float*)pos_attr->data;

    uint64_t item_count = 1;
    uint64_t vertex_count = 6;
    bool generated_quads = _scene_pick_visual_has_attr_data(visual, "extent");
    if (generated_quads)
    {
        int extent_idx = _attr_index(visual, "extent");
        DvzVisualAttr* extent_attr = extent_idx >= 0 ? &visual->attrs[extent_idx] : NULL;
        if (
            extent_attr == NULL || extent_attr->data == NULL ||
            extent_attr->item_count != pos_attr->item_count ||
            extent_attr->item_size != 2 * sizeof(float))
        {
            return false;
        }
        item_count = pos_attr->item_count;
        if (_dvz_mul_u64_overflows(item_count, 6, &vertex_count) || vertex_count > UINT32_MAX)
        {
            log_error("image pick request buffer size overflow");
            return false;
        }
    }
    else if (pos_attr->item_count != 4 && pos_attr->item_count != 6)
    {
        return false;
    }

    if (!_scene_pick_alloc(
            (void**)&out_plan->probe_positions, vertex_count, sizeof(vec3), "image") ||
        !_scene_pick_alloc(
            (void**)&out_plan->pick_colors, vertex_count, sizeof(DvzColor), "image"))
    {
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    if (generated_quads)
    {
        DvzVisualAttr* extent_attr = &visual->attrs[_attr_index(visual, "extent")];
        const float* extent = (const float*)extent_attr->data;
        int anchor_idx = _attr_index(visual, "anchor");
        const float* anchor = anchor_idx >= 0 ? (const float*)visual->attrs[anchor_idx].data : NULL;
        for (uint64_t i = 0; i < item_count; i++)
        {
            float x = position[3 * i + 0];
            float y = position[3 * i + 1];
            float z = position[3 * i + 2];
            float w = extent[2 * i + 0];
            float h = extent[2 * i + 1];
            float ax = anchor != NULL ? anchor[2 * i + 0] : 0.0f;
            float ay = anchor != NULL ? anchor[2 * i + 1] : 0.0f;
            float x0 = x - 0.5f * (ax + 1.0f) * w;
            float x1 = x0 + w;
            float y0 = y - 0.5f * (ay + 1.0f) * h;
            float y1 = y0 + h;
            const float quad_pos[6][3] = {
                {x0, y0, z}, {x0, y1, z}, {x1, y0, z},
                {x1, y0, z}, {x0, y1, z}, {x1, y1, z},
            };
            for (uint32_t j = 0; j < 6; j++)
            {
                uint64_t dst = 6 * i + j;
                dvz_memcpy(out_plan->probe_positions[dst], sizeof(vec3), quad_pos[j], sizeof(vec3));
                out_plan->pick_colors[dst] = _scene_pick_encode_item(i);
            }
        }
    }
    else if (pos_attr->item_count == 4)
    {
        const uint32_t order[6] = {0, 1, 2, 2, 1, 3};
        for (uint32_t j = 0; j < 6; j++)
        {
            dvz_memcpy(
                out_plan->probe_positions[j], sizeof(vec3), &position[3 * order[j]],
                sizeof(vec3));
            out_plan->pick_colors[j] = _scene_pick_encode_item(0);
        }
    }
    else
    {
        for (uint32_t j = 0; j < 6; j++)
        {
            dvz_memcpy(
                out_plan->probe_positions[j], sizeof(vec3), &position[3 * j], sizeof(vec3));
            out_plan->pick_colors[j] = _scene_pick_encode_item(0);
        }
    }

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_scene_pick_target_extent(figure, panel, &target_width, &target_height))
    {
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t color_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(DvzColor), &color_bytes))
    {
        log_error("image pick request buffer size overflow");
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
    out_plan->plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position", 0, position_bytes, "position",
                   out_plan->probe_positions);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_color", 0, color_bytes, "color", out_plan->pick_colors);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_PRIMITIVE;
    metadata.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    metadata.depth_compare_op = visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    dvz_strlcpy(metadata.position_id, "pick0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "pick0_color", sizeof(metadata.color_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.pick", "target.pick", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "pick0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _scene_pick_plan_apply_render_state(plan, panel, request_ndc, target_width, target_height);
    ok = ok && dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
         dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
    if (!ok)
    {
        log_error(
            "image pick request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    *out_target_width = target_width;
    *out_target_height = target_height;
    return true;
}


/**
 * Build a synthetic GPU readback frame plan for one volume visual-identity pick request.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the volume visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when the plan was assembled
 */
static bool _scene_volume_pick_plan(
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
    if (pos_idx < 0)
        return false;
    DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    if (pos_attr->data == NULL || pos_attr->item_count == 0 ||
        pos_attr->item_size != sizeof(vec3) || pos_attr->item_count > UINT32_MAX)
    {
        return false;
    }
    uint64_t vertex_count = pos_attr->item_count;
    if (!_scene_pick_alloc(
            (void**)&out_plan->pick_colors, vertex_count, sizeof(DvzColor), "volume"))
    {
        return false;
    }
    for (uint64_t i = 0; i < vertex_count; i++)
        out_plan->pick_colors[i] = _scene_pick_encode_item(0);

    uint32_t target_width = 0;
    uint32_t target_height = 0;
    if (!_scene_pick_target_extent(figure, panel, &target_width, &target_height))
    {
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    uint64_t position_bytes = 0;
    uint64_t color_bytes = 0;
    if (
        _dvz_mul_u64_overflows(vertex_count, sizeof(vec3), &position_bytes) ||
        _dvz_mul_u64_overflows(vertex_count, sizeof(DvzColor), &color_bytes))
    {
        log_error("volume pick request buffer size overflow");
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    DvzFramePlan* plan = dvz_frame_plan("figure.pick", pending->request.request_id);
    out_plan->plan = plan;
    bool ok = plan != NULL;
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_position", 0, position_bytes, "position", pos_attr->data);
    if (ok)
        ok = dvz_frame_plan_upload_set_topology(plan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ok = ok && dvz_frame_plan_upload_bytes(
                   plan, "pick0_color", 0, color_bytes, "color", out_plan->pick_colors);

    DvzFramePlanVisualMeta metadata = {0};
    metadata.has_metadata = true;
    metadata.visual_type = (uint32_t)DVZ_VISUAL_TYPE_PRIMITIVE;
    metadata.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    metadata.depth_test_enabled = visual->depth_test_enabled;
    metadata.depth_compare_op = visual->depth_compare_op;
    metadata.vertex_count = (uint32_t)vertex_count;
    dvz_strlcpy(metadata.position_id, "pick0_position", sizeof(metadata.position_id));
    dvz_strlcpy(metadata.color_id, "pick0_color", sizeof(metadata.color_id));

    ok = ok && dvz_frame_plan_render_panel(
                   plan, "panel.pick", "target.pick", true,
                   (DvzPanelDesc){.x = 0, .y = 0, .width = 1, .height = 1}) &&
         dvz_frame_plan_render_visual(plan, "pick0") &&
         dvz_frame_plan_render_visual_metadata(plan, &metadata);
    if (ok)
        _scene_pick_plan_apply_render_state(plan, panel, request_ndc, target_width, target_height);
    ok = ok && dvz_frame_plan_copy(plan, "target.pick", "buf.pick", 4) &&
         dvz_frame_plan_readback(plan, "buf.pick", "request.pick");
    if (!ok)
    {
        log_error(
            "volume pick request %" PRIu64 " failed to assemble the GPU readback plan",
            pending->request.request_id);
        _scene_probe_plan_destroy(out_plan);
        return false;
    }

    *out_target_width = target_width;
    *out_target_height = target_height;
    return true;
}


/**
 * Build the visual-family-specific GPU pick plan for one eligible visual.
 *
 * @param figure the parent figure
 * @param panel the panel receiving the request
 * @param visual the visual to pick
 * @param pending the pending pick request
 * @param request_ndc the request coordinate in panel-local NDC
 * @param out_plan the output plan wrapper
 * @param out_target_width output offscreen target width
 * @param out_target_height output offscreen target height
 * @return true when a matching plan was assembled
 */
static bool _scene_visual_pick_plan(
    const DvzFigure* figure, const DvzPanel* panel, DvzVisual* visual,
    const DvzPendingPickRequest* pending, const vec2 request_ndc, DvzSceneProbePlan* out_plan,
    uint32_t* out_target_width, uint32_t* out_target_height)
{
    ANN(figure);
    ANN(panel);
    ANN(visual);
    switch (visual->type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
    case DVZ_VISUAL_TYPE_SPHERE:
        return _scene_item_pick_plan(
            figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
            out_target_height);
    case DVZ_VISUAL_TYPE_SEGMENT:
        return _scene_stroke_pick_plan(
            figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
            out_target_height);
    case DVZ_VISUAL_TYPE_PATH:
        if (_scene_pick_visual_has_attr_data(visual, "line_width"))
        {
            return _scene_stroke_pick_plan(
                figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
                out_target_height);
        }
        return _scene_primitive_pick_plan(
            figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
            out_target_height);
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
        return _scene_primitive_pick_plan(
            figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
            out_target_height);
    case DVZ_VISUAL_TYPE_IMAGE:
        return _scene_image_pick_plan(
            figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
            out_target_height);
    case DVZ_VISUAL_TYPE_VOLUME:
        return _scene_volume_pick_plan(
            figure, panel, visual, pending, request_ndc, out_plan, out_target_width,
            out_target_height);
    default:
        return false;
    }
}


/**
 * Resolve one pick request by trying visible visuals from top to bottom.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor for GPU-backed resolvers
 * @param caps capability snapshot
 * @param pending pending pick request
 * @return whether a result was queued
 */
static bool _scene_process_pick_request(
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

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (miss.status == DVZ_PICK_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PICK_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("visual pick request requires a DRP2 runtime");
            miss.status = DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_visual_pick_plan(
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

    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
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
        if (!_scene_item_pick_plan(
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
 * Resolve one sphere pick request through the GPU impostor pick pass.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor for the GPU readback stream
 * @param caps capability snapshot
 * @param pending pending pick request
 * @return whether a result was queued
 */
static bool _scene_process_sphere_pick_request(
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

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PICK_STATUS_OUTSIDE_PANEL;
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_SPHERE)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (miss.status == DVZ_PICK_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PICK_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("sphere pick request requires a DRP2 runtime");
            miss.status = DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_item_pick_plan(
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

    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
}


/**
 * Resolve one segment/path pick request through the GPU stroke pick pass.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor for the GPU readback stream
 * @param caps capability snapshot
 * @param pending pending pick request
 * @return whether a result was queued
 */
static bool _scene_process_stroke_pick_request(
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

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PICK_STATUS_OUTSIDE_PANEL;
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        if (visual->type != DVZ_VISUAL_TYPE_SEGMENT && visual->type != DVZ_VISUAL_TYPE_PATH)
            continue;
        if (
            visual->type == DVZ_VISUAL_TYPE_PATH &&
            !_scene_pick_visual_has_attr_data(visual, "line_width"))
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (miss.status == DVZ_PICK_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PICK_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("stroke pick request requires a DRP2 runtime");
            miss.status = DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_stroke_pick_plan(
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

    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
}


/**
 * Resolve one primitive-pipeline pick request through a GPU primitive-id pick pass.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor for the GPU readback stream
 * @param caps capability snapshot
 * @param pending pending pick request
 * @return whether a result was queued
 */
static bool _scene_process_primitive_pick_request(
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

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PICK_STATUS_OUTSIDE_PANEL;
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible)
            continue;
        bool primitive_like = visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                              visual->type == DVZ_VISUAL_TYPE_MESH ||
                              visual->type == DVZ_VISUAL_TYPE_PATH ||
                              visual->type == DVZ_VISUAL_TYPE_VOLUME;
        if (!primitive_like)
            continue;
        if (
            visual->type == DVZ_VISUAL_TYPE_PATH &&
            _scene_pick_visual_has_attr_data(visual, "line_width"))
        {
            continue;
        }
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (miss.status == DVZ_PICK_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PICK_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("primitive pick request requires a DRP2 runtime");
            miss.status = DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_primitive_pick_plan(
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

    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
}


/**
 * Resolve one image pick request through encoded GPU item quads.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor for the GPU readback stream
 * @param caps capability snapshot
 * @param pending pending pick request
 * @return whether a result was queued
 */
static bool _scene_process_image_pick_request(
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

    vec2 request_ndc = {0};
    if (!_scene_pick_request_ndc(figure, panel, pending->x, pending->y, request_ndc))
    {
        miss.status = DVZ_PICK_STATUS_OUTSIDE_PANEL;
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    uint32_t order[DVZ_SCENE_MAX_VISUALS] = {0};
    _scene_panel_visual_order(panel, order);
    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if ((visual->pick_capabilities & DVZ_PICK_CAPABILITY_ITEM) == 0)
            continue;
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
            continue;

        if (miss.status == DVZ_PICK_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PICK_STATUS_MISS;

        if (executor == NULL || executor->runtime == NULL || executor->emitter == NULL)
        {
            log_error("image pick request requires a DRP2 runtime");
            miss.status = DVZ_PICK_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzSceneProbePlan pick_plan = {0};
        uint32_t target_width = 0;
        uint32_t target_height = 0;
        if (!_scene_image_pick_plan(
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



/**
 * Resolve one labels probe request.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor
 * @param caps runtime capability snapshot
 * @param pending pending probe request
 * @return whether a result was queued
 */
static bool _scene_process_labels_probe_request(
    DvzFigure* figure, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(pending);
    ANN(pending->panel);
    (void)caps;

    DvzScene* scene = figure->scene;
    DvzPanel* panel = pending->panel;
    DvzProbeResult miss =
        _scene_probe_miss_result(figure, panel, pending, DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL);

    if (
        pending->request.target != DVZ_SCENE_TARGET_NONE &&
        pending->request.target != DVZ_SCENE_TARGET_SEGMENT)
    {
        miss.status = DVZ_PROBE_STATUS_UNSUPPORTED_TARGET;
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
    }

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
        if (
            visual == NULL || !visual->visible || visual->type != DVZ_VISUAL_TYPE_LABELS ||
            attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            continue;
        }
        DvzSampledField* field = visual->field;
        if (field == NULL || field->data == NULL)
            continue;
        if (field->desc.dim != DVZ_FIELD_DIM_2D || field->desc.width == 0 ||
            field->desc.height == 0)
        {
            continue;
        }

        uint32_t texture_format = 0;
        uint32_t bytes_per_texel = 0;
        if (!_scene_labels_probe_integer_format(
                field->desc.format, &texture_format, &bytes_per_texel))
            continue;
        (void)texture_format;
        (void)bytes_per_texel;

        if (miss.status == DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL)
            miss.status = DVZ_PROBE_STATUS_MISS;

        double uv[2] = {0};
        if (!_scene_labels_probe_visual_uv(panel, visual, request_ndc, uv))
            continue;
        if (uv[0] < 0.0 || uv[0] > 1.0 || uv[1] < 0.0 || uv[1] > 1.0)
            continue;

        uint32_t texel_x = (uint32_t)floor(uv[0] * (double)field->desc.width);
        uint32_t texel_y = (uint32_t)floor(uv[1] * (double)field->desc.height);
        if (texel_x >= field->desc.width)
            texel_x = field->desc.width - 1;
        if (texel_y >= field->desc.height)
            texel_y = field->desc.height - 1;

        if (executor == NULL || executor->runtime == NULL)
        {
            log_error("labels probe request requires a DRP2 runtime");
            miss.status = DVZ_PROBE_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        uint8_t sample[4] = {0};
        bool executed = false;
        bool readback_ok = _scene_labels_probe_readback(
            scene, executor, field, texel_x, texel_y, sample, &executed);
        if (!readback_ok)
        {
            miss.status =
                executed ? DVZ_PROBE_STATUS_READBACK_FAILED : DVZ_PROBE_STATUS_GPU_EXEC_FAILED;
            continue;
        }

        DvzCategoryId label_id = 0;
        if (!_scene_labels_probe_decode_sample(field->desc.format, sample, &label_id))
            continue;
        if (label_id == visual->labels.background_id)
            continue;

        DvzProbeResult resolved = miss;
        resolved.hit = true;
        resolved.status = DVZ_PROBE_STATUS_HIT;
        resolved.visual_id = _scene_visual_public_id(scene, visual);
        resolved.visual_family = DVZ_SCENE_VISUAL_FAMILY_LABELS;
        resolved.target = DVZ_SCENE_TARGET_SEGMENT;
        resolved.value_kind = DVZ_PROBE_VALUE_LABEL;
        resolved.category_id = label_id;
        resolved.scale = visual->scale;
        resolved.has_uvw = true;
        resolved.uvw[0] = uv[0];
        resolved.uvw[1] = uv[1];
        resolved.uvw[2] = 0.0;
        resolved.target_id = label_id >= 0 ? (uint64_t)label_id : 0;
        resolved.group_id = resolved.target_id;
        _scene_labels_probe_category_label(
            visual, label_id, resolved.label, sizeof(resolved.label));
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

    for (int32_t oi = (int32_t)panel->visual_count - 1; oi >= 0; oi--)
    {
        DvzPanelAttach* attach = &panel->visuals[order[oi]];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_IMAGE)
            continue;
        if (!visual->visible || attach->controller_mode == DVZ_CONTROLLER_FIXED)
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
        if (readback_ok && rgba[3] == 0)
        {
            log_error(
                "image probe request %" PRIu64 " returned a transparent GPU pixel",
                pending->request.request_id);
        }
        _scene_probe_plan_destroy(&probe_plan);

        if (!hit)
            continue;

        DvzSceneProbePayload payload = {0};
        if (!_scene_decode_image_probe_payload(visual, rgba, &payload))
            continue;

        DvzProbeResult resolved = miss;
        _scene_apply_probe_payload(scene, visual, &payload, &resolved);
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &resolved);
    }

    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
}
