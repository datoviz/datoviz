/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan runtime emission                                                             */
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
#include "_frame_plan_emit.h"
#include "_frame_plan.h"
#include "_frame_plan_runtime_upload.h"
#include "_scene_common_bindings.h"
#include "_scene_shader_abi.h"
#include "_render_pass.h"
#include "_scene_resource_key.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "render_contract.h"
#include "datoviz/drp2.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/scene.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SceneRenderDraw SceneRenderDraw;
typedef struct SceneRenderBatch SceneRenderBatch;
typedef struct SceneGraphRuntimeTarget SceneGraphRuntimeTarget;
typedef struct SceneGraphRuntimeTargets SceneGraphRuntimeTargets;
typedef struct SceneGBufferTargets SceneGBufferTargets;
typedef struct SceneSsaoTargets SceneSsaoTargets;
typedef struct SceneEdlTargets SceneEdlTargets;
typedef struct SceneWboitTargets SceneWboitTargets;
typedef struct SceneDepthPeelTargets SceneDepthPeelTargets;

typedef struct DvzSceneOcclusionUniform
{
    float params[4];
} DvzSceneOcclusionUniform;


typedef struct DvzSceneGlyphUniform
{
    float params[4];
} DvzSceneGlyphUniform;


struct SceneRenderDraw
{
    uint64_t pipeline_id;
    uint64_t bg_set0;  /* MVP bg; 0 = none */
    uint64_t bg_set1;  /* image texture or primitive shading bg; 0 = none */
    uint64_t bg_set2;  /* scene occlusion bg; 0 = none */
    DvzSceneVisualDesc visual;
};


struct SceneRenderBatch
{
    const DvzFramePlanNode* render;
    SceneRenderDraw draws[DVZ_SCENE_MAX_RENDER_VISUALS];
    uint32_t draw_count;
};


struct SceneGraphRuntimeTarget
{
    char resource_id[DVZ_SCENE_LABEL_SIZE];
    uint64_t texture_id;
    uint32_t sample_count;
};


struct SceneGraphRuntimeTargets
{
    SceneGraphRuntimeTarget targets[DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY];
    uint32_t count;
};


struct SceneGBufferTargets
{
    uint64_t normal_id;
    uint64_t object_id;
    uint64_t depth_id;
    SceneGraphRuntimeTargets graph;
};


struct SceneEdlTargets
{
    uint64_t color_id;
    uint64_t depth_id;
    uint64_t params_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t resolve_bgl_id;
    uint64_t resolve_bg_id;
    uint64_t resolve_pipeline_id;
};


struct SceneSsaoTargets
{
    uint64_t normal_id;
    uint64_t depth_id;
    uint64_t occlusion_id;
    uint64_t blur_id;
    uint64_t composite_input_id;
    uint64_t params_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t ssao_bgl_id;
    uint64_t ssao_bg_id;
    uint64_t ssao_pipeline_id;
    uint64_t blur_bgl_id;
    uint64_t blur_bg_id;
    uint64_t blur_pipeline_id;
    uint64_t composite_bgl_id;
    uint64_t composite_bg_id;
    uint64_t composite_pipeline_id;
};


struct SceneWboitTargets
{
    uint64_t color_id;
    uint64_t accum_id;
    uint64_t weight_id;
    uint64_t depth_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t resolve_bgl_id;
    uint64_t resolve_bg_id;
    uint64_t resolve_pipeline_id;
};


struct SceneDepthPeelTargets
{
    uint64_t color_id;
    uint64_t depth_id;
    SceneGraphRuntimeTargets graph;
    uint64_t sampler_id;
    uint64_t sampled_bgl_id;
    uint64_t composite_bg_id;
    uint64_t composite_pipeline_id;
};


/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static uint64_t _graph_runtime_texture_id_for_resource(
    const char* resource_id, uint64_t final_color_id, const SceneGraphRuntimeTargets* targets,
    uint64_t fallback_id);

static bool _graph_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphResource* resource, uint32_t width,
    uint32_t height, uint32_t fallback_format, uint64_t* out_id);

static bool _graph_runtime_targets_add(
    SceneGraphRuntimeTargets* targets, const char* resource_id, uint64_t texture_id);

static void _label_render_pass_contract(
    DvzDrp2CommandStream* stream, uint64_t pass_id, const DvzFramePlanNode* render);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Select the depth-peeling fragment shader variant.
 *
 * @param lit whether the visual carries normals and uses lit shading.
 * @param back_pass whether the pass writes the back-shell accumulation.
 * @return the built-in shader key.
 */
static DvzSceneBuiltinShader _depth_peel_fragment_shader(bool lit, bool back_pass)
{
    if (lit)
    {
        return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT :
                           DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT;
    }
    return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK :
                       DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT;
}


/**
 * Return the SPIR-V resource key for one depth-peeling fragment shader.
 *
 * @param shader the built-in shader key.
 * @return the embedded SPIR-V key, or NULL when unsupported.
 */
static const char* _depth_peel_fragment_spirv_key(DvzSceneBuiltinShader shader)
{
    switch (shader)
    {
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT:
        return "depth_peel_front_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK:
        return "depth_peel_back_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT:
        return "depth_peel_front_lit_frag";
    case DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT:
        return "depth_peel_back_lit_frag";
    default:
        return NULL;
    }
}


/**
 * Fill pipeline bind-group layout ids from a visual pipeline descriptor.
 *
 * @param pipeline visual pipeline descriptor
 * @param common_bgl_id scene-common bind group layout id
 * @param image_bgl_id image bind group layout id
 * @param glyph_bgl_id glyph bind group layout id
 * @param volume_bgl_id volume bind group layout id
 * @param material_bgl_id material bind group layout id
 * @param scene_occlusion_bgl_id scene occlusion bind group layout id
 * @param scene_occlusion_uses_set2 whether scene occlusion must occupy set 2
 * @param out_layouts output bind group layout ids
 * @param out_count number of layout ids written
 */
static void _pipeline_bind_group_layouts(
    const DvzSceneVisualPipelineDesc* pipeline, uint64_t common_bgl_id, uint64_t image_bgl_id,
    uint64_t glyph_bgl_id, uint64_t volume_bgl_id, uint64_t material_bgl_id,
    uint64_t scene_occlusion_bgl_id, bool scene_occlusion_uses_set2, uint64_t* out_layouts,
    uint32_t* out_count)
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
    if (pipeline->needs_glyph_layout && glyph_bgl_id != 0)
        set1_layout = glyph_bgl_id;
    if (pipeline->needs_volume_layout && volume_bgl_id != 0)
        set1_layout = volume_bgl_id;
    if (pipeline->needs_material_layout && material_bgl_id != 0)
        set1_layout = material_bgl_id;

    bool scene_occlusion_layout_set2 =
        pipeline->needs_scene_occlusion_layout && scene_occlusion_bgl_id != 0 &&
        scene_occlusion_uses_set2;
    if (
        pipeline->needs_scene_occlusion_layout && scene_occlusion_bgl_id != 0 &&
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
static bool _resolve_material_bind_group_layout(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(out_id);

    bool is_new = false;
    uint64_t id = _obj_id(emitter, "_bgl_material_params", &is_new);
    if (id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_uniform_bind_group_layout(stream, id))
        return false;
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
static bool _create_glyph_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
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
 * Create the volume bind group layout used by slice/raymarch shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
static bool _create_volume_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
{
    ANN(stream);

    DvzDrp2BindGroupLayoutEntry entries[5] = {
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
    };
    return dvz_drp2_stream_create_bind_group_layout_entries(stream, id, 5, entries);
}



/**
 * Create the scene occlusion bind group layout used by occluded visual shaders.
 *
 * @param stream destination DRP2 command stream.
 * @param id bind group layout id.
 * @return whether the command was appended.
 */
static bool _create_scene_occlusion_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id)
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
 * Convert retained scene occlusion state into the shader uniform payload.
 *
 * @param desc retained scene occlusion descriptor.
 * @param out output uniform payload.
 */
static void _scene_occlusion_uniform_from_desc(
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
static bool _resolve_glyph_bind_group(
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

    float pixel_range = bind->glyph_pixel_range > 0.0f ? bind->glyph_pixel_range : 4.0f;
    uint32_t pixel_range_milli = (uint32_t)(pixel_range * 1000.0f + 0.5f);

    bool is_new = false;
    char params_buf_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_glyph_params_%" PRIu64 "_r%u",
        bind->glyph_texture_id, pixel_range_milli);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_glyph_%" PRIu64 "_r%u", bind->glyph_texture_id,
        pixel_range_milli);

    uint32_t usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                     DVZ_DRP2_BUFFER_USAGE_COPY_DST;
    uint64_t params_buf_id = _obj_id(emitter, params_buf_key, &is_new);
    if (params_buf_id == 0)
        return false;
    if (is_new && !dvz_drp2_stream_create_buffer(
                      stream, params_buf_id, sizeof(DvzSceneGlyphUniform), usage))
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
    uniform.params[0] = pixel_range;
    if (!dvz_drp2_stream_write_buffer_bytes(
            stream, params_buf_id, 0, sizeof(DvzSceneGlyphUniform), &uniform))
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
static bool _resolve_scene_occlusion_bind_group(
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
static bool _resolve_volume_dummy_depth(
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
                stream, depth_id, 1, 1, VK_FORMAT_R32_SFLOAT, usage))
            return false;
        if (!dvz_drp2_stream_write_texture_2d_bytes(
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
static bool _resolve_volume_dummy_transfer(
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
                stream, texture_id, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, usage))
            return false;
        if (!dvz_drp2_stream_write_texture_2d_bytes(
                stream, texture_id, 0, 1, 1, sizeof(rgba_value), 1, rgba_value))
            return false;
    }
    *out_id = texture_id;
    return true;
}


/**
 * Convert retained volume state into the shader uniform payload.
 *
 * @param state retained volume state.
 * @param transfer_rgba whether the bound volume texture already contains RGBA transfer colors.
 * @param out output uniform payload.
 */
static void _volume_uniform_from_state(
    const DvzVolumeState* state, bool transfer_rgba, const DvzVolumeOcclusionDesc* occlusion,
    DvzSceneVolumeUniform* out)
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
    double plane_d = -(
        state->clip_plane_normal[0] * state->clip_plane_point[0] +
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
        out->occlusion[2] =
            occlusion->occluded_alpha >= 0.0f ? occlusion->occluded_alpha : 0.20f;
        out->occlusion[3] = 1.0f;
    }
    else
    {
        out->occlusion[0] = 0.08f;
        out->occlusion[1] = 0.08f;
        out->occlusion[2] = 0.20f;
        out->occlusion[3] = 0.0f;
    }
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
static bool _resolve_volume_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t bgl_id,
    uint64_t sampler_id, const DvzSceneVisualBindDesc* bind, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(bind);
    ANN(out_bg_id);
    *out_bg_id = 0;

    uint64_t depth_texture_id = bind->volume_depth_texture_id;
    if (depth_texture_id == 0 &&
        !_resolve_volume_dummy_depth(emitter, stream, &depth_texture_id))
        return false;

    uint64_t transfer_texture_id = bind->volume_transfer_texture_id;
    if (transfer_texture_id == 0 || transfer_texture_id == bind->volume_texture_id)
    {
        if (!_resolve_volume_dummy_transfer(emitter, stream, &transfer_texture_id))
            return false;
    }

    bool is_new = false;
    char params_buf_key[96], params_slot_key[96], bg_key[128];
    dvz_snprintf(
        params_buf_key, sizeof(params_buf_key), "_buf_volume_params_%u_%u_%" PRIu64 "_tf_%" PRIu64,
        bind->volume_visual_index, bind->volume_bind_variant, bind->volume_texture_id,
        transfer_texture_id);
    dvz_snprintf(
        params_slot_key, sizeof(params_slot_key), "_slot_volume_params_%u_%u_%" PRIu64
                                               "_tf_%" PRIu64,
        bind->volume_visual_index, bind->volume_bind_variant, bind->volume_texture_id,
        transfer_texture_id);
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_volume_%u_%u_%" PRIu64 "_tf_%" PRIu64 "_depth_%" PRIu64,
        bind->volume_visual_index, bind->volume_bind_variant, bind->volume_texture_id,
        transfer_texture_id, depth_texture_id);

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
        DvzDrp2BindGroupEntry entries[5] = {
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
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 5, entries))
            return false;
    }

    DvzSceneVolumeUniform* slot = _emitter_volume_slot(emitter, params_slot_key);
    if (slot == NULL)
        return false;
    _volume_uniform_from_state(
        &bind->volume_state, bind->volume_transfer_rgba, &bind->volume_occlusion, slot);
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
static void _emitter_label_stream_ids(
    const DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);

    for (uint32_t i = 0; i < emitter->resources.count; i++)
    {
        const ResourceId* resource = &emitter->resources.resources[i];
        if (resource->id != 0 && resource->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, resource->id, resource->key);
    }

    for (uint32_t i = 0; i < emitter->objects.count; i++)
    {
        const ResourceId* object = &emitter->objects.resources[i];
        if (object->id != 0 && object->key[0] != '\0')
            dvz_drp2_stream_set_label(stream, object->id, object->key);
    }

    if (cfg != NULL && cfg->color_target_id != 0)
        dvz_drp2_stream_set_label(stream, cfg->color_target_id, "rt");
}



/**
 * Attach the FramePlan pass-contract id to an emitted DRP2 render-pass id.
 *
 * @param stream emitted DRP2 command stream
 * @param pass_id the emitted DRP2 render-pass id
 * @param render the source FramePlan render node
 */
static void _label_render_pass_contract(
    DvzDrp2CommandStream* stream, uint64_t pass_id, const DvzFramePlanNode* render)
{
    ANN(stream);
    ANN(render);
    if (pass_id != 0 && render->type == DVZ_FRAME_PLAN_NODE_RENDER &&
        render->u.render.has_pass_contract && render->u.render.pass_contract_id[0] != '\0')
    {
        dvz_drp2_stream_set_label(stream, pass_id, render->u.render.pass_contract_id);
    }
}



/**
 * Append one suffix to a runtime object key, reporting truncation as an emission error.
 *
 * @param key key buffer to append to
 * @param size key buffer size
 * @param suffix suffix to append
 * @param report optional diagnostic report
 * @return whether the suffix was appended without truncation
 */
static bool _runtime_key_append(
    char* key, size_t size, const char* suffix, DvzDiagnosticReport* report)
{
    ANN(key);
    ANN(suffix);
    size_t key_len = strlen(key);
    size_t suffix_len = strlen(suffix);
    if (key_len >= size || suffix_len >= size - key_len)
    {
        _diagnostic(report, "runtime pipeline key suffix would be truncated");
        return false;
    }
    int written = dvz_snprintf(key + key_len, size - key_len, "%s", suffix);
    if (written < 0 || (size_t)written != suffix_len)
    {
        _diagnostic(report, "runtime pipeline key suffix append failed");
        return false;
    }
    return true;
}



/**
 * Append a formatted suffix to a runtime object key, reporting truncation as an emission error.
 *
 * @param key key buffer to append to
 * @param size key buffer size
 * @param report optional diagnostic report
 * @param format suffix format string
 * @return whether the suffix was appended without truncation
 */
static bool _runtime_key_appendf(
    char* key, size_t size, DvzDiagnosticReport* report, const char* format, ...)
{
    ANN(key);
    ANN(format);
    char suffix[32];
    va_list args;
    va_start(args, format);
    int written = dvz_vsnprintf(suffix, sizeof(suffix), format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(suffix))
    {
        _diagnostic(report, "runtime pipeline key formatted suffix would be truncated");
        return false;
    }
    return _runtime_key_append(key, size, suffix, report);
}



/**
 * Prepare resources for one panel's draws before opening the render pass.
 *
 * @param emitter frame-plan emitter carrying scene/runtime state.
 * @param stream destination DRP2 command stream.
 * @param render render node to prepare.
 * @param cfg optional frame-plan emit configuration.
 * @param pass_has_depth_attachment whether the render pass will carry a depth attachment.
 * @param force_point_depth whether point-like visuals must emit depth writes.
 * @param sampled_depth_id depth texture sampled by volume shaders, or zero.
 * @param sampled_depth_is_volume_occlusion whether sampled_depth_id is a volume occlusion texture.
 * @param report diagnostic report receiving recoverable emission errors.
 * @param draws output draw descriptors filled from prepared visuals.
 * @param draw_count_out output number of prepared draw descriptors.
 * @return true when the render node has drawable prepared visuals, false otherwise.
 */
