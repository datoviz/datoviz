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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "query/internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzSceneVisualFamily _scene_visual_family(uint32_t visual_type);

static bool _scene_pick_target_supported(DvzSceneTargetKind target);

static bool _scene_probe_target_supported(DvzSceneTargetKind target);

static DvzPickResult _scene_pick_miss_result(
    const DvzFigure* figure, const DvzPanel* panel, const DvzPendingPickRequest* pending,
    DvzPickStatus status);

static DvzPickStatus _scene_pick_status_from_query(DvzQueryStatus status);

static void _scene_pick_result_from_query(const DvzQueryResult* query, DvzPickResult* out_result);

static DvzProbeResult _scene_probe_miss_result(
    const DvzFigure* figure, const DvzPanel* panel, const DvzPendingProbeRequest* pending,
    DvzProbeStatus status);

static DvzProbeStatus _scene_probe_status_from_query(DvzQueryStatus status);

static void _scene_probe_result_from_query(
    const DvzQueryResult* query, DvzProbeResult* out_result);

static bool _scene_runtime_config_matches(
    const DvzDrp2RuntimeConfig* a, const DvzDrp2RuntimeConfig* b);

static bool _scene_labels_probe_integer_format(
    DvzFieldFormat format, uint32_t* out_texture_format, uint32_t* out_bytes_per_texel);

static bool _scene_probe_request_has_labels_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_probe_request_has_volume_slice_candidate(
    const DvzFigure* figure, const DvzPendingProbeRequest* pending);

static bool _scene_process_pick_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingPickRequest* pending);

static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingProbeRequest* pending);

static bool _scene_process_labels_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingProbeRequest* pending);

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
        (void)_scene_process_pick_request(figure, runtime, executor, caps, &pending);
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
            (void)_scene_process_labels_probe_request(figure, runtime, executor, caps, &pending);
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
        (void)_scene_process_image_probe_request(figure, runtime, executor, caps, &pending);
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
 * Convert a native query status to a transitional pick status.
 *
 * @param status native query status
 * @return legacy pick status
 */
static DvzPickStatus _scene_pick_status_from_query(DvzQueryStatus status)
{
    switch (status)
    {
    case DVZ_QUERY_STATUS_HIT:
        return DVZ_PICK_STATUS_HIT;
    case DVZ_QUERY_STATUS_MISS:
        return DVZ_PICK_STATUS_MISS;
    case DVZ_QUERY_STATUS_OUTSIDE_PANEL:
        return DVZ_PICK_STATUS_OUTSIDE_PANEL;
    case DVZ_QUERY_STATUS_STALE_DROPPED:
        return DVZ_PICK_STATUS_STALE_DROPPED;
    case DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL:
    case DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY:
        return DVZ_PICK_STATUS_NO_CAPABLE_VISUAL;
    case DVZ_QUERY_STATUS_UNSUPPORTED_TARGET:
        return DVZ_PICK_STATUS_UNSUPPORTED_TARGET;
    case DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE:
    case DVZ_QUERY_STATUS_UNSUPPORTED_GPU_FORMAT:
    case DVZ_QUERY_STATUS_GPU_EXEC_FAILED:
        return DVZ_PICK_STATUS_GPU_EXEC_FAILED;
    case DVZ_QUERY_STATUS_READBACK_FAILED:
        return DVZ_PICK_STATUS_READBACK_FAILED;
    case DVZ_QUERY_STATUS_DECODE_FAILED:
        return DVZ_PICK_STATUS_INVALID_RESULT;
    case DVZ_QUERY_STATUS_UNKNOWN:
    default:
        return DVZ_PICK_STATUS_UNKNOWN;
    }
}



/**
 * Convert a native query result to a transitional pick result.
 *
 * @param query native query result
 * @param out_result legacy pick result
 */
