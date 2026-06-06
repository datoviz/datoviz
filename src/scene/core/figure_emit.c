/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene figure emission and live stream tracking                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/frame_plan.h"
#include "_log.h"
#include "scene_emit/scene_emit.h"
#include "_technique.h"
#include "datoviz/drp2/runtime.h"
#include "../../drp2/_stream.h"
#include "render_contract/render_contract.h"
#include "_scene.h"
#include "annotation/prepare_internal.h"
#include "core/figure_emit_internal.h"
#include "core/frame_trace_internal.h"
#include "domain/field_internal.h"
#include "plot/internal.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static bool _scene_visual_dirty_material_emits_upload(const DvzVisual* visual);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Normalize optional emit inputs to concrete stack-backed defaults.
 *
 * @param caps the optional capabilities pointer to normalize
 * @param default_caps the stack storage for default capabilities
 * @param report the optional diagnostic report pointer to normalize
 * @param local_report the stack storage for a local report
 * @param cfg the optional emit config pointer to normalize
 * @param default_cfg the stack storage for the default emit config
 */
static void _scene_emit_defaults(
    const DvzCapabilitySnapshot** caps, DvzCapabilitySnapshot* default_caps,
    DvzDiagnosticReport** report, DvzDiagnosticReport* local_report,
    const DvzFramePlanEmitConfig** cfg, DvzFramePlanEmitConfig* default_cfg)
{
    ANN(caps);
    ANN(default_caps);
    ANN(report);
    ANN(local_report);
    ANN(cfg);
    ANN(default_cfg);
    if (*caps == NULL)
    {
        *default_caps = dvz_capability_snapshot();
        *caps = default_caps;
    }
    *default_cfg = dvz_frame_plan_emit_config();
    if (*cfg == NULL)
        *cfg = default_cfg;
    if (*report == NULL)
    {
        dvz_diagnostic_report_init(local_report);
        *report = local_report;
    }
}



/**
 * Return whether one visual can contribute a drawable panel item.
 *
 * @param visual the visual
 * @return whether the visual is visible and has position data
 */
static bool _scene_emit_visual_drawable(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible || visual->type == DVZ_VISUAL_TYPE_TEXT)
        return false;
    const char* position_attr =
        visual->type == DVZ_VISUAL_TYPE_SEGMENT ? "position_start" : "position";
    int pos_idx = _attr_index(visual, position_attr);
    return pos_idx >= 0 && visual->attrs[pos_idx].item_count > 0;
}



/**
 * Validate per-panel transparency mode combinations before FramePlan emission.
 *
 * @param figure the figure
 * @param figure_id the stable emitted figure identifier
 * @param report the diagnostic report
 * @return whether transparency mode combinations are supported
 */
static bool _scene_figure_validate_transparency_modes(
    const DvzFigure* figure, const char* figure_id, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(figure_id);
    bool ok = true;
    char panel_id[DVZ_SCENE_LABEL_SIZE];
    char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        const DvzPanel* panel = &figure->panels[pi];
        bool has_wboit = false;
        bool has_depth_peel = false;
        uint32_t wboit_visual = UINT32_MAX;
        uint32_t depth_peel_visual = UINT32_MAX;
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            const DvzVisual* visual = panel->visuals[vi].visual;
            if (!_scene_emit_visual_drawable(visual))
                continue;
            uint32_t visual_index = UINT32_MAX;
            (void)_figure_visual_index(figure, visual, &visual_index);
            if (_scene_alpha_mode_is_wboit(visual->alpha_mode))
            {
                has_wboit = true;
                if (wboit_visual == UINT32_MAX)
                    wboit_visual = visual_index;
            }
            if (_scene_alpha_mode_is_depth_peel(visual->alpha_mode))
            {
                has_depth_peel = true;
                if (depth_peel_visual == UINT32_MAX)
                    depth_peel_visual = visual_index;
            }
        }
        if (has_wboit && has_depth_peel)
        {
            dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, pi);
            dvz_snprintf(
                message, sizeof(message),
                "panel %s mixes WBOIT visual %u and depth-peel visual %u; mixed OIT "
                "composition is not specified",
                panel_id, wboit_visual, depth_peel_visual);
            (void)dvz_diagnostic_report_add(report, message);
            ok = false;
        }
    }
    return ok;
}