static bool _emitter_prepare_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* render,
    const DvzFramePlanEmitConfig* cfg, bool pass_has_depth_attachment, bool force_point_depth,
    uint64_t sampled_depth_id, bool sampled_depth_is_volume_occlusion,
    uint64_t scene_occlusion_depth_id, uint32_t pass_sample_count, bool pass_alpha_to_coverage,
    DvzDiagnosticReport* report, SceneRenderDraw* draws, uint32_t* draw_count_out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(draws);
    ANN(draw_count_out);

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);
    bool wboit_accumulation =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
    bool volume_occlusion_pass =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION;
    bool scene_occlusion_pass =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION;
    bool gbuffer_pass = render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER;
    bool depth_peel_pass =
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
        render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;

    uint64_t common_bgl_id = 0;
    uint64_t apply_bg_id = 0;
    uint64_t fixed_bg_id = 0;
    uint64_t isotropic_bg_id = 0;
    if (!_scene_common_bindings_resolve_panel_sets(
            emitter, stream, render, &common_bgl_id, &apply_bg_id, &fixed_bg_id,
            &isotropic_bg_id))
        return false;

    /* Image BGL + sampler (shared, created lazily on first image visual). */
    uint64_t img_bgl_id = 0, img_sampler_id = 0;
    uint64_t glyph_bgl_id = 0, glyph_sampler_id = 0;
    uint64_t volume_bgl_id = 0, volume_sampler_linear_id = 0, volume_sampler_nearest_id = 0;
    uint64_t scene_occlusion_bgl_id = 0, scene_occlusion_sampler_id = 0;

    uint32_t draw_count = 0;

    for (uint32_t i = 0; ok && i < render->u.render.visual_count; i++)
    {
        DvzSceneVisualDesc desc = {0};
        const char* visual_error = NULL;
        if (!_scene_visual_desc_from_render(emitter, render, i, &desc, &visual_error))
        {
            if (render->u.render.visual_metadata[i].has_metadata)
            {
                _diagnostic(
                    report, visual_error != NULL ? visual_error :
                                                  "invalid typed visual metadata");
                ok = false;
                break;
            }
            continue;
        }

        DvzSceneVisualShaderDesc shader = {0};
        char* scene_occlusion_fragment_glsl = NULL;
        if (gbuffer_pass)
        {
            if (desc.kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE && !desc.has_normal)
                continue;
            if (desc.kind != DVZ_SCENE_VISUAL_DESC_PRIMITIVE &&
                desc.kind != DVZ_SCENE_VISUAL_DESC_SPHERE)
                continue;
            if (desc.kind != DVZ_SCENE_VISUAL_DESC_SPHERE)
                desc.material_buffer_id = 0;
            if (desc.kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
            {
                dvz_snprintf(
                    shader.vertex_key, sizeof(shader.vertex_key), "_vs_gbuffer_sphere%s", fmt);
                dvz_snprintf(
                    shader.fragment_key, sizeof(shader.fragment_key), "_fs_gbuffer_sphere%s",
                    fmt);
                dvz_snprintf(
                    shader.pipeline_key, sizeof(shader.pipeline_key), "_pipe_gbuffer_sphere%s",
                    fmt);
                shader.vertex_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPHERE_GBUFFER, false);
                shader.fragment_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPHERE_GBUFFER, true);
                shader.vertex_spirv_key = "sphere_gbuffer_vert";
                shader.fragment_spirv_key = "sphere_gbuffer_frag";
            }
            else
            {
                dvz_snprintf(
                    shader.vertex_key, sizeof(shader.vertex_key), "_vs_gbuffer_prim%s", fmt);
                dvz_snprintf(
                    shader.fragment_key, sizeof(shader.fragment_key), "_fs_gbuffer_normal%s",
                    fmt);
                dvz_snprintf(
                    shader.pipeline_key, sizeof(shader.pipeline_key), "_pipe_gbuffer_t%u%s",
                    desc.topology, fmt);
                shader.vertex_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL, false);
                shader.fragment_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL, true);
                shader.vertex_spirv_key = "primitive_lit_vert";
                shader.fragment_spirv_key = "gbuffer_normal_frag";
            }
        }
        else if (volume_occlusion_pass)
        {
            if (desc.kind != DVZ_SCENE_VISUAL_DESC_VOLUME)
                continue;
            dvz_snprintf(shader.vertex_key, sizeof(shader.vertex_key), "_vs_vol_occ%s", fmt);
            dvz_snprintf(shader.fragment_key, sizeof(shader.fragment_key), "_fs_vol_occ%s", fmt);
            dvz_snprintf(shader.pipeline_key, sizeof(shader.pipeline_key), "_pipe_vol_occ%s", fmt);
            shader.vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, false);
            shader.fragment_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, true);
            shader.vertex_spirv_key = "volume_slice_vert";
            shader.fragment_spirv_key = "volume_occlusion_depth_frag";
            shader.builtin_family = "scene.volume";
            shader.builtin_variant = "occlusion_depth";
            shader.builtin_pipeline = "scene.volume";
        }
        else if (scene_occlusion_pass)
        {
            if (desc.kind == DVZ_SCENE_VISUAL_DESC_VOLUME)
            {
                dvz_snprintf(shader.vertex_key, sizeof(shader.vertex_key), "_vs_scene_occ_vol%s",
                             fmt);
                dvz_snprintf(
                    shader.fragment_key, sizeof(shader.fragment_key), "_fs_scene_occ_vol%s",
                    fmt);
                dvz_snprintf(
                    shader.pipeline_key, sizeof(shader.pipeline_key), "_pipe_scene_occ_vol%s",
                    fmt);
                shader.vertex_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, false);
                shader.fragment_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, true);
                shader.vertex_spirv_key = "volume_slice_vert";
                scene_occlusion_fragment_glsl = _shader_glsl_variant(
                    shader.fragment_glsl, "#define DVZ_SCENE_OCCLUSION_DEPTH_FAR 1\n");
                shader.fragment_glsl = scene_occlusion_fragment_glsl;
                shader.fragment_spirv_key = NULL;
                if (shader.fragment_glsl == NULL)
                {
                    ok = false;
                    break;
                }
            }
            else
            {
                const char* stem = "prim";
                const char* vertex_spirv_key = "primitive_vert";
                DvzSceneBuiltinShader vertex_shader = DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE;
                if (desc.kind == DVZ_SCENE_VISUAL_DESC_POINT)
                {
                    stem = "point";
                    vertex_spirv_key = "point_vert";
                    vertex_shader = DVZ_SCENE_BUILTIN_SHADER_POINT;
                }
                else if (desc.kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
                {
                    stem = "pixel";
                    vertex_spirv_key = "pixel_vert";
                    vertex_shader = DVZ_SCENE_BUILTIN_SHADER_PIXEL;
                }
                else if (desc.kind == DVZ_SCENE_VISUAL_DESC_MARKER)
                {
                    stem = "marker";
                    vertex_spirv_key = "marker_vert";
                    vertex_shader = DVZ_SCENE_BUILTIN_SHADER_MARKER;
                }
                else if (desc.kind == DVZ_SCENE_VISUAL_DESC_IMAGE)
                {
                    stem = "image";
                    vertex_spirv_key = "image_vert";
                    vertex_shader = DVZ_SCENE_BUILTIN_SHADER_IMAGE;
                }
                else if (desc.kind != DVZ_SCENE_VISUAL_DESC_PRIMITIVE)
                    continue;

                dvz_snprintf(
                    shader.vertex_key, sizeof(shader.vertex_key), "_vs_scene_occ_%s%s", stem,
                    fmt);
                dvz_snprintf(
                    shader.fragment_key, sizeof(shader.fragment_key), "_fs_scene_occ_depth%s",
                    fmt);
                dvz_snprintf(
                    shader.pipeline_key, sizeof(shader.pipeline_key), "_pipe_scene_occ_%s_t%u%s",
                    stem, desc.topology, fmt);
                shader.vertex_glsl = _builtin_shader_glsl(vertex_shader, false);
                shader.fragment_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SCENE_OCCLUSION_DEPTH, true);
                shader.vertex_spirv_key = vertex_spirv_key;
                shader.fragment_spirv_key = "scene_occlusion_depth_frag";
                if (desc.kind != DVZ_SCENE_VISUAL_DESC_IMAGE)
                {
                    scene_occlusion_fragment_glsl = _shader_glsl_variant(
                        shader.fragment_glsl,
                        "#define DVZ_SCENE_OCCLUSION_DEPTH_COLOR 1\n");
                    shader.fragment_glsl = scene_occlusion_fragment_glsl;
                    shader.fragment_spirv_key = NULL;
                    if (shader.fragment_glsl == NULL)
                    {
                        ok = false;
                        break;
                    }
                }
                desc.has_normal = false;
                desc.material_buffer_id = 0;
                if (desc.kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE && desc.vbuf_count > 2)
                    desc.vbuf_count = 2;
            }
        }
        else if (!_scene_visual_shader_desc(
                     &desc, render->u.render.picking, wboit_accumulation, fmt, &shader))
            continue;
        DvzAlphaMode alpha_mode = render->u.render.visual_metadata[i].has_metadata
                                      ? render->u.render.visual_metadata[i].alpha_mode
                                      : DVZ_ALPHA_OPAQUE;
        bool segment_coverage_blend =
            !render->u.render.picking && desc.kind == DVZ_SCENE_VISUAL_DESC_SEGMENT &&
            !_scene_alpha_mode_is_blended(alpha_mode) && !wboit_accumulation &&
            !depth_peel_pass;
        bool point_like_desc =
            desc.kind == DVZ_SCENE_VISUAL_DESC_POINT ||
            desc.kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
            desc.kind == DVZ_SCENE_VISUAL_DESC_MARKER;
        if (_scene_alpha_mode_is_blended(alpha_mode))
        {
            ok = _runtime_key_append(
                shader.pipeline_key, sizeof(shader.pipeline_key), "_blend", report);
            if (!ok)
                break;
        }
        if (segment_coverage_blend)
        {
            ok = _runtime_key_append(
                shader.pipeline_key, sizeof(shader.pipeline_key), "_coverage_blend", report);
            if (!ok)
                break;
        }
        if (!desc.depth_test_enabled)
        {
            ok = _runtime_key_append(
                shader.pipeline_key, sizeof(shader.pipeline_key), "_no_depth_test", report);
            if (!ok)
                break;
        }
        if (_scene_alpha_mode_is_depth_peel(alpha_mode))
        {
            const char* peel_suffix =
                render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT
                    ? "_peel_init"
                    : "_peel_iter";
            ok = _runtime_key_append(
                     shader.pipeline_key, sizeof(shader.pipeline_key), peel_suffix, report) &&
                 _runtime_key_append(
                     shader.fragment_key, sizeof(shader.fragment_key), peel_suffix, report);
            if (!ok)
                break;
            bool back_pass =
                render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
            DvzSceneBuiltinShader peel_shader =
                _depth_peel_fragment_shader(desc.has_normal, back_pass);
            shader.fragment_glsl = _builtin_shader_glsl(peel_shader, true);
            shader.fragment_spirv_key = _depth_peel_fragment_spirv_key(peel_shader);
        }
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
        {
            ok = _runtime_key_append(
                shader.pipeline_key, sizeof(shader.pipeline_key), "_fixed", report);
            if (!ok)
                break;
        }
        if (pass_has_depth_attachment && !gbuffer_pass && !wboit_accumulation && !depth_peel_pass)
        {
            ok = _runtime_key_append(
                shader.pipeline_key, sizeof(shader.pipeline_key),
                force_point_depth ? "_zwrite" : "_depth", report);
            if (!ok)
                break;
        }
        if (pass_sample_count > 1)
        {
            ok = _runtime_key_appendf(
                shader.pipeline_key, sizeof(shader.pipeline_key), report, "_msaa%" PRIu32,
                pass_sample_count);
            if (!ok)
                break;
            if ((desc.kind == DVZ_SCENE_VISUAL_DESC_SPHERE || point_like_desc) &&
                pass_alpha_to_coverage)
            {
                ok = _runtime_key_append(
                    shader.pipeline_key, sizeof(shader.pipeline_key), "_a2c", report);
                if (!ok)
                    break;
                if (desc.kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
                {
                    ok = _runtime_key_append(
                        shader.fragment_key, sizeof(shader.fragment_key), "_a2c", report);
                    if (!ok)
                        break;
                    shader.fragment_glsl =
                        _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPHERE_A2C, true);
                    shader.fragment_spirv_key = "sphere_a2c_frag";
                }
            }
        }
        bool scene_occluded_shader =
            desc.scene_occluded && scene_occlusion_depth_id != 0 && !scene_occlusion_pass;
        bool scene_occlusion_uses_set2 =
            desc.image_texture_id != 0 || desc.volume_texture_id != 0 ||
            (desc.material_buffer_id != 0 && !gbuffer_pass);
        if (scene_occluded_shader)
        {
            ok = _runtime_key_append(
                     shader.pipeline_key, sizeof(shader.pipeline_key), "_scene_occ", report) &&
                 _runtime_key_append(
                     shader.fragment_key, sizeof(shader.fragment_key), "_scene_occ", report);
            if (!ok)
                break;
            char scene_occlusion_defines[96];
            dvz_snprintf(
                scene_occlusion_defines, sizeof(scene_occlusion_defines),
                "#define DVZ_SCENE_OCCLUSION 1\n#define DVZ_SCENE_OCCLUSION_SET %u\n",
                scene_occlusion_uses_set2 ? 2u : 1u);
            scene_occlusion_fragment_glsl =
                _shader_glsl_variant(shader.fragment_glsl, scene_occlusion_defines);
            shader.fragment_glsl = scene_occlusion_fragment_glsl;
            shader.fragment_spirv_key = NULL;
            shader.fragment_wgsl = NULL;
            shader.builtin_family = NULL;
            shader.builtin_variant = NULL;
            shader.builtin_pipeline = NULL;
            if (shader.fragment_glsl == NULL)
            {
                ok = false;
                break;
            }
        }

        /* Shaders (cached). */
        uint64_t vs_id = _obj_id(emitter, shader.vertex_key, &is_new);
        if (vs_id == 0) { ok = false; break; }
        if (is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.vertex_wgsl == NULL)
                    ok = false;
                else
                    ok = ok && _emit_shader(
                                     stream, vs_id, "VERTEX", shader.vertex_wgsl,
                                     shader.vertex_glsl, cfg);
            }
            else if (shader.vertex_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, vs_id, "VERTEX", shader.vertex_spirv_key,
                               shader.vertex_glsl, cfg);
            else
                ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, shader.vertex_glsl, cfg);
            if (ok && shader.builtin_family != NULL && shader.builtin_variant != NULL)
                ok = dvz_drp2_stream_shader_set_builtin_identity(
                    stream, vs_id, shader.builtin_family, shader.builtin_variant, 1);
        }

        uint64_t fs_id = _obj_id(emitter, shader.fragment_key, &is_new);
        if (fs_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            {
                if (shader.fragment_wgsl == NULL)
                    ok = false;
                else
                    ok = ok &&
                         _emit_shader(
                             stream, fs_id, "FRAGMENT", shader.fragment_wgsl,
                             shader.fragment_glsl, cfg);
            }
            else if (shader.fragment_spirv_key != NULL)
                ok = ok && _emit_shader_spirv(
                               stream, fs_id, "FRAGMENT", shader.fragment_spirv_key,
                               shader.fragment_glsl, cfg);
            else
                ok = ok &&
                     _emit_shader(stream, fs_id, "FRAGMENT", NULL, shader.fragment_glsl, cfg);
            if (ok && shader.builtin_family != NULL && shader.builtin_variant != NULL)
                ok = dvz_drp2_stream_shader_set_builtin_identity(
                    stream, fs_id, shader.builtin_family, shader.builtin_variant, 1);
        }
        _shader_glsl_variant_destroy(scene_occlusion_fragment_glsl);

        uint64_t pipe_id = _obj_id(emitter, shader.pipeline_key, &is_new);
        if (pipe_id == 0) { ok = false; break; }
        if (ok && is_new)
        {
            DvzSceneVisualPipelineDesc pipeline = {0};
            if (!_scene_visual_pipeline_desc(
                    &desc, render->u.render.picking, pass_has_depth_attachment,
                    wboit_accumulation || depth_peel_pass, alpha_mode,
                    render->u.render.controller_modes[i],
                    &pipeline))
            {
                ok = false;
                break;
            }
            if (gbuffer_pass && desc.kind != DVZ_SCENE_VISUAL_DESC_SPHERE)
                pipeline.needs_material_layout = false;
            if (scene_occlusion_pass)
            {
                pipeline.needs_image_layout = false;
                pipeline.needs_glyph_layout = false;
                pipeline.needs_material_layout = false;
                pipeline.needs_scene_occlusion_layout = false;
                pipeline.has_depth_state = true;
                pipeline.depth_write_enabled = true;
                pipeline.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
            }
            if (force_point_depth && point_like_desc)
            {
                pipeline.has_depth_state = true;
                pipeline.depth_write_enabled = true;
                pipeline.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
            }
            if (
                pass_sample_count > 1 &&
                (desc.kind == DVZ_SCENE_VISUAL_DESC_SPHERE || point_like_desc) &&
                pass_alpha_to_coverage)
                pipeline.alpha_to_coverage = true;
            uint64_t material_bgl_id = 0;
            if (pipeline.needs_material_layout)
            {
                if (!_resolve_material_bind_group_layout(emitter, stream, &material_bgl_id))
                {
                    ok = false;
                    break;
                }
            }
            if (pipeline.needs_image_layout && img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
            }
            if (pipeline.needs_glyph_layout && glyph_bgl_id == 0)
            {
                glyph_bgl_id = _obj_id(emitter, "_bgl_glyph", &is_new);
                if (glyph_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_glyph_bind_group_layout(stream, glyph_bgl_id);
            }
            if (pipeline.needs_volume_layout && volume_bgl_id == 0)
            {
                volume_bgl_id = _obj_id(emitter, "_bgl_volume", &is_new);
                if (volume_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_volume_bind_group_layout(stream, volume_bgl_id);
            }
            if (pipeline.needs_scene_occlusion_layout && scene_occlusion_bgl_id == 0)
            {
                scene_occlusion_bgl_id = _obj_id(emitter, "_bgl_scene_occ", &is_new);
                if (scene_occlusion_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_scene_occlusion_bind_group_layout(
                                   stream, scene_occlusion_bgl_id);
            }
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex2(
                           stream, pipe_id, vs_id, fs_id, pipeline.vertex_buffer_count,
                           pipeline.topology, pipeline.binding_count, pipeline.strides,
                           pipeline.step_modes,
                           pipeline.attr_count, pipeline.bindings, pipeline.locations,
                           pipeline.formats, pipeline.offsets);
            if (ok && pass_sample_count > 1)
                ok = dvz_drp2_stream_pipeline_set_multisampling(
                    stream, pass_sample_count, pipeline.alpha_to_coverage);
            if (ok && shader.builtin_pipeline != NULL)
                ok = dvz_drp2_stream_pipeline_set_builtin_identity(
                    stream, pipe_id, shader.builtin_pipeline, 1);
            if (ok)
            {
                uint64_t layouts[DVZ_DRP2_MAX_BIND_GROUPS] = {0};
                uint32_t layout_count = 0;
                _pipeline_bind_group_layouts(
                    &pipeline, common_bgl_id, img_bgl_id, glyph_bgl_id, volume_bgl_id,
                    material_bgl_id, scene_occlusion_bgl_id, scene_occlusion_uses_set2, layouts,
                    &layout_count);
                if (layout_count > 0)
                    ok = dvz_drp2_stream_pipeline_set_bind_group_layouts(
                        stream, layout_count, layouts);
            }
            if (ok && pipeline.has_depth_state)
                ok = dvz_drp2_stream_pipeline_set_depth_state(
                    stream, pipeline.depth_write_enabled, pipeline.depth_compare_op);
            if (ok && pipeline.has_raster_state)
                ok = dvz_drp2_stream_pipeline_set_raster_state(
                    stream, pipeline.cull_mode, pipeline.front_face);
            if (ok && wboit_accumulation)
            {
                ok = ok &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 1, VK_FORMAT_R16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) &&
                     dvz_drp2_stream_pipeline_set_color_blend(
                         stream, 1, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
                         VK_COLOR_COMPONENT_R_BIT);
            }
            else if (ok && depth_peel_pass)
            {
                ok = ok &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 1, VK_FORMAT_R16G16B16A16_SFLOAT) &&
                     dvz_drp2_stream_pipeline_set_color_target(
                         stream, 2, VK_FORMAT_R16G16B16A16_SFLOAT);
                if (ok && render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT)
                    ok = dvz_drp2_stream_pipeline_set_raster_state(
                        stream, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
                else if (ok)
                    ok = dvz_drp2_stream_pipeline_set_raster_state(
                        stream, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            }
            else if (ok && gbuffer_pass)
            {
                ok = dvz_drp2_stream_pipeline_set_color_target(
                    stream, 0, VK_FORMAT_R16G16B16A16_SFLOAT);
            }
            else if (ok && (volume_occlusion_pass || scene_occlusion_pass))
            {
                ok = dvz_drp2_stream_pipeline_set_color_target(
                    stream, 0, VK_FORMAT_R32_SFLOAT);
            }
            else if (
                ok &&
                (_scene_alpha_mode_is_blended(alpha_mode) || segment_coverage_blend))
            {
                ok = dvz_drp2_stream_pipeline_set_color_blend(
                    stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD,
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
            }
        }

        /* Bind group at set 0. */
        uint64_t vis_bg_set0 = 0;
        uint64_t vis_bg_set1 = 0;
        uint64_t vis_bg_set2 = 0;
        DvzSceneVisualBindDesc bind = {0};
        if (!_scene_visual_bind_desc(&desc, render->u.render.controller_modes[i], &bind))
        {
            ok = false;
            break;
        }
        if (scene_occlusion_pass)
        {
            bind.uses_image_set1 = false;
            bind.image_texture_id = 0;
            bind.uses_glyph_set1 = false;
            bind.glyph_texture_id = 0;
            bind.uses_material_set1 = false;
            bind.material_buffer_id = 0;
            bind.uses_scene_occlusion_set2 = false;
            bind.scene_occlusion_depth_texture_id = 0;
        }
        bool volume_depth_producer_pass = volume_occlusion_pass || scene_occlusion_pass;
        if (bind.uses_volume_set1 && !volume_depth_producer_pass && !bind.volume_occluded)
            bind.volume_occlusion.enabled = false;
        if (bind.uses_volume_set1 && volume_depth_producer_pass)
            bind.volume_occlusion.enabled = true;
        if (bind.uses_volume_set1)
        {
            if (volume_depth_producer_pass)
                bind.volume_bind_variant = 2;
            else if (sampled_depth_is_volume_occlusion && bind.volume_occluded)
                bind.volume_bind_variant = 1;
            else
                bind.volume_bind_variant = 0;
        }
        if (bind.uses_volume_set1 && sampled_depth_id != 0 &&
            (!sampled_depth_is_volume_occlusion || bind.volume_occluded))
            bind.volume_depth_texture_id = sampled_depth_id;
        if (bind.uses_scene_occlusion_set2)
            bind.scene_occlusion_depth_texture_id = scene_occlusion_depth_id;
        if (bind.uses_common_set0)
        {
            if (bind.uses_fixed_common)
                vis_bg_set0 = fixed_bg_id;
            else if (bind.controller_mode == DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL)
                vis_bg_set0 = isotropic_bg_id;
            else
                vis_bg_set0 = apply_bg_id;
        }
        if (bind.uses_material_set1)
        {
            uint64_t material_bgl_id = 0;
            if (!_resolve_material_bind_group_layout(emitter, stream, &material_bgl_id))
            {
                ok = false;
                break;
            }
            char material_bg_key[64];
            dvz_snprintf(
                material_bg_key, sizeof(material_bg_key), "_bg_material_params_%" PRIu64,
                bind.material_buffer_id);
            uint64_t material_bg_id = _obj_id(emitter, material_bg_key, &is_new);
            if (material_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_uniform_bind_group(
                               stream, material_bg_id, material_bgl_id,
                               bind.material_buffer_id, 0,
                               sizeof(DvzSceneMaterialParams));
            vis_bg_set1 = material_bg_id;
        }
        if (bind.uses_image_set1)
        {
            /* Image BGL + sampler (lazy). */
            if (img_bgl_id == 0)
            {
                img_bgl_id = _obj_id(emitter, "_bgl_img", &is_new);
                if (img_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(
                                   stream, img_bgl_id);
            }
            if (img_sampler_id == 0)
            {
                img_sampler_id = _obj_id(emitter, "_sampler_img", &is_new);
                if (img_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, img_sampler_id);
            }
            char img_bg_key[64];
            dvz_snprintf(
                img_bg_key, sizeof(img_bg_key), "_bg_img_%" PRIu64, bind.image_texture_id);
            uint64_t img_bg_id = _obj_id(emitter, img_bg_key, &is_new);
            if (img_bg_id == 0) { ok = false; break; }
            if (ok && is_new)
                ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                               stream, img_bg_id, img_bgl_id, bind.image_texture_id,
                               img_sampler_id);
            vis_bg_set1 = img_bg_id;
        }
        if (bind.uses_glyph_set1)
        {
            if (glyph_bgl_id == 0)
            {
                glyph_bgl_id = _obj_id(emitter, "_bgl_glyph", &is_new);
                if (glyph_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_glyph_bind_group_layout(stream, glyph_bgl_id);
            }
            if (glyph_sampler_id == 0)
            {
                glyph_sampler_id = _obj_id(emitter, "_sampler_glyph", &is_new);
                if (glyph_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, glyph_sampler_id);
            }
            uint64_t glyph_bg_id = 0;
            ok = ok && _resolve_glyph_bind_group(
                           emitter, stream, glyph_bgl_id, glyph_sampler_id, &bind,
                           &glyph_bg_id);
            vis_bg_set1 = glyph_bg_id;
        }
        if (bind.uses_volume_set1)
        {
            if (volume_bgl_id == 0)
            {
                volume_bgl_id = _obj_id(emitter, "_bgl_volume", &is_new);
                if (volume_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_volume_bind_group_layout(stream, volume_bgl_id);
            }
            bool nearest = bind.volume_state.sampling == DVZ_VOLUME_SAMPLING_NEAREST;
            uint64_t* volume_sampler_id =
                nearest ? &volume_sampler_nearest_id : &volume_sampler_linear_id;
            if (*volume_sampler_id == 0)
            {
                *volume_sampler_id =
                    _obj_id(emitter, nearest ? "_sampler_volume_nearest" : "_sampler_volume_linear",
                            &is_new);
                if (*volume_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                {
                    DvzDrp2FilterMode filter =
                        nearest ? DVZ_DRP2_FILTER_NEAREST : DVZ_DRP2_FILTER_LINEAR;
                    ok = ok && dvz_drp2_stream_create_sampler_filter(
                                   stream, *volume_sampler_id, filter, filter);
                }
            }
            uint64_t volume_bg_id = 0;
            ok = ok && _resolve_volume_bind_group(
                           emitter, stream, volume_bgl_id, *volume_sampler_id, &bind,
                           &volume_bg_id);
            vis_bg_set1 = volume_bg_id;
        }
        if (bind.uses_scene_occlusion_set2 && bind.scene_occlusion_depth_texture_id != 0)
        {
            if (scene_occlusion_bgl_id == 0)
            {
                scene_occlusion_bgl_id = _obj_id(emitter, "_bgl_scene_occ", &is_new);
                if (scene_occlusion_bgl_id == 0) { ok = false; break; }
                if (is_new)
                    ok = ok && _create_scene_occlusion_bind_group_layout(
                                   stream, scene_occlusion_bgl_id);
            }
            if (scene_occlusion_sampler_id == 0)
            {
                scene_occlusion_sampler_id = _obj_id(emitter, "_sampler_scene_occ", &is_new);
                if (scene_occlusion_sampler_id == 0) { ok = false; break; }
                if (ok && is_new)
                    ok = ok && dvz_drp2_stream_create_sampler(stream, scene_occlusion_sampler_id);
            }
            uint64_t scene_occ_bg_id = 0;
            ok = ok && _resolve_scene_occlusion_bind_group(
                           emitter, stream, scene_occlusion_bgl_id, scene_occlusion_sampler_id,
                           &bind, &scene_occ_bg_id);
            if (scene_occlusion_uses_set2)
                vis_bg_set2 = scene_occ_bg_id;
            else
                vis_bg_set1 = scene_occ_bg_id;
        }

        if (!ok)
            break;

        draws[draw_count].pipeline_id = pipe_id;
        draws[draw_count].bg_set0     = vis_bg_set0;
        draws[draw_count].bg_set1     = vis_bg_set1;
        draws[draw_count].bg_set2     = vis_bg_set2;
        draws[draw_count].visual      = desc;
        draw_count++;
    }

    if (!ok || draw_count == 0)
        return false;

    *draw_count_out = draw_count;
    return true;
}



/**
 * Emit one panel's already-prepared draws inside an open render pass.
 *
 * @param stream destination DRP2 command stream.
 * @param render render node whose viewport/scissor and visuals are emitted.
 * @param render_pass_id active render-pass id.
 * @param draws prepared draw descriptors.
 * @param draw_count number of prepared draw descriptors.
 * @param cache optional state cache shared across panels in the same render pass.
 * @return true when all draw commands were emitted successfully, false otherwise.
 */
static bool _emitter_emit_render_multi_draws(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render, uint64_t render_pass_id,
    const SceneRenderDraw* draws, uint32_t draw_count, SceneRenderStateCache* cache)
{
    ANN(stream);
    ANN(render);
    ANN(draws);

    bool ok = dvz_drp2_stream_set_viewport(
                  stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
                  render->u.render.desc.width, render->u.render.desc.height) &&
              dvz_drp2_stream_set_scissor(
                  stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
                  render->u.render.desc.width, render->u.render.desc.height);

    uint64_t last_pipeline = (cache != NULL) ? cache->pipeline_id : 0;
    uint64_t last_bg_set0 = (cache != NULL) ? cache->bg_set0 : 0;
    uint64_t last_bg_set1 = 0;
    uint64_t last_bg_set2 = 0;
    for (uint32_t d = 0; ok && d < draw_count; d++)
    {
        if (draws[d].pipeline_id != last_pipeline)
        {
            ok = ok && dvz_drp2_stream_set_pipeline(stream, render_pass_id, draws[d].pipeline_id);
            last_pipeline = draws[d].pipeline_id;
            last_bg_set0  = 0;
            last_bg_set1  = 0;
            last_bg_set2  = 0;
        }
        if (draws[d].bg_set0 != 0 && draws[d].bg_set0 != last_bg_set0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, draws[d].bg_set0);
            last_bg_set0 = draws[d].bg_set0;
        }
        if (draws[d].bg_set1 != 0 && draws[d].bg_set1 != last_bg_set1)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, draws[d].bg_set1);
            last_bg_set1 = draws[d].bg_set1;
        }
        if (draws[d].bg_set2 != 0 && draws[d].bg_set2 != last_bg_set2)
        {
            ok = ok &&
                 dvz_drp2_stream_set_bind_group(stream, render_pass_id, 2, draws[d].bg_set2);
            last_bg_set2 = draws[d].bg_set2;
        }
        for (uint32_t j = 0; ok && j < draws[d].visual.vbuf_count; j++)
            ok = ok && dvz_drp2_stream_set_vertex_buffer(
                           stream, render_pass_id, j, draws[d].visual.vbuf_ids[j], 0);
        if (ok && draws[d].visual.index_buffer_id != 0)
        {
            ok = ok &&
                 dvz_drp2_stream_set_index_buffer(
                     stream, render_pass_id, draws[d].visual.index_buffer_id,
                     draws[d].visual.index_format, 0) &&
                 dvz_drp2_stream_draw_indexed(
                     stream, render_pass_id, draws[d].visual.index_count,
                     draws[d].visual.instance_count, 0, 0, 0);
        }
        else
        {
            ok = ok &&
                 dvz_drp2_stream_draw(
                     stream, render_pass_id, draws[d].visual.vertex_count,
                     draws[d].visual.instance_count, 0, 0);
        }
    }

    if (cache != NULL)
    {
        cache->pipeline_id = last_pipeline;
        cache->bg_set0 = last_bg_set0;
    }

    return ok;
}


/**
 * Return the configured render-target extent, falling back to fixture dimensions.
 *
 * @param cfg optional frame-plan emit configuration.
 * @param width output width in pixels.
 * @param height output height in pixels.
 */
static void _emit_target_extent(
    const DvzFramePlanEmitConfig* cfg, uint32_t* width, uint32_t* height)
{
    ANN(width);
    ANN(height);
    *width = (cfg != NULL && cfg->target_width > 0) ? cfg->target_width : 4;
    *height = (cfg != NULL && cfg->target_height > 0) ? cfg->target_height : 4;
}



/**
 * Return a graph resource descriptor by id.
 *
 * @param plan the FramePlan.
 * @param resource_id the graph resource id.
 * @return the resource descriptor, or NULL when absent.
 */
static const DvzFrameGraphResource*
_graph_resource_by_id(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource != NULL && strcmp(resource->id, resource_id) == 0)
            return resource;
    }
    return NULL;
}



/**
 * Return a graph pass descriptor by panel and work label.
 *
 * @param plan the FramePlan.
 * @param panel_id the panel id.
 * @param work_label the graph pass work label.
 * @return the graph pass descriptor, or NULL when absent.
 */
static const DvzFrameGraphPass* _graph_pass_by_panel_work(
    const DvzFramePlan* plan, const char* panel_id, const char* work_label)
{
    ANN(plan);
    ANN(panel_id);
    ANN(work_label);
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass != NULL && strcmp(pass->panel_id, panel_id) == 0 &&
            strcmp(pass->work_label, work_label) == 0)
            return pass;
    }
    return NULL;
}



