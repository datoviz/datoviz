/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan runtime bind groups */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan_runtime_internal.h"
#include "_frame_plan_runtime_upload.h"
#include "_render_pass.h"
#include "_scene.h"
#include "_scene_common_bindings.h"
#include "_scene_resource_key.h"
#include "_scene_shader_abi.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "frame_plan/emit.h"
#include "frame_plan/frame_plan.h"
#include "render_contract/render_contract.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Fill pipeline bind-group layout ids from a visual pipeline descriptor.
 *
 * @param pipeline visual pipeline descriptor
 * @param common_bgl_id scene-common bind group layout id
 * @param image_bgl_id image bind group layout id
 * @param labels_bgl_id labels bind group layout id
 * @param glyph_bgl_id glyph bind group layout id
 * @param volume_bgl_id volume bind group layout id
 * @param material_bgl_id material bind group layout id
 * @param scene_occlusion_bgl_id scene occlusion bind group layout id
 * @param scene_occlusion_uses_set2 whether scene occlusion must occupy set 2
 * @param out_layouts output bind group layout ids
 * @param out_count number of layout ids written
 */
void _pipeline_bind_group_layouts(
    const DvzSceneVisualPipelineDesc* pipeline, uint64_t common_bgl_id, uint64_t image_bgl_id,
    uint64_t labels_bgl_id, uint64_t glyph_bgl_id, uint64_t volume_bgl_id,
    uint64_t material_bgl_id, uint64_t item_state_style_bgl_id, uint64_t scene_occlusion_bgl_id,
    bool scene_occlusion_uses_set2, uint64_t ambient_visibility_bgl_id, uint64_t dummy_bgl_id,
    uint64_t* out_layouts, uint32_t* out_count)
{
    ANN(pipeline);
    ANN(out_layouts);
    ANN(out_count);

    uint32_t count = 0;
    uint64_t set1_layout = 0;
    if (pipeline->needs_common_layout && common_bgl_id != 0)
        out_layouts[count++] = common_bgl_id;
    if (pipeline->needs_image_layout && image_bgl_id != 0)
        set1_layout = image_bgl_id;
    if (pipeline->needs_labels_layout && labels_bgl_id != 0)
        set1_layout = labels_bgl_id;
    if (pipeline->needs_glyph_layout && glyph_bgl_id != 0)
        set1_layout = glyph_bgl_id;
    if (pipeline->needs_volume_layout && volume_bgl_id != 0)
        set1_layout = volume_bgl_id;
    if (pipeline->needs_material_layout && material_bgl_id != 0)
        set1_layout = material_bgl_id;
    if (pipeline->needs_item_state_style_layout && item_state_style_bgl_id != 0)
        set1_layout = item_state_style_bgl_id;

    bool scene_occlusion_layout_set2 = pipeline->needs_scene_occlusion_layout &&
                                       scene_occlusion_bgl_id != 0 && scene_occlusion_uses_set2;
    if (pipeline->needs_scene_occlusion_layout && scene_occlusion_bgl_id != 0 &&
        !scene_occlusion_layout_set2)
        set1_layout = scene_occlusion_bgl_id;

    if (set1_layout != 0)
    {
        while (count < DVZ_SCENE_SHADER_SET_VISUAL)
            out_layouts[count++] = 0;
        out_layouts[count++] = set1_layout;
    }
    if (scene_occlusion_layout_set2)
    {
        while (count < DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION)
            out_layouts[count++] = 0;
        out_layouts[count++] = scene_occlusion_bgl_id;
    }
    if (pipeline->needs_ambient_visibility_layout && ambient_visibility_bgl_id != 0)
    {
        while (count < 3)
            out_layouts[count++] = dummy_bgl_id;
        out_layouts[count++] = ambient_visibility_bgl_id;
    }
    *out_count = count;
}



/**
 * Resolve the shared material-parameter bind group layout.
 *
 * @param emitter frame-plan emitter carrying persistent object ids
 * @param stream destination DRP2 command stream
 * @param out_id resolved bind group layout id
 * @return whether the layout exists or was appended
 */
