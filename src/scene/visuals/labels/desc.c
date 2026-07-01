/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Labels visual descriptor lowering                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "labels/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve labels descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_labels_visual_desc_from_metadata(
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

    DvzSceneVisualDescKind desc_kind =
        _scene_visual_meta_desc_kind(&emitter->resources, meta);
    out->kind = desc_kind;
    out->labels_visual_index = meta->visual_index;
    out->labels_state = meta->labels_state;
    out->labels_lookup_count = meta->labels_lookup_count;
    for (uint32_t i = 0; i < meta->labels_lookup_count; i++)
    {
        out->labels_lookup[i][0] = meta->labels_lookup[i][0];
        out->labels_lookup[i][1] = meta->labels_lookup[i][1];
        out->labels_lookup[i][2] = meta->labels_lookup[i][2];
        out->labels_lookup[i][3] = meta->labels_lookup[i][3];
    }
    out->vbuf_ids[out->vbuf_count++] = uv_id;
    out->image_texture_id = tex_id;
    uint64_t pos_buf = out->vbuf_ids[0];
    out->topology = _scene_visual_desc_resource_topology(emitter, pos_buf);
    if (out->topology == UINT32_MAX)
        out->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    return true;
}
