/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene compute FramePlan emission                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "domain/buffer_internal.h"
#include "frame_plan/frame_plan.h"
#include "scene_emit/internal.h"
#include "scene_emit/scene_emit.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _scene_emit_compute_buffer_upload(
    DvzFramePlan* plan, DvzSceneBuffer* buffer, uint32_t buffer_idx)
{
    ANN(plan);
    ANN(buffer);

    char resource_id[DVZ_SCENE_LABEL_SIZE];
    if (!_scene_resource_key_buffer(buffer_idx, resource_id, sizeof(resource_id)))
        return false;

    bool has_cpu_data = buffer->data != NULL;
    if ((has_cpu_data && !buffer->dirty) || buffer->desc.byte_size == 0)
        return true;

    if (!dvz_frame_plan_upload_bytes(
            plan, resource_id, 0, buffer->desc.byte_size, "compute.buffer", buffer->data))
        return false;

    DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
    node->u.upload.external = !has_cpu_data;
    node->u.upload.buffer_usage = _scene_buffer_drp2_usage(buffer->desc.usage);
    node->u.upload.item_stride = buffer->desc.stride;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_emit_compute_passes(
    DvzFigure* figure, DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(plan);
    (void)report;

    bool emitted_buffers[DVZ_SCENE_MAX_BUFFERS] = {0};
    for (uint32_t ci = 0; ci < figure->compute_count; ci++)
    {
        DvzSceneCompute* compute = figure->computes[ci];
        if (compute == NULL || compute->scene != figure->scene)
            return false;

        for (uint32_t bi = 0; bi < compute->binding_count; bi++)
        {
            DvzSceneComputeBinding* binding = &compute->bindings[bi];
            if (!binding->active || binding->buffer == NULL)
                continue;
            uint32_t buffer_idx = _scene_buffer_index(figure->scene, binding->buffer);
            if (buffer_idx == UINT32_MAX)
                return false;
            if (!emitted_buffers[buffer_idx])
            {
                if (!_scene_emit_compute_buffer_upload(plan, binding->buffer, buffer_idx))
                    return false;
                emitted_buffers[buffer_idx] = true;
            }
        }

        if (!dvz_frame_plan_compute(
                plan, compute->label, compute->dispatch[0], compute->dispatch[1],
                compute->dispatch[2]))
            return false;

        DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
        node->u.compute.shader_format = compute->desc.shader_format;
        node->u.compute.shader_source = compute->desc.shader_source;

        for (uint32_t bi = 0; bi < compute->binding_count; bi++)
        {
            DvzSceneComputeBinding* binding = &compute->bindings[bi];
            if (!binding->active || binding->buffer == NULL)
                continue;
            if (node->u.compute.binding_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
                return false;

            uint32_t buffer_idx = _scene_buffer_index(figure->scene, binding->buffer);
            if (buffer_idx == UINT32_MAX)
                return false;

            DvzFramePlanComputeBinding* dst =
                &node->u.compute.bindings[node->u.compute.binding_count++];
            dst->binding = binding->binding;
            dst->access = binding->access;
            dst->byte_offset = binding->byte_offset;
            dst->byte_size = binding->byte_size;
            if (!_scene_resource_key_buffer(
                    buffer_idx, dst->resource_id, sizeof(dst->resource_id)))
                return false;

            if (binding->access == DVZ_SCENE_COMPUTE_ACCESS_READ)
            {
                if (!dvz_frame_plan_compute_read(plan, dst->resource_id))
                    return false;
            }
            else
            {
                if (!dvz_frame_plan_compute_read(plan, dst->resource_id) ||
                    !dvz_frame_plan_compute_write(plan, dst->resource_id))
                    return false;
            }
        }
    }
    return true;
}
