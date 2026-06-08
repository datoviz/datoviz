/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan runtime compute emission                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/frame_plan.h"
#include "frame_plan/emit.h"
#include "_frame_plan_runtime_internal.h"
#include "_shader_registry.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzDrp2BindingAccess _compute_access(DvzSceneComputeAccess access)
{
    return access == DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE ? DVZ_DRP2_BINDING_ACCESS_READ_WRITE
                                                         : DVZ_DRP2_BINDING_ACCESS_READ;
}


static bool _compute_node_has_custom_bindings(const DvzFramePlanNode* node)
{
    return node != NULL && node->type == DVZ_FRAME_PLAN_NODE_COMPUTE &&
           node->u.compute.binding_count > 0;
}


static bool _compute_resolve_binding_resources(
    DvzFramePlanEmitter* emitter, const DvzFramePlanNode* compute, uint64_t* buffer_ids,
    bool* new_resources)
{
    ANN(emitter);
    ANN(compute);
    ANN(buffer_ids);
    ANN(new_resources);

    for (uint32_t i = 0; i < compute->u.compute.binding_count; i++)
    {
        const DvzFramePlanComputeBinding* binding = &compute->u.compute.bindings[i];
        bool is_new = false;
        ResourceId* resource =
            _resource_entry(&emitter->resources, binding->resource_id, &is_new);
        if (resource == NULL)
            return false;
        if (!_resource_ensure_byte_size(
                &emitter->resources, resource, binding->byte_offset + binding->byte_size,
                &is_new))
            return false;

        uint32_t usage = resource->usage != 0 ? resource->usage : DVZ_DRP2_BUFFER_USAGE_STORAGE;
        if ((usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0)
            usage |= DVZ_DRP2_BUFFER_USAGE_STORAGE;
        resource->usage = usage;
        buffer_ids[i] = resource->id;
        new_resources[i] = is_new;
    }
    return true;
}


static bool _compute_emit_new_buffers(
    DvzDrp2CommandStream* stream, const DvzFramePlanEmitter* emitter,
    const DvzFramePlanNode* compute, const uint64_t* buffer_ids, const bool* new_resources)
{
    ANN(stream);
    ANN(emitter);
    ANN(compute);
    ANN(buffer_ids);
    ANN(new_resources);

    for (uint32_t i = 0; i < compute->u.compute.binding_count; i++)
    {
        if (!new_resources[i])
            continue;
        const uint64_t id = buffer_ids[i];
        uint64_t byte_size = _resource_byte_size(&emitter->resources, id);
        uint32_t usage = _resource_usage(&emitter->resources, id);
        if (byte_size == 0 || usage == 0)
            return false;

        if (!dvz_drp2_stream_create_buffer(stream, id, byte_size, usage))
            return false;
    }
    return true;
}


static bool _compute_emit_one(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* compute,
    const DvzFramePlanEmitConfig* cfg, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(compute);

    if (compute->u.compute.shader_source == NULL || compute->u.compute.shader_source[0] == '\0')
    {
        _diagnostic(report, "scene compute requires shader source");
        return false;
    }

    uint64_t buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    bool new_resources[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    if (!_compute_resolve_binding_resources(emitter, compute, buffer_ids, new_resources) ||
        !_compute_emit_new_buffers(stream, emitter, compute, buffer_ids, new_resources))
        return false;

    char key[DVZ_SCENE_LABEL_SIZE];
    bool is_new = false;

    if (dvz_snprintf(
            key, sizeof(key), "compute.%s.shader.%u", compute->u.compute.shader_key,
            (uint32_t)compute->u.compute.shader_format) < 0)
        return false;
    uint64_t shader_id = _obj_id(emitter, key, &is_new);
    if (shader_id == 0)
        return false;
    if (is_new)
    {
        const char* wgsl = compute->u.compute.shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL
                               ? compute->u.compute.shader_source
                               : NULL;
        const char* glsl = compute->u.compute.shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL
                               ? compute->u.compute.shader_source
                               : NULL;
        if (!_emit_shader(stream, shader_id, "COMPUTE", wgsl, glsl, cfg))
            return false;
    }

    if (dvz_snprintf(key, sizeof(key), "compute.%s.bgl", compute->u.compute.shader_key) < 0)
        return false;
    uint64_t bgl_id = _obj_id(emitter, key, &is_new);
    if (bgl_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        for (uint32_t i = 0; i < compute->u.compute.binding_count; i++)
        {
            entries[i].binding = compute->u.compute.bindings[i].binding;
            entries[i].binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER;
            entries[i].visibility = DVZ_DRP2_SHADER_STAGE_COMPUTE;
            entries[i].access = _compute_access(compute->u.compute.bindings[i].access);
        }
        if (!dvz_drp2_stream_create_bind_group_layout_entries(
                stream, bgl_id, compute->u.compute.binding_count, entries))
            return false;
    }

    if (dvz_snprintf(key, sizeof(key), "compute.%s.pipeline", compute->u.compute.shader_key) < 0)
        return false;
    uint64_t pipeline_id = _obj_id(emitter, key, &is_new);
    if (pipeline_id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
                      stream, pipeline_id, shader_id, bgl_id))
        return false;

    uint64_t fingerprint = UINT64_C(1469598103934665603);
    DvzDrp2BindGroupEntry entries[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    for (uint32_t i = 0; i < compute->u.compute.binding_count; i++)
    {
        const DvzFramePlanComputeBinding* binding = &compute->u.compute.bindings[i];
        fingerprint = (fingerprint ^ (uint64_t)binding->binding) * UINT64_C(1099511628211);
        fingerprint = (fingerprint ^ (uint64_t)binding->access) * UINT64_C(1099511628211);
        fingerprint = (fingerprint ^ buffer_ids[i]) * UINT64_C(1099511628211);
        fingerprint = (fingerprint ^ binding->byte_offset) * UINT64_C(1099511628211);
        fingerprint = (fingerprint ^ binding->byte_size) * UINT64_C(1099511628211);
        entries[i].binding = compute->u.compute.bindings[i].binding;
        entries[i].binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER;
        entries[i].resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER;
        entries[i].resource_id = buffer_ids[i];
        entries[i].offset = compute->u.compute.bindings[i].byte_offset;
        entries[i].size = compute->u.compute.bindings[i].byte_size;
    }
    if (fingerprint == 0)
        fingerprint = UINT64_C(1);

    if (dvz_snprintf(
            key, sizeof(key), "compute.%s.bg.%016" PRIx64, compute->u.compute.shader_key,
            fingerprint) < 0)
        return false;
    uint64_t bg_id = _obj_id(emitter, key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        if (!dvz_drp2_stream_create_bind_group_entries(
                stream, bg_id, bgl_id, compute->u.compute.binding_count, entries))
            return false;
    }

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submit_id = _emitter_next_transient_id(emitter);
    bool ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
              dvz_drp2_stream_begin_compute_pass(stream, pass_id, encoder_id) &&
              dvz_drp2_stream_set_pipeline(stream, pass_id, pipeline_id) &&
              dvz_drp2_stream_set_bind_group(stream, pass_id, 0, bg_id) &&
              dvz_drp2_stream_dispatch_workgroups(
                  stream, pass_id, compute->u.compute.dispatch[0],
                  compute->u.compute.dispatch[1], compute->u.compute.dispatch[2]) &&
              dvz_drp2_stream_end_compute_pass(stream, pass_id);

    for (uint32_t i = 0; ok && i < compute->u.compute.binding_count; i++)
    {
        const DvzFramePlanComputeBinding* binding = &compute->u.compute.bindings[i];
        if (binding->access != DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE)
            continue;
        if ((_resource_usage(&emitter->resources, buffer_ids[i]) & DVZ_DRP2_BUFFER_USAGE_VERTEX) == 0)
            continue;
        ok = dvz_drp2_stream_resource_barrier(
            stream, encoder_id, buffer_ids[i], "COMPUTE", "STORAGE_WRITE", "VERTEX_INPUT",
            "VERTEX_READ", binding->byte_offset, binding->byte_size);
    }

    return ok && dvz_drp2_stream_finish_command_encoder(stream, encoder_id, command_buffer_id) &&
           dvz_drp2_stream_queue_submit(stream, command_buffer_id, submit_id);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _emitter_emit_compute_passes(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (!_compute_node_has_custom_bindings(node))
            continue;
        if (!_compute_emit_one(emitter, stream, node, cfg, report))
            return false;
    }
    return true;
}
