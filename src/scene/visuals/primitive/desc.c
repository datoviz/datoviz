/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Primitive visual descriptor lowering                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "primitive/internal.h"

#include <stdint.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve primitive descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_primitive_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_id, "typed visual metadata missing position resource",
            out, error))
        return false;

    uint64_t color_id = _scene_visual_desc_resource(emitter, meta->color_id);
    if (color_id == 0)
    {
        if (error != NULL)
            *error = "typed primitive metadata missing color resource";
        return false;
    }

    out->kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
    out->vbuf_ids[out->vbuf_count++] = color_id;
    uint64_t normal_id = _scene_visual_desc_resource(emitter, meta->normal_id);
    if (normal_id != 0)
    {
        out->vbuf_ids[out->vbuf_count++] = normal_id;
        out->has_normal = true;
    }
    uint64_t instance_transform_id =
        _scene_visual_desc_resource(emitter, meta->instance_transform_id);
    if (instance_transform_id != 0)
    {
        out->vbuf_ids[out->vbuf_count++] = instance_transform_id;
        out->has_instance_transform = true;
        uint64_t transform_bytes = _scene_visual_desc_resource_size(emitter, instance_transform_id);
        uint64_t instance_count = transform_bytes / (16 * sizeof(float));
        if (instance_count == 0 || instance_count > UINT32_MAX)
        {
            if (error != NULL)
                *error = "typed mesh instance_transform count is invalid";
            return false;
        }
        out->instance_count = (uint32_t)instance_count;
    }

    uint64_t pos_buf = out->vbuf_ids[0];
    out->topology = _scene_visual_desc_resource_topology(emitter, pos_buf);
    if (out->topology == UINT32_MAX)
        out->topology = meta->topology;
    if (out->topology == UINT32_MAX)
    {
        if (error != NULL)
            *error = "typed primitive metadata missing topology resource";
        return false;
    }
    out->index_buffer_id = _scene_visual_desc_resource(emitter, meta->index_id);
    out->material_buffer_id = _scene_visual_desc_resource(emitter, meta->material_id);
    return _scene_visual_desc_finish_index(
        emitter, meta, out, "typed index metadata count exceeds uint32", error);
}
