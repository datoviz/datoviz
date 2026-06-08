/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual shader descriptors                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"
#include "_shader_registry.h"
#include "_scene_shader_abi.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"
#include "registry/registry.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append a suffix to one shader cache key.
 *
 * @param key destination key buffer
 * @param size key buffer size
 * @param suffix suffix to append
 * @return whether the suffix fit
 */
static bool _shader_key_append(char* key, size_t size, const char* suffix)
{
    ANN(key);
    ANN(suffix);
    size_t len = strlen(key);
    size_t extra = strlen(suffix);
    if (len >= size || extra >= size - len)
        return false;
    dvz_snprintf(key + len, size - len, "%s", suffix);
    return true;
}



/**
 * Attach built-in shader source pointers to a shader descriptor.
 *
 * @param out the output shader descriptor
 * @param shader the built-in shader id
 */
void _scene_shader_desc_set_builtin(DvzSceneVisualShaderDesc* out, DvzSceneBuiltinShader shader)
{
    ANN(out);
    out->vertex_glsl = _builtin_shader_glsl(shader, false);
    out->fragment_glsl = _builtin_shader_glsl(shader, true);
    out->vertex_wgsl = _builtin_shader_wgsl(shader, false);
    out->fragment_wgsl = _builtin_shader_wgsl(shader, true);
}


/**
 * Attach stable built-in identity metadata to a shader descriptor.
 *
 * @param out the output shader descriptor
 * @param family the shader family id
 * @param variant the shader variant id
 */
void _scene_shader_desc_set_identity(
    DvzSceneVisualShaderDesc* out, const char* family, const char* variant)
{
    ANN(out);
    out->builtin_family = family;
    out->builtin_variant = variant != NULL ? variant : "default";
    out->builtin_version = DVZ_SCENE_SHADER_BUILTIN_CONTRACT_VERSION;
    out->builtin_pipeline = family;
    out->builtin_pipeline_version = DVZ_SCENE_SHADER_BUILTIN_CONTRACT_VERSION;
}



/**
 * Resolve shader and pipeline cache-key metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_visual_shader_desc_resolve(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualShaderDesc), 0, sizeof(DvzSceneVisualShaderDesc));

    DvzVisualType type = _scene_visual_family_desc_default_type(visual->kind);
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(type);
    if (ops == NULL || ops->resolve_shader_desc == NULL)
        return false;
    return ops->resolve_shader_desc(visual, picking, wboit_accumulation, format_tag, out);
}


/**
 * Apply integer query shader overrides to one picking shader descriptor when needed.
 *
 * @param visual the visual descriptor
 * @param color_target_format picking color target format
 * @param shader shader descriptor to update
 * @param out_applied whether a query shader override was applied
 * @return whether the descriptor was updated successfully
 */
