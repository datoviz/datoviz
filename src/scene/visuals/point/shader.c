/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Point visual shader descriptors                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "point/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve point-like visual shader metadata.
 *
 * @param visual_name visual key stem
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_point_like_visual_shader_desc(
    const char* visual_name, const DvzSceneVisualDesc* visual, bool picking,
    bool wboit_accumulation, const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual_name);
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    (void)wboit_accumulation;

    bool point = visual->point_like_kind == DVZ_SCENE_POINT_LIKE_POINT;
    bool pixel = visual->point_like_kind == DVZ_SCENE_POINT_LIKE_PIXEL;
    bool marker = visual->point_like_kind == DVZ_SCENE_POINT_LIKE_MARKER;
    bool point_like = point || pixel || marker;
    if (!point_like)
        return false;

    bool depth_cue = visual->depth_cue_enabled && (point || pixel);
    bool point_style = visual->point_style_enabled && point;
    bool selection = visual->has_item_state && !picking && (point || marker) &&
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
    if (marker)
        shader = picking     ? DVZ_SCENE_BUILTIN_SHADER_PIXEL_PICK
                 : selection ? DVZ_SCENE_BUILTIN_SHADER_MARKER_SELECTION
                             : DVZ_SCENE_BUILTIN_SHADER_MARKER;
    else if (pixel)
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
        out, pixel ? "scene.pixel" : marker ? "scene.marker" : "scene.point",
        picking                    ? "pick"
        : selection                ? "selection"
        : point_style && depth_cue ? "style_depth_cue"
        : point_style              ? "style"
        : depth_cue                ? "depth_cue"
                                   : "default");
    if (!picking)
    {
        if (marker)
        {
            out->vertex_spirv_key = selection ? "marker_select_vert" : "marker_vert";
            out->fragment_spirv_key = selection ? "marker_select_frag" : "marker_frag";
        }
        else if (pixel)
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
        out->vertex_spirv_key = pixel || marker ? "pixel_pick_vert" : "point_pick_vert";
        out->fragment_spirv_key = pixel || marker ? "pixel_pick_frag" : "point_pick_frag";
    }
    return true;
}



/**
 * Resolve point visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_point_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    return _scene_point_like_visual_shader_desc(
        "point", visual, picking, wboit_accumulation, format_tag, out);
}