bool _resolve_material_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, "_bgl_material_params_lights_v1", &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[2] = {
            {
                .binding = DVZ_SCENE_SHADER_BINDING_MATERIAL_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_PANEL_LIGHTS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 2, entries))
            return false;
    }
    *out_id = id;
    return true;
}



/**
 * Resolve the item-state style bind group layout.
 *
 * @param emitter frame-plan emitter carrying persistent object ids
 * @param stream destination DRP2 command stream
 * @param out_id resolved bind group layout id
 * @return whether the layout exists or was appended
 */
bool _resolve_item_state_style_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, "_bgl_item_state_style_lights_v1", &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = DVZ_SCENE_SHADER_BINDING_MATERIAL_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_ITEM_STATE_STYLE,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_PANEL_LIGHTS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_VERTEX | DVZ_DRP2_SHADER_STAGE_FRAGMENT,
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
 * Create the glyph bind group layout used by text shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
bool _create_glyph_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[3] = {
        {
            .binding = DVZ_SCENE_SHADER_BINDING_GLYPH_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_GLYPH_SAMPLER,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_GLYPH_PARAMS,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 3, entries);
}


/**
 * Create the labels bind group layout used by integer label shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
bool _create_labels_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[3] = {
        {
            .binding = DVZ_SCENE_SHADER_BINDING_LABELS_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_LABELS_SAMPLER,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_LABELS_PARAMS,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 3, entries);
}


/**
 * Create the volume bind group layout used by slice/raymarch shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
bool _create_volume_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[6] = {
        {
            .binding = DVZ_SCENE_SHADER_BINDING_VOLUME_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_VOLUME_SAMPLER,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_VOLUME_PARAMS,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_VOLUME_DEPTH_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_VOLUME_TRANSFER_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_VOLUME_LABEL_LOOKUP,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 6, entries);
}



/**
 * Create the scene occlusion bind group layout used by occluded visual shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
bool _create_scene_occlusion_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[3] = {
        {
            .binding = DVZ_SCENE_SHADER_BINDING_OCCLUSION_DEPTH_TEXTURE,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_OCCLUSION_SAMPLER,
            .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = DVZ_SCENE_SHADER_BINDING_OCCLUSION_PARAMS,
            .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 3, entries);
}


/**
 * Create a placeholder bind group layout used to reserve unused set slots.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
bool _create_dummy_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entry = {
        .binding = 0,
        .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
        .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
        .access = DVZ_DRP2_BINDING_ACCESS_READ,
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 1, &entry);
}


/**
 * Convert retained scene occlusion state into the shader uniform payload.
 *
 * @param desc retained scene occlusion descriptor.
 * @param out output uniform payload.
 */
void _scene_occlusion_uniform_from_desc(
    const DvzSceneOcclusionDesc* desc, DvzSceneOcclusionUniform* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneOcclusionUniform), 0, sizeof(DvzSceneOcclusionUniform));
    if (desc == NULL || !desc->enabled)
        return;
    out->params[0] = desc->depth_bias;
    out->params[1] = desc->soft_edge > 0.0f ? desc->soft_edge : 0.002f;
    out->params[2] = desc->hidden_alpha;
    if (out->params[2] < 0.0f)
        out->params[2] = 0.0f;
    if (out->params[2] > 1.0f)
        out->params[2] = 1.0f;
    out->params[3] = 1.0f;
}


/**
 * Resolve a glyph texture/sampler/parameter bind group for one glyph visual.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param bgl_id glyph bind group layout id.
 * @param sampler_id glyph sampler id.
 * @param bind visual bind descriptor.
 * @param out_bg_id output bind group id.
 * @return whether the bind group and current uniform payload are available.
 */
