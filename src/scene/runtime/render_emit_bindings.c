/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime render bindings                                                      */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_runtime_internal.h"
#include "_scene_shader_abi.h"
#include "datoviz/drp2/stream.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve the textured-mesh material plus texture bind-group layout.
 *
 * @param emitter frame-plan emitter carrying persistent object ids
 * @param stream destination DRP2 command stream
 * @param out_id resolved bind group layout id
 * @return whether the layout exists or was appended
 */
bool _resolve_textured_mesh_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, "_bgl_mesh_textured", &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 3, entries))
            return false;
    }
    *out_id = id;
    return true;
}



/**
 * Resolve a textured-mesh bind group containing material params, texture, and sampler.
 *
 * @param emitter frame-plan emitter carrying persistent object ids
 * @param stream destination DRP2 command stream
 * @param bind_group_layout_id bind group layout id
 * @param material_buffer_id material uniform buffer id
 * @param texture_id sampled texture id
 * @param sampler_id sampler id
 * @param out_id resolved bind group id
 * @return whether the bind group exists or was appended
 */
bool _resolve_textured_mesh_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id,
    uint64_t material_buffer_id, uint64_t texture_id, uint64_t sampler_id, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);
    if (bind_group_layout_id == 0 || material_buffer_id == 0 || texture_id == 0 || sampler_id == 0)
        return false;

    char bg_key[96];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_mesh_textured_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
        material_buffer_id, texture_id, sampler_id);
    bool is_new = false;
    uint64_t id = _obj_id(emitter, bg_key, &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = material_buffer_id,
                .offset = 0,
                .size = sizeof(DvzSceneMaterialParams),
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = texture_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(
                stream, id, bind_group_layout_id, 3, entries))
        {
            return false;
        }
    }
    *out_id = id;
    return true;
}
