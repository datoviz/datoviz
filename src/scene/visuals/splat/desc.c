/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Splat visual descriptor lowering                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "splat/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve splat descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_splat_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_id, "typed visual metadata missing position resource",
            out, error))
        return false;

    uint64_t color_id = _scene_visual_desc_resource(emitter, meta->color_id);
    uint64_t sigma_id = _scene_visual_desc_resource(emitter, meta->sigma_id);
    uint64_t angle_id = _scene_visual_desc_resource(emitter, meta->angle_id);
    if (color_id == 0 || sigma_id == 0 || angle_id == 0)
    {
        if (error != NULL)
            *error = "typed splat metadata missing color/sigma/angle resource";
        return false;
    }

    uint32_t item_count = out->vertex_count;
    out->kind = DVZ_SCENE_VISUAL_DESC_SPLAT;
    out->vbuf_ids[out->vbuf_count++] = color_id;
    out->vbuf_ids[out->vbuf_count++] = sigma_id;
    out->vbuf_ids[out->vbuf_count++] = angle_id;
    out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    out->vertex_count = 6;
    out->instance_count = item_count;
    return true;
}