bool _resolve_glyph_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(bind);
    ANN(out_bg_id);
    *out_bg_id = 0;
    if (bind->glyph_texture_id == 0)
        return false;

    float distance_range_px =
        bind->glyph_distance_range_px > 0.0f ? bind->glyph_distance_range_px : 4.0f;
    uint32_t encoding = bind->glyph_atlas_encoding;
    uint32_t distance_range_milli = (uint32_t)(distance_range_px * 1000.0f + 0.5f);

    bool is_new = false;
    char params_buf_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_glyph_params_%" PRIu64 "_e%u_r%u",
        bind->glyph_texture_id, encoding, distance_range_milli);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_glyph_%" PRIu64 "_e%u_r%u", bind->glyph_texture_id, encoding,
        distance_range_milli);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    if (is_new &&
        !dvz_drp2_stream_create_buffer(stream, params_buf_id, sizeof(DvzSceneGlyphUniform), usage))
        return false;

    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = DVZ_SCENE_SHADER_BINDING_GLYPH_TEXTURE,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = bind->glyph_texture_id,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_GLYPH_SAMPLER,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_GLYPH_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneGlyphUniform),
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 3, entries))
            return false;
    }

    DvzSceneGlyphUniform uniform = {0};
    uniform.params[0] = distance_range_px;
    uniform.params[1] = (float)encoding;
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneGlyphUniform), &uniform))
        return false;

    *out_bg_id = bg_id;
    return true;
}


/**
 * Convert retained labels state into the shader uniform payload.
 *
 * @param state retained labels state.
 * @param out output uniform payload.
 */
void _labels_uniform_from_state(const DvzLabelsState* state, DvzSceneLabelsUniform* out)
{
    ANN(state);
    ANN(out);

    dvz_memset(out, sizeof(DvzSceneLabelsUniform), 0, sizeof(DvzSceneLabelsUniform));
    out->ids[0] = (uint32_t)(int32_t)state->background_id;
    out->ids[1] = (uint32_t)(int32_t)state->selected_id;
    out->params[0] = state->selected_enabled ? DVZ_SCENE_LABELS_FLAG_SELECTED : 0u;
    if (state->boundary_enabled)
        out->params[0] |= DVZ_SCENE_LABELS_FLAG_BOUNDARY;
    out->params[1] = state->fallback_seed;
    out->params[2] = state->hidden_count;
    out->floats[0] = state->opacity;
    out->floats[1] = state->boundary_width_px;
    out->boundary_color[0] = (float)state->boundary_color.r / 255.0f;
    out->boundary_color[1] = (float)state->boundary_color.g / 255.0f;
    out->boundary_color[2] = (float)state->boundary_color.b / 255.0f;
    out->boundary_color[3] = (float)state->boundary_color.a / 255.0f;

    uint32_t hidden_count = state->hidden_count;
    if (hidden_count > DVZ_LABELS_MAX_HIDDEN)
        hidden_count = DVZ_LABELS_MAX_HIDDEN;
    out->params[2] = hidden_count;
    for (uint32_t i = 0; i < hidden_count; i++)
        out->hidden_ids[i / 4u][i % 4u] = (uint32_t)(int32_t)state->hidden_ids[i];
}


/**
 * Resolve the labels texture/sampler/parameter bind group for one visual.
 *
 * @param emitter frame-plan emitter carrying persistent object ids and uniform cache.
 * @param stream destination DRP2 command stream.
 * @param bgl_id labels bind group layout id.
 * @param sampler_id shared nearest labels sampler id.
 * @param bind labels bind descriptor.
 * @param out_bg_id resolved bind group id.
 * @return whether the bind group was resolved.
 */
bool _resolve_labels_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(bind);
    ANN(out_bg_id);
    *out_bg_id = 0;
    if (bind->labels_texture_id == 0)
        return false;

    bool is_new = false;
    char params_buf_key[96], params_slot_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_labels_params_%u_%" PRIu64,
        bind->labels_visual_index, bind->labels_texture_id);
    dvz_snprintf(
        params_slot_key, sizeof(params_slot_key), "_slot_labels_params_%u_%" PRIu64,
        bind->labels_visual_index, bind->labels_texture_id);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_labels_%u_%" PRIu64, bind->labels_visual_index,
        bind->labels_texture_id);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneLabelsUniform), usage))
        return false;

    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = DVZ_SCENE_SHADER_BINDING_LABELS_TEXTURE,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = bind->labels_texture_id,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_LABELS_SAMPLER,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = DVZ_SCENE_SHADER_BINDING_LABELS_PARAMS,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneLabelsUniform),
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 3, entries))
            return false;
    }

    DvzSceneLabelsUniform* slot = _emitter_labels_slot(emitter, params_slot_key);
    if (slot == NULL)
        return false;
    _labels_uniform_from_state(&bind->labels_state, slot);
    uint32_t lookup_count = bind->labels_lookup_count;
    if (lookup_count > DVZ_SCENE_LABELS_LOOKUP_CAPACITY)
        lookup_count = DVZ_SCENE_LABELS_LOOKUP_CAPACITY;
    for (uint32_t i = 0; i < lookup_count; i++)
    {
        slot->label_lookup[i][0] = bind->labels_lookup[i][0];
        slot->label_lookup[i][1] = bind->labels_lookup[i][1];
        slot->label_lookup[i][2] = bind->labels_lookup[i][2];
        slot->label_lookup[i][3] = bind->labels_lookup[i][3];
    }
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneLabelsUniform), slot))
        return false;

    *out_bg_id = bg_id;
    return true;
}


