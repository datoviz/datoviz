/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph — DvzScene / DvzFigure / DvzPanel / DvzVisual                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_controllers.h"
#include "_frame_plan.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene_emit.h"
#include "render_contract.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "../../drp2/_stream.h"
#include "_scene.h"
#include "_technique.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _scene_stream_release(void* owner);

static bool _scene_visual_has_attr_data(const DvzVisual* visual, const char* attr_name);


static bool _scene_stream_register(DvzScene* scene, DvzDrp2CommandStream* stream);

static bool _scene_has_live_streams(const DvzScene* scene);

static void _scene_emit_defaults(
    const DvzCapabilitySnapshot** caps, DvzCapabilitySnapshot* default_caps,
    DvzDiagnosticReport** report, DvzDiagnosticReport* local_report,
    const DvzFramePlanEmitConfig** cfg, DvzFramePlanEmitConfig* default_cfg);

static bool _scene_figure_validate_transparency_modes(
    const DvzFigure* figure, const char* figure_id, DvzDiagnosticReport* report);

static void _scene_report_capability_fallbacks(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report);

static void _scene_commit_emit_success(DvzFigure* figure);

static bool _scene_visual_has_pending_render_work(const DvzVisual* visual);

static bool _scene_panel_has_pending_adornment_work(const DvzPanel* panel);

static bool _scene_panel_has_pending_visual_work(const DvzPanel* panel);

static bool _scene_visual_dirty_material_emits_upload(const DvzVisual* visual);

/**
 * Allocate the next monotonically increasing request freshness serial.
 *
 * Serial 0 stays reserved as the "no freshness tracking" sentinel used by legacy synthetic tests.
 *
 * @param scene the owning scene
 * @return the next non-zero request freshness serial
 */
uint64_t _scene_next_request_serial(DvzScene* scene)
{
    ANN(scene);
    scene->next_request_serial++;
    if (scene->next_request_serial == 0)
        scene->next_request_serial++;
    return scene->next_request_serial;
}



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
        dvz_capability_snapshot_default(default_caps);
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

    if (_scene_visual_dirty_material_emits_upload(visual) || visual->texture.dirty)
        return true;
    if (
        visual->type == DVZ_VISUAL_TYPE_VOLUME &&
        visual->volume_realized_version != visual->volume.version)
    {
        return true;
    }
    if (
        visual->type == DVZ_VISUAL_TYPE_LABELS &&
        visual->labels_realized_version != visual->labels.version)
    {
        return true;
    }
    if (visual->field != NULL && visual->field->dirty)
        return true;
    if (visual->buffer != NULL && visual->buffer->dirty)
        return true;
    if (visual->segment.gpu.dirty || visual->path.gpu.dirty || visual->image_gpu.dirty)
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
    if (visual == NULL || !visual->material_params_dirty)
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
            uint64_t version = visual->text.strings_version + visual->text.renderer_version;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                version += visual->attrs[ai].version;
            if (
                visual->text.string_count > 0 && visual->text.strings != NULL &&
                (visual->text.realized_version != version ||
                 (figure != NULL &&
                  (visual->text.visual_figure_width != figure->width ||
                   visual->text.visual_figure_height != figure->height))))
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
            if (visual->buffer != NULL)
                visual->buffer->dirty = false;
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
                    visual->material_params_dirty = false;
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
                visual->volume_realized_version = visual->volume.version;
            if (visual->type == DVZ_VISUAL_TYPE_LABELS)
                visual->labels_realized_version = visual->labels.version;
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


/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)dvz_calloc(1, sizeof(DvzScene));
    if (scene == NULL)
        return NULL;
    dvz_capability_snapshot_default(&scene->caps);
    _scene_technique_state_init(&scene->techniques);
    scene->font_defaults = dvz_font_defaults();
    scene->clock.mode = DVZ_CLOCK_REALTIME;
    scene->clock.fps = 60.0;
    _scene_request_executor_init(&scene->query_executor);
    scene->emitter = dvz_frame_plan_emitter();
    if (scene->emitter == NULL)
    {
        dvz_free(scene);
        return NULL;
    }
    return scene;
}


/**
 * Set the scene font defaults used by text objects without an explicit font.
 *
 * @param scene the scene
 * @param defaults font defaults, or NULL for built-in defaults
 */
