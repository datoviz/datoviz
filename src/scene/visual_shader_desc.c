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
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef enum
{
    DVZ_SCENE_SHADER_FEATURE_NONE = 0,
    DVZ_SCENE_SHADER_FEATURE_PICKING = 1u << 0,
    DVZ_SCENE_SHADER_FEATURE_DEPTH_CUE = 1u << 1,
    DVZ_SCENE_SHADER_FEATURE_LIGHTING = 1u << 2,
    DVZ_SCENE_SHADER_FEATURE_WBOIT_ACCUM = 1u << 3,
    DVZ_SCENE_SHADER_FEATURE_VOLUME_MIP = 1u << 4,
    DVZ_SCENE_SHADER_FEATURE_VOLUME_COMPOSITE = 1u << 5,
    DVZ_SCENE_SHADER_FEATURE_POINT_STYLE = 1u << 6,
    DVZ_SCENE_SHADER_FEATURE_INSTANCING = 1u << 7,
    DVZ_SCENE_SHADER_FEATURE_SELECTION_MASK = 1u << 8,
} DvzSceneShaderFeatureFlag;


typedef struct DvzSceneShaderFeatures
{
    DvzSceneVisualDescKind kind;
    uint32_t topology;
    uint32_t flags;
} DvzSceneShaderFeatures;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether one shader-feature flag is set.
 *
 * @param features the shader feature descriptor
 * @param flag the feature flag
 * @return whether the flag is present
 */
static bool
_shader_features_has(const DvzSceneShaderFeatures* features, DvzSceneShaderFeatureFlag flag)
{
    ANN(features);
    return (features->flags & (uint32_t)flag) != 0;
}



/**
 * Resolve shader feature flags from a visual descriptor and pass mode.
 *
 * @param visual the visual descriptor
 * @param picking whether the pass writes pick ids
 * @param wboit_accumulation whether the pass is the WBOIT accumulation pass
 * @param out the output feature descriptor
 */
static void _scene_shader_features_resolve(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    DvzSceneShaderFeatures* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneShaderFeatures), 0, sizeof(DvzSceneShaderFeatures));

    out->kind = visual->kind;
    out->topology = visual->topology;
    if (picking)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_PICKING;
    if (wboit_accumulation)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_WBOIT_ACCUM;
    if (visual->depth_cue_enabled && (visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                                      visual->kind == DVZ_SCENE_VISUAL_DESC_PIXEL))
        out->flags |= DVZ_SCENE_SHADER_FEATURE_DEPTH_CUE;
    if (visual->point_style_enabled && visual->kind == DVZ_SCENE_VISUAL_DESC_POINT)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_POINT_STYLE;
    if (visual->has_selection_mask && !picking &&
        (visual->kind == DVZ_SCENE_VISUAL_DESC_POINT ||
         visual->kind == DVZ_SCENE_VISUAL_DESC_MARKER))
    {
        out->flags |= DVZ_SCENE_SHADER_FEATURE_SELECTION_MASK;
    }
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_SPHERE && visual->material_buffer_id != 0)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_LIGHTING;
    if (visual->has_normal)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_LIGHTING;
    if (visual->has_instance_transform)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_INSTANCING;
    if (visual->volume_state.render_mode == DVZ_VOLUME_RENDER_MIP)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_VOLUME_MIP;
    if (visual->volume_state.render_mode == DVZ_VOLUME_RENDER_COMPOSITE)
        out->flags |= DVZ_SCENE_SHADER_FEATURE_VOLUME_COMPOSITE;
}



/**
 * Attach built-in shader source pointers to a shader descriptor.
 *
 * @param out the output shader descriptor
 * @param shader the built-in shader id
 */
static void
_scene_shader_desc_set_builtin(DvzSceneVisualShaderDesc* out, DvzSceneBuiltinShader shader)
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
static void _scene_shader_desc_set_identity(
    DvzSceneVisualShaderDesc* out, const char* family, const char* variant)
{
    ANN(out);
    out->builtin_family = family;
    out->builtin_variant = variant != NULL ? variant : "default";
    out->builtin_pipeline = family;
}