/**
 * Return the graph pass associated with a render node.
 *
 * @param plan the FramePlan.
 * @param render render node.
 * @return the graph pass descriptor, or NULL when absent.
 */
static const DvzFrameGraphPass*
_graph_pass_for_render(const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(plan);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return NULL;
    const char* work_label = _scene_render_role_work_label(render->u.render.pass_role);
    if (work_label[0] == '\0')
        return NULL;

    uint32_t ordinal = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* candidate = &plan->nodes[i];
        if (candidate == render)
            break;
        if (candidate->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        const char* candidate_label =
            _scene_render_role_work_label(candidate->u.render.pass_role);
        if (candidate_label[0] != '\0' &&
            strcmp(candidate->u.render.panel_id, render->u.render.panel_id) == 0 &&
            strcmp(candidate_label, work_label) == 0)
            ordinal++;
    }

    uint32_t seen = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL || strcmp(pass->panel_id, render->u.render.panel_id) != 0 ||
            strcmp(pass->work_label, work_label) != 0)
            continue;
        if (seen == ordinal)
            return pass;
        seen++;
    }
    return NULL;
}



/**
 * Return the render node associated with a graph pass descriptor.
 *
 * @param plan the FramePlan.
 * @param pass graph pass descriptor.
 * @return the matching render node, or NULL when absent.
 */
static const DvzFramePlanNode*
_graph_render_for_pass(const DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    ANN(plan);
    ANN(pass);
    uint32_t ordinal = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* candidate = dvz_frame_plan_graph_pass_get(plan, i);
        if (candidate == pass)
            break;
        if (candidate != NULL && strcmp(candidate->panel_id, pass->panel_id) == 0 &&
            strcmp(candidate->work_label, pass->work_label) == 0)
            ordinal++;
    }

    uint32_t seen = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        const char* work_label = _scene_render_role_work_label(render->u.render.pass_role);
        if (work_label[0] != '\0' && strcmp(render->u.render.panel_id, pass->panel_id) == 0 &&
            strcmp(work_label, pass->work_label) == 0)
        {
            if (seen == ordinal)
                return render;
            seen++;
        }
    }
    return NULL;
}



/**
 * Convert graph texture usage flags to DRP2 texture usage flags.
 *
 * @param usage_flags graph resource usage flags.
 * @return DRP2 texture usage flags.
 */
static uint32_t _graph_texture_usage_to_drp2(uint32_t usage_flags)
{
    uint32_t out = 0;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0 ||
        (usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_STORAGE) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    if ((usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_DST) != 0)
        out |= DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    return out != 0 ? out : DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
}



/**
 * Convert one declared graph access into DRP2 texture usage flags.
 *
 * @param usage graph pass access usage.
 * @return DRP2 texture usage flags.
 */
static uint32_t _graph_access_usage_to_drp2(DvzFrameGraphAccessUsage usage)
{
    switch (usage)
    {
    case DVZ_FRAME_GRAPH_ACCESS_SAMPLED:
        return DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_READ:
    case DVZ_FRAME_GRAPH_ACCESS_STORAGE_WRITE:
        return DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING;
    case DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ:
    case DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE:
        return DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_SRC:
        return DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    case DVZ_FRAME_GRAPH_ACCESS_COPY_DST:
        return DVZ_DRP2_TEXTURE_USAGE_COPY_DST;
    default:
        return 0;
    }
}



/**
 * Return the graph access implied by a depth attachment declaration.
 *
 * @param attachment graph attachment descriptor.
 * @return graph access usage.
 */
static DvzFrameGraphAccessUsage
_graph_depth_attachment_usage(const DvzFrameGraphAttachment* attachment)
{
    ANN(attachment);
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
        return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ;
    if (attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE)
        return DVZ_FRAME_GRAPH_ACCESS_NONE;
    return DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
}


/**
 * Compute DRP2 texture usage from all graph pass access declarations for a resource.
 *
 * @param plan the FramePlan.
 * @param resource_id graph resource id.
 * @return DRP2 texture usage flags implied by graph passes.
 */
