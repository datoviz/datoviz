/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Volume visual shader descriptors                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "volume/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve volume visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_volume_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    (void)picking;
    if (wboit_accumulation)
        return false;

    bool composite = visual->volume_state.render_mode == DVZ_VOLUME_RENDER_COMPOSITE;
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME)
    {
        bool mip = visual->volume_state.render_mode == DVZ_VOLUME_RENDER_MIP;
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

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT)
    {
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_vol_labels_sint%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key),
            composite ? "_fs_vol_labels_sint_composite%s" : "_fs_vol_labels_sint%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key),
            composite ? "_pipe_vol_labels_sint_composite%s" : "_pipe_vol_labels_sint%s",
            format_tag);
        _scene_shader_desc_set_builtin(
            out, composite ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_SINT_COMPOSITE
                           : DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_SINT_SLICE);
        _scene_shader_desc_set_identity(
            out, "scene.volume", composite ? "labels_sint_composite" : "labels_sint_slice");
        out->vertex_spirv_key = "volume_slice_vert";
        out->fragment_spirv_key =
            composite ? "volume_labels_sint_composite_frag" : "volume_labels_sint_slice_frag";
        return true;
    }

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT)
    {
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_vol_labels_uint%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key),
            composite ? "_fs_vol_labels_uint_composite%s" : "_fs_vol_labels_uint%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key),
            composite ? "_pipe_vol_labels_uint_composite%s" : "_pipe_vol_labels_uint%s",
            format_tag);
        _scene_shader_desc_set_builtin(
            out, composite ? DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_UINT_COMPOSITE
                           : DVZ_SCENE_BUILTIN_SHADER_VOLUME_LABELS_UINT_SLICE);
        _scene_shader_desc_set_identity(
            out, "scene.volume", composite ? "labels_uint_composite" : "labels_uint_slice");
        out->vertex_spirv_key = "volume_slice_vert";
        out->fragment_spirv_key =
            composite ? "volume_labels_uint_composite_frag" : "volume_labels_uint_slice_frag";
        return true;
    }

    return false;
}