/**
 * Clamp a requested sample count to a supported power-of-two sample count.
 *
 * @param sample_count requested sample count
 * @param max_sample_count maximum supported sample count
 * @return supported sample count
 */
static uint32_t _scene_lowered_sample_count(uint32_t sample_count, uint32_t max_sample_count)
{
    if (sample_count <= 1 || max_sample_count <= 1)
        return 1;
    if (sample_count >= 16 && max_sample_count >= 16)
        return 16;
    if (sample_count >= 8 && max_sample_count >= 8)
        return 8;
    if (sample_count >= 4 && max_sample_count >= 4)
        return 4;
    if (sample_count >= 2 && max_sample_count >= 2)
        return 2;
    return 1;
}



/**
 * Return the sample-count limit for one graph resource.
 *
 * @param resource the graph resource
 * @param caps the active capability snapshot
 * @return maximum supported sample count for the resource
 */
static uint32_t _scene_resource_sample_limit(
    const DvzFrameGraphResource* resource, const DvzCapabilitySnapshot* caps)
{
    ANN(resource);
    ANN(caps);
    uint32_t max_sample_count = 16;
    bool color = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0;
    bool depth = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0;
    if (color || depth)
    {
        uint32_t color_max = caps->max_color_sample_count != 0 ? caps->max_color_sample_count : 1;
        uint32_t depth_max = caps->max_depth_sample_count != 0 ? caps->max_depth_sample_count : 1;
        max_sample_count = color_max < depth_max ? color_max : depth_max;
    }
    return max_sample_count != 0 ? max_sample_count : 1;
}



/**
 * Report capability fallbacks that the runtime emitter will apply.
 *
 * @param plan the emitted FramePlan
 * @param caps the active capability snapshot
 * @param report optional diagnostic report
 */
static void _scene_report_capability_fallbacks(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    if (plan == NULL || caps == NULL || report == NULL)
        return;

    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource == NULL)
            continue;
        uint32_t sample_count = resource->sample_count != 0 ? resource->sample_count : 1;
        if (sample_count <= 1)
            continue;
        uint32_t max_sample_count = _scene_resource_sample_limit(resource, caps);
        uint32_t lowered = _scene_lowered_sample_count(sample_count, max_sample_count);
        if (lowered >= sample_count)
            continue;

        char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
        int ret = dvz_snprintf(
            message, sizeof(message),
            "scene capability fallback: graph resource '%s' sample count lowered from %u to %u",
            resource->id, sample_count, lowered);
        if (ret >= 0 && (size_t)ret < sizeof(message))
            (void)dvz_diagnostic_report_add(report, message);
        else
            (void)dvz_diagnostic_report_add(
                report, "scene capability fallback: sample count lowered");
    }
}



/**
 * Return whether one visual carries dirty state that should trigger a new emitted frame.
 *
 * @param visual the visual to inspect
 * @return whether rendering work is pending for this visual
 */
static bool _scene_visual_has_pending_render_work(const DvzVisual* visual)
{
    if (visual == NULL || !visual->visible)
        return false;

    if (_scene_visual_dirty_material_emits_upload(visual) ||
        _visual_family_state(visual)->item_state_style_params_dirty ||
        _visual_family_state(visual)->texture.dirty)
        return true;
    if (
        visual->type == DVZ_VISUAL_TYPE_VOLUME &&
        _visual_family_state(visual)->volume_realized_version != _visual_family_state(visual)->volume.version)
    {
        return true;
    }
    if (
        visual->type == DVZ_VISUAL_TYPE_LABELS &&
        _visual_family_state(visual)->labels_realized_version != _visual_family_state(visual)->labels.version)
    {
        return true;
    }
    if (_visual_family_state(visual)->field != NULL && _visual_family_state(visual)->field->dirty)
        return true;
    if (_visual_family_state(visual)->buffer != NULL && _visual_family_state(visual)->buffer->dirty)
        return true;
    if (_visual_family_state(visual)->segment.gpu.dirty || _visual_family_state(visual)->path.gpu.dirty || _visual_family_state(visual)->image_gpu.dirty)
        return true;

    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        if (attr->dirty_item_count > 0)
            return true;
        if (attr->buffer != NULL && attr->buffer->dirty)
            return true;
    }
    return false;
}