/**
 * Resolve point-like shader metadata from feature flags.
 *
 * @param visual_name the visual key stem
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_point_like(
    const char* visual_name, const DvzSceneShaderFeatures* features, const char* format_tag,
    DvzSceneVisualShaderDesc* out)
{
    ANN(visual_name);
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool picking = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_PICKING);
    bool depth_cue = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_DEPTH_CUE);
    bool point_style = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_POINT_STYLE);
    bool selection = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_SELECTION_MASK) &&
                     !depth_cue && !point_style;

    const char* suffix = picking                    ? "_pick"
                         : selection                ? "_select"
                         : point_style && depth_cue ? "_cue_style"
                         : point_style              ? "_style"
                         : depth_cue                ? "_cue"
                                                    : "";
    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_%s%s%s", visual_name, suffix, format_tag);
    dvz_snprintf(
        out->fragment_key, sizeof(out->fragment_key), "_fs_%s%s%s", visual_name, suffix,
        format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_%s%s%s", visual_name, suffix,
        format_tag);

    DvzSceneBuiltinShader shader = DVZ_SCENE_BUILTIN_SHADER_POINT;
    if (features->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
        shader = picking     ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK
                 : selection ? DVZ_SCENE_BUILTIN_SHADER_MARKER_SELECTION
                             : DVZ_SCENE_BUILTIN_SHADER_MARKER;
    else if (features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
        shader = picking     ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK
                 : depth_cue ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_DEPTH_CUE
                             : DVZ_SCENE_BUILTIN_SHADER_PIXEL;
    else if (picking)
        shader = DVZ_SCENE_BUILTIN_SHADER_POINT_PICK;
    else if (selection)
        shader = DVZ_SCENE_BUILTIN_SHADER_POINT_SELECTION;
    else if (point_style)
        shader = depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE_DEPTH_CUE
                           : DVZ_SCENE_BUILTIN_SHADER_POINT_STYLE;
    else
        shader =
            depth_cue ? DVZ_SCENE_BUILTIN_SHADER_POINT_DEPTH_CUE : DVZ_SCENE_BUILTIN_SHADER_POINT;

    _scene_shader_desc_set_builtin(out, shader);
    _scene_shader_desc_set_identity(
        out,
        features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL    ? "scene.pixel"
        : features->kind == DVZ_SCENE_VISUAL_DESC_MARKER ? "scene.marker"
                                                         : "scene.point",
        picking                    ? "pick"
        : selection                ? "selection"
        : point_style && depth_cue ? "style_depth_cue"
        : point_style              ? "style"
        : depth_cue                ? "depth_cue"
                                   : "default");
    if (!picking)
    {
        if (features->kind == DVZ_SCENE_VISUAL_DESC_MARKER)
        {
            out->vertex_spirv_key = selection ? "marker_select_vert" : "marker_vert";
            out->fragment_spirv_key = selection ? "marker_select_frag" : "marker_frag";
        }
        else if (features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL)
        {
            out->vertex_spirv_key = depth_cue ? "pixel_cue_vert" : "pixel_vert";
            out->fragment_spirv_key = depth_cue ? "pixel_cue_frag" : "pixel_frag";
        }
        else if (point_style)
        {
            out->vertex_spirv_key = depth_cue ? "point_cue_style_vert" : "point_style_vert";
            out->fragment_spirv_key = depth_cue ? "point_cue_style_frag" : "point_style_frag";
        }
        else
        {
            out->vertex_spirv_key = selection   ? "point_select_vert"
                                    : depth_cue ? "point_cue_vert"
                                                : "point_vert";
            out->fragment_spirv_key = selection   ? "point_select_frag"
                                      : depth_cue ? "point_cue_frag"
                                                  : "point_frag";
        }
    }
    else
    {
        out->vertex_spirv_key = features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                                        features->kind == DVZ_SCENE_VISUAL_DESC_MARKER
                                    ? "pixel_pick_vert"
                                    : "point_pick_vert";
        out->fragment_spirv_key = features->kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                                          features->kind == DVZ_SCENE_VISUAL_DESC_MARKER
                                      ? "pixel_pick_frag"
                                      : "point_pick_frag";
    }
    return true;
}



/**
 * Resolve primitive shader metadata from feature flags.
 *
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_primitive(
    const DvzSceneShaderFeatures* features, const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool lit = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_LIGHTING);
    bool instanced = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_INSTANCING);
    bool picking = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_PICKING);
    if (picking)
    {
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_prim_pick%s%s",
            instanced ? "_inst" : "", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim_pick%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_pick_t%u%s%s",
            features->topology, instanced ? "_inst" : "", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_PICK);
        if (instanced)
        {
            out->vertex_glsl =
                _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED, false);
            out->vertex_wgsl =
                _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED, false);
            out->vertex_spirv_key = "primitive_instanced_vert";
        }
        _scene_shader_desc_set_identity(
            out, "scene.primitive", instanced ? "pick_instanced" : "pick");
        out->fragment_spirv_key = "primitive_pick_frag";
        return true;
    }
    if (_shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_WBOIT_ACCUM))
    {
        DvzSceneBuiltinShader shader =
            lit ? DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM_LIT : DVZ_SCENE_BUILTIN_SHADER_WBOIT_ACCUM;
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_wboit_accum_n%u%s", lit ? 1u : 0u,
            format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key), "_fs_wboit_accum_n%u%s", lit ? 1u : 0u,
            format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_wboit_accum_t%u_n%u%s",
            features->topology, lit ? 1u : 0u, format_tag);
        _scene_shader_desc_set_builtin(out, shader);
        if (instanced)
        {
            DvzSceneBuiltinShader vertex_shader =
                lit ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED
                    : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED;
            out->vertex_glsl = _builtin_shader_glsl(vertex_shader, false);
            out->vertex_wgsl = _builtin_shader_wgsl(vertex_shader, false);
            out->vertex_spirv_key =
                lit ? "primitive_lit_instanced_vert" : "primitive_instanced_vert";
        }
        _scene_shader_desc_set_identity(
            out, "scene.primitive",
            instanced ? (lit ? "wboit_lit_instanced" : "wboit_instanced")
                      : (lit ? "wboit_lit" : "wboit"));
        return true;
    }

    if (lit)
    {
        dvz_snprintf(
            out->vertex_key, sizeof(out->vertex_key), "_vs_prim_lit%s%s", instanced ? "_inst" : "",
            format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim_lit%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_lit_t%u%s%s",
            features->topology, instanced ? "_inst" : "", format_tag);
        _scene_shader_desc_set_builtin(
            out, instanced ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT_INSTANCED
                           : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT);
        _scene_shader_desc_set_identity(
            out, "scene.primitive", instanced ? "lit_instanced" : "lit");
        return true;
    }

    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_prim%s%s", instanced ? "_inst" : "",
        format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_prim%s", format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_prim_t%u%s%s", features->topology,
        instanced ? "_inst" : "", format_tag);
    _scene_shader_desc_set_builtin(
        out, instanced ? DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_INSTANCED
                       : DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE);
    _scene_shader_desc_set_identity(out, "scene.primitive", instanced ? "instanced" : "default");
    out->vertex_spirv_key = instanced ? "primitive_instanced_vert" : "primitive_vert";
    out->fragment_spirv_key = "primitive_frag";
    return true;
}


/**
 * Resolve segment shader metadata.
 *
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_segment(
    const DvzSceneShaderFeatures* features, const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool picking = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_PICKING);
    const char* suffix = picking ? "_pick" : "";
    dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_segment%s%s", suffix, format_tag);
    dvz_snprintf(
        out->fragment_key, sizeof(out->fragment_key), "_fs_segment%s%s", suffix, format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_segment%s%s", suffix, format_tag);
    _scene_shader_desc_set_builtin(
        out, picking ? DVZ_SCENE_BUILTIN_SHADER_SEGMENT_PICK : DVZ_SCENE_BUILTIN_SHADER_SEGMENT);
    _scene_shader_desc_set_identity(out, "scene.segment", picking ? "pick" : "default");
    out->vertex_spirv_key = "segment_vert";
    out->fragment_spirv_key = picking ? "segment_pick_frag" : "segment_frag";
    return true;
}


/**
 * Resolve path shader metadata.
 *
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_path(
    const DvzSceneShaderFeatures* features, const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool picking = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_PICKING);
    const char* suffix = picking ? "_pick" : "";
    dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_path%s%s", suffix, format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_path%s%s", suffix, format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_path%s%s", suffix, format_tag);
    _scene_shader_desc_set_builtin(
        out, picking ? DVZ_SCENE_BUILTIN_SHADER_PATH_PICK : DVZ_SCENE_BUILTIN_SHADER_PATH);
    _scene_shader_desc_set_identity(out, "scene.path", picking ? "pick" : "default");
    out->vertex_spirv_key = "path_vert";
    out->fragment_spirv_key = picking ? "path_pick_frag" : "path_frag";
    return true;
}


/**
 * Resolve sphere shader metadata from feature flags.
 *
 * @param features the shader features
 * @param format_tag the shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
static bool _scene_shader_desc_sphere(
    const DvzSceneShaderFeatures* features, const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(features);
    ANN(format_tag);
    ANN(out);

    bool picking = _shader_features_has(features, DVZ_SCENE_SHADER_FEATURE_PICKING);
    const char* suffix = picking ? "_pick" : "";
    dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_sphere%s%s", suffix, format_tag);
    dvz_snprintf(
        out->fragment_key, sizeof(out->fragment_key), "_fs_sphere%s%s", suffix, format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_sphere%s%s", suffix, format_tag);
    _scene_shader_desc_set_builtin(
        out, picking ? DVZ_SCENE_BUILTIN_SHADER_SPHERE_PICK : DVZ_SCENE_BUILTIN_SHADER_SPHERE);
    _scene_shader_desc_set_identity(out, "scene.sphere", picking ? "pick" : "default");
    out->vertex_spirv_key = "sphere_vert";
    out->fragment_spirv_key = picking ? "sphere_pick_frag" : "sphere_frag";
    return true;
}



/**
 * Resolve shader and pipeline cache-key metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
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

    DvzSceneShaderFeatures features = {0};
    _scene_shader_features_resolve(visual, picking, wboit_accumulation, &features);

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
        return _scene_shader_desc_point_like("pixel", &features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_POINT:
        return _scene_shader_desc_point_like("point", &features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_MARKER:
        return _scene_shader_desc_point_like("marker", &features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_SPHERE:
        return _scene_shader_desc_sphere(&features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
        return _scene_shader_desc_segment(&features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_PATH:
        return _scene_shader_desc_path(&features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        return _scene_shader_desc_primitive(&features, format_tag, out);

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_img%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_img%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_img%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_IMAGE);
        _scene_shader_desc_set_identity(out, "scene.image", "default");
        out->vertex_spirv_key = "image_vert";
        out->fragment_spirv_key = "image_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_LABELS_SINT:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_labels_sint%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key), "_fs_labels_sint%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_labels_sint%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_LABELS_SINT);
        _scene_shader_desc_set_identity(out, "scene.labels", "sint");
        out->vertex_spirv_key = "image_vert";
        out->fragment_spirv_key = "labels_sint_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_LABELS_UINT:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_labels_uint%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key), "_fs_labels_uint%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_labels_uint%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_LABELS_UINT);
        _scene_shader_desc_set_identity(out, "scene.labels", "uint");
        out->vertex_spirv_key = "image_vert";
        out->fragment_spirv_key = "labels_uint_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_glyph%s", format_tag);
        dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_glyph%s", format_tag);
        dvz_snprintf(out->pipeline_key, sizeof(out->pipeline_key), "_pipe_glyph%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_GLYPH);
        _scene_shader_desc_set_identity(out, "scene.glyph", "msdf");
        out->vertex_spirv_key = "glyph_vert";
        out->fragment_spirv_key = "glyph_frag";
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
    {
        bool mip = _shader_features_has(&features, DVZ_SCENE_SHADER_FEATURE_VOLUME_MIP);
        bool composite =
            _shader_features_has(&features, DVZ_SCENE_SHADER_FEATURE_VOLUME_COMPOSITE);
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_vol_slice%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key),
            composite ? "_fs_vol_composite%s"
            : mip     ? "_fs_vol_mip%s"
                      : "_fs_vol_slice%s",
            format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key),
            composite ? "_pipe_vol_composite%s"
            : mip     ? "_pipe_vol_mip%s"
                      : "_pipe_vol_slice%s",
            format_tag);
        DvzSceneBuiltinShader shader = composite ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_COMPOSITE
                                       : mip     ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_MIP
                                                 : DVZ_SCENE_BUILTIN_SHADER_VOLUME_SLICE;
        _scene_shader_desc_set_builtin(out, shader);
        _scene_shader_desc_set_identity(
            out, "scene.volume",
            composite ? "composite"
            : mip     ? "mip"
                      : "slice");
        out->vertex_spirv_key = "volume_slice_vert";
        out->fragment_spirv_key = composite ? "volume_composite_frag"
                                  : mip     ? "volume_mip_frag"
                                            : "volume_slice_frag";
        return true;
    }

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}