/**
 * Resolve the scene occlusion texture/sampler/parameter bind group for one visual.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param bgl_id scene occlusion bind group layout id.
 * @param sampler_id shared scene occlusion sampler id.
 * @param bind visual bind descriptor.
 * @param out_bg_id resolved bind group id.
 * @return whether the bind group was resolved.
 */
bool _resolve_scene_occlusion_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(bind);
    ANN(out_bg_id);
    *out_bg_id = 0;
    if (bind->scene_occlusion_depth_texture_id == 0)
        return false;

    bool is_new = false;
    char params_buf_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_scene_occ_params_%" PRIu64,
        bind->scene_occlusion_depth_texture_id);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_scene_occ_depth_%" PRIu64,
        bind->scene_occlusion_depth_texture_id);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneOcclusionUniform), usage))
        return false;

    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = bind->scene_occlusion_depth_texture_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneOcclusionUniform),
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 3, entries))
            return false;
    }

    DvzSceneOcclusionUniform uniform = {0};
    _scene_occlusion_uniform_from_desc(&bind->scene_occlusion, &uniform);
    uniform.viewport[0] = bind->sampled_panel_origin[0];
    uniform.viewport[1] = bind->sampled_panel_origin[1];
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneOcclusionUniform), &uniform))
        return false;

    *out_bg_id = bg_id;
    return true;
}


/**
 * Resolve a 1x1 far-depth texture for volume passes without shared scene depth.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param out_id output texture id.
 * @return whether the fallback texture is available.
 */
bool _resolve_volume_dummy_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    static const float depth_value = 1.0f;
    bool is_new = false;
    uint64_t depth_id = _obj_id(emitter, "_tex_volume_dummy_depth", &is_new);
    if (depth_id == 0)
        return false;
    if (is_new)
    {
        uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, depth_id, 1, 1, DVZ_FORMAT_R32_SFLOAT, usage))
            return false;
        if (!dvz_drp2_stream_write_texture_2d_borrowed(
                stream, depth_id, 0, 1, 1, sizeof(float), 1, &depth_value))
            return false;
    }
    *out_id = depth_id;
    return true;
}


/**
 * Resolve a 1x1 RGBA transfer texture for volumes that do not need scalar transfer lookup.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param out_id output texture id.
 * @return whether the fallback texture is available.
 */
bool _resolve_volume_dummy_transfer(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    static const uint8_t rgba_value[4] = {255, 255, 255, 255};
    bool is_new = false;
    uint64_t texture_id = _obj_id(emitter, "_tex_volume_dummy_transfer", &is_new);
    if (texture_id == 0)
        return false;
    if (is_new)
    {
        uint32_t usage = DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING | DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
        if (!dvz_drp2_stream_create_texture_2d_format_usage(
                stream, texture_id, 1, 1, DVZ_FORMAT_R8G8B8A8_UNORM, usage))
            return false;
        if (!dvz_drp2_stream_write_texture_2d_borrowed(
                stream, texture_id, 0, 1, 1, sizeof(rgba_value), 1, rgba_value))
            return false;
    }
    *out_id = texture_id;
    return true;
}


/**
 * Resolve a dummy sparse label lookup buffer for non-label volume pipelines.
 *
 * @param emitter frame-plan emitter carrying persistent object ids.
 * @param stream destination DRP2 command stream.
 * @param out_id resolved dummy buffer id.
 * @return whether the buffer was resolved.
 */
