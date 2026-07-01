/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Image visual descriptor lowering                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "image/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve image descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_image_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_id, "typed visual metadata missing position resource",
            out, error))
        return false;

    uint64_t uv_id = _scene_visual_desc_resource(emitter, meta->texcoords_id);
    uint64_t tex_id = _scene_visual_desc_resource(emitter, meta->texture_id);
    if (uv_id == 0 || tex_id == 0)
    {
        if (error != NULL)
            *error = "typed image metadata missing texcoords/texture resource";
        return false;
    }

    out->kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
    out->image_pixel_space = meta->image_pixel_space;
    out->image_nearest_sampler = meta->image_nearest_sampler;
    out->image_color_role = _scene_visual_desc_resource_color_role(emitter, tex_id);
    if (out->image_color_role == DVZ_COLOR_ROLE_NONE)
        out->image_color_role = meta->image_color_role;
    out->vbuf_ids[out->vbuf_count++] = uv_id;
    out->image_texture_id = tex_id;
    uint64_t pos_buf = out->vbuf_ids[0];
    out->topology = _scene_visual_desc_resource_topology(emitter, pos_buf);
    if (out->topology == UINT32_MAX)
        out->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    return true;
}