/**
 * Return whether one visual has CPU-side data for an attribute.
 *
 * @param visual the visual to inspect
 * @param attr_name the attribute name
 * @return whether the attribute exists and has at least one item
 */
static bool _scene_visual_has_attr_data(const DvzVisual* visual, const char* attr_name)
{
    if (visual == NULL || attr_name == NULL)
        return false;

    int attr_idx = _attr_index(visual, attr_name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}



/**
 * Return whether a dirty material payload is emitted for one visual family.
 *
 * @param visual the visual to inspect
 * @return whether material_params_dirty should trigger app rendering
 */
static bool _scene_visual_dirty_material_emits_upload(const DvzVisual* visual)
{
    if (visual == NULL || !_visual_family_state(visual)->material_params_dirty)
        return false;

    switch (visual->type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
    case DVZ_VISUAL_TYPE_SEGMENT:
    case DVZ_VISUAL_TYPE_PATH:
    case DVZ_VISUAL_TYPE_SPHERE:
        return true;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
        return _scene_visual_has_attr_data(visual, "normal");
    default:
        return false;
    }
}



/**
 * Return whether panel-owned adornments need realization before the next emitted frame.
 *
 * @param panel the panel to inspect
 * @return whether axis, text, or annotation work is pending
 */
static bool _scene_panel_has_pending_adornment_work(const DvzPanel* panel)
{
    if (panel == NULL)
        return false;

    for (uint32_t dim = 0; dim < 2; dim++)
    {
        const DvzAxis* axis = &panel->axes[dim];
        if (axis->panel == panel && axis->enabled && axis->dirty)
            return true;
    }

    const DvzScene* scene = panel->figure != NULL ? panel->figure->scene : NULL;
    if (scene == NULL)
        return false;

    for (uint32_t i = 0; i < scene->annotation_count; i++)
    {
        const DvzAnnotation* annotation = &scene->annotations[i];
        if (annotation->panel == panel && annotation->dirty_flags != DVZ_TEXT_DIRTY_NONE)
            return true;
    }
    for (uint32_t i = 0; i < scene->pinned_readout_count; i++)
    {
        const DvzPinnedReadout* readout = &scene->pinned_readouts[i];
        if (readout->panel == panel && readout->card.dirty)
            return true;
    }
    for (uint32_t i = 0; i < scene->selection_count; i++)
    {
        const DvzSelection* selection = &scene->selections[i];
        if (selection->card_panel == panel && selection->card.dirty)
            return true;
    }
    for (uint32_t i = 0; i < scene->overlay_card_count; i++)
    {
        const DvzOverlayCard* card = &scene->overlay_cards[i];
        if (card->active && card->panel == panel && card->card.dirty)
            return true;
    }
    for (uint32_t i = 0; i < scene->text_count; i++)
    {
        const DvzText* text = &scene->texts[i];
        if (text->panel == panel && text->dirty_flags != DVZ_TEXT_DIRTY_NONE)
            return true;
    }
    return false;
}



/**
 * Return whether one panel has visible dirty visuals attached.
 *
 * @param panel the panel to inspect
 * @return whether render work is pending for attached visuals
 */
static bool _scene_panel_has_pending_visual_work(const DvzPanel* panel)
{
    if (panel == NULL)
        return false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (_scene_visual_has_pending_render_work(visual))
            return true;
        if (visual != NULL && visual->type == DVZ_VISUAL_TYPE_TEXT)
        {
            const DvzFigure* figure = panel->figure;
            uint64_t version = _visual_family_state(visual)->text.strings_version + _visual_family_state(visual)->text.renderer_version;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                version += visual->attrs[ai].version;
            if (
                _visual_family_state(visual)->text.string_count > 0 && _visual_family_state(visual)->text.strings != NULL &&
                (_visual_family_state(visual)->text.realized_version != version ||
                 (figure != NULL &&
                  (_visual_family_state(visual)->text.visual_figure_width != figure->width ||
                   _visual_family_state(visual)->text.visual_figure_height != figure->height))))
            {
                return true;
            }
        }
    }
    return false;
}



/**
 * Clear dirty scene state after one successful figure emit.
 *
 * @param figure the emitted figure
 */