static bool _resolve_volume_dummy_label_lookup(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, "_buf_volume_label_lookup_dummy", &is_new);
    if (id == 0)
        return false;
    if (is_new)
    {
        uint32_t zero[4] = {0, 0, 0, 0};
        uint32_t usage = DVZ_DRP2_BUFFER_USAGE_STORAGE | DVZ_DRP2_BUFFER_USAGE_COPY_DST;
        if (!dvz_drp2_stream_create_buffer(stream, id, sizeof(zero), usage) ||
            !dvz_drp2_stream_write_buffer_bytes(stream, id, 0, sizeof(zero), zero))
        {
            return false;
        }
    }
    *out_id = id;
    return true;
}


/**
 * Convert retained volume state into the shader uniform payload.
 *
 * @param state retained volume state.
 * @param transfer_rgba whether the bound volume texture already contains RGBA transfer colors.
 * @param out output uniform payload.
 */
void _volume_uniform_from_state(
    const DvzVolumeState* state, bool transfer_rgba, DvzColorRole color_role,
    const DvzVolumeOcclusionDesc* occlusion, DvzSceneVolumeUniform* out)
{
    ANN(state);
    ANN(out);

    dvz_memset(out, sizeof(DvzSceneVolumeUniform), 0, sizeof(DvzSceneVolumeUniform));
    for (uint32_t i = 0; i < 3; i++)
    {
        out->clip_min[i] = state->clipping_enabled ? (float)state->clip_min[i] : 0.0f;
        out->clip_max[i] = state->clipping_enabled ? (float)state->clip_max[i] : 1.0f;
    }
    out->clip_min[3] = transfer_rgba ? 1.0f : 0.0f;
    out->clip_max[3] = 1.0f;
    double plane_d =
        -(state->clip_plane_normal[0] * state->clip_plane_point[0] +
          state->clip_plane_normal[1] * state->clip_plane_point[1] +
          state->clip_plane_normal[2] * state->clip_plane_point[2]);
    out->clip_plane[0] = (float)state->clip_plane_normal[0];
    out->clip_plane[1] = (float)state->clip_plane_normal[1];
    out->clip_plane[2] = (float)state->clip_plane_normal[2];
    out->clip_plane[3] = (float)plane_d;
    out->clip_plane_params[0] = state->clip_plane_enabled ? 1.0f : 0.0f;
    out->clip_plane_params[1] = state->clip_plane_keep_positive ? 1.0f : 0.0f;
    out->clip_plane_params[2] = 0.0f;
    out->clip_plane_params[3] = 1.0f;
    out->params[0] = state->opacity;
    out->params[1] = state->clipping_enabled ? 1.0f : 0.0f;
    out->params[2] = (float)state->step_count;
    out->params[3] = (float)state->render_mode;
    out->slice[0] = (float)state->slice_axis;
    out->slice[1] = (float)state->slice_position;
    out->slice[2] = 0.0f;
    out->slice[3] = 1.0f;
    for (uint32_t i = 0; i < 3; i++)
    {
        out->bounds_min[i] = (float)state->bounds_min[i];
        out->bounds_max[i] = (float)state->bounds_max[i];
        out->axis_order[i] = (float)state->axis_order[i];
        out->axis_flip[i] = state->axis_flip[i] ? 1.0f : 0.0f;
    }
    out->bounds_min[3] = 1.0f;
    out->bounds_max[3] = 1.0f;
    out->axis_order[3] = 0.0f;
    out->axis_flip[3] = 0.0f;
    out->value_range[0] = (float)state->value_min;
    out->value_range[1] = (float)state->value_max;
    out->value_range[2] = 0.0f;
    out->value_range[3] = 1.0f;
    if (occlusion != NULL && occlusion->enabled)
    {
        out->occlusion[0] = occlusion->alpha_threshold > 0.0f ? occlusion->alpha_threshold : 0.08f;
        out->occlusion[1] = occlusion->fade_distance > 0.0f ? occlusion->fade_distance : 0.08f;
        out->occlusion[2] = occlusion->occluded_alpha >= 0.0f ? occlusion->occluded_alpha : 0.20f;
        out->occlusion[3] = 1.0f;
    }
    else
    {
        out->occlusion[0] = 0.08f;
        out->occlusion[1] = 0.08f;
        out->occlusion[2] = 0.20f;
        out->occlusion[3] = 0.0f;
    }
    out->texture_params[0] = color_role == DVZ_COLOR_ROLE_SRGB_COLOR ? 1.0f : 0.0f;
}