static uint32_t
_graph_declared_texture_usage_to_drp2(const DvzFramePlan* plan, const char* resource_id)
{
    ANN(plan);
    ANN(resource_id);
    uint32_t usage = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL)
            continue;
        for (uint32_t j = 0; j < pass->read_count; j++)
        {
            if (strcmp(pass->reads[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(pass->reads[j].usage);
        }
        for (uint32_t j = 0; j < pass->write_count; j++)
        {
            if (strcmp(pass->writes[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(pass->writes[j].usage);
        }
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
        {
            if (strcmp(pass->color_attachments[j].resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(
                    DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT);
            if (strcmp(pass->color_attachments[j].resolve_resource_id, resource_id) == 0)
                usage |= _graph_access_usage_to_drp2(
                    DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT);
        }
        if (pass->has_depth_attachment &&
            strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        {
            usage |= _graph_access_usage_to_drp2(
                _graph_depth_attachment_usage(&pass->depth_attachment));
        }
    }
    return usage;
}



/**
 * Convert a graph attachment load operation to a DRP2 attachment load operation.
 *
 * @param op graph attachment load operation.
 * @return DRP2 attachment load operation.
 */
static DvzDrp2AttachmentLoadOp _graph_load_op_to_drp2(DvzFrameGraphAttachmentLoadOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR:
        return DVZ_DRP2_ATTACHMENT_LOAD_CLEAR;
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD:
        return DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    case DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE:
        return DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE;
    default:
        return DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    }
}



/**
 * Convert a graph attachment store operation to a DRP2 attachment store operation.
 *
 * @param op graph attachment store operation.
 * @return DRP2 attachment store operation.
 */
static DvzDrp2AttachmentStoreOp _graph_store_op_to_drp2(DvzFrameGraphAttachmentStoreOp op)
{
    switch (op)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE:
        return DVZ_DRP2_ATTACHMENT_STORE_STORE;
    case DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE:
        return DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE;
    default:
        return DVZ_DRP2_ATTACHMENT_STORE_STORE;
    }
}



/**
 * Convert graph attachment access to DRP2 attachment access.
 *
 * @param access graph attachment access.
 * @return DRP2 attachment access.
 */
static DvzDrp2AttachmentAccess _graph_attachment_access_to_drp2(
    DvzFrameGraphAttachmentAccess access)
{
    switch (access)
    {
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ:
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ;
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE:
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE:
    case DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE:
    default:
        return DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
    }
}


/**
 * Return a graph resource's effective sample count.
 *
 * @param resource the graph resource
 * @return sample count, defaulting to 1 when unset
 */
static uint32_t _graph_resource_sample_count(const DvzFrameGraphResource* resource)
{
    return resource != NULL && resource->sample_count != 0 ? resource->sample_count : 1;
}


/**
 * Clamp a requested sample count to a supported power-of-two sample count.
 *
 * @param sample_count requested sample count.
 * @param max_sample_count maximum supported sample count.
 * @return supported sample count.
 */
static uint32_t _sample_count_lowered(uint32_t sample_count, uint32_t max_sample_count)
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
 * Return a graph resource's capability-lowered sample count.
 *
 * @param emitter runtime emitter carrying current device limits.
 * @param resource the graph resource.
 * @return supported sample count, defaulting to one when unset.
 */
static uint32_t _graph_resource_lowered_sample_count(
    const DvzFramePlanEmitter* emitter, const DvzFrameGraphResource* resource)
{
    uint32_t sample_count = _graph_resource_sample_count(resource);
    if (sample_count <= 1 || emitter == NULL || resource == NULL)
        return sample_count;

    uint32_t max_sample_count = 16;
    bool color = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT) != 0;
    bool depth = (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0;
    if (color || depth)
    {
        uint32_t color_max =
            emitter->max_color_sample_count != 0 ? emitter->max_color_sample_count : 1;
        uint32_t depth_max =
            emitter->max_depth_sample_count != 0 ? emitter->max_depth_sample_count : 1;
        max_sample_count = color_max < depth_max ? color_max : depth_max;
    }
    if (max_sample_count == 0)
        max_sample_count = 1;
    return _sample_count_lowered(sample_count, max_sample_count);
}


/**
 * Return the raster sample count implied by a render graph pass.
 *
 * @param emitter runtime emitter carrying current device limits.
 * @param plan the FramePlan carrying graph resources
 * @param pass graph pass descriptor
 * @return raster sample count, defaulting to 1
 */
static uint32_t
_graph_render_pass_sample_count(
    const DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzFrameGraphPass* pass)
{
    if (plan == NULL || pass == NULL || pass->color_attachment_count == 0)
        return 1;
    const DvzFrameGraphResource* resource =
        _graph_resource_by_id(plan, pass->color_attachments[0].resource_id);
    uint32_t sample_count = _graph_resource_lowered_sample_count(emitter, resource);
    if (pass->has_depth_attachment)
    {
        const DvzFrameGraphResource* depth =
            _graph_resource_by_id(plan, pass->depth_attachment.resource_id);
        uint32_t depth_sample_count = _graph_resource_lowered_sample_count(emitter, depth);
        if (depth_sample_count < sample_count)
            sample_count = depth_sample_count;
    }
    return sample_count;
}



/**
 * Apply graph color attachment load/store operations to the current DRP2 render pass command.
 *
 * @param stream destination DRP2 command stream.
 * @param pass graph pass descriptor.
 * @return whether the command was updated.
 */
static bool _stream_apply_graph_color_ops(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets)
{
    ANN(stream);
    if (pass == NULL)
        return true;
    bool ok = true;
    for (uint32_t i = 0; ok && i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        ok = dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
            stream, i, _graph_load_op_to_drp2(attachment->load_op),
            _graph_store_op_to_drp2(attachment->store_op)) &&
             dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
                 stream, i, _graph_attachment_access_to_drp2(attachment->access));
        if (ok && attachment->resolve_resource_id[0] != '\0')
        {
            uint64_t resolve_id = _graph_runtime_texture_id_for_resource(
                attachment->resolve_resource_id, final_color_id, targets, 0);
            ok = resolve_id != 0 &&
                 dvz_drp2_stream_begin_render_pass_set_color_attachment_resolve(
                     stream, i, resolve_id, attachment->resolve_mode);
        }
    }
    return ok;
}



/**
 * Apply graph depth attachment state to the current DRP2 render pass command.
 *
 * @param stream destination DRP2 command stream.
 * @param pass graph pass descriptor.
 * @param depth_id named depth texture id, or zero for no graph depth.
 * @return whether the command was updated.
 */
static bool _stream_apply_graph_depth(
    DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass, uint64_t depth_id)
{
    ANN(stream);
    if (pass == NULL || !pass->has_depth_attachment || depth_id == 0)
        return true;
    const DvzFrameGraphAttachment* attachment = &pass->depth_attachment;
    return dvz_drp2_stream_begin_render_pass_set_depth_texture(
               stream, depth_id, attachment->clear_depth) &&
           dvz_drp2_stream_begin_render_pass_set_depth_ops(
               stream, _graph_load_op_to_drp2(attachment->load_op),
               _graph_store_op_to_drp2(attachment->store_op)) &&
           dvz_drp2_stream_begin_render_pass_set_depth_access(
               stream, _graph_attachment_access_to_drp2(attachment->access));
}



/**
 * Register one graph resource id to runtime texture id mapping.
 *
 * @param targets runtime target map.
 * @param resource_id graph resource id.
 * @param texture_id runtime texture id.
 * @return whether the mapping was registered.
 */
static bool _graph_runtime_targets_add(
    SceneGraphRuntimeTargets* targets, const char* resource_id, uint64_t texture_id)
{
    ANN(targets);
    ANN(resource_id);

    if (resource_id[0] == '\0' || texture_id == 0)
        return true;
    for (uint32_t i = 0; i < targets->count; i++)
    {
        if (strcmp(targets->targets[i].resource_id, resource_id) == 0)
        {
            targets->targets[i].texture_id = texture_id;
            return true;
        }
    }
    if (targets->count >= DVZ_FRAME_PLAN_INITIAL_GRAPH_RESOURCE_CAPACITY)
        return false;

    SceneGraphRuntimeTarget* target = &targets->targets[targets->count++];
    dvz_strlcpy(target->resource_id, resource_id, sizeof(target->resource_id));
    target->texture_id = texture_id;
    return true;
}



/**
 * Return one registered runtime texture id by graph resource id.
 *
 * @param targets runtime target map.
 * @param resource_id graph resource id.
 * @return runtime texture id, or zero when no mapping exists.
 */
static uint64_t
_graph_runtime_targets_get(const SceneGraphRuntimeTargets* targets, const char* resource_id)
{
    if (targets == NULL || resource_id == NULL || resource_id[0] == '\0')
        return 0;
    for (uint32_t i = 0; i < targets->count; i++)
    {
        if (strcmp(targets->targets[i].resource_id, resource_id) == 0)
            return targets->targets[i].texture_id;
    }
    return 0;
}



/**
 * Return the runtime texture id for one graph resource id.
 *
 * @param resource_id graph resource id.
 * @param final_color_id runtime id of the final render target.
 * @param targets optional runtime targets for the panel.
 * @param fallback_id fallback runtime id when no graph declaration is available.
 * @return runtime texture id for the graph resource.
 */
static uint64_t _graph_runtime_texture_id_for_resource(
    const char* resource_id, uint64_t final_color_id, const SceneGraphRuntimeTargets* targets,
    uint64_t fallback_id)
{
    if (resource_id == NULL || resource_id[0] == '\0')
        return fallback_id;
    if (strcmp(resource_id, "rt") == 0)
        return final_color_id;
    if (targets != NULL)
    {
        uint64_t texture_id = _graph_runtime_targets_get(targets, resource_id);
        if (texture_id != 0)
            return texture_id;
    }
    return fallback_id;
}



/**
 * Return the runtime texture id declared by one graph color attachment.
 *
 * @param pass graph pass descriptor, or NULL.
 * @param attachment_index color attachment index.
 * @param final_color_id runtime id of the final render target.
 * @param targets optional runtime targets for the panel.
 * @param fallback_id fallback runtime id when no graph declaration is available.
 * @return runtime texture id for the graph color attachment.
 */
static uint64_t _graph_color_attachment_texture_id(
    const DvzFrameGraphPass* pass, uint32_t attachment_index, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets, uint64_t fallback_id)
{
    if (pass == NULL || attachment_index >= pass->color_attachment_count)
        return fallback_id;

    return _graph_runtime_texture_id_for_resource(
        pass->color_attachments[attachment_index].resource_id, final_color_id, targets,
        fallback_id);
}



/**
 * Return the runtime texture id declared by one graph sampled read.
 *
 * @param pass graph pass descriptor, or NULL.
 * @param read_index sampled read index.
 * @param final_color_id runtime id of the final render target.
 * @param targets optional runtime targets for the panel.
 * @param fallback_id fallback runtime id when no graph declaration is available.
 * @return runtime texture id for the graph sampled read.
 */
static uint64_t _graph_sampled_read_texture_id(
    const DvzFrameGraphPass* pass, uint32_t read_index, uint64_t final_color_id,
    const SceneGraphRuntimeTargets* targets, uint64_t fallback_id)
{
    if (pass == NULL || read_index >= pass->read_count)
        return fallback_id;

    return _graph_runtime_texture_id_for_resource(
        pass->reads[read_index].resource_id, final_color_id, targets, fallback_id);
}


/**
 * Return the index of a sampled volume-occlusion read in a graph pass.
 *
 * @param pass graph pass descriptor, or NULL
 * @param out_read_index output read index
 * @return whether a volume occlusion read was found
 */
static bool _graph_volume_occlusion_read_index(
    const DvzFrameGraphPass* pass, uint32_t* out_read_index)
{
    ANN(out_read_index);
    if (pass == NULL)
        return false;
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (
            pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED &&
            _scene_resource_id_has_suffix(pass->reads[i].resource_id, ".volume_occlusion.depth"))
        {
            *out_read_index = i;
            return true;
        }
    }
    return false;
}


/**
 * Resolve the sampled volume-occlusion texture read by a graph pass.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param cfg optional frame-plan emit configuration
 * @param pass graph pass descriptor, or NULL
 * @param out_id output texture id
 * @return whether the lookup succeeded
 */
static bool _graph_resolve_volume_occlusion_read(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphPass* pass, uint64_t* out_id)
{
    ANN(out_id);
    *out_id = 0;
    uint32_t read_index = 0;
    if (!_graph_volume_occlusion_read_index(pass, &read_index))
        return true;

    const DvzFrameGraphResource* resource =
        _graph_resource_by_id(plan, pass->reads[read_index].resource_id);
    if (resource == NULL)
        return false;
    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, resource, width, height, VK_FORMAT_R32_SFLOAT, out_id);
}


/**
 * Return the index of a sampled scene-occlusion read in a graph pass.
 *
 * @param pass graph pass descriptor, or NULL
 * @param out_read_index output read index
 * @return whether a scene occlusion read was found
 */
static bool _graph_scene_occlusion_read_index(
    const DvzFrameGraphPass* pass, uint32_t* out_read_index)
{
    ANN(out_read_index);
    if (pass == NULL)
        return false;
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (
            pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_SAMPLED &&
            _scene_resource_id_has_suffix(pass->reads[i].resource_id, ".scene_occlusion.depth"))
        {
            *out_read_index = i;
            return true;
        }
    }
    return false;
}


/**
 * Resolve the sampled scene-occlusion texture read by a graph pass.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param cfg optional frame-plan emit configuration
 * @param pass graph pass descriptor, or NULL
 * @param out_id output texture id
 * @return whether the lookup succeeded
 */
static bool _graph_resolve_scene_occlusion_read(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphPass* pass, uint64_t* out_id)
{
    ANN(out_id);
    *out_id = 0;
    uint32_t read_index = 0;
    if (!_graph_scene_occlusion_read_index(pass, &read_index))
        return true;

    const DvzFrameGraphResource* resource =
        _graph_resource_by_id(plan, pass->reads[read_index].resource_id);
    if (resource == NULL)
        return false;
    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, resource, width, height, VK_FORMAT_R32_SFLOAT, out_id);
}


/**
 * Prepare graph-declared color targets for one render pass.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param pass graph pass descriptor
 * @param cfg optional frame-plan emit configuration
 * @param out output runtime target map
 * @return whether all non-external color targets were prepared
 */
static bool _graph_prepare_render_color_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFrameGraphPass* pass, const DvzFramePlanEmitConfig* cfg,
    SceneGraphRuntimeTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(out);
    if (pass == NULL)
        return true;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    for (uint32_t i = 0; ok && i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphResource* resource =
            _graph_resource_by_id(plan, pass->color_attachments[i].resource_id);
        if (resource == NULL || resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET)
            continue;
        uint64_t texture_id = 0;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height, VK_FORMAT_R8G8B8A8_UNORM,
            &texture_id);
        ok = ok && _graph_runtime_targets_add(out, resource->id, texture_id);
    }
    return ok;
}



/**
 * Resolve or create one runtime 2D texture.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param key persistent resource key.
 * @param width texture width in pixels.
 * @param height texture height in pixels.
 * @param format Vulkan texture format.
 * @param usage DRP2 texture usage flags.
 * @param out_id output texture id.
 * @return whether the texture id is available.
 */
static bool _runtime_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const char* key, uint32_t width,
    uint32_t height, uint32_t format, uint32_t usage, uint32_t sample_count, uint64_t* out_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(key);
    ANN(out_id);

    bool is_new = false;
    ResourceId* resource = _resource_entry(&emitter->resources, key, &is_new);
    if (resource == NULL)
        return false;
    if (width == 0 || height == 0)
        return false;
    if (
        is_new || resource->texture_width == 0 || resource->texture_height == 0 ||
        resource->texture_depth == 0)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        resource->texture_sample_count = sample_count;
        is_new = true;
    }
    else if (
        width != resource->texture_width || height != resource->texture_height ||
        resource->texture_depth != 1 || format != resource->texture_format ||
        sample_count != resource->texture_sample_count)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = 1;
        resource->texture_format = format;
        resource->texture_sample_count = sample_count;
        is_new = true;
    }

    if (is_new)
    {
        if (!dvz_drp2_stream_create_texture_2d_format_usage_samples(
                stream, resource->id, width, height, format, usage, sample_count))
            return false;
    }
    *out_id = resource->id;
    return true;
}



/**
 * Build a runtime resource key scoped to the current frame target when requested.
 *
 * @param cfg optional emission configuration.
 * @param base_key unscoped runtime resource key.
 * @param out_key output key buffer.
 * @param out_size output key buffer size.
 */
static void _runtime_scope_key(
    const DvzFramePlanEmitConfig* cfg, const char* base_key, char* out_key, size_t out_size)
{
    ANN(base_key);
    ANN(out_key);

    if (out_size == 0)
        return;
    if (cfg != NULL && cfg->runtime_resource_scope_id != 0)
    {
        dvz_snprintf(
            out_key, out_size, "%s_scope_%016" PRIx64, base_key,
            cfg->runtime_resource_scope_id);
        return;
    }
    dvz_strlcpy(out_key, base_key, out_size);
}



/**
 * Resolve or create one graph-declared 2D texture resource.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan carrying access declarations.
 * @param cfg optional emission configuration with runtime resource scope.
 * @param resource graph resource descriptor.
 * @param width texture width in pixels.
 * @param height texture height in pixels.
 * @param fallback_format fallback Vulkan format when the graph format is zero.
 * @param out_id output texture id.
 * @return whether the texture id is available.
 */
static bool _graph_resolve_texture_2d(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanEmitConfig* cfg, const DvzFrameGraphResource* resource, uint32_t width,
    uint32_t height, uint32_t fallback_format, uint64_t* out_id)
{
    ANN(resource);
    uint32_t format = resource->format != 0 ? resource->format : fallback_format;
    uint32_t usage = _graph_texture_usage_to_drp2(resource->usage_flags);
    if (plan != NULL)
        usage |= _graph_declared_texture_usage_to_drp2(plan, resource->id);

    char key[DVZ_SCENE_LABEL_SIZE];
    _runtime_scope_key(cfg, resource->id, key, sizeof(key));
    return _runtime_resolve_texture_2d(
        emitter, stream, key, width, height, format, usage,
        _graph_resource_lowered_sample_count(emitter, resource), out_id);
}



/**
 * Resolve the named graph depth texture for a render node.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param render render node.
 * @param cfg optional frame-plan emit configuration.
 * @param graph_pass output graph pass descriptor, or NULL.
 * @param out_depth_id output depth texture id, or zero when no graph depth exists.
 * @return whether graph depth resolution succeeded.
 */
static bool _graph_resolve_render_depth(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg,
    const DvzFrameGraphPass** graph_pass, uint64_t* out_depth_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(graph_pass);
    ANN(out_depth_id);

    *graph_pass = _graph_pass_for_render(plan, render);
    *out_depth_id = 0;
    if (*graph_pass == NULL || !(*graph_pass)->has_depth_attachment)
        return true;

    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, (*graph_pass)->depth_attachment.resource_id);
    if (depth_resource == NULL)
        return true;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);
    return _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, depth_resource, width, height, VK_FORMAT_D32_SFLOAT,
        out_depth_id);
}



/**
 * Prepare graph-declared G-buffer targets for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the G-buffer render node
 * @param cfg optional frame-plan emit configuration
 * @param out output G-buffer target ids
 * @return whether all declared targets were prepared
 */
static bool _emitter_prepare_gbuffer_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg, SceneGBufferTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(out);

    const DvzFrameGraphPass* pass = _graph_pass_for_render(plan, render);
    if (pass == NULL)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    for (uint32_t i = 0; ok && i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        const DvzFrameGraphResource* resource =
            _graph_resource_by_id(plan, attachment->resource_id);
        if (resource == NULL)
            return false;
        uint64_t texture_id = 0;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height,
            VK_FORMAT_R16G16B16A16_SFLOAT, &texture_id);
        ok = ok && _graph_runtime_targets_add(&out->graph, resource->id, texture_id);
        if (ok && i == 0)
            out->normal_id = texture_id;
        else if (ok && i == 1)
            out->object_id = texture_id;
    }

    if (ok && pass->has_depth_attachment)
    {
        const DvzFrameGraphResource* resource =
            _graph_resource_by_id(plan, pass->depth_attachment.resource_id);
        if (resource == NULL)
            return false;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height, VK_FORMAT_D32_SFLOAT,
            &out->depth_id);
        ok = ok && _graph_runtime_targets_add(&out->graph, resource->id, out->depth_id);
    }

    return ok && out->normal_id != 0;
}



/**
 * Return a compact fingerprint for an EDL sampled bind group dependency set.
 *
 * @param color_id scene color texture id
 * @param depth_id scene depth texture id
 * @param sampler_id sampler id
 * @param params_id EDL uniform buffer id
 * @return dependency fingerprint
 */
static uint64_t _edl_bind_group_fingerprint(
    uint64_t color_id, uint64_t depth_id, uint64_t sampler_id, uint64_t params_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ color_id) * UINT64_C(1099511628211);
    hash = (hash ^ depth_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    hash = (hash ^ params_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare graph-declared EDL targets and resolve resources for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the EDL resolve render node
 * @param cfg optional frame-plan emit configuration
 * @param out output EDL target ids
 * @return whether all declared targets and resolve resources were prepared
 */
static bool _emitter_prepare_edl_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg, SceneEdlTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(out);

    const DvzFrameGraphPass* pass = _graph_pass_for_render(plan, render);
    if (pass == NULL || pass->read_count < 2)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    const DvzFrameGraphResource* color_resource =
        _graph_resource_by_id(plan, pass->reads[0].resource_id);
    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, pass->reads[1].resource_id);
    if (color_resource == NULL || depth_resource == NULL)
        return false;

    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, color_resource, width, height, VK_FORMAT_R8G8B8A8_UNORM,
        &out->color_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, depth_resource, width, height,
                   VK_FORMAT_D32_SFLOAT, &out->depth_id);
    ok = ok && _graph_runtime_targets_add(&out->graph, color_resource->id, out->color_id);
    ok = ok && _graph_runtime_targets_add(&out->graph, depth_resource->id, out->depth_id);
    if (!ok)
        return false;

    char params_key[DVZ_SCENE_LABEL_SIZE];
    char scoped_params_key[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(params_key, sizeof(params_key), "%s.edl.params", render->u.render.panel_id);
    _runtime_scope_key(cfg, params_key, scoped_params_key, sizeof(scoped_params_key));
    ResourceId* params = _resource_find(&emitter->resources, scoped_params_key);
    if (params == NULL)
        params = _resource_find(&emitter->resources, params_key);
    if (params == NULL || params->id == 0 || params->byte_size < sizeof(DvzSceneEdlUniform))
        return false;
    out->params_id = params->id;

    bool is_new = false;
    out->sampler_id = _obj_id(emitter, "_sampler_edl", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->resolve_bgl_id = _obj_id(emitter, "_bgl_edl_resolve", &is_new);
    if (out->resolve_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
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
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->resolve_bgl_id, 4, entries);
    }

    char bg_key[96];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_edl_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
        out->color_id, out->depth_id, out->params_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->resolve_bg_id = bg_resource->id;
    uint64_t fingerprint =
        _edl_bind_group_fingerprint(out->color_id, out->depth_id, out->sampler_id, out->params_id);
    if (!is_new && bg_resource->byte_size != fingerprint)
        is_new = true;
    bg_resource->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t color_id =
            _graph_sampled_read_texture_id(pass, 0, 0, &out->graph, out->color_id);
        uint64_t depth_id =
            _graph_sampled_read_texture_id(pass, 1, 0, &out->graph, out->depth_id);
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = color_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = depth_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = out->params_id,
                .offset = 0,
                .size = sizeof(DvzSceneEdlUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->resolve_bg_id, out->resolve_bgl_id, 4, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[40];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_edl_resolve%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_edl_resolve%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_edl_resolve%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_EDL_RESOLVE, false),
                       cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "edl_resolve_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_EDL_RESOLVE, true),
                       cfg);

    out->resolve_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->resolve_pipeline_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, out->resolve_pipeline_id, vs_id, fs_id, 0, out->resolve_bgl_id);
    return ok;
}



/**
 * Return a compact fingerprint for an SSAO sampled bind group dependency set.
 *
 * @param first_id first sampled texture id
 * @param second_id second sampled texture id
 * @param third_id third sampled texture id, or zero
 * @param sampler_id sampler id
 * @param params_id SSAO uniform buffer id, or zero for composite
 * @return dependency fingerprint
 */
