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

static void _texture_uniform_from_color_role(DvzColorRole color_role, DvzSceneTextureUniform* out)
{
    ANN(out);
    out->params[0] = color_role == DVZ_COLOR_ROLE_SRGB_COLOR ? 1.0f : 0.0f;
}



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
    uint64_t id = _obj_id(emitter, "_bgl_mesh_textured_lights_v1", &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[5] = {
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
            {
                .binding = DVZ_SCENE_SHADER_BINDING_MESH_TEXTURE_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_PANEL_LIGHTS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 5, entries))
            return false;
    }
    *out_id = id;
    return true;
}


/**
 * Create the image bind group layout used by image shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
bool _create_image_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[3] = {
        {
            .binding = DVZ_SCENE_SHADER_BINDING_IMAGE_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_IMAGE_SAMPLER,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_IMAGE_PARAMS,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 3, entries);
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
    uint64_t material_buffer_id, uint64_t panel_light_buffer_id, uint64_t texture_id, uint64_t sampler_id,
    DvzColorRole color_role, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);
    if (
        bind_group_layout_id == 0 || material_buffer_id == 0 || panel_light_buffer_id == 0 ||
        texture_id == 0 || sampler_id == 0)
        return false;

    uint32_t role_key = (uint32_t)color_role;
    char params_buf_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_mesh_texture_params_%" PRIu64 "_r%u",
        texture_id, role_key);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_mesh_textured_%" PRIu64 "_l%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_r%u",
        material_buffer_id, panel_light_buffer_id, texture_id, sampler_id, role_key);
    bool is_new = false;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneTextureUniform), usage))
        return false;

    uint64_t id = _obj_id(emitter, bg_key, &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[5] = {
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
            {
                .binding = DVZ_SCENE_SHADER_BINDING_MESH_TEXTURE_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneTextureUniform),
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_PANEL_LIGHTS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = panel_light_buffer_id,
                .offset = 0,
                .size = sizeof(DvzScenePanelLightsGpu),
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(
                stream, id, bind_group_layout_id, 5, entries))
        {
            return false;
        }
    }

    DvzSceneTextureUniform uniform = {0};
    _texture_uniform_from_color_role(color_role, &uniform);
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneTextureUniform), &uniform))
        return false;

    *out_id = id;
    return true;
}


/**
 * Resolve an image texture/sampler/params bind group.
 *
 * @param emitter frame-plan emitter carrying persistent object ids
 * @param stream destination DRP2 command stream
 * @param bind_group_layout_id bind group layout id
 * @param texture_id sampled texture id
 * @param sampler_id sampler id
 * @param nearest whether the sampler uses nearest filtering
 * @param color_role sampled texture color role
 * @param out_id resolved bind group id
 * @return whether the bind group exists or was appended
 */
bool _resolve_image_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id,
    uint64_t texture_id, uint64_t sampler_id, bool nearest, DvzColorRole color_role,
    uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);
    if (bind_group_layout_id == 0 || texture_id == 0 || sampler_id == 0)
        return false;

    uint32_t role_key = (uint32_t)color_role;
    char params_buf_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_img_texture_params_%" PRIu64 "_r%u",
        texture_id, role_key);
    dvz_snprintf(
        bg_key, sizeof(bg_key), nearest ? "_bg_img_nearest_%" PRIu64 "_r%u" :
                                          "_bg_img_%" PRIu64 "_r%u",
        texture_id, role_key);

    bool is_new = false;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneTextureUniform), usage))
        return false;

    uint64_t id = _obj_id(emitter, bg_key, &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = DVZ_SCENE_SHADER_BINDING_IMAGE_TEXTURE,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = texture_id,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_IMAGE_SAMPLER,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_IMAGE_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneTextureUniform),
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(
                stream, id, bind_group_layout_id, 3, entries))
            return false;
    }

    DvzSceneTextureUniform uniform = {0};
    _texture_uniform_from_color_role(color_role, &uniform);
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneTextureUniform), &uniform))
        return false;

    *out_id = id;
    return true;
}



/**
 * Resolve an item-state style bind group containing material params and item-state style params.
 *
 * @param emitter frame-plan emitter carrying persistent object ids
 * @param stream destination DRP2 command stream
 * @param bind_group_layout_id bind group layout id
 * @param material_buffer_id material uniform buffer id
 * @param item_state_style_buffer_id item-state style uniform buffer id
 * @param out_id resolved bind group id
 * @return whether the bind group exists or was appended
 */
bool _resolve_item_state_style_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id,
    uint64_t material_buffer_id, uint64_t panel_light_buffer_id,
    uint64_t item_state_style_buffer_id, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);
    if (
        bind_group_layout_id == 0 || material_buffer_id == 0 || panel_light_buffer_id == 0 ||
        item_state_style_buffer_id == 0)
        return false;

    char bg_key[128];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_item_state_style_%" PRIu64 "_l%" PRIu64 "_%" PRIu64,
        material_buffer_id, panel_light_buffer_id, item_state_style_buffer_id);
    bool is_new = false;
    uint64_t id = _obj_id(emitter, bg_key, &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = DVZ_SCENE_SHADER_BINDING_MATERIAL_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = material_buffer_id,
                .offset = 0,
                .size = sizeof(DvzSceneMaterialParams),
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_ITEM_STATE_STYLE,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = item_state_style_buffer_id,
                .offset = 0,
                .size = sizeof(DvzSceneItemStateStyleParams),
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_PANEL_LIGHTS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = panel_light_buffer_id,
                .offset = 0,
                .size = sizeof(DvzScenePanelLightsGpu),
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