static void _scene_pick_result_from_query(const DvzQueryResult* query, DvzPickResult* out_result)
{
    ANN(query);
    ANN(out_result);
    *out_result = (DvzPickResult){0};
    out_result->request_id = query->request_id;
    out_result->status = _scene_pick_status_from_query(query->status);
    out_result->hit = query->hit && out_result->status == DVZ_PICK_STATUS_HIT;
    out_result->panel_id = query->panel_id;
    out_result->visual_id = query->visual_id;
    out_result->visual_family = query->visual_family;
    out_result->item_id = query->item_id;
    out_result->group_id = query->group_id;
    out_result->auxiliary_id = query->auxiliary_id;
    out_result->raw_parent_target = query->raw_parent_target;
    out_result->raw_parent_id = query->raw_parent_id;
    out_result->raw_target = query->raw_target;
    out_result->raw_id = query->raw_id;
    out_result->resolved_parent_target = query->resolved_parent_target;
    out_result->resolved_parent_id = query->resolved_parent_id;
    out_result->resolved_target = query->resolved_target;
    out_result->resolved_id = query->resolved_id;
    out_result->instance_id = query->instance_id;
    out_result->link_key = query->link_key;
    out_result->panel_position[0] = query->panel_position[0];
    out_result->panel_position[1] = query->panel_position[1];
    out_result->has_data_position = query->has_data_position;
    out_result->data_position[0] = query->data_position[0];
    out_result->data_position[1] = query->data_position[1];
    out_result->data_position[2] = query->data_position[2];
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
 * Convert a native query status to a transitional probe status.
 *
 * @param status native query status
 * @return legacy probe status
 */
static DvzProbeStatus _scene_probe_status_from_query(DvzQueryStatus status)
{
    switch (status)
    {
    case DVZ_QUERY_STATUS_HIT:
        return DVZ_PROBE_STATUS_HIT;
    case DVZ_QUERY_STATUS_MISS:
        return DVZ_PROBE_STATUS_MISS;
    case DVZ_QUERY_STATUS_OUTSIDE_PANEL:
        return DVZ_PROBE_STATUS_OUTSIDE_PANEL;
    case DVZ_QUERY_STATUS_STALE_DROPPED:
        return DVZ_PROBE_STATUS_STALE_DROPPED;
    case DVZ_QUERY_STATUS_NO_CAPABLE_VISUAL:
    case DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY:
        return DVZ_PROBE_STATUS_NO_CAPABLE_VISUAL;
    case DVZ_QUERY_STATUS_UNSUPPORTED_TARGET:
        return DVZ_PROBE_STATUS_UNSUPPORTED_TARGET;
    case DVZ_QUERY_STATUS_UNSUPPORTED_QUERY_PROFILE:
    case DVZ_QUERY_STATUS_UNSUPPORTED_GPU_FORMAT:
    case DVZ_QUERY_STATUS_GPU_EXEC_FAILED:
        return DVZ_PROBE_STATUS_GPU_EXEC_FAILED;
    case DVZ_QUERY_STATUS_READBACK_FAILED:
        return DVZ_PROBE_STATUS_READBACK_FAILED;
    case DVZ_QUERY_STATUS_DECODE_FAILED:
        return DVZ_PROBE_STATUS_INVALID_RESULT;
    case DVZ_QUERY_STATUS_UNKNOWN:
    default:
        return DVZ_PROBE_STATUS_UNKNOWN;
    }
}



/**
 * Convert a native query result to a transitional probe result.
 *
 * @param query native query result
 * @param out_result legacy probe result
 */
static void _scene_probe_result_from_query(
    const DvzQueryResult* query, DvzProbeResult* out_result)
{
    ANN(query);
    ANN(out_result);
    *out_result = (DvzProbeResult){0};
    out_result->request_id = query->request_id;
    out_result->status = _scene_probe_status_from_query(query->status);
    out_result->hit = query->hit && out_result->status == DVZ_PROBE_STATUS_HIT;
    out_result->panel_id = query->panel_id;
    out_result->visual_id = query->visual_id;
    out_result->visual_family = query->visual_family;
    out_result->item_id = query->item_id;
    out_result->group_id = query->group_id;
    out_result->auxiliary_id = query->auxiliary_id;
    out_result->target = query->resolved_target;
    out_result->target_id = query->resolved_id;
    out_result->panel_position[0] = query->panel_position[0];
    out_result->panel_position[1] = query->panel_position[1];
    out_result->has_coordinate = query->has_data_position;
    out_result->coordinate[0] = query->data_position[0];
    out_result->coordinate[1] = query->data_position[1];
    out_result->coordinate[2] = query->data_position[2];
    out_result->has_uvw = query->has_uvw;
    out_result->uvw[0] = query->uvw[0];
    out_result->uvw[1] = query->uvw[1];
    out_result->uvw[2] = query->uvw[2];
    switch (query->value_kind)
    {
    case DVZ_QUERY_VALUE_SCALAR:
        out_result->value_kind = DVZ_PROBE_VALUE_SCALAR;
        break;
    case DVZ_QUERY_VALUE_VEC2:
        out_result->value_kind = DVZ_PROBE_VALUE_VEC2;
        break;
    case DVZ_QUERY_VALUE_VEC3:
        out_result->value_kind = DVZ_PROBE_VALUE_VEC3;
        break;
    case DVZ_QUERY_VALUE_VEC4:
        out_result->value_kind = DVZ_PROBE_VALUE_VEC4;
        break;
    case DVZ_QUERY_VALUE_CATEGORY:
        out_result->value_kind = DVZ_PROBE_VALUE_LABEL;
        break;
    case DVZ_QUERY_VALUE_NONE:
    case DVZ_QUERY_VALUE_TEXT:
    case DVZ_QUERY_VALUE_OPAQUE_FAMILY_PAYLOAD:
    default:
        out_result->value_kind = DVZ_PROBE_VALUE_NONE;
        break;
    }
    out_result->scalar = query->scalar;
    for (uint32_t i = 0; i < 4; i++)
        out_result->vector[i] = query->vector[i];
    out_result->category_id = query->category_id;
    dvz_strlcpy(out_result->label, query->label, sizeof(out_result->label));
    dvz_strlcpy(out_result->unit, query->unit, sizeof(out_result->unit));
    out_result->scale = query->scale;
    out_result->source_request_id = query->request_id;
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
bool _scene_request_executor_prepare(DvzSceneRequestExecutor* executor, DvzDrp2Runtime* source_runtime)
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
 * Resolve one pick request by trying visible visuals from top to bottom.
 *
 * @param figure figure whose request queue is being processed
 * @param executor retained request executor for GPU-backed resolvers
 * @param caps capability snapshot
 * @param pending pending pick request
 * @return whether a result was queued
 */
static bool _scene_process_pick_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingPickRequest* pending)
{
    ANN(figure);
    ANN(executor);
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

    DvzPendingQueryRequest query_pending = {
        .panel = panel,
        .x = pending->x,
        .y = pending->y,
        .freshness_serial = pending->freshness_serial,
        .request = {
            .request_id = pending->request.request_id,
            .target = pending->request.target,
            .hit_policy = pending->request.hit_policy,
            .flags = pending->request.flags,
        },
    };
    DvzQueryResult query = {0};
    if (!_dvz_scene_query_process_pending(
            figure, runtime, executor, caps, &query_pending, &query))
    {
        miss.status = DVZ_PICK_STATUS_INVALID_RESULT;
        return _scene_push_pick_result(scene, panel, pending->freshness_serial, &miss);
    }

    DvzPickResult pick = {0};
    _scene_pick_result_from_query(&query, &pick);
    return _scene_push_pick_result(scene, panel, pending->freshness_serial, &pick);
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
 * Resolve one labels probe request through the native query executor.
 *
 * @param figure figure whose request queue is being processed
 * @param runtime caller runtime
 * @param executor retained request executor
 * @param caps runtime capability snapshot
 * @param pending pending probe request
 * @return whether a result was queued
 */
static bool _scene_process_labels_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(executor);
    ANN(caps);
    ANN(pending);
    ANN(pending->panel);

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

    DvzPendingQueryRequest query_pending = {
        .panel = panel,
        .x = pending->x,
        .y = pending->y,
        .freshness_serial = pending->freshness_serial,
        .request = {
            .request_id = pending->request.request_id,
            .target = DVZ_SCENE_TARGET_SEGMENT,
            .flags = pending->request.flags | DVZ_SCENE_QUERY_FLAG_COMPAT_PROBE,
        },
    };
    DvzQueryResult query = {0};
    if (!_dvz_scene_query_process_pending(
            figure, runtime, executor, caps, &query_pending, &query))
    {
        miss.status = DVZ_PROBE_STATUS_INVALID_RESULT;
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
    }

    DvzProbeResult probe = {0};
    _scene_probe_result_from_query(&query, &probe);
    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &probe);
}