static uint64_t _ssao_bind_group_fingerprint(
    uint64_t first_id, uint64_t second_id, uint64_t third_id, uint64_t sampler_id,
    uint64_t params_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ first_id) * UINT64_C(1099511628211);
    hash = (hash ^ second_id) * UINT64_C(1099511628211);
    hash = (hash ^ third_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    hash = (hash ^ params_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare graph-declared SSAO targets and fullscreen resources for one panel.
 *
 * @param emitter the persistent emitter
 * @param stream destination DRP2 command stream
 * @param plan the FramePlan
 * @param render the SSAO render node
 * @param cfg optional frame-plan emit configuration
 * @param out output SSAO target ids
 * @return whether all declared targets and fullscreen resources were prepared
 */
static bool _emitter_prepare_ssao_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanEmitConfig* cfg, SceneSsaoTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(out);

    const DvzFrameGraphPass* pass = _graph_pass_for_render(plan, render);
    const DvzFrameGraphPass* blur_pass =
        _graph_pass_by_panel_work(plan, render->u.render.panel_id, "ssao_blur");
    const DvzFrameGraphPass* composite_pass =
        _graph_pass_by_panel_work(plan, render->u.render.panel_id, "ssao_composite");
    if (pass == NULL || composite_pass == NULL || pass->read_count < 2 ||
        pass->color_attachment_count < 1 || composite_pass->read_count < 1)
        return false;

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    const DvzFrameGraphResource* normal_resource =
        _graph_resource_by_id(plan, pass->reads[0].resource_id);
    const DvzFrameGraphResource* depth_resource =
        _graph_resource_by_id(plan, pass->reads[1].resource_id);
    const DvzFrameGraphResource* occlusion_resource =
        _graph_resource_by_id(plan, pass->color_attachments[0].resource_id);
    if (normal_resource == NULL || depth_resource == NULL || occlusion_resource == NULL)
        return false;
    const DvzFrameGraphResource* blur_resource =
        blur_pass != NULL && blur_pass->color_attachment_count > 0
            ? _graph_resource_by_id(plan, blur_pass->color_attachments[0].resource_id)
            : NULL;
    if (blur_pass != NULL && (blur_pass->read_count < 3 || blur_resource == NULL))
        return false;

    bool ok = _graph_resolve_texture_2d(
        emitter, stream, plan, cfg, normal_resource, width, height,
        VK_FORMAT_R16G16B16A16_SFLOAT, &out->normal_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, depth_resource, width, height,
                   VK_FORMAT_D32_SFLOAT, &out->depth_id);
    ok = ok && _graph_resolve_texture_2d(
                   emitter, stream, plan, cfg, occlusion_resource, width, height,
                   VK_FORMAT_R8_UNORM, &out->occlusion_id);
    ok = ok && _graph_runtime_targets_add(&out->graph, normal_resource->id, out->normal_id);
    ok = ok && _graph_runtime_targets_add(&out->graph, depth_resource->id, out->depth_id);
    ok = ok &&
         _graph_runtime_targets_add(&out->graph, occlusion_resource->id, out->occlusion_id);
    if (blur_resource != NULL)
    {
        ok = ok && _graph_resolve_texture_2d(
                       emitter, stream, plan, cfg, blur_resource, width, height,
                       VK_FORMAT_R8_UNORM, &out->blur_id);
        ok = ok && _graph_runtime_targets_add(&out->graph, blur_resource->id, out->blur_id);
    }
    if (!ok)
        return false;
    out->composite_input_id = blur_resource != NULL ? out->blur_id : out->occlusion_id;

    char params_key[DVZ_SCENE_LABEL_SIZE];
    char scoped_params_key[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(params_key, sizeof(params_key), "%s.ssao.params", render->u.render.panel_id);
    _runtime_scope_key(cfg, params_key, scoped_params_key, sizeof(scoped_params_key));
    ResourceId* params = _resource_find(&emitter->resources, scoped_params_key);
    if (params == NULL)
        params = _resource_find(&emitter->resources, params_key);
    if (params == NULL || params->id == 0 || params->byte_size < sizeof(DvzSceneSsaoUniform))
        return false;
    out->params_id = params->id;

    bool is_new = false;
    out->sampler_id = _obj_id(emitter, "_sampler_ssao", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->ssao_bgl_id = _obj_id(emitter, "_bgl_ssao", &is_new);
    if (out->ssao_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
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
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok &&
             dvz_drp2_stream_create_bind_group_layout_entries(
                 stream, out->ssao_bgl_id, 4, entries);
    }

    char bg_key[112];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_ssao_%" PRIu64 "_%" PRIu64 "_%" PRIu64,
        out->normal_id, out->depth_id, out->params_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->ssao_bg_id = bg_resource->id;
    uint64_t fingerprint = _ssao_bind_group_fingerprint(
        out->normal_id, out->depth_id, 0, out->sampler_id, out->params_id);
    if (!is_new && bg_resource->byte_size != fingerprint)
        is_new = true;
    bg_resource->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t normal_id =
            _graph_sampled_read_texture_id(pass, 0, 0, &out->graph, out->normal_id);
        uint64_t depth_id =
            _graph_sampled_read_texture_id(pass, 1, 0, &out->graph, out->depth_id);
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = normal_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = depth_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = out->params_id,
                .offset = 0,
                .size = sizeof(DvzSceneSsaoUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->ssao_bg_id, out->ssao_bgl_id, 4, entries);
    }

    if (blur_pass != NULL)
    {
        out->blur_bgl_id = _obj_id(emitter, "_bgl_ssao_blur", &is_new);
        if (out->blur_bgl_id == 0)
            return false;
        if (ok && is_new)
        {
            DvzDrp2BindGroupLayoutEntry entries[5] = {
                {
                    .binding = 0,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
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
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 3,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
                {
                    .binding = 4,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                    .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                    .access = DVZ_DRP2_BINDING_ACCESS_READ,
                },
            };
            ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                           stream, out->blur_bgl_id, 5, entries);
        }

        dvz_snprintf(
            bg_key, sizeof(bg_key), "_bg_ssao_blur_%" PRIu64 "_%" PRIu64 "_%" PRIu64 "_%"
                                      PRIu64,
            out->occlusion_id, out->normal_id, out->depth_id, out->params_id);
        ResourceId* blur_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
        if (blur_bg == NULL || blur_bg->id == 0)
            return false;
        out->blur_bg_id = blur_bg->id;
        fingerprint = _ssao_bind_group_fingerprint(
            out->occlusion_id, out->normal_id, out->depth_id, out->sampler_id, out->params_id);
        if (!is_new && blur_bg->byte_size != fingerprint)
            is_new = true;
        blur_bg->byte_size = fingerprint;
        if (ok && is_new)
        {
            uint64_t occlusion_id =
                _graph_sampled_read_texture_id(blur_pass, 0, 0, &out->graph, out->occlusion_id);
            uint64_t normal_id =
                _graph_sampled_read_texture_id(blur_pass, 1, 0, &out->graph, out->normal_id);
            uint64_t depth_id =
                _graph_sampled_read_texture_id(blur_pass, 2, 0, &out->graph, out->depth_id);
            DvzDrp2BindGroupEntry entries[5] = {
                {
                    .binding = 0,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = occlusion_id,
                },
                {
                    .binding = 1,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = normal_id,
                },
                {
                    .binding = 2,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                    .resource_id = depth_id,
                },
                {
                    .binding = 3,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                    .resource_id = out->sampler_id,
                },
                {
                    .binding = 4,
                    .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                    .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                    .resource_id = out->params_id,
                    .offset = 0,
                    .size = sizeof(DvzSceneSsaoUniform),
                },
            };
            ok = ok && dvz_drp2_stream_create_bind_group_entries(
                           stream, out->blur_bg_id, out->blur_bgl_id, 5, entries);
        }
    }

    out->composite_bgl_id = _obj_id(emitter, "_bgl_ssao_composite", &is_new);
    if (out->composite_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->composite_bgl_id, 3, entries);
    }

    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_ssao_composite_%" PRIu64 "_%" PRIu64,
        out->composite_input_id, out->params_id);
    ResourceId* composite_bg = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (composite_bg == NULL || composite_bg->id == 0)
        return false;
    out->composite_bg_id = composite_bg->id;
    fingerprint =
        _ssao_bind_group_fingerprint(out->composite_input_id, 0, 0, out->sampler_id,
                                     out->params_id);
    if (!is_new && composite_bg->byte_size != fingerprint)
        is_new = true;
    composite_bg->byte_size = fingerprint;
    if (ok && is_new)
    {
        uint64_t composite_input_id = _graph_sampled_read_texture_id(
            composite_pass, 0, 0, &out->graph, out->composite_input_id);
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = composite_input_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
                .resource_id = out->params_id,
                .offset = 0,
                .size = sizeof(DvzSceneSsaoUniform),
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_entries(
                       stream, out->composite_bg_id, out->composite_bgl_id, 3, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[40];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_ssao%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_ssao%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_ssao%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO, false), cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "ssao_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO, true), cfg);

    out->ssao_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->ssao_pipeline_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, out->ssao_pipeline_id, vs_id, fs_id, 0, out->ssao_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_target(stream, 0, VK_FORMAT_R8_UNORM);

    if (blur_pass != NULL)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_ssao_blur%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_ssao_blur%s", fmt);
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_ssao_blur%s", fmt);
        vs_id = _obj_id(emitter, vs_key, &is_new);
        if (vs_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(
                           stream, vs_id, "VERTEX", "fullscreen_vert",
                           _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_BLUR, false),
                           cfg);

        fs_id = _obj_id(emitter, fs_key, &is_new);
        if (fs_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && _emit_shader_spirv(
                           stream, fs_id, "FRAGMENT", "ssao_blur_frag",
                           _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_BLUR, true),
                           cfg);

        out->blur_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
        if (out->blur_pipeline_id == 0)
            return false;
        if (ok && is_new)
            ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                           stream, out->blur_pipeline_id, vs_id, fs_id, 0, out->blur_bgl_id) &&
                 dvz_drp2_stream_pipeline_set_color_target(stream, 0, VK_FORMAT_R8_UNORM);
    }

    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_ssao_comp%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_ssao_comp%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_ssao_comp%s", fmt);
    vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_COMPOSITE, false),
                       cfg);

    fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "ssao_composite_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SSAO_COMPOSITE, true), cfg);

    out->composite_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->composite_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, out->composite_pipeline_id, vs_id, fs_id, 0,
                       out->composite_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD,
                 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                     VK_COLOR_COMPONENT_A_BIT);
    }
    return ok;
}



/**
 * Return a compact fingerprint for a WBOIT resolve bind group dependency set.
 *
 * @param accum_id accumulation texture id.
 * @param weight_id weight texture id.
 * @param sampler_id sampler id.
 * @return dependency fingerprint.
 */
static uint64_t _wboit_bind_group_fingerprint(
    uint64_t accum_id, uint64_t weight_id, uint64_t sampler_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ accum_id) * UINT64_C(1099511628211);
    hash = (hash ^ weight_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Prepare WBOIT intermediate targets and resolve pipeline resources for one panel.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param render transparent accumulation render node.
 * @param color_id final color target id.
 * @param cfg optional frame-plan emit configuration.
 * @param out output WBOIT target ids.
 * @return whether all resources were prepared.
 */
static bool _emitter_prepare_wboit_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneWboitTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(render);
    ANN(out);

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    bool is_new = false;
    out->color_id = color_id;

    char accum_key[DVZ_SCENE_LABEL_SIZE];
    char weight_key[DVZ_SCENE_LABEL_SIZE];
    char scoped_accum_key[DVZ_SCENE_LABEL_SIZE];
    char scoped_weight_key[DVZ_SCENE_LABEL_SIZE];
    dvz_snprintf(accum_key, sizeof(accum_key), "_wboit_accum_%s", render->u.render.panel_id);
    dvz_snprintf(weight_key, sizeof(weight_key), "_wboit_weight_%s", render->u.render.panel_id);
    _runtime_scope_key(cfg, accum_key, scoped_accum_key, sizeof(scoped_accum_key));
    _runtime_scope_key(cfg, weight_key, scoped_weight_key, sizeof(scoped_weight_key));

    const DvzFrameGraphPass* graph_pass = _graph_pass_for_render(plan, render);
    const DvzFrameGraphResource* accum_resource = NULL;
    const DvzFrameGraphResource* weight_resource = NULL;
    const DvzFrameGraphResource* depth_resource = NULL;
    if (graph_pass != NULL && graph_pass->color_attachment_count >= 2)
    {
        accum_resource = _graph_resource_by_id(
            plan, graph_pass->color_attachments[0].resource_id);
        weight_resource = _graph_resource_by_id(
            plan, graph_pass->color_attachments[1].resource_id);
        if (graph_pass->has_depth_attachment)
            depth_resource = _graph_resource_by_id(plan, graph_pass->depth_attachment.resource_id);
    }

    uint32_t fallback_usage =
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING;

    ok = ok && (accum_resource != NULL
                    ? _graph_resolve_texture_2d(
                          emitter, stream, plan, cfg, accum_resource, width, height,
                          VK_FORMAT_R16G16B16A16_SFLOAT, &out->accum_id)
                    : _runtime_resolve_texture_2d(
                          emitter, stream, scoped_accum_key, width, height,
                          VK_FORMAT_R16G16B16A16_SFLOAT, fallback_usage, 1, &out->accum_id));
    ok = ok && (weight_resource != NULL
                    ? _graph_resolve_texture_2d(
                          emitter, stream, plan, cfg, weight_resource, width, height,
                          VK_FORMAT_R16_SFLOAT, &out->weight_id)
                    : _runtime_resolve_texture_2d(
                          emitter, stream, scoped_weight_key, width, height, VK_FORMAT_R16_SFLOAT,
                          fallback_usage, 1, &out->weight_id));
    if (!ok)
        return false;
    if (depth_resource != NULL)
    {
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, depth_resource, width, height, VK_FORMAT_D32_SFLOAT,
            &out->depth_id);
    }
    if (!ok)
        return false;
    ok = ok &&
         (accum_resource == NULL ||
          _graph_runtime_targets_add(&out->graph, accum_resource->id, out->accum_id));
    ok = ok &&
         (weight_resource == NULL ||
          _graph_runtime_targets_add(&out->graph, weight_resource->id, out->weight_id));
    ok = ok &&
         (depth_resource == NULL ||
          _graph_runtime_targets_add(&out->graph, depth_resource->id, out->depth_id));
    if (!ok)
        return false;

    out->sampler_id = _obj_id(emitter, "_sampler_wboit", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->resolve_bgl_id = _obj_id(emitter, "_bgl_wboit_resolve", &is_new);
    if (out->resolve_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
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
        ok = ok &&
             dvz_drp2_stream_create_bind_group_layout_entries(
                 stream, out->resolve_bgl_id, 3, entries);
    }

    char bg_key[96];
    dvz_snprintf(
        bg_key, sizeof(bg_key), "_bg_wboit_%" PRIu64 "_%" PRIu64, out->accum_id,
        out->weight_id);
    ResourceId* bg_resource = _resource_entry(&emitter->objects, bg_key, &is_new);
    if (bg_resource == NULL || bg_resource->id == 0)
        return false;
    out->resolve_bg_id = bg_resource->id;

    uint64_t bg_fingerprint =
        _wboit_bind_group_fingerprint(out->accum_id, out->weight_id, out->sampler_id);
    if (!is_new && bg_resource->byte_size != bg_fingerprint)
        is_new = true;
    bg_resource->byte_size = bg_fingerprint;
    if (ok && is_new)
    {
        const DvzFrameGraphPass* resolve_graph_pass =
            _graph_pass_by_panel_work(plan, render->u.render.panel_id, "wboit_resolve");
        uint64_t accum_id = _graph_sampled_read_texture_id(
            resolve_graph_pass, 0, out->color_id, &out->graph, out->accum_id);
        uint64_t weight_id = _graph_sampled_read_texture_id(
            resolve_graph_pass, 1, out->color_id, &out->graph, out->weight_id);
        DvzDrp2BindGroupEntry entries[3] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = accum_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = weight_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = out->sampler_id,
            },
        };
        ok = ok &&
             dvz_drp2_stream_create_bind_group_entries(
                 stream, out->resolve_bg_id, out->resolve_bgl_id, 3, entries);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[32];
    char fs_key[32];
    char pipe_key[48];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_wboit_resolve%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_wboit_resolve%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_wboit_resolve%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", NULL,
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, false),
                       cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", NULL,
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_WBOIT_RESOLVE, true),
                       cfg);

    out->resolve_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->resolve_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok &&
             dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                 stream, out->resolve_pipeline_id, vs_id, fs_id, 0, out->resolve_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD,
                 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                     VK_COLOR_COMPONENT_A_BIT);
    }
    return ok;
}



/**
 * Return a compact fingerprint for a depth-peel sampled bind group dependency set.
 *
 * @param front_id front accumulation texture id.
 * @param back_id back accumulation texture id.
 * @param depth_id depth pair texture id.
 * @param sampler_id sampler id.
 * @return dependency fingerprint.
 */
static uint64_t _depth_peel_bind_group_fingerprint(
    uint64_t front_id, uint64_t back_id, uint64_t depth_id, uint64_t sampler_id)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = (hash ^ front_id) * UINT64_C(1099511628211);
    hash = (hash ^ back_id) * UINT64_C(1099511628211);
    hash = (hash ^ depth_id) * UINT64_C(1099511628211);
    hash = (hash ^ sampler_id) * UINT64_C(1099511628211);
    return hash != 0 ? hash : UINT64_C(1);
}



/**
 * Resolve one sampled bind group for a depth-peeling graph pass.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param pass graph pass whose reads are sampled.
 * @param targets runtime graph target map.
 * @param key bind group cache key.
 * @param bgl_id sampled bind group layout id.
 * @param sampler_id sampler id.
 * @param out_bg_id output bind group id.
 * @return whether the bind group is available.
 */
static bool _depth_peel_resolve_sampled_bind_group(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFrameGraphPass* pass,
    const SceneGraphRuntimeTargets* targets, const char* key, uint64_t bgl_id,
    uint64_t sampler_id, uint64_t* out_bg_id)
{
    ANN(emitter);
    ANN(stream);
    ANN(pass);
    ANN(targets);
    ANN(key);
    ANN(out_bg_id);
    if (pass->read_count < 3)
        return false;

    uint64_t front_id = _graph_runtime_targets_get(targets, pass->reads[0].resource_id);
    uint64_t back_id = _graph_runtime_targets_get(targets, pass->reads[1].resource_id);
    uint64_t depth_id = _graph_runtime_targets_get(targets, pass->reads[2].resource_id);
    if (front_id == 0 || back_id == 0 || depth_id == 0)
        return false;

    bool is_new = false;
    ResourceId* resource = _resource_entry(&emitter->objects, key, &is_new);
    if (resource == NULL || resource->id == 0)
        return false;
    uint64_t bg_id = resource->id;
    uint64_t fingerprint =
        _depth_peel_bind_group_fingerprint(front_id, back_id, depth_id, sampler_id);
    if (!is_new && resource->byte_size != fingerprint)
        is_new = true;
    resource->byte_size = fingerprint;
    if (is_new)
    {
        DvzDrp2BindGroupEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = front_id,
            },
            {
                .binding = 1,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = back_id,
            },
            {
                .binding = 2,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_TEXTURE,
                .resource_id = depth_id,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .resource_kind = DVZ_DRP2_BINDING_RESOURCE_SAMPLER,
                .resource_id = sampler_id,
            },
        };
        if (!dvz_drp2_stream_create_bind_group_entries(stream, bg_id, bgl_id, 4, entries))
            return false;
    }

    *out_bg_id = bg_id;
    return true;
}


/**
 * Prepare depth-peeling graph resources and composite state for one panel.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param render depth-peel init render node.
 * @param color_id final color target id.
 * @param cfg optional frame-plan emit configuration.
 * @param out output depth-peeling target ids.
 * @return whether all resources were prepared.
 */