/**
 * Resolve the volume texture/sampler/parameter bind group for one visual.
 *
 * @param emitter frame-plan emitter carrying persistent object ids and uniform cache.
 * @param stream destination DRP2 command stream.
 * @param bgl_id volume bind group layout id.
 * @param sampler_id shared volume sampler id.
 * @param bind volume bind descriptor.
 * @param out_bg_id resolved bind group id.
 * @return whether the bind group was resolved.
 */
bool _resolve_volume_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(bind);
    ANN(out_bg_id);
    *out_bg_id = 0;

    uint64_t depth_texture_id = bind->volume_depth_texture_id;
    if (depth_texture_id == 0 && !_resolve_volume_dummy_depth(emitter, stream, &depth_texture_id))
        return false;

    uint64_t transfer_texture_id = bind->volume_transfer_texture_id;
    if (transfer_texture_id == 0 || transfer_texture_id == bind->volume_texture_id)
    {
        if (!_resolve_volume_dummy_transfer(emitter, stream, &transfer_texture_id))
            return false;
    }
    uint64_t label_lookup_buffer_id = bind->volume_label_lookup_buffer_id;
    uint64_t label_lookup_buffer_size = bind->volume_label_lookup_buffer_size;
    if (label_lookup_buffer_id == 0)
    {
        if (!_resolve_volume_dummy_label_lookup(emitter, stream, &label_lookup_buffer_id))
            return false;
        label_lookup_buffer_size = 4 * sizeof(uint32_t);
    }
    if (label_lookup_buffer_size == 0)
        label_lookup_buffer_size = 4 * sizeof(uint32_t);

    bool is_new = false;
    char params_buf_key[96], params_slot_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_volume_params_%u_%u_%" PRIu64 "_tf_%" PRIu64,
        bind->volume_visual_index, bind->volume_bind_variant, bind->volume_texture_id,
        transfer_texture_id);
    dvz_snprintf(
        params_slot_key, sizeof(params_slot_key),
        "_slot_volume_params_%u_%u_%" PRIu64 "_tf_%" PRIu64, bind->volume_visual_index,
        bind->volume_bind_variant, bind->volume_texture_id, transfer_texture_id);
    dvz_snprintf(
        bg_key, sizeof(bg_key),
        "_bg_volume_%u_%u_%" PRIu64 "_tf_%" PRIu64 "_depth_%" PRIu64 "_lut_%" PRIu64,
        bind->volume_visual_index, bind->volume_bind_variant, bind->volume_texture_id,
        transfer_texture_id, depth_texture_id, label_lookup_buffer_id);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneVolumeUniform), usage))
        return false;

    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[6] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = bind->volume_texture_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = params_buf_id,
                .offset = 0,
                .size = sizeof(DvzSceneVolumeUniform),
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = depth_texture_id,
            },
            {
                .binding = 4,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = transfer_texture_id,
            },
            {
                .binding = 5,
                .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = label_lookup_buffer_id,
                .offset = 0,
                .size = label_lookup_buffer_size,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 6, entries))
            return false;
    }

    DvzSceneVolumeUniform* slot = _emitter_volume_slot(emitter, params_slot_key);
    if (slot == NULL)
        return false;
    _volume_uniform_from_state(
        &bind->volume_state, bind->volume_transfer_rgba, bind->volume_color_role,
        &bind->volume_occlusion, slot);
    slot->texture_params[1] = bind->sampled_panel_origin[0];
    slot->texture_params[2] = bind->sampled_panel_origin[1];
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneVolumeUniform), slot))
        return false;

    *out_bg_id = bg_id;
    return true;
}



/**
 * Attach scene/runtime labels to ids in an emitted DRP2 stream.
 *
 * @param emitter frame-plan emitter carrying scene/resource id maps
 * @param stream emitted DRP2 command stream
 * @param cfg optional emission configuration with borrowed target id
 */