static void _scene_commit_emit_success(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->scene);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                visual->attrs[ai].dirty_item_count = 0;
                if (visual->attrs[ai].buffer != NULL)
                    visual->attrs[ai].buffer->dirty = false;
            }
            if (_visual_family_state(visual)->buffer != NULL)
                _visual_family_state(visual)->buffer->dirty = false;
            if (
                visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                visual->type == DVZ_VISUAL_TYPE_MARKER ||
                visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
                visual->type == DVZ_VISUAL_TYPE_PATH ||
                visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH ||
                visual->type == DVZ_VISUAL_TYPE_SPHERE)
            {
                int normal_idx = _attr_index(visual, "normal");
                bool has_normals =
                    normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                    visual->attrs[normal_idx].item_count > 0;
                bool point_like = visual->type == DVZ_VISUAL_TYPE_POINT ||
                                  visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                                  visual->type == DVZ_VISUAL_TYPE_MARKER;
                if (point_like || has_normals || visual->type == DVZ_VISUAL_TYPE_SEGMENT ||
                    visual->type == DVZ_VISUAL_TYPE_PATH || visual->type == DVZ_VISUAL_TYPE_SPHERE)
                    _visual_family_state(visual)->material_params_dirty = false;
                if (point_like)
                    _visual_family_state(visual)->item_state_style_params_dirty = false;
            }
            if (
                visual->type == DVZ_VISUAL_TYPE_IMAGE ||
                visual->type == DVZ_VISUAL_TYPE_GLYPH ||
                visual->type == DVZ_VISUAL_TYPE_LABELS ||
                visual->type == DVZ_VISUAL_TYPE_VOLUME ||
                visual->type == DVZ_VISUAL_TYPE_MESH)
            {
                _scene_visual_texture_mark_clean(visual);
            }
            if (visual->type == DVZ_VISUAL_TYPE_VOLUME)
                _visual_family_state(visual)->volume_realized_version = _visual_family_state(visual)->volume.version;
            if (visual->type == DVZ_VISUAL_TYPE_LABELS)
                _visual_family_state(visual)->labels_realized_version = _visual_family_state(visual)->labels.version;
        }
    }
    for (uint32_t ci = 0; ci < figure->compute_count; ci++)
    {
        DvzSceneCompute* compute = figure->computes[ci];
        if (compute == NULL)
            continue;
        for (uint32_t bi = 0; bi < compute->binding_count; bi++)
        {
            if (compute->bindings[bi].active && compute->bindings[bi].buffer != NULL)
                compute->bindings[bi].buffer->dirty = false;
        }
    }
    for (uint32_t i = 0; i < figure->scene->field_count; i++)
        _scene_refresh_field_dirty_state(figure->scene, &figure->scene->fields[i]);
}



/**
 * Return whether a figure has retained scene work waiting for another emitted frame.
 *
 * @param figure the figure to inspect
 * @return whether dirty retained state is pending for this figure
 */
bool _scene_figure_has_pending_render_work(const DvzFigure* figure)
{
    if (figure == NULL)
        return false;

    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        const DvzPanel* panel = &figure->panels[i];
        if (
            _scene_panel_has_pending_visual_work(panel) ||
            _scene_panel_has_pending_adornment_work(panel))
        {
            return true;
        }
    }
    return false;
}



static void _scene_stream_release(void* owner)
{
    DvzScene* scene = (DvzScene*)owner;
    if (scene == NULL)
        return;
    if (scene->outstanding_emitted_streams == 0)
    {
        log_error("scene emitted stream release underflow");
        return;
    }
    scene->outstanding_emitted_streams--;
}



static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream)
{
    ANN(scene);
    ANN(stream);
    if (scene->outstanding_emitted_streams == UINT32_MAX)
    {
        log_error("scene emitted stream count overflow");
        return false;
    }
    scene->outstanding_emitted_streams++;
    stream->owner = scene;
    stream->owner_release = _scene_stream_release;
    stream->owner_released = false;
    return true;
}



static bool _scene_has_live_streams(const DvzScene* scene)
{
    return scene != NULL && scene->outstanding_emitted_streams > 0;
}



bool _scene_visual_mutation_allowed(const DvzScene* scene, const char* action)
{
    ANN(action);
    if (!_scene_has_live_streams(scene))
        return true;
    log_error(
        "cannot %s while an emitted stream is still live; destroy the stream first", action);
    return false;
}


