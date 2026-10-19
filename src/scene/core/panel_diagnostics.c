/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "_log.h"
#include "_compat.h"
#include "datoviz/common/functions.h"
#include "_visual_family.h"
#include "panel_diagnostics.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Mix one integer into a stable diagnostic fingerprint.
 * @param hash current hash value
 * @param value integer to add
 * @return updated hash value
 */
static uint64_t _panel_diag_mix(uint64_t hash, uint64_t value)
{
    hash ^= value;
    hash *= UINT64_C(1099511628211);
    return hash;
}



/**
 * Mix the exact float components of one resolved rectangle.
 * @param hash current hash value
 * @param rect rectangle to add
 * @return updated hash value
 */
static uint64_t _panel_diag_rect(uint64_t hash, const DvzRect* rect)
{
    const float values[4] = {rect->x, rect->y, rect->width, rect->height};
    for (uint32_t i = 0; i < 4; i++)
    {
        uint32_t bits = 0;
        dvz_memcpy(&bits, sizeof(bits), &values[i], sizeof(values[i]));
        hash = _panel_diag_mix(hash, bits);
    }
    return hash;
}



/**
 * Fingerprint the resolved panel, inner, and plot rectangles plus figure scales.
 * @param panel panel to inspect
 * @return layout fingerprint, or zero when the snapshot cannot be resolved
 */
static uint64_t _panel_diag_layout_signature(const DvzPanel* panel)
{
    DvzPanelFrameResolved resolved = {0};
    uint64_t hash = UINT64_C(1469598103934665603);
    if (!_scene_panel_frame_snapshot(panel, &resolved))
        return 0;
    hash = _panel_diag_rect(hash, &resolved.panel_px);
    hash = _panel_diag_rect(hash, &resolved.inner_px);
    hash = _panel_diag_rect(hash, &resolved.plot_px);
    hash = _panel_diag_mix(hash, panel->figure->width);
    hash = _panel_diag_mix(hash, panel->figure->height);
    const float scales[4] = {
        panel->figure->device_scale_x, panel->figure->device_scale_y,
        panel->figure->render_scale, panel->figure->user_scale};
    for (uint32_t i = 0; i < 4; i++)
    {
        uint32_t bits = 0;
        dvz_memcpy(&bits, sizeof(bits), &scales[i], sizeof(scales[i]));
        hash = _panel_diag_mix(hash, bits);
    }
    return hash;
}



/**
 * Fingerprint attached visual identity and panel ordering/visibility properties.
 * @param panel panel to inspect
 * @return membership fingerprint
 */
static uint64_t _panel_diag_membership_signature(const DvzPanel* panel)
{
    uint64_t hash = _panel_diag_mix(UINT64_C(1469598103934665603), panel->visual_count);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        hash = _panel_diag_mix(hash, attach->visual != NULL ? attach->visual->id : 0);
        hash = _panel_diag_mix(hash, attach->visual != NULL && attach->visual->visible);
        hash = _panel_diag_mix(hash, (uint32_t)attach->z_layer);
        hash = _panel_diag_mix(hash, (uint32_t)attach->controller_mode);
        hash = _panel_diag_mix(hash, (uint32_t)attach->coord_space);
        hash = _panel_diag_mix(hash, attach->insertion_index);
    }
    return hash;
}



/**
 * Fingerprint attached visual data, material, texture, transform, and item-state revisions.
 * @param panel panel to inspect
 * @return visual fingerprint
 */
static uint64_t _panel_diag_visual_signature(const DvzPanel* panel)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL)
            continue;
        hash = _panel_diag_mix(hash, _visual_family_diagnostic_signature(visual));
    }
    return hash;
}



/**
 * Fingerprint panel-owned axis, colorbar, and legend revisions.
 * @param panel panel to inspect
 * @return adornment fingerprint
 */
static uint64_t _panel_diag_adornment_signature(const DvzPanel* panel)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (uint32_t i = 0; i < 2; i++)
    {
        hash = _panel_diag_mix(hash, panel->axes[i].version);
        hash = _panel_diag_mix(hash, panel->axes[i].enabled);
    }
    for (uint32_t i = 0; i < panel->colorbar_count; i++)
        if (panel->colorbars[i] != NULL)
            hash = _panel_diag_mix(hash, panel->colorbars[i]->version);
    for (uint32_t i = 0; i < panel->legend_count; i++)
        if (panel->legends[i] != NULL)
            hash = _panel_diag_mix(hash, panel->legends[i]->version);
    return hash;
}



