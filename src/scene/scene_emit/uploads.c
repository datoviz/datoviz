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
#include "_scene.h"
#include "scene_emit/scene_emit.h"
#include "scene_emit/internal.h"
#include "registry/registry.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit dirty uploads for all panel-visible visuals in one figure.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param report optional diagnostic report
 */
void _scene_emit_visual_uploads(
    DvzFigure* figure, DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    _scene_prepare_composite_visuals(figure);
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
            if (visual->type == DVZ_VISUAL_TYPE_TEXT)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
            bool skip_dense_attrs = false;
            bool finished_visual = false;
            if (!_scene_emit_visual_family_derived_uploads(
                    figure, plan, visual, vidx, &skip_dense_attrs, &finished_visual))
            {
                continue;
            }
            if (finished_visual)
                continue;

            const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual->type);
            bool upload_position_topology = ops != NULL && ops->upload_position_topology;
            bool upload_material_params = ops != NULL && ops->upload_material_params;

            if (!skip_dense_attrs)
                _scene_emit_visual_dense_attr_uploads(
                    figure, plan, visual, vidx, upload_position_topology, emitted_buffers);
            if (upload_material_params)
            {
                if (_scene_visual_needs_material_params(visual) && visual->material_params_dirty)
                {
                    if (!_scene_emit_visual_material_upload(figure, plan, visual, vidx))
                        continue;
                }
            }
            if (visual->buffer != NULL && visual->buffer->data != NULL)
            {
                uint32_t buffer_idx = _scene_buffer_index(figure->scene, visual->buffer);
                if (visual->buffer->dirty && buffer_idx != UINT32_MAX &&
                    !emitted_buffers[buffer_idx])
                {
                    char buffer_resource_id[128];
                    if (!_scene_resource_key_buffer(
                            buffer_idx, buffer_resource_id, sizeof(buffer_resource_id)))
                        continue;
                    dvz_frame_plan_upload_bytes(
                        plan, buffer_resource_id, 0, visual->buffer->desc.byte_size, "index",
                        visual->buffer->data);
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, DVZ_FRAME_PLAN_RESOURCE_ROLE_INDEX,
                        DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx,
                        visual->buffer->desc.stride > 0
                            ? visual->buffer->desc.byte_size / visual->buffer->desc.stride
                            : 0);
                    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                    node->u.upload.buffer_usage =
                        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_INDEX;
                    node->u.upload.item_stride = visual->buffer->desc.stride;
                    emitted_buffers[buffer_idx] = true;
                }
            }
            _scene_emit_visual_family_texture_uploads(figure, plan, visual, vidx);
        }
    }
}