bool _scene_visual_shader_desc_apply_query_pick(
    const DvzSceneVisualDesc* visual, uint32_t color_target_format,
    DvzSceneVisualShaderDesc* shader, bool* out_applied)
{
    ANN(visual);
    ANN(shader);
    ANN(out_applied);
    *out_applied = false;

    bool point_like = visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                      visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool segment_query =
        visual->kind == DVZ_SCENE_VISUAL_DESC_SEGMENT &&
        color_target_format == VK_FORMAT_R32_UINT;
    bool path_query =
        visual->kind == DVZ_SCENE_VISUAL_DESC_PATH &&
        color_target_format == VK_FORMAT_R32_UINT;
    bool primitive_query =
        visual->kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE &&
        color_target_format == VK_FORMAT_R32_UINT;
    bool labels_query =
        (visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
         visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT) &&
        color_target_format == VK_FORMAT_R32_UINT;
    bool volume_query_u32 =
        (visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME ||
         visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
         visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT) &&
        color_target_format == VK_FORMAT_R32_UINT;
    bool volume_query_rg32 =
        visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME &&
        color_target_format == VK_FORMAT_R32G32_UINT;
    bool query =
        (point_like || visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE || segment_query ||
         path_query || primitive_query || labels_query || volume_query_u32 || volume_query_rg32) &&
        (color_target_format == VK_FORMAT_R32_UINT ||
         color_target_format == VK_FORMAT_R32G32_UINT);
    if (!query)
        return true;

    DvzSceneBuiltinShader query_shader = DVZ_SCENE_BUILTIN_SHADER_POINT_QUERY_U32;
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_SPHERE_QUERY_U32;
    else if (segment_query)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_SEGMENT_QUERY_U32;
    else if (path_query)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_PATH_QUERY_U32;
    else if (primitive_query)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_QUERY_U32;
    else if (
        visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
    {
        query_shader = DVZ_SCENE_BUILTIN_SHADER_PIXEL_QUERY_U32;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_LABELS_SINT_QUERY_U32;
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_LABELS_UINT_QUERY_U32;
    else if (volume_query_u32)
    {
        query_shader = visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT
                           ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_SINT_QUERY_U32
                       : visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT
                           ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_UINT_QUERY_U32
                           : DVZ_SCENE_BUILTIN_SHADER_VOLUME_QUERY_U32;
    }
    else if (volume_query_rg32)
        query_shader = DVZ_SCENE_BUILTIN_SHADER_VOLUME_QUERY_RG32;

    if (!_shader_key_append(shader->fragment_key, sizeof(shader->fragment_key), "_query_u32") ||
        !_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), "_query_u32"))
    {
        return false;
    }

    if (
        point_like || visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE ||
        query_shader == DVZ_SCENE_BUILTIN_SHADER_SEGMENT_QUERY_U32 ||
        query_shader == DVZ_SCENE_BUILTIN_SHADER_PATH_QUERY_U32 ||
        query_shader == DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_QUERY_U32)
    {
        if (!_shader_key_append(shader->vertex_key, sizeof(shader->vertex_key), "_query_u32"))
            return false;
        shader->vertex_glsl = _builtin_shader_glsl(query_shader, false);
        shader->vertex_wgsl = _builtin_shader_wgsl(query_shader, false);
        shader->vertex_spirv_key =
            query_shader == DVZ_SCENE_BUILTIN_SHADER_SPHERE_QUERY_U32
                ? "sphere_query_u32_vert"
            : query_shader == DVZ_SCENE_BUILTIN_SHADER_SEGMENT_QUERY_U32
                ? "segment_query_u32_vert"
            : query_shader == DVZ_SCENE_BUILTIN_SHADER_PATH_QUERY_U32 ? "path_query_u32_vert"
            : query_shader == DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_QUERY_U32
                ? "primitive_query_u32_vert"
            : query_shader == DVZ_SCENE_BUILTIN_SHADER_PIXEL_QUERY_U32 ? "pixel_pick_vert"
                                                                       : "point_pick_vert";
    }

    shader->fragment_glsl = _builtin_shader_glsl(query_shader, true);
    shader->fragment_wgsl = _builtin_shader_wgsl(query_shader, true);
    if (query_shader == DVZ_SCENE_BUILTIN_SHADER_SPHERE_QUERY_U32)
        shader->fragment_spirv_key = "sphere_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_SEGMENT_QUERY_U32)
        shader->fragment_spirv_key = "segment_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_PATH_QUERY_U32)
        shader->fragment_spirv_key = "path_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_QUERY_U32)
        shader->fragment_spirv_key = "primitive_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_PIXEL_QUERY_U32)
        shader->fragment_spirv_key = "pixel_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_LABELS_SINT_QUERY_U32)
        shader->fragment_spirv_key = "labels_sint_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_LABELS_UINT_QUERY_U32)
        shader->fragment_spirv_key = "labels_uint_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_VOLUME_QUERY_U32)
        shader->fragment_spirv_key = "volume_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_VOLUME_QUERY_RG32)
        shader->fragment_spirv_key = "volume_query_rg32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_SINT_QUERY_U32)
        shader->fragment_spirv_key = "volume_labels_sint_query_u32_frag";
    else if (query_shader == DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_UINT_QUERY_U32)
        shader->fragment_spirv_key = "volume_labels_uint_query_u32_frag";
    else
        shader->fragment_spirv_key = "point_query_u32_frag";
    shader->builtin_family = NULL;
    shader->builtin_variant = NULL;
    shader->builtin_version = 0;
    shader->builtin_pipeline = NULL;
    shader->builtin_pipeline_version = 0;
    *out_applied = true;
    return true;
}