void dvz_scene_set_font_defaults(DvzScene* scene, const DvzFontDefaults* defaults)
{
    ANN(scene);
    scene->font_defaults = defaults != NULL ? *defaults : dvz_font_defaults();
}


/**
 * Return the scene font defaults.
 *
 * @param scene the scene
 * @return scene font defaults
 */
DvzFontDefaults dvz_scene_font_defaults(const DvzScene* scene)
{
    if (scene == NULL)
        return dvz_font_defaults();
    return scene->font_defaults;
}


void dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps)
{
    ANN(scene);
    ANN(caps);
    dvz_capability_snapshot_copy(&scene->caps, caps);
}


void dvz_scene_destroy(DvzScene* scene)
{
    if (scene == NULL)
        return;
    if (!_scene_visual_mutation_allowed(scene, "destroy scene-owned visual data"))
        return;
    for (uint32_t i = 0; i < scene->figure_count; i++)
        _scene_figure_frame_plan_trace_reset(&scene->figures[i]);
    for (uint32_t i = 0; i < scene->composite_count; i++)
        _scene_composite_reset(&scene->composites[i]);
    for (uint32_t i = 0; i < scene->polygon_set_count; i++)
        _scene_polygon_set_reset(&scene->polygon_sets[i]);
    for (uint32_t i = 0; i < scene->polygon_count; i++)
        _scene_polygon_reset(&scene->polygons[i]);
    for (uint32_t i = 0; i < scene->visual_count; i++)
        _scene_visual_reset(&scene->visuals[i], true);
    for (uint32_t i = 0; i < scene->scale_count; i++)
        dvz_scale_destroy(&scene->scales[i]);
    for (uint32_t i = 0; i < scene->font_count; i++)
        _scene_font_release(&scene->fonts[i]);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
        _scene_field_reset(&scene->fields[i]);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_BUFFERS; i++)
        _scene_buffer_reset(&scene->buffers[i]);
    for (uint32_t i = 0; i < scene->selection_count; i++)
        scene->selections[i].scene = NULL;
    for (uint32_t i = 0; i < scene->interaction_count; i++)
        scene->interactions[i].scene = NULL;
    for (uint32_t i = 0; i < scene->link_channel_count; i++)
        scene->link_channels[i].scene = NULL;
    for (uint32_t i = 0; i < scene->pinned_readout_count; i++)
        scene->pinned_readouts[i].scene = NULL;
    for (uint32_t i = 0; i < scene->overlay_count; i++)
        scene->overlays[i].scene = NULL;
    for (uint32_t i = 0; i < scene->overlay_card_count; i++)
        scene->overlay_cards[i].scene = NULL;
    for (uint32_t i = 0; i < scene->controller_count; i++)
        _scene_controller_destroy(&scene->controllers[i]);
    _scene_request_executor_destroy(&scene->query_executor);
    if (scene->emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(scene->emitter);
        scene->emitter = NULL;
    }
    dvz_free(scene);
}



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t flags)
{
    ANN(scene);
    if (scene->figure_count >= DVZ_SCENE_MAX_FIGURES)
        return NULL;
    DvzFigure* fig = &scene->figures[scene->figure_count++];
    fig->scene  = scene;
    fig->width  = width;
    fig->height = height;
    fig->flags  = flags;
    fig->device_scale_x = 1.0f;
    fig->device_scale_y = 1.0f;
    fig->render_scale = 1.0f;
    fig->user_scale = 1.0f;
    return fig;
}




/**
 * Return the scene that owns a figure.
 *
 * @param figure the figure
 * @return the owning scene, or NULL
 */
DvzScene* dvz_figure_scene(DvzFigure* figure)
{
    if (figure == NULL)
        return NULL;
    return figure->scene;
}




/**
 * Update a figure logical size.
 *
 * @param figure the figure
 * @param width width in logical pixels
 * @param height height in logical pixels
 */
void dvz_figure_resize(DvzFigure* figure, uint32_t width, uint32_t height)
{
    ANN(figure);
    if (figure->width == width && figure->height == height)
        return;
    figure->width = width;
    figure->height = height;
    (void)_scene_figure_resolve_layouts(figure);
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        DvzPanel* panel = &figure->panels[i];
        if (panel->camera != NULL)
        {
            float panel_width = 0.0f;
            float panel_height = 0.0f;
            _scene_panel_pixel_size(panel, &panel_width, &panel_height);
            dvz_camera_resize(panel->camera, panel_width, panel_height);
        }
    }
    _scene_notify_request_frame(figure);
}