static bool _emitter_prepare_depth_peel_targets(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, uint64_t color_id, const DvzFramePlanEmitConfig* cfg,
    SceneDepthPeelTargets* out)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);
    ANN(out);

    uint32_t width = 0;
    uint32_t height = 0;
    _emit_target_extent(cfg, &width, &height);

    bool ok = true;
    bool is_new = false;
    out->color_id = color_id;

    for (uint32_t i = 0; ok && i < dvz_frame_plan_graph_resource_count(plan); i++)
    {
        const DvzFrameGraphResource* resource = dvz_frame_plan_graph_resource_get(plan, i);
        if (resource == NULL || resource->kind == DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET)
            continue;
        size_t panel_id_len = strlen(render->u.render.panel_id);
        if (strncmp(resource->id, render->u.render.panel_id, panel_id_len) != 0)
            continue;

        uint64_t texture_id = 0;
        ok = _graph_resolve_texture_2d(
            emitter, stream, plan, cfg, resource, width, height,
            (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0
                ? VK_FORMAT_D32_SFLOAT
                : VK_FORMAT_R16G16B16A16_SFLOAT,
            &texture_id);
        ok = ok && _graph_runtime_targets_add(&out->graph, resource->id, texture_id);
        if (ok && (resource->usage_flags & DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT) != 0)
            out->depth_id = texture_id;
    }
    if (!ok)
        return false;

    out->sampler_id = _obj_id(emitter, "_sampler_depth_peel", &is_new);
    if (out->sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, out->sampler_id);

    out->sampled_bgl_id = _obj_id(emitter, "_bgl_depth_peel_sampled", &is_new);
    if (out->sampled_bgl_id == 0)
        return false;
    if (ok && is_new)
    {
        DvzDrp2BindGroupLayoutEntry entries[4] = {
            {
                .binding = 0,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
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
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
            {
                .binding = 3,
                .binding_type = DVZ_DRP2_BINDING_TYPE_SAMPLER,
                .visibility = DVZ_DRP2_SHADER_STAGE_FRAGMENT,
                .access = DVZ_DRP2_BINDING_ACCESS_READ,
            },
        };
        ok = ok && dvz_drp2_stream_create_bind_group_layout_entries(
                       stream, out->sampled_bgl_id, 4, entries);
    }

    const DvzFrameGraphPass* composite_pass =
        _graph_pass_by_panel_work(plan, render->u.render.panel_id, "depth_peel_composite");
    ok = ok && composite_pass != NULL;
    if (ok)
    {
        char composite_bg_key[DVZ_SCENE_LABEL_SIZE];
        _runtime_scope_key(
            cfg, "_bg_depth_peel_composite", composite_bg_key, sizeof(composite_bg_key));
        ok = ok && _depth_peel_resolve_sampled_bind_group(
            emitter, stream, composite_pass, &out->graph, composite_bg_key,
            out->sampled_bgl_id, out->sampler_id, &out->composite_bg_id);
    }

    const char* fmt = _shader_format_tag(cfg);
    char vs_key[40];
    char fs_key[40];
    char pipe_key[48];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_depth_peel_comp%s", fmt);
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_depth_peel_comp%s", fmt);
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_depth_peel_comp%s", fmt);

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, vs_id, "VERTEX", "fullscreen_vert",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_COMPOSITE, false),
                       cfg);

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader_spirv(
                       stream, fs_id, "FRAGMENT", "depth_peel_composite_frag",
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_COMPOSITE, true),
                       cfg);

    out->composite_pipeline_id = _obj_id(emitter, pipe_key, &is_new);
    if (out->composite_pipeline_id == 0)
        return false;
    if (ok && is_new)
    {
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, out->composite_pipeline_id, vs_id, fs_id, 0,
                       out->sampled_bgl_id) &&
             dvz_drp2_stream_pipeline_set_color_blend(
                 stream, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                 VK_BLEND_OP_ADD,
                 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                     VK_COLOR_COMPONENT_A_BIT);
    }
    return ok;
}



/**
 * Return G-buffer targets associated with a panel id.
 *
 * @param targets target array
 * @param renders render-node array parallel to targets
 * @param count target count
 * @param panel_id panel id to find
 * @return target entry, or NULL when absent
 */
static const SceneGBufferTargets* _gbuffer_targets_for_panel(
    const SceneGBufferTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return EDL targets associated with a panel id.
 *
 * @param targets target array
 * @param renders render-node array parallel to targets
 * @param count target count
 * @param panel_id panel id to find
 * @return target entry, or NULL when absent
 */
static const SceneEdlTargets* _edl_targets_for_panel(
    const SceneEdlTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return SSAO targets associated with a panel id.
 *
 * @param targets target array
 * @param renders render-node array parallel to targets
 * @param count target count
 * @param panel_id panel id to find
 * @return target entry, or NULL when absent
 */
static const SceneSsaoTargets* _ssao_targets_for_panel(
    const SceneSsaoTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return WBOIT targets associated with a panel id.
 *
 * @param targets target array.
 * @param renders render-node array parallel to targets.
 * @param count target count.
 * @param panel_id panel id to find.
 * @return target entry, or NULL when absent.
 */
static const SceneWboitTargets* _wboit_targets_for_panel(
    const SceneWboitTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}


/**
 * Return depth-peeling targets associated with a panel id.
 *
 * @param targets target array.
 * @param renders render-node array parallel to targets.
 * @param count target count.
 * @param panel_id panel id to find.
 * @return target entry, or NULL when absent.
 */
static const SceneDepthPeelTargets* _depth_peel_targets_for_panel(
    const SceneDepthPeelTargets* targets, const DvzFramePlanNode* const* renders, uint32_t count,
    const char* panel_id)
{
    ANN(targets);
    ANN(renders);
    ANN(panel_id);

    for (uint32_t i = 0; i < count; i++)
    {
        if (renders[i] != NULL && strcmp(renders[i]->u.render.panel_id, panel_id) == 0)
            return &targets[i];
    }
    return NULL;
}



/**
 * Return the prepared draw batch for a render node.
 *
 * @param batches prepared render batches.
 * @param count number of prepared render batches.
 * @param render render node.
 * @return the matching batch, or NULL when no draws were prepared.
 */
static const SceneRenderBatch* _render_batch_for_node(
    const SceneRenderBatch* batches, uint32_t count, const DvzFramePlanNode* render)
{
    ANN(render);
    for (uint32_t i = 0; i < count; i++)
    {
        if (batches[i].render == render)
            return &batches[i];
    }
    return NULL;
}



/**
 * Emit a WBOIT resolve pass into the final color target.
 *
 * @param stream destination DRP2 command stream.
 * @param render resolve render node.
 * @param render_pass_id active render-pass id.
 * @param targets WBOIT target ids.
 * @return whether all commands were emitted.
 */
static bool _emitter_emit_wboit_resolve(
    DvzDrp2CommandStream* stream, const DvzFramePlanNode* render, uint64_t render_pass_id,
    const SceneWboitTargets* targets)
{
    ANN(stream);
    ANN(render);
    ANN(targets);

    return dvz_drp2_stream_set_viewport(
               stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
               render->u.render.desc.width, render->u.render.desc.height) &&
           dvz_drp2_stream_set_scissor(
               stream, render_pass_id, render->u.render.desc.x, render->u.render.desc.y,
               render->u.render.desc.width, render->u.render.desc.height) &&
           dvz_drp2_stream_set_pipeline(stream, render_pass_id, targets->resolve_pipeline_id) &&
           dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, targets->resolve_bg_id) &&
           dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0);
}



/* Scene render path: one BeginRenderPass per panel, one Draw per visual inside it. */
static bool _emitter_emit_render_multi(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const DvzFramePlanNode* readback, bool clear,
    const DvzFramePlanEmitConfig* cfg, SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    bool needs_depth = _scene_render_needs_depth(emitter, render);
    const DvzFrameGraphPass* graph_pass = NULL;
    uint64_t graph_depth_id = 0;
    if (!_graph_resolve_render_depth(
            emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
        return false;
    uint64_t sampled_depth_id =
        graph_pass != NULL && graph_pass->has_depth_attachment &&
                graph_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
            ? graph_depth_id
            : 0;
    bool pass_has_depth_attachment =
        graph_pass != NULL && graph_pass->has_depth_attachment && graph_depth_id != 0;
    if (!pass_has_depth_attachment && needs_depth && graph_depth_id == 0)
        pass_has_depth_attachment = true;

    SceneRenderDraw draws[DVZ_SCENE_MAX_RENDER_VISUALS] = {0};
    uint32_t draw_count = 0;
    uint32_t pass_sample_count = _graph_render_pass_sample_count(emitter, plan, graph_pass);
    ok = _emitter_prepare_render_multi(
        emitter, stream, render, cfg, pass_has_depth_attachment, false, sampled_depth_id,
        false, 0, pass_sample_count, graph_pass != NULL && graph_pass->alpha_to_coverage,
        report, draws, &draw_count);
    if (!ok)
        return false;

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, clear);
    _label_render_pass_contract(stream, render_pass_id, render);
    ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, NULL);
    ok = ok && _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
    if (ok && needs_depth && graph_depth_id == 0)
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
    ok = ok &&
         _emitter_emit_render_multi_draws(
             stream, render, render_pass_id, draws, draw_count, cache) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    return ok;
}



/**
 * Emit all scene render nodes inside one figure-wide render pass.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param plan the FramePlan
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @param needs_depth whether the figure pass needs a transient depth attachment
 * @return whether the commands were emitted
 */
static bool _emitter_emit_scene_figure_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    bool needs_depth, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;

    SceneRenderBatch* batches =
        (SceneRenderBatch*)dvz_calloc(plan->count, sizeof(SceneRenderBatch));
    if (batches == NULL)
        return false;
    uint32_t batch_count = 0;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
            continue;
        SceneRenderBatch* batch = &batches[batch_count];
        batch->render = render;
        ok = _emitter_prepare_render_multi(
            emitter, stream, render, cfg, needs_depth, false, 0, false, 0, 1, false, report,
            batch->draws, &batch->draw_count);
        if (ok)
            batch_count++;
    }
    if (!ok || batch_count == 0)
    {
        dvz_free(batches);
        return false;
    }

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, 0.0f, 0.0f, 1.0f,
             1.0f, true);
    if (ok && needs_depth)
        ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);

    SceneRenderStateCache scene_cache = {0};
    for (uint32_t i = 0; ok && i < batch_count; i++)
    {
        ok = _emitter_emit_render_multi_draws(
            stream, batches[i].render, render_pass_id, batches[i].draws, batches[i].draw_count,
            &scene_cache);
    }

    ok = ok && dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    dvz_free(batches);
    return ok;
}


/**
 * Return whether the plan contains graph-backed render passes.
 *
 * @param plan the FramePlan.
 * @return whether graph-backed scene render nodes are present.
 */
static bool _plan_has_graph_render_passes(const DvzFramePlan* plan)
{
    ANN(plan);
    if (dvz_frame_plan_graph_pass_count(plan) > 0)
        return true;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* node = &plan->nodes[i];
        if (node->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER ||
            node->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE)
            return true;
    }
    return false;
}



/**
 * Emit scene render nodes with graph-backed technique passes.
 *
 * @param emitter the persistent emitter.
 * @param stream destination DRP2 command stream.
 * @param plan the FramePlan.
 * @param readback optional readback copy node.
 * @param cfg frame-plan emit configuration.
 * @param report diagnostic report receiving recoverable emission errors.
 * @return whether the commands were emitted.
 */