/**
 * Resolve shader metadata for graph-driven special render passes.
 *
 * @param visual mutable visual descriptor; pass policies may disable material data
 * @param pass_role render pass role being prepared
 * @param format_tag shader-format cache-key suffix
 * @param shader output shader descriptor
 * @param out_fragment_glsl_variant owned GLSL variant, when one is generated
 * @param out_handled whether this helper handled the pass role
 * @param out_skip whether this visual should be skipped for this pass
 * @return whether the pass-specific shader descriptor was resolved successfully
 */
bool _scene_visual_shader_desc_for_pass(
    DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role, const char* format_tag,
    DvzSceneVisualShaderDesc* shader, char** out_fragment_glsl_variant, bool* out_handled,
    bool* out_skip)
{
    ANN(visual);
    ANN(format_tag);
    ANN(shader);
    ANN(out_fragment_glsl_variant);
    ANN(out_handled);
    ANN(out_skip);

    *out_fragment_glsl_variant = NULL;
    *out_handled = false;
    *out_skip = false;

    if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER)
    {
        *out_handled = true;
        if (visual->kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE && !visual->has_normal)
        {
            *out_skip = true;
            return true;
        }
        if (
            visual->kind != DVZ_SCENE_VISUAL_DESC_PRIMITIVE &&
            visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE)
        {
            *out_skip = true;
            return true;
        }
        if (visual->kind != DVZ_SCENE_VISUAL_DESC_SPHERE)
            visual->material_buffer_id = 0;
        if (visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
        {
            dvz_snprintf(
                shader->vertex_key, sizeof(shader->vertex_key), "_vs_gbuffer_sphere%s",
                format_tag);
            dvz_snprintf(
                shader->fragment_key, sizeof(shader->fragment_key), "_fs_gbuffer_sphere%s",
                format_tag);
            dvz_snprintf(
                shader->pipeline_key, sizeof(shader->pipeline_key), "_pipe_gbuffer_sphere%s",
                format_tag);
            shader->vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPHERE_GBUFFER, false);
            shader->fragment_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPHERE_GBUFFER, true);
            shader->vertex_spirv_key = "sphere_gbuffer_vert";
            shader->fragment_spirv_key = "sphere_gbuffer_frag";
        }
        else
        {
            dvz_snprintf(
                shader->vertex_key, sizeof(shader->vertex_key), "_vs_gbuffer_prim%s",
                format_tag);
            dvz_snprintf(
                shader->fragment_key, sizeof(shader->fragment_key), "_fs_gbuffer_normal%s",
                format_tag);
            dvz_snprintf(
                shader->pipeline_key, sizeof(shader->pipeline_key), "_pipe_gbuffer_t%u%s",
                visual->topology, format_tag);
            shader->vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL, false);
            shader->fragment_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GBUFFER_NORMAL, true);
            shader->vertex_spirv_key = "primitive_lit_vert";
            shader->fragment_spirv_key = "gbuffer_normal_frag";
        }
        return true;
    }

    if (pass_role == DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION)
    {
        *out_handled = true;
        if (visual->kind != DVZ_SCENE_VISUAL_DESC_VOLUME)
        {
            *out_skip = true;
            return true;
        }
        dvz_snprintf(shader->vertex_key, sizeof(shader->vertex_key), "_vs_vol_occ%s", format_tag);
        dvz_snprintf(
            shader->fragment_key, sizeof(shader->fragment_key), "_fs_vol_occ%s", format_tag);
        dvz_snprintf(
            shader->pipeline_key, sizeof(shader->pipeline_key), "_pipe_vol_occ%s", format_tag);
        shader->vertex_glsl =
            _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, false);
        shader->fragment_glsl =
            _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, true);
        shader->vertex_spirv_key = "volume_slice_vert";
        shader->fragment_spirv_key = "volume_occlusion_depth_frag";
        _scene_shader_desc_set_identity(shader, "scene.volume", "occlusion_depth");
        return true;
    }

    if (pass_role != DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION)
        return true;

    *out_handled = true;
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME)
    {
        dvz_snprintf(
            shader->vertex_key, sizeof(shader->vertex_key), "_vs_scene_occ_vol%s", format_tag);
        dvz_snprintf(
            shader->fragment_key, sizeof(shader->fragment_key), "_fs_scene_occ_vol%s",
            format_tag);
        dvz_snprintf(
            shader->pipeline_key, sizeof(shader->pipeline_key), "_pipe_scene_occ_vol%s",
            format_tag);
        shader->vertex_glsl =
            _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, false);
        shader->fragment_glsl =
            _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_VOLUME_OCCLUSION_DEPTH, true);
        shader->vertex_spirv_key = "volume_slice_vert";
        *out_fragment_glsl_variant = _shader_glsl_variant(
            shader->fragment_glsl, "#define DVZ_SCENE_OCCLUSION_DEPTH_FAR 1\n");
        shader->fragment_glsl = *out_fragment_glsl_variant;
        shader->fragment_spirv_key = NULL;
        return shader->fragment_glsl != NULL;
    }

    const char* stem = "prim";
    const char* vertex_spirv_key = "primitive_vert";
    DvzSceneBuiltinShader vertex_shader = DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE;
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_POINT)
    {
        stem = "point";
        vertex_spirv_key = "point_vert";
        vertex_shader = DVZ_SCENE_BUILTIN_SHADER_POINT;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
    {
        stem = "pixel";
        vertex_spirv_key = "pixel_vert";
        vertex_shader = DVZ_SCENE_BUILTIN_SHADER_PIXEL;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
    {
        stem = "marker";
        vertex_spirv_key = "marker_vert";
        vertex_shader = DVZ_SCENE_BUILTIN_SHADER_MARKER;
    }
    else if (visual->kind == DVZ_SCENE_VISUAL_DESC_IMAGE)
    {
        stem = visual->image_pixel_space ? "image_px" : "image";
        vertex_spirv_key = visual->image_pixel_space ? "image_pixel_vert" : "image_vert";
        vertex_shader = visual->image_pixel_space ? DVZ_SCENE_BUILTIN_SHADER_IMAGE_PIXEL
                                                  : DVZ_SCENE_BUILTIN_SHADER_IMAGE;
    }
    else if (visual->kind != DVZ_SCENE_VISUAL_DESC_PRIMITIVE)
    {
        *out_skip = true;
        return true;
    }

    dvz_snprintf(
        shader->vertex_key, sizeof(shader->vertex_key), "_vs_scene_occ_%s%s", stem, format_tag);
    dvz_snprintf(
        shader->fragment_key, sizeof(shader->fragment_key), "_fs_scene_occ_depth%s", format_tag);
    dvz_snprintf(
        shader->pipeline_key, sizeof(shader->pipeline_key), "_pipe_scene_occ_%s_t%u%s", stem,
        visual->topology, format_tag);
    shader->vertex_glsl = _builtin_shader_glsl(vertex_shader, false);
    shader->fragment_glsl =
        _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SCENE_OCCLUSION_DEPTH, true);
    shader->vertex_spirv_key = vertex_spirv_key;
    shader->fragment_spirv_key = "scene_occlusion_depth_frag";
    if (visual->kind != DVZ_SCENE_VISUAL_DESC_IMAGE)
    {
        *out_fragment_glsl_variant = _shader_glsl_variant(
            shader->fragment_glsl, "#define DVZ_SCENE_OCCLUSION_DEPTH_COLOR 1\n");
        shader->fragment_glsl = *out_fragment_glsl_variant;
        shader->fragment_spirv_key = NULL;
        if (shader->fragment_glsl == NULL)
            return false;
    }
    visual->has_normal = false;
    visual->material_buffer_id = 0;
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE && visual->vbuf_count > 2)
        visual->vbuf_count = 2;
    return true;
}


