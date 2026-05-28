/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Splat visual shader descriptors                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "splat/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve splat visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_splat_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    if (visual->kind != DVZ_SCENE_VISUAL_DESC_SPLAT || picking)
        return false;

    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_splat%s%s",
        wboit_accumulation ? "_wboit" : "", format_tag);
    dvz_snprintf(
        out->fragment_key, sizeof(out->fragment_key), "_fs_splat%s%s",
        wboit_accumulation ? "_wboit" : "", format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_splat%s%s",
        wboit_accumulation ? "_wboit" : "", format_tag);
    _scene_shader_desc_set_builtin(
        out, wboit_accumulation ? DVZ_SCENE_BUILTIN_SHADER_SPLAT_WBOIT
                                : DVZ_SCENE_BUILTIN_SHADER_SPLAT);
    _scene_shader_desc_set_identity(
        out, "scene.splat", wboit_accumulation ? "wboit" : "default");
    out->vertex_spirv_key = "splat_vert";
    out->fragment_spirv_key = wboit_accumulation ? "splat_wboit_frag" : "splat_frag";
    return true;
}