static bool _emitter_emit_scene_graph_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    SceneRenderBatch* batches =
        (SceneRenderBatch*)dvz_calloc(plan->count, sizeof(SceneRenderBatch));
    SceneGBufferTargets* gbuffer_targets =
        (SceneGBufferTargets*)dvz_calloc(plan->count, sizeof(SceneGBufferTargets));
    const DvzFramePlanNode** gbuffer_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneEdlTargets* edl_targets =
        (SceneEdlTargets*)dvz_calloc(plan->count, sizeof(SceneEdlTargets));
    const DvzFramePlanNode** edl_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneSsaoTargets* ssao_targets =
        (SceneSsaoTargets*)dvz_calloc(plan->count, sizeof(SceneSsaoTargets));
    const DvzFramePlanNode** ssao_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneWboitTargets* wboit_targets =
        (SceneWboitTargets*)dvz_calloc(plan->count, sizeof(SceneWboitTargets));
    const DvzFramePlanNode** wboit_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    SceneDepthPeelTargets* depth_peel_targets =
        (SceneDepthPeelTargets*)dvz_calloc(plan->count, sizeof(SceneDepthPeelTargets));
    const DvzFramePlanNode** depth_peel_renders =
        (const DvzFramePlanNode**)dvz_calloc(plan->count, sizeof(DvzFramePlanNode*));
    if (batches == NULL || gbuffer_targets == NULL || gbuffer_renders == NULL ||
        edl_targets == NULL || edl_renders == NULL || wboit_targets == NULL ||
        wboit_renders == NULL || ssao_targets == NULL || ssao_renders == NULL ||
        depth_peel_targets == NULL || depth_peel_renders == NULL)
    {
        dvz_free(depth_peel_renders);
        dvz_free(depth_peel_targets);
        dvz_free(ssao_renders);
        dvz_free(ssao_targets);
        dvz_free(wboit_renders);
        dvz_free(wboit_targets);
        dvz_free(edl_renders);
        dvz_free(edl_targets);
        dvz_free(gbuffer_renders);
        dvz_free(gbuffer_targets);
        dvz_free(batches);
        return false;
    }

    bool ok = true;
    uint32_t batch_count = 0;
    uint32_t gbuffer_target_count = 0;
    uint32_t edl_target_count = 0;
    uint32_t ssao_target_count = 0;
    uint32_t target_count = 0;
    uint32_t depth_target_count = 0;
    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE)
        {
            if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO)
            {
                ok = _emitter_prepare_ssao_targets(
                    emitter, stream, plan, render, cfg, &ssao_targets[ssao_target_count]);
                if (ok)
                    ssao_renders[ssao_target_count++] = render;
            }
            else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE)
            {
                ok = _emitter_prepare_edl_targets(
                    emitter, stream, plan, render, cfg, &edl_targets[edl_target_count]);
                if (ok)
                    edl_renders[edl_target_count++] = render;
            }
            continue;
        }

        const DvzFrameGraphPass* render_graph_pass = NULL;
        uint64_t render_graph_depth_id = 0;
        ok = _graph_resolve_render_depth(
            emitter, stream, plan, render, cfg, &render_graph_pass, &render_graph_depth_id);
        if (!ok)
            break;
        bool pass_has_depth_attachment =
            render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
            render_graph_depth_id != 0;
        bool depth_peel_render =
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
        bool transient_depth_allowed =
            render->u.render.pass_role != DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION;
        if (!depth_peel_render && transient_depth_allowed && !pass_has_depth_attachment &&
            _scene_render_needs_depth(emitter, render))
            pass_has_depth_attachment = true;
        uint64_t sampled_depth_id = 0;
        if (render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
            render_graph_pass->depth_attachment.access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
            sampled_depth_id = render_graph_depth_id;
        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER)
        {
            ok = _emitter_prepare_gbuffer_targets(
                emitter, stream, plan, render, cfg, &gbuffer_targets[gbuffer_target_count]);
            if (ok)
                gbuffer_renders[gbuffer_target_count++] = render;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION)
        {
            ok = _emitter_prepare_wboit_targets(
                emitter, stream, plan, render, color_id, cfg, &wboit_targets[target_count]);
            if (ok)
            {
                sampled_depth_id = wboit_targets[target_count].depth_id;
                wboit_renders[target_count++] = render;
            }
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT)
        {
            ok = _emitter_prepare_depth_peel_targets(
                emitter, stream, plan, render, color_id, cfg,
                &depth_peel_targets[depth_target_count]);
            if (ok)
            {
                if (render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
                    render_graph_pass->depth_attachment.access ==
                        DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
                    sampled_depth_id = depth_peel_targets[depth_target_count].depth_id;
                depth_peel_renders[depth_target_count++] = render;
            }
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER)
        {
            const SceneDepthPeelTargets* targets = _depth_peel_targets_for_panel(
                depth_peel_targets, depth_peel_renders, depth_target_count,
                render->u.render.panel_id);
            if (targets != NULL && render_graph_pass != NULL &&
                render_graph_pass->has_depth_attachment &&
                render_graph_pass->depth_attachment.access ==
                    DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ)
                sampled_depth_id = targets->depth_id;
        }
        bool sampled_depth_is_volume_occlusion = false;
        uint64_t volume_occlusion_depth_id = 0;
        ok = _graph_resolve_volume_occlusion_read(
            emitter, stream, plan, cfg, render_graph_pass, &volume_occlusion_depth_id);
        if (!ok)
            break;
        if (volume_occlusion_depth_id != 0)
        {
            sampled_depth_id = volume_occlusion_depth_id;
            sampled_depth_is_volume_occlusion = true;
        }
        uint64_t scene_occlusion_depth_id = 0;
        ok = _graph_resolve_scene_occlusion_read(
            emitter, stream, plan, cfg, render_graph_pass, &scene_occlusion_depth_id);
        if (!ok)
            break;

        if (render->u.render.visual_count > 0)
        {
            SceneRenderBatch* batch = &batches[batch_count];
            batch->render = render;
            bool force_point_depth =
                render_graph_pass != NULL && render_graph_pass->has_depth_attachment &&
                _scene_resource_id_has_suffix(
                    render_graph_pass->depth_attachment.resource_id, ".edl.depth");
            ok = _emitter_prepare_render_multi(
                emitter, stream, render, cfg, pass_has_depth_attachment, force_point_depth,
                sampled_depth_id, sampled_depth_is_volume_occlusion, scene_occlusion_depth_id,
                _graph_render_pass_sample_count(emitter, plan, render_graph_pass),
                render_graph_pass != NULL && render_graph_pass->alpha_to_coverage, report,
                batch->draws, &batch->draw_count);
            if (ok)
                batch_count++;
        }
    }
    if (!ok)
    {
        dvz_free(depth_peel_renders);
        dvz_free(depth_peel_targets);
        dvz_free(ssao_renders);
        dvz_free(ssao_targets);
        dvz_free(wboit_renders);
        dvz_free(wboit_targets);
        dvz_free(edl_renders);
        dvz_free(edl_targets);
        dvz_free(gbuffer_renders);
        dvz_free(gbuffer_targets);
        dvz_free(batches);
        return false;
    }

    uint64_t encoder_id = _emitter_next_transient_id(emitter);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    bool clear_final = true;
    SceneRenderStateCache scene_cache = {0};

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id);
    bool use_graph_order = dvz_frame_plan_graph_pass_count(plan) > 0;
    uint32_t order_count = use_graph_order ? dvz_frame_plan_graph_pass_count(plan) : plan->count;
    for (uint32_t i = 0; ok && i < order_count; i++)
    {
        const DvzFrameGraphPass* ordered_graph_pass =
            use_graph_order ? dvz_frame_plan_graph_pass_get(plan, i) : NULL;
        const DvzFramePlanNode* render =
            use_graph_order ? _graph_render_for_pass(plan, ordered_graph_pass) : &plan->nodes[i];
        if (render == NULL || render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER)
        {
            const SceneGBufferTargets* targets =
                _gbuffer_targets_for_panel(
                    gbuffer_targets, gbuffer_renders, gbuffer_target_count,
                    render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->normal_id);
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, target_id, 0.5f, 0.5f, 1.0f, 0.0f,
                render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                render->u.render.desc.height, true);
            if (ok && graph_pass != NULL && graph_pass->color_attachment_count > 1)
            {
                uint64_t object_id = _graph_color_attachment_texture_id(
                    graph_pass, 1, color_id, &targets->graph, targets->object_id);
                ok = dvz_drp2_stream_begin_render_pass_add_color_attachment(
                    stream, object_id, 0.0f, 0.0f, 0.0f, 0.0f, true);
            }
            ok = ok && _stream_apply_graph_color_ops(
                           stream, graph_pass, color_id, &targets->graph);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
        }
        else if (
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION)
        {
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            SceneGraphRuntimeTargets graph_targets = {0};
            ok = _graph_prepare_render_color_targets(
                emitter, stream, plan, graph_pass, cfg, &graph_targets);
            if (!ok)
                break;
            uint64_t target_id =
                _graph_color_attachment_texture_id(graph_pass, 0, color_id, &graph_targets, 0);
            if (target_id == 0)
            {
                ok = false;
                break;
            }
            uint64_t graph_depth_id = 0;
            if (graph_pass != NULL && graph_pass->has_depth_attachment)
            {
                if (!_graph_resolve_render_depth(
                        emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
                {
                    ok = false;
                    break;
                }
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            bool scene_depth =
                render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, target_id, scene_depth ? 1.0f : 0.0f,
                scene_depth ? 1.0f : 0.0f, scene_depth ? 1.0f : 0.0f,
                scene_depth ? 1.0f : 0.0f,
                render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                render->u.render.desc.height, true);
            ok = ok && _stream_apply_graph_color_ops(stream, graph_pass, color_id, &graph_targets);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE)
        {
            const SceneEdlTargets* edl =
                _edl_targets_for_panel(
                    edl_targets, edl_renders, edl_target_count, render->u.render.panel_id);
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            const SceneDepthPeelTargets* depth_targets =
                _depth_peel_targets_for_panel(
                    depth_peel_targets, depth_peel_renders, depth_target_count,
                    render->u.render.panel_id);
            const SceneGraphRuntimeTargets* graph_targets =
                edl != NULL     ? &edl->graph :
                targets != NULL ? &targets->graph :
                depth_targets != NULL ? &depth_targets->graph :
                                        NULL;
            uint64_t graph_depth_id = edl != NULL     ? edl->depth_id :
                                      targets != NULL ? targets->depth_id :
                                      depth_targets != NULL ? depth_targets->depth_id :
                                                              0;
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            SceneGraphRuntimeTargets local_graph_targets = {0};
            if (ok && graph_targets == NULL)
            {
                ok = _graph_prepare_render_color_targets(
                    emitter, stream, plan, graph_pass, cfg, &local_graph_targets);
                if (!ok)
                    break;
                if (local_graph_targets.count > 0)
                    graph_targets = &local_graph_targets;
            }
            if (ok && graph_depth_id == 0 && graph_pass != NULL &&
                graph_pass->has_depth_attachment)
            {
                if (!_graph_resolve_render_depth(
                        emitter, stream, plan, render, cfg, &graph_pass, &graph_depth_id))
                {
                    ok = false;
                    break;
                }
            }
            uint64_t target_id =
                _graph_color_attachment_texture_id(
                    graph_pass, 0, color_id, graph_targets, color_id);
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = ok && dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, cr, cg, cb, ca, 0.0f, 0.0f,
                     1.0f, 1.0f, clear_final);
            ok = ok && _stream_apply_graph_color_ops(
                           stream, graph_pass, color_id, graph_targets);
            if (ok && graph_depth_id != 0)
                ok = _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
            if (ok && has_draws && graph_depth_id == 0 &&
                _scene_render_needs_depth(emitter, render))
                ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION)
        {
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t accum_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->accum_id);
            uint64_t weight_id = _graph_color_attachment_texture_id(
                graph_pass, 1, color_id, &targets->graph, targets->weight_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, accum_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, weight_id, 0.0f, 0.0f, 0.0f, 0.0f, true);
            ok = ok && _stream_apply_graph_color_ops(
                           stream, graph_pass, color_id, &targets->graph);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_BLEND)
        {
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            SceneGraphRuntimeTargets graph_targets = {0};
            ok = _graph_prepare_render_color_targets(
                emitter, stream, plan, graph_pass, cfg, &graph_targets);
            if (!ok)
                break;
            uint64_t graph_depth_id = 0;
            if (graph_pass != NULL && graph_pass->has_depth_attachment)
            {
                const DvzFrameGraphPass* depth_graph_pass = graph_pass;
                ok = _graph_resolve_render_depth(
                    emitter, stream, plan, render, cfg, &depth_graph_pass, &graph_depth_id);
                if (!ok)
                    break;
                graph_pass = depth_graph_pass;
            }
            uint64_t target_id =
                _graph_color_attachment_texture_id(
                    graph_pass, 0, color_id, &graph_targets, color_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                stream, pass_id, encoder_id, target_id, cr, cg, cb, ca,
                render->u.render.desc.x, render->u.render.desc.y, render->u.render.desc.width,
                render->u.render.desc.height, false);
            ok = ok && _stream_apply_graph_color_ops(
                           stream, graph_pass, color_id, &graph_targets);
            if (ok && graph_depth_id != 0)
                ok = _stream_apply_graph_depth(stream, graph_pass, graph_depth_id);
            else if (ok && has_draws && _scene_render_needs_depth(emitter, render))
                ok = dvz_drp2_stream_begin_render_pass_set_depth(stream, 1.0f);
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
            render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER)
        {
            const SceneDepthPeelTargets* targets =
                _depth_peel_targets_for_panel(
                    depth_peel_targets, depth_peel_renders, depth_target_count,
                    render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t first_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, color_id);
            uint64_t second_id = _graph_color_attachment_texture_id(
                graph_pass, 1, color_id, &targets->graph, color_id);
            uint64_t third_id = _graph_color_attachment_texture_id(
                graph_pass, 2, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, first_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, second_id, 0.0f, 0.0f, 0.0f, 0.0f, true) &&
                 dvz_drp2_stream_begin_render_pass_add_color_attachment(
                     stream, third_id, 0.0f, 0.0f, 0.0f, 0.0f, true);
            ok = ok && _stream_apply_graph_color_ops(
                           stream, graph_pass, color_id, &targets->graph);
            ok = ok && _stream_apply_graph_depth(stream, graph_pass, targets->depth_id);
            const SceneRenderBatch* batch = _render_batch_for_node(batches, batch_count, render);
            bool has_draws = batch != NULL;
            if (ok && has_draws)
            {
                scene_cache.pipeline_id = 0;
                scene_cache.bg_set0 = 0;
                ok = _emitter_emit_render_multi_draws(
                    stream, render, pass_id, batch->draws, batch->draw_count, &scene_cache);
            }
            ok = ok && dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE)
        {
            const SceneDepthPeelTargets* targets =
                _depth_peel_targets_for_panel(
                    depth_peel_targets, depth_peel_renders, depth_target_count,
                    render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id =
                _graph_color_attachment_texture_id(
                    graph_pass, 0, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_pipeline(
                     stream, pass_id, targets->composite_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(
                     stream, pass_id, 0, targets->composite_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO)
        {
            const SceneSsaoTargets* targets =
                _ssao_targets_for_panel(
                    ssao_targets, ssao_renders, ssao_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->occlusion_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 1.0f, 1.0f, 1.0f, 1.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, true) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->ssao_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->ssao_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR)
        {
            const SceneSsaoTargets* targets =
                _ssao_targets_for_panel(
                    ssao_targets, ssao_renders, ssao_target_count, render->u.render.panel_id);
            if (targets == NULL || targets->blur_id == 0 || targets->blur_pipeline_id == 0)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id = _graph_color_attachment_texture_id(
                graph_pass, 0, color_id, &targets->graph, targets->blur_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 1.0f, 1.0f, 1.0f, 1.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, true) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_pipeline(stream, pass_id, targets->blur_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(stream, pass_id, 0, targets->blur_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE)
        {
            const SceneSsaoTargets* targets =
                _ssao_targets_for_panel(
                    ssao_targets, ssao_renders, ssao_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id =
                _graph_color_attachment_texture_id(
                    graph_pass, 0, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_pipeline(
                     stream, pass_id, targets->composite_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(
                     stream, pass_id, 0, targets->composite_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_EDL_RESOLVE)
        {
            const SceneEdlTargets* targets =
                _edl_targets_for_panel(
                    edl_targets, edl_renders, edl_target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id =
                _graph_color_attachment_texture_id(
                    graph_pass, 0, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, cr, cg, cb, ca,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, true) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 dvz_drp2_stream_set_viewport(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_scissor(
                     stream, pass_id, render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height) &&
                 dvz_drp2_stream_set_pipeline(
                     stream, pass_id, targets->resolve_pipeline_id) &&
                 dvz_drp2_stream_set_bind_group(
                     stream, pass_id, 0, targets->resolve_bg_id) &&
                 dvz_drp2_stream_draw(stream, pass_id, 3, 1, 0, 0) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
            clear_final = false;
        }
        else if (render->u.render.pass_role == DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE)
        {
            const SceneWboitTargets* targets =
                _wboit_targets_for_panel(
                    wboit_targets, wboit_renders, target_count, render->u.render.panel_id);
            if (targets == NULL)
            {
                ok = false;
                break;
            }
            uint64_t pass_id = _emitter_next_transient_id(emitter);
            _label_render_pass_contract(stream, pass_id, render);
            const DvzFrameGraphPass* graph_pass = ordered_graph_pass != NULL
                                                      ? ordered_graph_pass
                                                      : _graph_pass_for_render(plan, render);
            uint64_t target_id =
                _graph_color_attachment_texture_id(
                    graph_pass, 0, color_id, &targets->graph, color_id);
            ok = dvz_drp2_stream_begin_render_pass_region_clear(
                     stream, pass_id, encoder_id, target_id, 0.0f, 0.0f, 0.0f, 0.0f,
                     render->u.render.desc.x, render->u.render.desc.y,
                     render->u.render.desc.width, render->u.render.desc.height, false) &&
                 _stream_apply_graph_color_ops(stream, graph_pass, color_id, &targets->graph) &&
                 _emitter_emit_wboit_resolve(stream, render, pass_id, targets) &&
                 dvz_drp2_stream_end_render_pass(stream, pass_id);
        }
    }

    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    dvz_free(depth_peel_renders);
    dvz_free(depth_peel_targets);
    dvz_free(ssao_renders);
    dvz_free(ssao_targets);
    dvz_free(wboit_renders);
    dvz_free(wboit_targets);
    dvz_free(edl_renders);
    dvz_free(edl_targets);
    dvz_free(gbuffer_renders);
    dvz_free(gbuffer_targets);
    dvz_free(batches);
    return ok;
}



/**
 * Emit runtime-mode static render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param vertex_buffer_ids the vertex buffer ids
 * @param vertex_buffer_count the vertex buffer count
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const DvzFramePlanNode* render, const uint64_t* vertex_buffer_ids, uint32_t vertex_buffer_count,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg,
    SceneRenderStateCache* cache, DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);
    ANN(render);

    /* Scene render node: per-visual multi-draw in a single pass. */
    if (vertex_buffer_count == 0 && render->u.render.visual_count > 0 &&
        cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        return _emitter_emit_render_multi(
            emitter, stream, plan, render, readback, clear, cfg, cache, report);

    /* Generic single-draw path (non-scene nodes, WGSL, or fallback). */
    ANN(vertex_buffer_ids);
    if (vertex_buffer_count == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    const DvzFramePlanVisualMeta* visual_meta = NULL;
    uint32_t visual_type = DVZ_VISUAL_TYPE_NONE;
    if (render->u.render.visual_count == 1 &&
        render->u.render.visual_metadata[0].has_metadata)
    {
        visual_meta = &render->u.render.visual_metadata[0];
        visual_type = visual_meta->visual_type;
    }

    /* Detect point-like visual data (position + color + size attributes). */
    bool is_point = _is_point_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    bool is_pixel = is_point && visual_type == DVZ_VISUAL_TYPE_PIXEL;
    bool is_marker = is_point && visual_type == DVZ_VISUAL_TYPE_MARKER;
    bool is_point_like = is_point;
    bool is_primitive =
        !is_point_like && _is_primitive_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count);
    uint64_t image_pos = 0, image_uv = 0, image_tex = 0;
    bool is_image = !is_point_like && !is_primitive &&
                    _is_image_visual(&emitter->resources, vertex_buffer_ids, vertex_buffer_count,
                                     &image_pos, &image_uv, &image_tex);

    const char* vs_glsl = NULL;
    const char* fs_glsl = NULL;
    const char* vs_wgsl = NULL;
    const char* fs_wgsl = NULL;
    uint32_t topology = 0;
    uint32_t vertex_count = 3; /* default for stub / non-point path */
    uint64_t bgl_id = 0;
    uint64_t bg_id  = 0;
    DvzScenePointLikeLoweringDesc point_like_lowering = {0};
    bool has_point_like_lowering = false;

    /* Common bind group IDs used for GLSL/WGSL point, primitive, and image paths. */
    uint64_t common_bgl_id = 0;
    uint64_t common_bg_id  = 0;
    bool uses_common =
        (is_point || is_primitive || is_image) &&
        cfg != NULL &&
        (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL ||
         (cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL &&
          (is_point || is_primitive || is_image)));

    /* When IMAGE: re-narrow vertex_buffer_ids to (position, texcoords) only — the texture
     * is bound through a bind group, not as a vertex buffer. */
    uint64_t image_vertex_ids[2];
    if (is_image)
    {
        image_vertex_ids[0] = image_pos;
        image_vertex_ids[1] = image_uv;
        vertex_buffer_ids   = image_vertex_ids;
        vertex_buffer_count = 2;
    }

    char vs_key[32];
    char fs_key[16];
    char pipe_key[48];

    if (is_point_like)
    {
        /* Point-like visuals: native points for GLSL, instanced quads for WGSL. */
        bool picking = render->u.render.picking;
        bool depth_cue =
            visual_meta != NULL && visual_meta->depth_cue_enabled && !picking;
        bool point_style =
            visual_meta != NULL && visual_meta->point_style_enabled && !is_pixel && !is_marker &&
            !picking;
        const char* suffix =
            picking ? "_pick" : point_style && depth_cue ? "_cue_style" :
            point_style ? "_style" : depth_cue ? "_cue" : "";

        DvzSceneBuiltinShader shader = DVZ_SCENE_BUILTIN_SHADER_POINT;
        if (is_marker)
            shader = picking ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK :
                               DVZ_SCENE_BUILTIN_SHADER_MARKER;
        else if (is_pixel)
            shader = picking ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK :
                     depth_cue ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_DEPTH_CUE :
                                 DVZ_SCENE_BUILTIN_SHADER_PIXEL;
        else if (picking)
            shader = DVZ_SCENE_BUILTIN_SHADER_POINT_PICK;
        else if (point_style)
            shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE_DEPTH_CUE :
                                 DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE;
        else
            shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_DEPTH_CUE :
                                 DVZ_SCENE_BUILTIN_SHADER_POINT;

        const char* key = is_marker ? "marker" : is_pixel ? "pixel" : "point";
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_%s%s%s", key, suffix, fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_%s%s%s", key, suffix, fmt);
        vs_glsl = _builtin_shader_glsl(shader, false);
        fs_glsl = _builtin_shader_glsl(shader, true);
        vs_wgsl = _builtin_shader_wgsl(shader, false);
        fs_wgsl = _builtin_shader_wgsl(shader, true);

        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
        }
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        DvzSceneShaderFormat shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL)
            shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
        has_point_like_lowering = _scene_point_like_lowering_desc(
            is_marker ? DVZ_SCENE_POINT_LIKE_MARKER :
            is_pixel ? DVZ_SCENE_POINT_LIKE_PIXEL : DVZ_SCENE_POINT_LIKE_POINT,
            shader_format, vertex_count, &point_like_lowering);
        if (!has_point_like_lowering)
            return false;
        topology = point_like_lowering.topology;
    }
    else if (is_primitive)
    {
        uint64_t pos_id = _scene_visual_resource_by_role(
            &emitter->resources, vertex_buffer_ids, vertex_buffer_count,
            DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_id != 0)
        {
            uint64_t sz = _resource_byte_size(&emitter->resources, pos_id);
            if (sz > 0)
                vertex_count = (uint32_t)(sz / (3 * sizeof(float)));
            topology = _resource_topology(&emitter->resources, pos_id);
        }
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        /* Primitive visual: pass-through shaders with visual-selected topology. */
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_prim%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_prim%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE, true);
    }
    else if (is_image)
    {
        /* Image visual: textured-quad shaders, TRIANGLE_STRIP topology, 4 vertices. */
        uint64_t pos_size = _resource_byte_size(&emitter->resources, image_pos);
        if (pos_size > 0)
            vertex_count = (uint32_t)(pos_size / (3 * sizeof(float)));
        if (visual_meta != NULL && visual_meta->vertex_count > 0)
            vertex_count = visual_meta->vertex_count;
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_img%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs_img%s", fmt);
        vs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        fs_glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);
        vs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, false);
        fs_wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_IMAGE, true);

        /* Sampler + texture-sampler bind-group layout + bind-group, all persistent. */
        bool bgl_new = false;
        bgl_id = _obj_id(emitter, "_bgl_img", &bgl_new);
        if (bgl_id == 0)
            return false;
        if (bgl_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

        bool sampler_new = false;
        uint64_t sampler_id = _obj_id(emitter, "_sampler_img", &sampler_new);
        if (sampler_id == 0)
            return false;
        if (ok && sampler_new)
            ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

        char bg_key[48];
        dvz_snprintf(bg_key, sizeof(bg_key), "_bg_img_%" PRIu64, image_tex);
        bool bg_new = false;
        bg_id = _obj_id(emitter, bg_key, &bg_new);
        if (bg_id == 0)
            return false;
        if (ok && bg_new)
            ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                           stream, bg_id, bgl_id, image_tex, sampler_id);
    }
    else if (cfg != NULL && cfg->fullscreen_triangle)
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs_full%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }
    else
    {
        dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
        dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    }

    /* Common bind group infrastructure. */
    if (uses_common)
    {
        ok = ok && _scene_common_bindings_resolve_single_set(
                       emitter, stream, render, &common_bgl_id, &common_bg_id);
        if (!ok)
            return false;
    }

    /* SPIR-V resource names (stem of .vert.spv / .frag.spv after embed_resources key mangling). */
    const char* vs_spirv_key = NULL;
    const char* fs_spirv_key = NULL;
    if (is_point_like)
    {
        bool picking = render->u.render.picking;
        bool depth_cue =
            visual_meta != NULL && visual_meta->depth_cue_enabled && !picking;
        bool point_style =
            visual_meta != NULL && visual_meta->point_style_enabled && !is_pixel && !is_marker &&
            !picking;
        if (picking)
        {
            vs_spirv_key = (is_pixel || is_marker) ? "pixel_pick_vert" : "point_pick_vert";
            fs_spirv_key = (is_pixel || is_marker) ? "pixel_pick_frag" : "point_pick_frag";
        }
        else if (is_marker)
        {
            vs_spirv_key = "marker_vert";
            fs_spirv_key = "marker_frag";
        }
        else if (is_pixel)
        {
            vs_spirv_key = depth_cue ? "pixel_cue_vert" : "pixel_vert";
            fs_spirv_key = depth_cue ? "pixel_cue_frag" : "pixel_frag";
        }
        else if (point_style)
        {
            vs_spirv_key = depth_cue ? "point_cue_style_vert" : "point_style_vert";
            fs_spirv_key = depth_cue ? "point_cue_style_frag" : "point_style_frag";
        }
        else
        {
            vs_spirv_key = depth_cue ? "point_cue_vert" : "point_vert";
            fs_spirv_key = depth_cue ? "point_cue_frag" : "point_frag";
        }
    }
    else if (is_primitive)
    {
        vs_spirv_key = "primitive_vert";
        fs_spirv_key = "primitive_frag";
    }
    else if (is_image)
    {
        vs_spirv_key = "image_vert";
        fs_spirv_key = "image_frag";
    }

    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (is_new)
    {
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && vs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vs_wgsl, vs_glsl, cfg);
        }
        else if (vs_glsl != NULL && vs_spirv_key != NULL &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            ok = ok && _emit_shader_spirv(stream, vs_id, "VERTEX", vs_spirv_key, vs_glsl, cfg);
        }
        else if (vs_glsl != NULL)
        {
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", NULL, vs_glsl, cfg);
        }
        else
        {
            const char* vertex_wgsl = NULL;
            const char* vertex_glsl_src = NULL;
            _render_vertex_shader_source(cfg, &vertex_wgsl, &vertex_glsl_src);
            ok = ok && _emit_shader(stream, vs_id, "VERTEX", vertex_wgsl, vertex_glsl_src, cfg);
        }
    }

    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
    {
        if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL && fs_wgsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", fs_wgsl, fs_glsl, cfg);
        }
        else if (fs_glsl != NULL && fs_spirv_key != NULL &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            ok = ok && _emit_shader_spirv(stream, fs_id, "FRAGMENT", fs_spirv_key, fs_glsl, cfg);
        }
        else if (fs_glsl != NULL)
        {
            ok = ok && _emit_shader(stream, fs_id, "FRAGMENT", NULL, fs_glsl, cfg);
        }
        else
        {
            ok = ok && _emit_shader(
                           stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);
        }
    }

    if (is_point_like)
    {
        bool picking = render->u.render.picking;
        bool depth_cue =
            visual_meta != NULL && visual_meta->depth_cue_enabled && !picking;
        bool point_style =
            visual_meta != NULL && visual_meta->point_style_enabled && !is_pixel && !is_marker &&
            !picking;
        const char* suffix =
            picking ? "_pick" : point_style && depth_cue ? "_cue_style" :
            point_style ? "_style" : depth_cue ? "_cue" : "";
        dvz_snprintf(
            pipe_key, sizeof(pipe_key), "_pipe_%s%s%s",
            is_marker ? "marker" : is_pixel ? "pixel" : "point",
            suffix, fmt);
    }
    else if (is_primitive)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_prim_t%u%s", topology, fmt);
    else if (is_image)
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_img%s", fmt);
    else
        dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe%u%s", vertex_buffer_count, fmt);

    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
    {
        if (is_point_like)
        {
            uint32_t strides[5] = {
                3 * sizeof(float), 4 * sizeof(uint8_t), sizeof(float), sizeof(float),
                sizeof(uint32_t)};
            uint32_t bindings[5] = {0, 1, 2, 3, 4};
            uint32_t locations[5] = {0, 1, 2, 3, 4};
            uint32_t formats[5] = {
                VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32_SFLOAT,
                VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT};
            uint32_t offsets[5] = {0, 0, 0, 0, 0};
            uint32_t point_like_attr_count = is_marker && !render->u.render.picking ? 5 : 3;
            if (point_like_lowering.lowering ==
                DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS)
            {
                uint32_t step_modes[5] = {
                    point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                    point_like_lowering.vertex_step_mode,
                };
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex2(
                               stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                               topology, point_like_attr_count, strides, step_modes,
                               point_like_attr_count, bindings, locations, formats, offsets);
            }
            else
            {
                ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                               stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                               topology,
                               point_like_attr_count, strides,
                               point_like_attr_count, bindings, locations, formats, offsets);
            }
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_primitive)
        {
            /* binding0=position(vec3), binding1=color(u8vec4) */
            uint32_t strides[2]   = {3*sizeof(float), 4*sizeof(uint8_t)};
            uint32_t bindings[2]  = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM};
            uint32_t offsets[2]   = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           2, strides,
                           2, bindings, locations, formats, offsets);
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
        }
        else if (is_image)
        {
            /* binding0=position(vec3), binding1=texcoords(vec2); set0=common, set1=image */
            uint32_t strides[2]   = {3*sizeof(float), 2*sizeof(float)};
            uint32_t bindings[2]  = {0, 1};
            uint32_t locations[2] = {0, 1};
            uint32_t formats[2]   = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
            uint32_t offsets[2]   = {0, 0};
            ok = ok && dvz_drp2_stream_create_render_pipeline_ex(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count,
                           topology,
                           2, strides,
                           2, bindings, locations, formats, offsets);
            if (ok && uses_common && common_bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout(stream, common_bgl_id);
            if (ok && bgl_id != 0)
                ok = dvz_drp2_stream_pipeline_set_bind_group_layout2(stream, bgl_id);
        }
        else
        {
            ok = ok && dvz_drp2_stream_create_render_pipeline(
                           stream, pipe_id, vs_id, fs_id, vertex_buffer_count);
        }
    }

    uint64_t color_id = 0;
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

    if (!ok)
        return false;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca, render->u.render.desc.x,
             render->u.render.desc.y, render->u.render.desc.width, render->u.render.desc.height,
             clear) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id);
    if (ok && uses_common && common_bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, common_bg_id);
    if (ok && is_image && bg_id != 0)
        ok = dvz_drp2_stream_set_bind_group(stream, render_pass_id, 1, bg_id);
    for (uint32_t i = 0; ok && i < vertex_buffer_count; i++)
        ok = dvz_drp2_stream_set_vertex_buffer(stream, render_pass_id, i, vertex_buffer_ids[i], 0);
    uint32_t draw_vertex_count = vertex_count;
    uint32_t draw_instance_count = 1;
    if (is_point_like && has_point_like_lowering)
    {
        draw_vertex_count = point_like_lowering.draw_vertex_count;
        draw_instance_count = point_like_lowering.draw_instance_count;
    }
    ok = ok && dvz_drp2_stream_draw(
                   stream, render_pass_id, draw_vertex_count, draw_instance_count, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    return ok;
}



