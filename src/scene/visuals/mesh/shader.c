/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Mesh visual shader descriptors                                                               */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "mesh/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "_visual_pipeline_internal.h"
#include "primitive/internal.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve mesh visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_mesh_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);

    if (_scene_visual_desc_is_primitive(visual->kind))
    {
        return _scene_primitive_visual_shader_desc(
            visual, picking, wboit_accumulation, format_tag, out);
    }

    if (!_scene_visual_desc_is_textured_mesh(visual->kind) || picking || wboit_accumulation)
        return false;

    dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_mesh_textured%s", format_tag);
    dvz_snprintf(
        out->fragment_key, sizeof(out->fragment_key), "_fs_mesh_textured%s", format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_mesh_textured_t%u%s",
        visual->topology, format_tag);
    _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_MESH_TEXTURED);
    _scene_shader_desc_set_identity(out, "scene.mesh", "textured");
    out->vertex_spirv_key = "mesh_textured_vert";
    out->fragment_spirv_key = "mesh_textured_frag";
    return true;
}
