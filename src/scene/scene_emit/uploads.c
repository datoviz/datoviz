/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lowering upload emission                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "annotation/prepare_internal.h"
#include "core/orientation_gizmo_internal.h"
#include "domain/buffer_internal.h"
#include "domain/polygon_internal.h"
#include "registry/registry.h"
#include "scene_emit/internal.h"
#include "scene_emit/scene_emit.h"
#include "visuals/bounds_internal.h"
#include "_visual_pipeline_internal.h"


/** Emit copied typed indices transactionally rather than treating them as optional uploads.
 * @param figure the consumer
 * @param plan destination plan
 * @param visual the bound visual
 * @param visual_index scene visual index
 * @return whether the required mapping upload was appended
 */
static bool _emit_typed_attr(DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index,
    const char* name, uint32_t stride)
{
    int idx = _attr_index(visual, name);
    if (idx < 0)
        return false;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    uint32_t slot = strcmp(name, "fragment_clip") == 0 ? 1 : 0;
    if (figure->typed_attr_visual_ids[visual_index] == visual->id &&
        figure->typed_attr_consumed[visual_index][slot] == attr->version)
        return true;
    char key[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_visual_attr_resource_key(
            figure, visual, visual_index, name, key, sizeof(key)))
        return false;
    uint64_t offset = 0;
    uint64_t bytes = (uint64_t)attr->item_count * stride;
    if (!dvz_frame_plan_upload_bytes(
            plan, key, offset, bytes, name, (const uint8_t*)attr->data + offset))
        return false;
    _scene_attach_upload_metadata(
        plan, visual, visual_index, DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE,
        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, UINT32_MAX, attr->item_count);
    plan->nodes[plan->count - 1].u.upload.item_stride = stride;
    return true;
}



/** Emit a bank resource using retained per-slot and per-figure revisions.
 * @param figure the consumer
 * @param plan destination plan
 * @param bank the typed bank
 * @param buffer words or style buffer
 * @param emitted within-frame emission guards
 * @return whether all required uploads were appended
 */
static bool _emit_bank_resource(
    DvzFigure* figure, DvzFramePlan* plan, DvzItemStateBuffer* bank, DvzSceneBuffer* buffer,
    bool* emitted)
{
    uint32_t idx = _scene_buffer_index(figure->scene, buffer);
    if (idx == UINT32_MAX || emitted[idx])
        return idx != UINT32_MAX;
    emitted[idx] = true;
    uint64_t consumed =
        figure->item_state_consumed_ids[idx] == buffer->id ? figure->item_state_consumed[idx] : 0;
    if (consumed >= buffer->content_revision)
        return true;
    char key[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_resource_key_buffer(buffer->id, key, sizeof(key)))
        return false;
    bool words = buffer == bank->words;
    uint32_t count = words ? bank->slot_count : 1;
    uint32_t stride = buffer->desc.stride;
    for (uint32_t first = 0; first < count;)
    {
        if (words && consumed > 0 && bank->slot_revisions[first] <= consumed)
        {
            first++;
            continue;
        }
        uint32_t end = first + 1;
        while (end < count && (!words || consumed == 0 || bank->slot_revisions[end] > consumed))
            end++;
        uint64_t offset = (uint64_t)first * stride;
        if (!dvz_frame_plan_upload_bytes(
                plan, key, offset, (uint64_t)(end - first) * stride,
                words ? "item_state_words" : "item_state_style",
                (const uint8_t*)buffer->data + offset))
            return false;
        DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
        node->u.upload.buffer_usage = _scene_buffer_drp2_usage(buffer->desc.usage);
        node->u.upload.item_stride = stride;
        node->u.upload.metadata.has_metadata = true;
        node->u.upload.metadata.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
        node->u.upload.metadata.role = words ? DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE
                                             : DVZ_FRAME_PLAN_RESOURCE_ROLE_ITEM_STATE_STYLE;
        node->u.upload.metadata.has_logical_extent = true;
        node->u.upload.metadata.logical_item_count = count;
        first = end;
    }
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit dirty uploads for all panel-visible visuals in one figure.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param report optional diagnostic report
 * @return whether required upload preparation succeeded
 */
bool _scene_emit_visual_uploads(
    DvzFigure* figure, DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    _scene_prepare_composite_visuals(figure);
    _scene_prepare_orientation_gizmos(figure);
    _scene_prepare_axis_visuals(figure);
    _scene_prepare_colorbar_visuals(figure, report);
    _scene_prepare_legend_visuals(figure, report);
    _scene_prepare_text_visuals(figure);
    _scene_prepare_bounds_visuals(figure);
    bool emitted_buffers[DVZ_SCENE_MAX_BUFFERS] = {0};
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL || !visual->visible)
                continue;
            const DvzVisualFamilyOps* ops = visual->ops;
            if (ops != NULL && ops->skip_visual_uploads)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
            DvzItemStateBuffer* bank = _visual_family_state(visual)->item_state_bank;
            bool clipped = _scene_visual_has_dense_attr(visual, "fragment_clip");
            bool skip_dense_attrs = false;
            bool finished_visual = false;
            if (!_scene_emit_visual_family_derived_uploads(
                    figure, plan, visual, vidx, &skip_dense_attrs, &finished_visual))
            {
                if (bank != NULL || clipped)
                    return false;
                continue;
            }
            if (finished_visual)
                continue;

            bool upload_position_topology = ops != NULL && ops->upload_position_topology;
            bool upload_material_params = ops != NULL && ops->upload_material_params;

            if (!skip_dense_attrs)
                _scene_emit_visual_dense_attr_uploads(
                    figure, plan, visual, vidx, upload_position_topology, emitted_buffers);
            if (!_scene_emit_visual_material_upload_if_needed(
                    figure, plan, visual, vidx, upload_material_params))
            {
                if (bank != NULL || clipped)
                    return false;
                continue;
            }
            if (bank == NULL && !clipped &&
                !_scene_emit_visual_item_state_style_upload(figure, plan, visual, vidx))
                continue;
            if (clipped &&
                !_emit_typed_attr(figure, plan, visual, vidx, "fragment_clip", sizeof(vec2)))
                return false;
            if (bank != NULL)
            {
                if (!_emit_typed_attr(figure, plan, visual, vidx, "state_index", sizeof(uint32_t)) ||
                    !_emit_bank_resource(figure, plan, bank, bank->words, emitted_buffers) ||
                    !_emit_bank_resource(figure, plan, bank, bank->style, emitted_buffers))
                {
                    (void)dvz_diagnostic_report_add(
                        report, "indexed state upload emission failed");
                    return false;
                }
            }
            _scene_emit_visual_index_buffer_upload(figure, plan, visual, vidx, emitted_buffers);
            if (!_scene_emit_visual_family_texture_uploads(figure, plan, visual, vidx))
                return false;
        }
    }
    return true;
}