/**
 * Return the revision of the currently active panel view.
 * @param panel panel to inspect
 * @return active view revision, or zero when no view is active
 */
static uint64_t _panel_diag_view_revision(const DvzPanel* panel)
{
    if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_2D)
        return panel->view2d_revision;
    if (panel->active_view_kind == DVZ_PANEL_VIEW_KIND_3D)
        return panel->view3d_revision;
    return 0;
}



/**
 * Fingerprint the effective panel model, view, and projection matrices.
 * @param panel panel whose effective transform is resolved
 * @return matrix fingerprint
 */
static uint64_t _panel_diag_mvp_signature(const DvzPanel* panel)
{
    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    uint64_t hash = UINT64_C(1469598103934665603);
    const mat4* matrices[] = {&mvp.model, &mvp.view, &mvp.proj};
    for (uint32_t matrix = 0; matrix < 3; matrix++)
    {
        for (uint32_t column = 0; column < 4; column++)
        {
            for (uint32_t row = 0; row < 4; row++)
            {
                uint32_t bits = 0;
                const float value = (*matrices[matrix])[column][row];
                dvz_memcpy(&bits, sizeof(bits), &value, sizeof(value));
                hash = _panel_diag_mix(hash, bits);
            }
        }
    }
    return hash;
}



/**
 * Compare observed panel facts and report unknown frame-only changes explicitly.
 * @param previous previous diagnostic sample
 * @param frame_revision current figure frame revision
 * @param view_revision current active view revision
 * @param mvp_signature current effective model/view/projection fingerprint
 * @param layout_signature current layout fingerprint
 * @param membership_signature current membership fingerprint
 * @param visual_signature current visual fingerprint
 * @param adornment_signature current adornment fingerprint
 * @param light_revision current panel light revision
 * @return combined change bits
 */
static uint32_t _panel_diag_changes(
    const DvzScenePanelDiagnostic* previous, uint64_t frame_revision, uint64_t view_revision,
    uint64_t mvp_signature, uint64_t layout_signature, uint64_t membership_signature,
    uint64_t visual_signature, uint64_t adornment_signature, uint64_t light_revision)
{
    if (!previous->has_previous)
        return 0;
    uint32_t changes = 0;
    if (previous->observed_view_revision != view_revision ||
        previous->observed_mvp_signature != mvp_signature)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW;
    if (previous->observed_layout_signature != layout_signature)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_LAYOUT;
    if (previous->observed_membership_signature != membership_signature)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_MEMBERSHIP;
    if (previous->observed_visual_signature != visual_signature)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_VISUAL;
    if (previous->observed_adornment_signature != adornment_signature)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_ADORNMENT;
    if (previous->observed_light_revision != light_revision)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_LIGHTS;
    if (previous->observed_frame_revision != frame_revision && changes == 0)
        changes |= DVZ_PANEL_DIAGNOSTIC_CHANGE_UNKNOWN_FRAME;
    return changes;
}



/**
 * Return a printable name for one diagnostic change bit.
 * @param bit one diagnostic change bit
 * @return printable name, or NULL for an unknown bit
 */
