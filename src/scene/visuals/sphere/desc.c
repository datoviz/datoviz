/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Sphere visual descriptor lowering                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "sphere/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve sphere descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_sphere_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_id, "typed visual metadata missing position resource",
            out, error))
        return false;

    uint64_t color_id = _scene_visual_desc_resource(emitter, meta->color_id);
    uint64_t size_id = _scene_visual_desc_resource(emitter, meta->size_id);
    uint64_t selection_id = _scene_visual_desc_resource(emitter, meta->selection_id);
    uint64_t item_state_style_id =
        _scene_visual_desc_resource(emitter, meta->item_state_style_id);
    if (color_id == 0 || size_id == 0)
    {
        if (error != NULL)
            *error = "typed sphere metadata missing color/size resource";
        return false;
    }

    out->kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_SPHERE;
    out->vbuf_ids[out->vbuf_count++] = color_id;
    out->vbuf_ids[out->vbuf_count++] = size_id;
    if (selection_id != 0)
    {
        if (item_state_style_id == 0)
        {
            if (error != NULL)
                *error = "typed sphere metadata missing item-state style resource";
            return false;
        }
        out->vbuf_ids[out->vbuf_count++] = selection_id;
        out->has_item_state = true;
        out->item_state_style_buffer_id = item_state_style_id;
    }
    out->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    out->material_buffer_id = _scene_visual_desc_resource(emitter, meta->material_id);
    return true;
}