/**
 * Select the depth-peeling fragment shader variant.
 *
 * @param lit whether the visual carries normals and uses lit shading
 * @param back_pass whether the pass writes the back-shell accumulation
 * @return the built-in shader key
 */
static DvzSceneBuiltinShader _shader_depth_peel_fragment(bool lit, bool back_pass)
{
    if (lit)
    {
        return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK_LIT
                         : DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT_LIT;
    }
    return back_pass ? DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_BACK
                     : DVZ_SCENE_BUILTIN_SHADER_DEPTH_PEEL_FRONT;
}


/**
 * Return the SPIR-V resource key for one depth-peeling fragment shader.
 *
 * @param shader the built-in shader key
 * @return the embedded SPIR-V key, or NULL when unsupported
 */
static const char* _shader_depth_peel_fragment_spirv_key(DvzSceneBuiltinShader shader)
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
 * Apply render-pass shader key and source policy after base shader resolution.
 *
 * @param visual the visual descriptor
 * @param pass_role render pass role being prepared
 * @param alpha_mode visual alpha mode
 * @param controller_mode controller attachment mode for the visual
 * @param picking whether the render pass is a picking pass
 * @param pass_has_depth_attachment whether the pass carries a depth attachment
 * @param force_point_depth whether point-like visuals must write depth
 * @param wboit_accumulation whether the pass is WBOIT accumulation
 * @param pass_sample_count multisample count for the pass
 * @param pass_alpha_to_coverage whether alpha-to-coverage is enabled for the pass
 * @param scene_occluded_shader whether the shader samples scene occlusion depth
 * @param scene_occlusion_uses_set2 whether scene occlusion occupies bind set 2
 * @param shader shader descriptor to update
 * @param out_fragment_glsl_variant owned GLSL variant, when one is generated
 * @param out_segment_coverage_blend whether analytic coverage blending should be configured
 * @return whether the shader descriptor was updated successfully
 */