/**
 * Return a figure logical size.
 *
 * @param figure the figure
 * @param out_width output width in logical pixels, may be NULL
 * @param out_height output height in logical pixels, may be NULL
 */
void dvz_figure_size(const DvzFigure* figure, uint32_t* out_width, uint32_t* out_height)
{
    ANN(figure);
    if (out_width != NULL)
        *out_width = figure->width;
    if (out_height != NULL)
        *out_height = figure->height;
}




void dvz_figure_destroy(DvzFigure* figure)
{
    if (figure == NULL)
        return;
    _scene_figure_frame_plan_trace_reset(figure);
    /* Mark slot as empty */
    figure->scene = NULL;
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

    DvzFramePlan* plan = dvz_frame_plan(figure_id, 0);
    if (plan == NULL)
        return NULL;

    _scene_emit_visual_uploads(figure, plan, report);

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


/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 * Return default panel MSAA options.
 *
 * @return MSAA descriptor with 4x samples and alpha-to-coverage enabled
 */
DvzMsaaDesc dvz_msaa_desc(void)
{
    return (DvzMsaaDesc){
        .enabled = true,
        .sample_count = 4,
        .alpha_to_coverage = true,
    };
}


DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc)
{
    ANN(figure);
    if (!_panel_desc_valid(desc))
        return NULL;
    if (figure->panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    DvzPanel* panel       = &figure->panels[figure->panel_count++];
    panel->figure         = figure;
    panel->desc           = desc;
    panel->grid           = NULL;
    panel->grid_cell      = (DvzGridCell){0};
    panel->base_reserve   = (DvzPanelReserve){0};
    panel->axis_reserve   = (DvzPanelReserve){0};
    panel->colorbar_reserve = (DvzPanelReserve){0};
    panel->legend_reserve = (DvzPanelReserve){0};
    panel->reserve        = (DvzPanelReserve){0};
    panel->padding        = (DvzPanelReserve){0};
    _scene_technique_state_init(&panel->techniques);
    panel->visual_count = 0;
    panel->bounds_visual = NULL;
    panel->bounds_occluded_visual = NULL;
    panel->bounds_visible = false;
    return panel;
}



/**
 * Update a panel rectangle in normalized figure coordinates.
 *
 * @param panel the panel
 * @param desc panel position and size in normalized [0, 1] figure coordinates
 * @return whether the descriptor was accepted
 */
bool dvz_panel_set_desc(DvzPanel* panel, DvzPanelDesc desc)
{
    if (panel == NULL)
        return false;
    if (!_panel_desc_valid(desc))
        return false;
    panel->grid = NULL;
    panel->grid_cell = (DvzGridCell){0};
    return _scene_panel_set_desc_internal(panel, desc);
}



/**
 * Create a panel that fills the whole figure.
 *
 * @param figure the figure
 * @return the panel
 */
DvzPanel* dvz_panel_full(DvzFigure* figure)
{
    return dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
}



/**
 * Configure Eye-Dome Lighting for one panel.
 *
 * @param panel the panel
 * @param desc EDL descriptor, or NULL to disable
 * @return whether the panel EDL state was updated
 */
bool dvz_panel_set_edl(DvzPanel* panel, const DvzEdlDesc* desc)
{
    ANN(panel);
    bool ok = _scene_technique_state_set_edl(&panel->techniques, desc);
    if (ok)
        _scene_notify_request_frame(panel->figure);
    return ok;
}


/**
 * Configure internal multisample antialiasing for one panel.
 *
 * @param panel the panel
 * @param desc MSAA descriptor, or NULL to disable
 * @return whether the panel MSAA state was updated
 */
bool dvz_panel_set_msaa(DvzPanel* panel, const DvzMsaaDesc* desc)
{
    ANN(panel);
    bool ok = _scene_technique_state_set_msaa(&panel->techniques, desc);
    if (ok)
        _scene_notify_request_frame(panel->figure);
    return ok;
}


/**
 * Configure screen-space ambient occlusion for one panel.
 *
 * @param panel the panel
 * @param desc SSAO descriptor, or NULL to disable
 * @return whether the panel SSAO state was updated
 */
bool dvz_panel_set_ssao(DvzPanel* panel, const DvzSsaoDesc* desc)
{
    ANN(panel);
    bool ok = _scene_technique_state_set_ssao(&panel->techniques, desc);
    if (ok)
        _scene_notify_request_frame(panel->figure);
    return ok;
}


/**
 * Configure generic screen-space scene occlusion for one panel.
 *
 * @param panel the panel
 * @param desc scene occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_scene_occlusion(DvzPanel* panel, const DvzSceneOcclusionDesc* desc)
{
    ANN(panel);
    if (desc == NULL || !desc->enabled)
    {
        panel->scene_occlusion_enabled = false;
        dvz_memset(
            &panel->scene_occlusion, sizeof(DvzSceneOcclusionDesc), 0,
            sizeof(DvzSceneOcclusionDesc));
        _scene_notify_request_frame(panel->figure);
        return 0;
    }

    panel->scene_occlusion = *desc;
    panel->scene_occlusion.enabled = true;
    if (panel->scene_occlusion.soft_edge <= 0.0f)
        panel->scene_occlusion.soft_edge = 0.002f;
    if (panel->scene_occlusion.hidden_alpha < 0.0f)
        panel->scene_occlusion.hidden_alpha = 0.0f;
    if (panel->scene_occlusion.hidden_alpha > 1.0f)
        panel->scene_occlusion.hidden_alpha = 1.0f;
    panel->scene_occlusion_enabled = true;
    _scene_notify_request_frame(panel->figure);
    return 0;
}


/**
 * Configure a panel volume visual as the screen-space occluder for embedded visuals.
 *
 * @param panel the panel
 * @param volume the volume visual attached to the same panel, or NULL to disable
 * @param desc volume occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_volume_occluder(
    DvzPanel* panel, DvzVisual* volume, const DvzVolumeOcclusionDesc* desc)
{
    ANN(panel);
    if (volume == NULL || desc == NULL || !desc->enabled)
    {
        panel->volume_occluder_visual = NULL;
        panel->volume_occlusion_enabled = false;
        dvz_memset(
            &panel->volume_occlusion, sizeof(DvzVolumeOcclusionDesc), 0,
            sizeof(DvzVolumeOcclusionDesc));
        _scene_notify_request_frame(panel->figure);
        return 0;
    }
    if (volume->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_panel_set_volume_occluder requires a volume visual");
        return -1;
    }

    bool attached = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
        attached = attached || panel->visuals[i].visual == volume;
    if (!attached)
    {
        log_error("volume occluder must be attached to the panel");
        return -1;
    }

    panel->volume_occluder_visual = volume;
    panel->volume_occlusion = *desc;
    panel->volume_occlusion.enabled = true;
    if (panel->volume_occlusion.alpha_threshold <= 0.0f)
        panel->volume_occlusion.alpha_threshold = 0.08f;
    if (panel->volume_occlusion.fade_distance <= 0.0f)
        panel->volume_occlusion.fade_distance = 0.08f;
    if (panel->volume_occlusion.occluded_alpha <= 0.0f)
        panel->volume_occlusion.occluded_alpha = 0.20f;
    panel->volume_occlusion_enabled = true;
    _scene_notify_request_frame(panel->figure);
    return 0;
}



void dvz_panel_destroy(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    (void)dvz_panel_connect_input(panel, NULL);
    panel->panzoom = NULL;
    panel->arcball = NULL;
    panel->fly = NULL;
    for (uint32_t i = 0; i < 3; i++)
        panel->controllers[i] = NULL;
    panel->turntable = NULL;
    panel->grid = NULL;
    panel->grid_cell = (DvzGridCell){0};
    if (panel->camera != NULL)
    {
        dvz_camera_destroy(panel->camera);
        panel->camera = NULL;
    }
    panel->figure       = NULL;
    panel->visual_count = 0;
    panel->bounds_visual = NULL;
    panel->bounds_occluded_visual = NULL;
    panel->bounds_visible = false;
    panel->colorbar_count = 0;
    panel->legend_count = 0;
    panel->interaction = NULL;
    panel->pinned_readout_count = 0;
    dvz_memset(&panel->hover, sizeof(DvzHoverState), 0, sizeof(DvzHoverState));
}