static const char* _panel_diag_change_name(uint32_t bit)
{
    switch (bit)
    {
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_VIEW:
        return "view";
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_LAYOUT:
        return "layout";
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_MEMBERSHIP:
        return "membership";
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_VISUAL:
        return "visual";
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_ADORNMENT:
        return "adornment";
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_LIGHTS:
        return "lights";
    case DVZ_PANEL_DIAGNOSTIC_CHANGE_UNKNOWN_FRAME:
        return "unknown-frame-change";
    default:
        return NULL;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Check whether panel diagnostics were requested through the process environment.
 *
 * @return whether DVZ_SCENE_PANEL_DIAGNOSTICS is set to a nonzero value
 */
bool _scene_panel_diagnostics_env_enabled(void)
{
    const char* value = getenv("DVZ_SCENE_PANEL_DIAGNOSTICS");
    return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
}



/**
 * Start one diagnostic generation for a figure emit attempt.
 *
 * @param figure figure being emitted
 */
void _scene_panel_diagnostics_begin(DvzFigure* figure)
{
    if (figure == NULL || !figure->panel_diagnostics_enabled)
        return;
    figure->panel_diagnostics_generation++;
    if (figure->panel_diagnostics_generation == 0)
        figure->panel_diagnostics_generation = 1;
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        DvzScenePanelDiagnostic* diag = &figure->panel_diagnostics[i];
        figure->panels[i].composition_ns = 0;
        diag->generation = figure->panel_diagnostics_generation;
        diag->elapsed_ns = 0;
        diag->composition_ns = 0;
        diag->planned_node_count = 0;
        diag->render_node_count = 0;
        diag->draw_count = 0;
        diag->upload_count = 0;
        diag->uniform_upload_count = 0;
        diag->resource_upload_count = 0;
        diag->prelude_upload_count = 0;
        diag->prelude_uniform_upload_count = 0;
        diag->prelude_resource_upload_count = 0;
        diag->change_mask = 0;
        diag->panel_planning_succeeded = false;
        diag->stream_emitted = false;
    }
}



/**
 * Record planned work and observed mutations for one panel.
 *
 * @param figure figure being emitted
 * @param panel_index panel index in the figure
 * @param plan current plan containing the newly appended panel nodes
 * @param first_node first node appended by this panel's planner
 * @param elapsed_ns time spent planning this panel
 * @param planning_succeeded whether panel planning completed
 */
void _scene_panel_diagnostics_record(
    DvzFigure* figure, uint32_t panel_index, const DvzFramePlan* plan, uint32_t first_node,
    uint64_t elapsed_ns, bool planning_succeeded)
{
    if (figure == NULL || !figure->panel_diagnostics_enabled || panel_index >= figure->panel_count)
        return;
    DvzPanel* panel = &figure->panels[panel_index];
    DvzScenePanelDiagnostic* diag = &figure->panel_diagnostics[panel_index];
    uint64_t frame_revision = figure->frame_revision;
    uint64_t query_revision = figure->query_input_revision;
    uint64_t view_revision = _panel_diag_view_revision(panel);
    uint64_t mvp_signature = _panel_diag_mvp_signature(panel);
    uint64_t layout_signature = _panel_diag_layout_signature(panel);
    uint64_t membership_signature = _panel_diag_membership_signature(panel);
    uint64_t visual_signature = _panel_diag_visual_signature(panel);
    uint64_t adornment_signature = _panel_diag_adornment_signature(panel);
    uint64_t light_revision = panel->lights.revision;

    diag->elapsed_ns = elapsed_ns;
    diag->composition_ns = panel->composition_ns;
    diag->planned_node_count = plan != NULL && plan->count >= first_node
                                   ? (uint64_t)(plan->count - first_node)
                                   : 0;
    diag->panel_planning_succeeded = planning_succeeded;
    diag->change_mask = _panel_diag_changes(
        diag, frame_revision, view_revision, mvp_signature, layout_signature, membership_signature,
        visual_signature, adornment_signature, light_revision);
    if (plan != NULL && first_node <= plan->count)
    {
        for (uint32_t i = first_node; i < plan->count; i++)
        {
            const DvzFramePlanNode* node = &plan->nodes[i];
            if (node->type == DVZ_FRAME_PLAN_NODE_RENDER)
            {
                diag->render_node_count++;
                diag->draw_count += node->u.render.visual_count;
            }
            else if (node->type == DVZ_FRAME_PLAN_NODE_UPLOAD)
            {
                diag->upload_count++;
                if ((node->u.upload.buffer_usage & DVZ_DRP2_BUFFER_USAGE_UNIFORM) != 0)
                    diag->uniform_upload_count++;
                else
                    diag->resource_upload_count++;
            }
        }
    }
    if (panel_index == 0 && plan != NULL && first_node <= plan->count)
    {
        uint64_t prelude_uploads = 0;
        uint64_t prelude_uniform_uploads = 0;
        uint64_t prelude_resource_uploads = 0;
        for (uint32_t i = 0; i < first_node; i++)
        {
            const DvzFramePlanNode* node = &plan->nodes[i];
            if (node->type != DVZ_FRAME_PLAN_NODE_UPLOAD)
                continue;
            prelude_uploads++;
            if ((node->u.upload.buffer_usage & DVZ_DRP2_BUFFER_USAGE_UNIFORM) != 0)
                prelude_uniform_uploads++;
            else
                prelude_resource_uploads++;
        }
        for (uint32_t i = 0; i < figure->panel_count; i++)
        {
            DvzScenePanelDiagnostic* panel_diag = &figure->panel_diagnostics[i];
            panel_diag->prelude_upload_count = prelude_uploads;
            panel_diag->prelude_uniform_upload_count = prelude_uniform_uploads;
            panel_diag->prelude_resource_upload_count = prelude_resource_uploads;
        }
    }

    diag->observed_frame_revision = frame_revision;
    diag->observed_query_revision = query_revision;
    diag->observed_view_revision = view_revision;
    diag->observed_mvp_signature = mvp_signature;
    diag->observed_layout_signature = layout_signature;
    diag->observed_membership_signature = membership_signature;
    diag->observed_visual_signature = visual_signature;
    diag->observed_adornment_signature = adornment_signature;
    diag->observed_light_revision = light_revision;
    diag->has_previous = true;
}



/**
 * Record stream emission outcome and print one line per attempted panel.
 *
 * @param figure figure whose emission has completed
 * @param stream_emitted whether DRP2 stream generation succeeded
 */
void _scene_panel_diagnostics_finish(DvzFigure* figure, bool stream_emitted)
{
    if (figure == NULL || !figure->panel_diagnostics_enabled)
        return;
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        const DvzScenePanelDiagnostic* diag = &figure->panel_diagnostics[i];
        if (diag->generation != figure->panel_diagnostics_generation)
            continue;
        char changes[128] = {0};
        size_t offset = 0;
        for (uint32_t bit = 1; bit <= DVZ_PANEL_DIAGNOSTIC_CHANGE_UNKNOWN_FRAME; bit <<= 1)
        {
            if ((diag->change_mask & bit) == 0)
                continue;
            const char* name = _panel_diag_change_name(bit);
            if (name == NULL)
                continue;
            int n = dvz_snprintf(
                &changes[offset], sizeof(changes) - offset, "%s%s", offset == 0 ? "" : ",", name);
            if (n <= 0 || (size_t)n >= sizeof(changes) - offset)
                break;
            offset += (size_t)n;
        }
        if (changes[0] == '\0')
            dvz_snprintf(changes, sizeof(changes), "%s", "none-or-first-sample");
        log_info(
            "scene_panel_diag figure=%" PRIu64 " panel=%" PRIu64 " generation=%" PRIu64
            " plan=%s stream=%s gpu=not-submitted planned_nodes=%" PRIu64
            " render_nodes=%" PRIu64
            " draws=%" PRIu64 " panel_plan_upload_nodes=%" PRIu64
            " panel_uniform_upload_nodes=%" PRIu64 " panel_resource_upload_nodes=%" PRIu64
            " prelude_upload_nodes=%" PRIu64 " prelude_uniform_upload_nodes=%" PRIu64
            " prelude_resource_upload_nodes=%" PRIu64
            " frame_rev=%" PRIu64 " query_rev=%" PRIu64
            " view_rev=%" PRIu64 " mvp_sig=%" PRIu64 " plan_ns=%" PRIu64
            " composition_ns=%" PRIu64
            " upload_scope=explicit_plan_nodes_implicit_common_bindings_excluded changes=%s",
            figure->id, figure->panels[i].id, diag->generation,
            diag->panel_planning_succeeded ? "ok" : "failed",
            stream_emitted ? "emitted" : "failed", diag->planned_node_count,
            diag->render_node_count, diag->draw_count, diag->upload_count,
            diag->uniform_upload_count, diag->resource_upload_count,
            diag->prelude_upload_count, diag->prelude_uniform_upload_count,
            diag->prelude_resource_upload_count,
            diag->observed_frame_revision, diag->observed_query_revision,
            diag->observed_view_revision, diag->observed_mvp_signature, diag->elapsed_ns,
            diag->composition_ns, changes);
        figure->panel_diagnostics[i].stream_emitted = stream_emitted;
    }
}