/**
 * Emit all plain render nodes in a runtime-mode FramePlan.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param plan the FramePlan
 * @param fallback_vertex_buffer_ids uploaded vertex buffer ids used when visual ids are generic
 * @param fallback_vertex_buffer_count number of fallback vertex buffer ids
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether all render commands were emitted
 */
static bool _emitter_emit_plain_renders(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlan* plan,
    const uint64_t* fallback_vertex_buffer_ids, uint32_t fallback_vertex_buffer_count,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg,
    DvzDiagnosticReport* report)
{
    ANN(emitter);
    ANN(stream);
    ANN(plan);

    if (cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL &&
        _plan_has_graph_render_passes(plan))
        return _emitter_emit_scene_graph_renders(emitter, stream, plan, readback, cfg, report);

    uint32_t render_node_count = 0;
    uint32_t scene_render_node_count = 0;
    bool any_scene_render_needs_depth = false;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        render_node_count++;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            if (_scene_render_visual_has_position_resource(emitter, render, 0))
            {
                scene_render_node_count++;
                any_scene_render_needs_depth =
                    any_scene_render_needs_depth || _scene_render_needs_depth(emitter, render);
            }
        }
    }
    if (dvz_frame_plan_graph_pass_count(plan) == 0 && render_node_count > 0 &&
        render_node_count == scene_render_node_count)
        return _emitter_emit_scene_figure_renders(
            emitter, stream, plan, readback, cfg, any_scene_render_needs_depth, report);

    bool ok = true;
    uint32_t render_count = 0;
    SceneRenderStateCache scene_cache = {0};
    bool use_graph_order = dvz_frame_plan_graph_pass_count(plan) > 0;
    uint32_t order_count = use_graph_order ? dvz_frame_plan_graph_pass_count(plan) : plan->count;
    for (uint32_t i = 0; ok && i < order_count; i++)
    {
        const DvzFrameGraphPass* graph_pass =
            use_graph_order ? dvz_frame_plan_graph_pass_get(plan, i) : NULL;
        const DvzFramePlanNode* render =
            use_graph_order ? _graph_render_for_pass(plan, graph_pass) : &plan->nodes[i];
        if (render == NULL || render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        uint64_t vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
        uint32_t vertex_buffer_count = 0;

        /* Scene render nodes (visual_count > 0 with named resources) skip flat resolution;
         * _emitter_emit_render dispatches to _emitter_emit_render_multi instead. */
        bool is_scene_node = false;
        if (render->u.render.visual_count > 0 &&
            cfg != NULL && cfg->shader_format == DVZ_SCENE_SHADER_FORMAT_GLSL)
        {
            is_scene_node = _scene_render_visual_has_position_resource(emitter, render, 0);
        }

        if (!is_scene_node)
        {
            ok = _emitter_resolve_render_vertex_buffers(
                emitter, render, vertex_buffer_ids, &vertex_buffer_count);
            if (!ok && fallback_vertex_buffer_ids != NULL && fallback_vertex_buffer_count > 0)
            {
                ok = true;
                vertex_buffer_count = fallback_vertex_buffer_count;
                for (uint32_t j = 0; j < vertex_buffer_count; j++)
                    vertex_buffer_ids[j] = fallback_vertex_buffer_ids[j];
            }
        }

        if (ok)
        {
            scene_cache.pipeline_id = 0;
            scene_cache.bg_set0 = 0;
            ok = _emitter_emit_render(
                emitter, stream, plan, render, vertex_buffer_ids, vertex_buffer_count,
                render_count == 0 ? readback : NULL, render_count == 0, cfg,
                is_scene_node ? &scene_cache : NULL, report);
        }
        render_count++;
    }
    return ok && render_count > 0;
}



/**
 * Emit runtime-mode clear-only render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_clear_only(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* clear_node,
    const DvzFramePlanNode* readback, bool clear, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(clear_node);

    uint64_t color_id = 0;
    if (!_render_pass_resolve_color_target(emitter, stream, cfg, &color_id))
        return false;

    uint64_t rb_id = 0;
    if (!_render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id))
        return false;

    bool ok = true;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr = cfg ? cfg->clear_color[0] : 0.0f;
    float cg = cfg ? cfg->clear_color[1] : 0.0f;
    float cb = cfg ? cfg->clear_color[2] : 0.0f;
    float ca = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_region_clear(
             stream, render_pass_id, encoder_id, color_id, cr, cg, cb, ca,
             clear_node->u.clear.desc.x, clear_node->u.clear.desc.y, clear_node->u.clear.desc.width,
             clear_node->u.clear.desc.height, clear) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    return ok;
}


/**
 * Emit runtime-mode texture render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param texture_id the sampled texture id
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_texture_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, uint64_t texture_id,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    if (texture_id == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint64_t sampler_id = _obj_id(emitter, "_sampler", &is_new);
    if (sampler_id == 0)
        return false;
    if (is_new)
        ok = ok && dvz_drp2_stream_create_sampler(stream, sampler_id);

    uint64_t bgl_id = _obj_id(emitter, "_bgl_tex", &is_new);
    if (bgl_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group_layout(stream, bgl_id);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs_tex%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", _texture_vertex_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs_tex%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _texture_fragment_wgsl(),
                       _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_TEXTURE, true), cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe_tex%s_%" PRIu64, fmt, bgl_id);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
                       stream, pipe_id, vs_id, fs_id, 0, bgl_id);

    char bg_key[32];
    dvz_snprintf(bg_key, sizeof(bg_key), "_bg_tex_%" PRIu64, texture_id);
    uint64_t bg_id = _obj_id(emitter, bg_key, &is_new);
    if (bg_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_texture_sampler_bind_group(
                       stream, bg_id, bgl_id, texture_id, sampler_id);

    uint64_t color_id = 0;
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

    if (!ok)
        return false;

    uint64_t encoder_id = 0;
    uint64_t render_pass_id = 0;
    uint64_t command_buffer_id = 0;
    uint64_t submission_id = 0;
    _render_pass_next_ids(
        emitter, &encoder_id, &render_pass_id, &command_buffer_id, &submission_id);

    float cr2 = cfg ? cfg->clear_color[0] : 0.0f;
    float cg2 = cfg ? cfg->clear_color[1] : 0.0f;
    float cb2 = cfg ? cfg->clear_color[2] : 0.0f;
    float ca2 = cfg ? cfg->clear_color[3] : 1.0f;
    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_render_pass_clear(
             stream, render_pass_id, encoder_id, color_id, cr2, cg2, cb2, ca2) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, render_pass_id, 0, bg_id) &&
         dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
         dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    return ok;
}


/**
 * Emit runtime-mode compute pass followed by render commands.
 *
 * @param emitter the persistent emitter
 * @param stream the DRP2 command stream
 * @param compute the compute node
 * @param readback the optional readback copy node
 * @param cfg the emission config
 * @return whether the commands were emitted
 */
static bool _emitter_emit_compute_assisted_render(
    DvzFramePlanEmitter* emitter, DvzDrp2CommandStream* stream, const DvzFramePlanNode* compute,
    const DvzFramePlanNode* readback, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(stream);
    ANN(compute);
    if (emitter->resources.first_compute_input_id == 0 ||
        emitter->resources.first_compute_output_id == 0 ||
        emitter->resources.compute_buffer_size == 0)
        return false;

    bool ok = true;
    bool is_new = false;
    const char* fmt = _shader_format_tag(cfg);

    uint64_t bgl_stor_id = _obj_id(emitter, "_bgl_stor", &is_new);
    if (bgl_stor_id == 0)
        return false;
    if (is_new)
        ok = ok &&
             dvz_drp2_stream_create_storage_bind_group_layout(stream, bgl_stor_id);

    char cs_key[16];
    dvz_snprintf(cs_key, sizeof(cs_key), "_cs%s", fmt);
    uint64_t cs_id = _obj_id(emitter, cs_key, &is_new);
    if (cs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, cs_id, "COMPUTE", _compute_copy_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY, false), cfg);

    char cpipe_key[32];
    dvz_snprintf(cpipe_key, sizeof(cpipe_key), "_cpipe%s_%" PRIu64, fmt, bgl_stor_id);
    uint64_t cpipe_id = _obj_id(emitter, cpipe_key, &is_new);
    if (cpipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
                       stream, cpipe_id, cs_id, bgl_stor_id);

    char bg_stor_key[64];
    dvz_snprintf(
        bg_stor_key, sizeof(bg_stor_key), "_bg_stor_%" PRIu64 "_%" PRIu64,
        emitter->resources.first_compute_input_id,
        emitter->resources.first_compute_output_id);
    uint64_t bg_stor_id = _obj_id(emitter, bg_stor_key, &is_new);
    if (bg_stor_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_storage_bind_group(
                       stream, bg_stor_id, bgl_stor_id,
                       emitter->resources.first_compute_input_id,
                       emitter->resources.first_compute_output_id,
                       emitter->resources.compute_buffer_size);

    char vs_key[16];
    dvz_snprintf(vs_key, sizeof(vs_key), "_vs%s", fmt);
    uint64_t vs_id = _obj_id(emitter, vs_key, &is_new);
    if (vs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, vs_id, "VERTEX", _fixture_vertex_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, false), cfg);

    char fs_key[16];
    dvz_snprintf(fs_key, sizeof(fs_key), "_fs%s", fmt);
    uint64_t fs_id = _obj_id(emitter, fs_key, &is_new);
    if (fs_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && _emit_shader(
                       stream, fs_id, "FRAGMENT", _fixture_fragment_wgsl(), _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_FIXTURE, true), cfg);

    char pipe_key[32];
    dvz_snprintf(pipe_key, sizeof(pipe_key), "_pipe1%s", fmt);
    uint64_t pipe_id = _obj_id(emitter, pipe_key, &is_new);
    if (pipe_id == 0)
        return false;
    if (ok && is_new)
        ok = ok && dvz_drp2_stream_create_render_pipeline(stream, pipe_id, vs_id, fs_id, 1);

    uint64_t color_id = 0;
    ok = ok && _render_pass_resolve_color_target(emitter, stream, cfg, &color_id);

    uint64_t rb_id = 0;
    ok = ok && _render_pass_resolve_readback_buffer(emitter, stream, readback, &rb_id);

    if (!ok)
        return false;

    uint64_t encoder_id = _emitter_next_transient_id(emitter);
    uint64_t compute_pass_id = _emitter_next_transient_id(emitter);
    uint64_t render_pass_id = _emitter_next_transient_id(emitter);
    uint64_t command_buffer_id = _emitter_next_transient_id(emitter);
    uint64_t submission_id = _emitter_next_transient_id(emitter);

    ok = dvz_drp2_stream_begin_command_encoder(stream, encoder_id) &&
         dvz_drp2_stream_begin_compute_pass(stream, compute_pass_id, encoder_id) &&
         dvz_drp2_stream_set_pipeline(stream, compute_pass_id, cpipe_id) &&
         dvz_drp2_stream_set_bind_group(stream, compute_pass_id, 0, bg_stor_id) &&
         dvz_drp2_stream_dispatch_workgroups(
             stream, compute_pass_id, compute->u.compute.dispatch[0],
             compute->u.compute.dispatch[1], compute->u.compute.dispatch[2]) &&
         dvz_drp2_stream_end_compute_pass(stream, compute_pass_id) &&
         dvz_drp2_stream_begin_render_pass_clear(
             stream, render_pass_id, encoder_id, color_id,
             cfg ? cfg->clear_color[0] : 0.0f, cfg ? cfg->clear_color[1] : 0.0f,
             cfg ? cfg->clear_color[2] : 0.0f, cfg ? cfg->clear_color[3] : 1.0f) &&
         dvz_drp2_stream_set_pipeline(stream, render_pass_id, pipe_id) &&
         dvz_drp2_stream_set_vertex_buffer(
             stream, render_pass_id, 0, emitter->resources.first_compute_output_id, 0) &&
        dvz_drp2_stream_draw(stream, render_pass_id, 3, 1, 0, 0) &&
        dvz_drp2_stream_end_render_pass(stream, render_pass_id);
    ok = ok && _render_pass_copy_finish_submit(
                   stream, encoder_id, command_buffer_id, submission_id, color_id, rb_id,
                   readback);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit a runtime-mode DRP2 command stream from a FramePlan.
 *
 * @param emitter the persistent emitter
 * @param plan the FramePlan
 * @param caps the capability snapshot
 * @param report the diagnostic report
 * @param cfg the emission configuration
 * @return an owned DRP2 command stream, or NULL on failure
 */
DvzDrp2CommandStream* dvz_frame_plan_emitter_emit_drp2(
    DvzFramePlanEmitter* emitter, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report, const DvzFramePlanEmitConfig* cfg)
{
    ANN(emitter);
    ANN(plan);

    const DvzFramePlanNode* upload = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_UPLOAD);
    const DvzFramePlanNode* compute = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COMPUTE);
    const DvzFramePlanNode* render = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_RENDER);
    const DvzFramePlanNode* clear = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_CLEAR);
    const DvzFramePlanNode* copy = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_COPY);
    const DvzFramePlanNode* readback = _first_node_of_type(plan, DVZ_FRAME_PLAN_NODE_READBACK);
    bool clear_only = compute == NULL && clear != NULL && render == NULL;
    bool retained_render = upload == NULL && compute == NULL && render != NULL &&
                           render->u.render.visual_count > 0;

    if ((!clear_only && !retained_render && upload == NULL) || (!clear_only && render == NULL))
    {
        _diagnostic(report, "runtime converter requires upload+render");
        return NULL;
    }
    bool texture_render = !clear_only && _render_uses_texture(render);
    if (compute != NULL)
    {
        if (compute->u.compute.write_count == 0)
        {
            _diagnostic(report, "runtime converter requires compute output");
            return NULL;
        }
    }
    if (readback != NULL && copy == NULL)
    {
        _diagnostic(report, "runtime converter requires copy before readback");
        return NULL;
    }
    if (caps != NULL && !_validate_capabilities(plan, caps, cfg, report))
        return NULL;
    emitter->max_color_sample_count =
        caps != NULL && caps->max_color_sample_count != 0 ? caps->max_color_sample_count : 16;
    emitter->max_depth_sample_count =
        caps != NULL && caps->max_depth_sample_count != 0 ? caps->max_depth_sample_count : 16;

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    ANN(stream);

    bool ok = true;
    uint64_t fallback_vertex_buffer_ids[DVZ_SCENE_MAX_NODE_RESOURCES] = {0};
    uint32_t fallback_vertex_buffer_count = 0;
    uint64_t texture_id = 0;
    if (!emitter->handshake_sent)
    {
        ok = dvz_drp2_stream_hello_renderer(stream, "scene-runtime") &&
             dvz_drp2_stream_renderer_hello_reply(stream, "datoviz-drp2-runtime");
        emitter->handshake_sent = ok;
    }

    for (uint32_t i = 0; ok && i < plan->count; i++)
    {
        if (plan->nodes[i].type == DVZ_FRAME_PLAN_NODE_UPLOAD)
        {
            if (compute != NULL)
            {
                ok = _emitter_emit_compute_buffers(emitter, stream, &plan->nodes[i], compute);
            }
            else if (texture_render)
            {
                ok = _emitter_emit_texture_upload(emitter, stream, &plan->nodes[i], &texture_id);
            }
            else
            {
                uint64_t uploaded_id = 0;
                ok = _emitter_emit_upload(
                    emitter, stream, &plan->nodes[i], &uploaded_id);
                if (ok && fallback_vertex_buffer_count < DVZ_SCENE_MAX_NODE_RESOURCES)
                    fallback_vertex_buffer_ids[fallback_vertex_buffer_count++] = uploaded_id;
            }
        }
    }

    ok = ok && (clear_only
                    ? _emitter_emit_clear_only(emitter, stream, clear, copy, true, cfg)
                    : compute != NULL
                    ? _emitter_emit_compute_assisted_render(emitter, stream, compute, copy, cfg)
                    : texture_render
                    ? _emitter_emit_texture_render(emitter, stream, texture_id, copy, cfg)
                    : _emitter_emit_plain_renders(
                          emitter, stream, plan, fallback_vertex_buffer_ids,
                          fallback_vertex_buffer_count, copy, cfg, report));
    if (!ok)
    {
        _diagnostic(report, "failed to emit runtime DRP2 stream");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    _emitter_label_stream_ids(emitter, stream, cfg);
    if (!_scene_frame_plan_drp2_contracts_validate(plan, stream, report))
    {
        _diagnostic(report, "emitted runtime DRP2 stream failed scene contract validation");
        dvz_drp2_stream_destroy(stream);
        return NULL;
    }
    return stream;
}
