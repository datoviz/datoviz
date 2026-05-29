/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Labels visual shader descriptors                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "labels/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve labels visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_labels_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    (void)picking;
    if (wboit_accumulation)
        return false;

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT)
    {
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
    }

    if (visual->kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT)
    {
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
    }

    return false;
}