/**
 * Resolve one image probe request through the native query executor.
 *
 * @param figure figure whose request queue is being processed
 * @param runtime caller runtime
 * @param executor retained request executor
 * @param caps runtime capability snapshot
 * @param pending pending probe request
 * @return whether a result was queued
 */
static bool _scene_process_image_probe_request(
    DvzFigure* figure, DvzDrp2Runtime* runtime, DvzSceneRequestExecutor* executor,
    const DvzCapabilitySnapshot* caps, const DvzPendingProbeRequest* pending)
{
    ANN(figure);
    ANN(executor);
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

    DvzPendingQueryRequest query_pending = {
        .panel = panel,
        .x = pending->x,
        .y = pending->y,
        .freshness_serial = pending->freshness_serial,
        .request = {
            .request_id = pending->request.request_id,
            .target = DVZ_SCENE_TARGET_PIXEL,
            .flags = pending->request.flags | DVZ_SCENE_QUERY_FLAG_COMPAT_PROBE,
        },
    };
    DvzQueryResult query = {0};
    if (!_dvz_scene_query_process_pending(
            figure, runtime, executor, caps, &query_pending, &query))
    {
        miss.status = DVZ_PROBE_STATUS_INVALID_RESULT;
        return _scene_push_probe_result(scene, panel, pending->freshness_serial, &miss);
    }

    DvzProbeResult probe = {0};
    _scene_probe_result_from_query(&query, &probe);
    return _scene_push_probe_result(scene, panel, pending->freshness_serial, &probe);
}
