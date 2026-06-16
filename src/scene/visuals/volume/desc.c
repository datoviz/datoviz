/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Volume visual descriptor lowering                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "volume/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve volume descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_volume_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_id, "typed visual metadata missing position resource",
            out, error))
        return false;

    uint64_t uvw_id = _scene_visual_desc_resource(emitter, meta->texcoords_id);
    uint64_t tex_id = _scene_visual_desc_resource(emitter, meta->volume_texture_id);
    uint64_t transfer_tex_id =
        _scene_visual_desc_resource(emitter, meta->volume_transfer_texture_id);
    uint64_t label_lookup_id =
        _scene_visual_desc_resource(emitter, meta->volume_label_lookup_id);
    if (tex_id == 0)
        tex_id = _scene_visual_desc_resource(emitter, meta->texture_id);
    if (uvw_id == 0 || tex_id == 0)
    {
        if (error != NULL)
            *error = "typed volume metadata missing texcoords/texture resource";
        return false;
    }
    if (!meta->volume_transfer_rgba && transfer_tex_id == 0)
    {
        if (error != NULL)
            *error = "typed scalar volume metadata missing transfer texture resource";
        return false;
    }

    DvzSceneVisualDescKind desc_kind =
        _scene_visual_meta_desc_kind(&emitter->resources, meta);
    if (
        (desc_kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
         desc_kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT) &&
        meta->volume_state.render_mode != DVZ_VOLUME_RENDER_SLICE &&
        meta->volume_state.render_mode != DVZ_VOLUME_RENDER_COMPOSITE)
    {
        if (error != NULL)
            *error = "label volumes only support slice and composite render modes";
        return false;
    }

    out->kind = desc_kind;
    out->vbuf_ids[out->vbuf_count++] = uvw_id;
    out->volume_texture_id = tex_id;
    out->volume_transfer_texture_id = transfer_tex_id;
    out->volume_label_lookup_buffer_id = label_lookup_id;
    out->volume_label_lookup_buffer_size =
        _scene_visual_desc_resource_size(emitter, label_lookup_id);
    out->volume_visual_index = meta->visual_index;
    out->volume_transfer_rgba = meta->volume_transfer_rgba;
    out->volume_color_role = meta->volume_color_role;
    out->volume_occluded = meta->volume_occluded;
    out->volume_occlusion = meta->volume_occlusion;
    out->volume_state = meta->volume_state;
    out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    return true;
}