bool _scene_visual_shader_desc_apply_pass_policy(
    const DvzSceneVisualDesc* visual, DvzFramePlanRenderPassRole pass_role,
    DvzAlphaMode alpha_mode, DvzControllerMode controller_mode, bool picking,
    bool pass_has_depth_attachment, bool force_point_depth, bool wboit_accumulation,
    uint32_t pass_sample_count, bool pass_alpha_to_coverage, bool scene_occluded_shader,
    bool scene_occlusion_uses_set2, DvzSceneVisualShaderDesc* shader,
    char** out_fragment_glsl_variant, bool* out_segment_coverage_blend)
{
    ANN(visual);
    ANN(shader);
    ANN(out_fragment_glsl_variant);
    ANN(out_segment_coverage_blend);

    bool depth_peel_pass = pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ||
                           pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
    bool gbuffer_pass = pass_role == DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER;
    bool point_like = visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                      visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool styled_point =
        visual->kind == DVZ_SCENE_VISUAL_DESC_POINT && visual->point_style_enabled;
    bool analytic_coverage =
        _scene_visual_desc_is_stroke(visual->kind) || styled_point ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    *out_segment_coverage_blend =
        !picking &&
        !_scene_alpha_mode_is_blended(alpha_mode) && !wboit_accumulation && !depth_peel_pass &&
        analytic_coverage;

    if (_scene_alpha_mode_is_blended(alpha_mode) &&
        !_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), "_blend"))
    {
        return false;
    }
    if (
        *out_segment_coverage_blend &&
        !_shader_key_append(
            shader->pipeline_key, sizeof(shader->pipeline_key), "_coverage_blend"))
    {
        return false;
    }
    if (!visual->depth_test_enabled)
    {
        if (!_shader_key_append(
                shader->pipeline_key, sizeof(shader->pipeline_key), "_no_depth_test"))
            return false;
    }
    else if (visual->depth_compare_op == VK_COMPARE_OP_GREATER)
    {
        if (!_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), "_depth_gt"))
            return false;
    }

    if (_scene_alpha_mode_is_depth_peel(alpha_mode))
    {
        const char* peel_suffix =
            pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT ? "_peel_init" : "_peel_iter";
        if (!_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), peel_suffix) ||
            !_shader_key_append(shader->fragment_key, sizeof(shader->fragment_key), peel_suffix))
        {
            return false;
        }
        bool back_pass = pass_role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER;
        DvzSceneBuiltinShader peel_shader =
            _shader_depth_peel_fragment(visual->has_normal, back_pass);
        shader->fragment_glsl = _builtin_shader_glsl(peel_shader, true);
        shader->fragment_spirv_key = _shader_depth_peel_fragment_spirv_key(peel_shader);
    }
    if (controller_mode == DVZ_CONTROLLER_FIXED)
    {
        if (!_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), "_fixed"))
            return false;
    }
    if (pass_has_depth_attachment && !gbuffer_pass && !wboit_accumulation && !depth_peel_pass)
    {
        if (!_shader_key_append(
                shader->pipeline_key, sizeof(shader->pipeline_key),
                force_point_depth ? "_zwrite" : "_depth"))
        {
            return false;
        }
    }
    if (pass_sample_count > 1)
    {
        char msaa_suffix[32];
        dvz_snprintf(msaa_suffix, sizeof(msaa_suffix), "_msaa%u", pass_sample_count);
        if (!_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), msaa_suffix))
            return false;
        if ((visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE || point_like) && pass_alpha_to_coverage)
        {
            if (!_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), "_a2c"))
                return false;
            if (visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
            {
                if (!_shader_key_append(shader->fragment_key, sizeof(shader->fragment_key), "_a2c"))
                    return false;
                shader->fragment_glsl =
                    _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_SPHERE_A2C, true);
                shader->fragment_spirv_key = "sphere_a2c_frag";
            }
        }
    }
    if (scene_occluded_shader)
    {
        if (!_shader_key_append(shader->pipeline_key, sizeof(shader->pipeline_key), "_scene_occ") ||
            !_shader_key_append(shader->fragment_key, sizeof(shader->fragment_key), "_scene_occ"))
        {
            return false;
        }
        char scene_occlusion_defines[96];
        dvz_snprintf(
            scene_occlusion_defines, sizeof(scene_occlusion_defines),
            "#define DVZ_SCENE_OCCLUSION 1\n#define DVZ_SCENE_OCCLUSION_SET %u\n",
            scene_occlusion_uses_set2 ? 2u : 1u);
        *out_fragment_glsl_variant =
            _shader_glsl_variant(shader->fragment_glsl, scene_occlusion_defines);
        shader->fragment_glsl = *out_fragment_glsl_variant;
        shader->fragment_spirv_key = NULL;
        shader->fragment_wgsl = NULL;
        shader->builtin_family = NULL;
        shader->builtin_variant = NULL;
        shader->builtin_version = 0;
        shader->builtin_pipeline = NULL;
        shader->builtin_pipeline_version = 0;
        if (shader->fragment_glsl == NULL)
            return false;
    }
    return true;
}


/**
 * Resolve shader and pipeline cache-key metadata through the visual-family registry.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualShaderDesc), 0, sizeof(DvzSceneVisualShaderDesc));

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops((DvzVisualType)visual->visual_type);
    if (ops == NULL || ops->resolve_shader_desc == NULL)
        return false;
    return ops->resolve_shader_desc(visual, picking, wboit_accumulation, format_tag, out);
}
