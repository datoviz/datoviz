/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual shader descriptors                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "image/internal.h"

#include "_assertions.h"
#include "_compat.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve image visual shader metadata.
 *
 * @param visual the visual descriptor
 * @param picking whether the render pass is a picking pass
 * @param wboit_accumulation whether the pass is an order-independent transparency pass
 * @param format_tag shader-format cache-key suffix
 * @param out the output shader descriptor
 * @return whether a shader descriptor was resolved
 */
bool _scene_image_visual_shader_desc(
    const DvzSceneVisualDesc* visual, bool picking, bool wboit_accumulation,
    const char* format_tag, DvzSceneVisualShaderDesc* out)
{
    ANN(visual);
    ANN(format_tag);
    ANN(out);
    if (visual->kind != DVZ_SCENE_VISUAL_DESC_IMAGE || picking || wboit_accumulation)
        return false;

    dvz_snprintf(
        out->vertex_key, sizeof(out->vertex_key), "_vs_img%s%s",
        visual->image_pixel_space ? "_px" : "", format_tag);
    dvz_snprintf(out->fragment_key, sizeof(out->fragment_key), "_fs_img%s", format_tag);
    dvz_snprintf(
        out->pipeline_key, sizeof(out->pipeline_key), "_pipe_img%s%s",
        visual->image_pixel_space ? "_px" : "", format_tag);
    _scene_shader_desc_set_builtin(
        out, visual->image_pixel_space ? DVZ_SCENE_BUILTIN_SHADER_IMAGE_PIXEL
                                       : DVZ_SCENE_BUILTIN_SHADER_IMAGE);
    _scene_shader_desc_set_identity(
        out, "scene.image", visual->image_pixel_space ? "pixel" : "default");
    out->vertex_spirv_key = visual->image_pixel_space ? "image_pixel_vert" : "image_vert";
    out->fragment_spirv_key = "image_frag";
    return true;
}