DvzDrp2CommandStream* dvz_figure_emit_ex(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(figure->scene->emitter);
    DvzFramePlanEmitter* emitter = figure->scene->emitter;

    DvzCapabilitySnapshot default_caps;
    DvzDiagnosticReport local_report;
    DvzFramePlanEmitConfig default_cfg;
    _scene_emit_defaults(&caps, &default_caps, &report, &local_report, &cfg, &default_cfg);
    if (!dvz_capability_snapshot_valid(caps))
        return NULL;
    float next_device_scale_x = _scene_scale_or_one(cfg->device_scale_x);
    float next_device_scale_y = _scene_scale_or_one(cfg->device_scale_y);
    float next_render_scale = _scene_scale_or_one(cfg->render_scale);
    float next_user_scale = _scene_scale_or_one(cfg->user_scale);
    bool screen_scale_changed =
        fabsf(figure->device_scale_x - next_device_scale_x) > 1e-6f ||
        fabsf(figure->device_scale_y - next_device_scale_y) > 1e-6f ||
        fabsf(figure->render_scale - next_render_scale) > 1e-6f ||
        fabsf(figure->user_scale - next_user_scale) > 1e-6f;
    figure->device_scale_x = next_device_scale_x;
    figure->device_scale_y = next_device_scale_y;
    figure->render_scale = next_render_scale;
    figure->user_scale = next_user_scale;
    if (screen_scale_changed)
        _scene_figure_mark_screen_space_dirty(figure);

    if (!_scene_figure_resolve_layouts(figure))
    {
        (void)dvz_diagnostic_report_add(report, "scene grid layout resolution failed");
        return NULL;
    }

    char figure_id[64];
    _scene_figure_id(figure, figure_id, sizeof(figure_id));

    if (!_scene_figure_validate_transparency_modes(figure, figure_id, report))
        return NULL;

    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        (void)_scene_panel_sync_fly_pivot_marker(&figure->panels[pi]);
    _scene_prepare_guide_visuals(figure);
    _scene_prepare_bars_visuals(figure);
    _scene_prepare_band_visuals(figure);

    DvzFramePlan* plan = dvz_frame_plan(figure_id, 0);
    if (plan == NULL)
        return NULL;

    _scene_emit_visual_uploads(figure, plan, report);
    if (!_scene_emit_compute_passes(figure, plan, report))
    {
        (void)dvz_diagnostic_report_add(report, "scene compute FramePlan emission failed");
        dvz_frame_plan_destroy(plan);
        return NULL;
    }

    bool panels_ok = true;
    uint32_t graph_report_start = dvz_diagnostic_report_count(report);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        panels_ok = _scene_emit_panel_render_ex(figure, pi, plan, figure_id, report) && panels_ok;
    if (!panels_ok)
    {
        if (dvz_diagnostic_report_count(report) == graph_report_start)
            (void)dvz_diagnostic_report_add(report, "scene FramePlan graph emission failed");
        dvz_frame_plan_destroy(plan);
        return NULL;
    }

    DvzDiagnosticReport contract_report;
    dvz_diagnostic_report_init(&contract_report);
    bool contracts_ok =
        _scene_frame_plan_contracts_validate_ex(figure, plan, caps, &contract_report);
    if (!contracts_ok)
    {
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&contract_report); i++)
        {
            const char* message = dvz_diagnostic_report_get(&contract_report, i);
            if (message != NULL)
            {
                log_error("scene render contract validation failed: %s", message);
                (void)dvz_diagnostic_report_add(report, message);
            }
        }
        dvz_frame_plan_destroy(plan);
        return NULL;
    }

    _scene_report_capability_fallbacks(plan, caps, report);

    _scene_frame_plan_trace(figure, plan);

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, caps, report, cfg);
    if (stream != NULL && !_scene_stream_register(figure->scene, stream))
    {
        dvz_drp2_stream_destroy(stream);
        stream = NULL;
    }

    if (stream != NULL)
        _scene_commit_emit_success(figure);

    dvz_frame_plan_destroy(plan);
    return stream;
}



DvzDrp2CommandStream* dvz_figure_emit(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    return dvz_figure_emit_ex(figure, caps, report, NULL);
}
