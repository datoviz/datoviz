/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual shader descriptors                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "point/internal.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_marker_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    if (visual->image_texture_id != 0 && !picking && !wboit_accumulation)
    {
        dvz_snprintf(out->vertex_key, sizeof(out->vertex_key), "_vs_marker_bitmap%s", format_tag);
        dvz_snprintf(
            out->fragment_key, sizeof(out->fragment_key), "_fs_marker_bitmap%s", format_tag);
        dvz_snprintf(
            out->pipeline_key, sizeof(out->pipeline_key), "_pipe_marker_bitmap%s", format_tag);
        _scene_shader_desc_set_builtin(out, DVZ_SCENE_BUILTIN_SHADER_MARKER_BITMAP);
        _scene_shader_desc_set_identity(out, "scene.marker", "bitmap");
        out->vertex_spirv_key = "marker_bitmap_vert";
        out->fragment_spirv_key = "marker_bitmap_frag";
        return true;
    }
    return _scene_point_like_visual_shader_desc(
        "marker", visual, picking, wboit_accumulation, format_tag, out);
}
