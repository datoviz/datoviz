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

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "_overflow.h"
#include "_scene.h"
#include "scene_emit/scene_emit.h"
#include "scene_emit/internal.h"
#include "_scene_resource_key.h"
#include "_visual_pipeline.h"
#include "_visual_internal.h"
#include "datoviz/drp2/runtime.h"


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

            if (!skip_dense_attrs)
            {
                for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                {
                    DvzVisualAttr* attr = &visual->attrs[ai];
                    if (attr->buffer != NULL)
                    {
                        uint32_t buffer_idx = _scene_buffer_index(figure->scene, attr->buffer);
                        if (buffer_idx == UINT32_MAX || emitted_buffers[buffer_idx])
                            continue;
                        char buffer_resource_id[128];
                        if (!_scene_resource_key_buffer(
                                buffer_idx, buffer_resource_id, sizeof(buffer_resource_id)))
                            continue;
                        bool has_cpu_data = attr->buffer->data != NULL;
                        if ((has_cpu_data && attr->buffer->dirty) || !has_cpu_data)
                        {
                            dvz_frame_plan_upload_bytes(
                                plan, buffer_resource_id, 0, attr->buffer->desc.byte_size,
                                attr->name, attr->buffer->data);
                            _scene_attach_upload_metadata(
                                plan, visual, vidx, _scene_attr_frame_plan_role(attr->name),
                                DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER, buffer_idx,
                                attr->item_count);
                            DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
                            node->u.upload.external = !has_cpu_data;
                            node->u.upload.buffer_usage =
                                _scene_buffer_drp2_usage(attr->buffer->desc.usage);
                            node->u.upload.item_stride = attr->buffer->desc.stride;
                            if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                                 visual->type == DVZ_VISUAL_TYPE_MESH ||
                                 visual->type == DVZ_VISUAL_TYPE_PATH ||
                                 visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                                 visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
                                strcmp(attr->name, "position") == 0)
                            {
                                dvz_frame_plan_upload_set_topology(
                                    plan, (uint32_t)visual->topology);
                            }
                        }
                        emitted_buffers[buffer_idx] = true;
                        continue;
                    }
                    if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
                        continue;
                    char resource_id[128];
                    if (!_scene_visual_attr_resource_key(
                            figure, visual, vidx, attr->name, resource_id, sizeof(resource_id)))
                    {
                        continue;
                    }
                    uint64_t byte_offset = (uint64_t)attr->dirty_first_item * attr->item_size;
                    uint64_t byte_size = (uint64_t)attr->dirty_item_count * attr->item_size;
                    const void* data_ptr = (const uint8_t*)attr->data + byte_offset;
                    DvzFramePlanResourceRole role = _scene_attr_frame_plan_role(attr->name);
                    if (!_scene_frame_plan_upload_style_bytes(
                            figure, plan, resource_id, byte_offset, byte_size, attr->name,
                            data_ptr, role))
                    {
                        continue;
                    }
                    _scene_attach_upload_metadata(
                        plan, visual, vidx, role, DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
                        UINT32_MAX, attr->item_count);
                    if ((visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                         visual->type == DVZ_VISUAL_TYPE_MESH ||
                         visual->type == DVZ_VISUAL_TYPE_PATH ||
                         visual->type == DVZ_VISUAL_TYPE_SPHERE ||
                         visual->type == DVZ_VISUAL_TYPE_GLYPH) &&
                        strcmp(attr->name, "position") == 0)
                    {
                        dvz_frame_plan_upload_set_topology(plan, (uint32_t)visual->topology);
                    }
                }
            }
            if (visual->type == DVZ_VISUAL_TYPE_POINT || visual->type == DVZ_VISUAL_TYPE_PIXEL ||
                visual->type == DVZ_VISUAL_TYPE_MARKER ||
                visual->type == DVZ_VISUAL_TYPE_PRIMITIVE ||
                visual->type == DVZ_VISUAL_TYPE_MESH || visual->type == DVZ_VISUAL_TYPE_SPHERE)
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
