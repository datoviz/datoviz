/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Point visual descriptor lowering                                                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "point/internal.h"

#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve point-like descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param desc_kind the descriptor kind owned by the caller
 * @param point_like_kind the point-like variant
 * @param require_marker_resources whether angle and shape are required
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_point_like_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta,
    DvzSceneVisualDescKind desc_kind, DvzScenePointLikeKind point_like_kind,
    bool require_marker_resources, DvzSceneVisualDesc* out, const char** error)
{
    ANN(emitter);
    ANN(meta);
    ANN(out);

    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_id, "typed visual metadata missing position resource",
            out, error))
        return false;

    uint64_t color_id = _scene_visual_desc_resource(emitter, meta->color_id);
    uint64_t size_id = _scene_visual_desc_resource(emitter, meta->size_id);
    uint64_t angle_id = _scene_visual_desc_resource(emitter, meta->angle_id);
    uint64_t shape_id = _scene_visual_desc_resource(emitter, meta->shape_id);
    uint64_t selection_id = _scene_visual_desc_resource(emitter, meta->selection_id);
    uint64_t item_state_style_id =
        _scene_visual_desc_resource(emitter, meta->item_state_style_id);
    if (color_id == 0 || size_id == 0)
    {
        if (error != NULL)
            *error = point_like_kind == DVZ_SCENE_POINT_LIKE_POINT
                         ? "typed point metadata missing color/size resource"
                         : "typed point-like metadata missing color/size resource";
        return false;
    }
    if (require_marker_resources && (angle_id == 0 || shape_id == 0))
    {
        if (error != NULL)
            *error = "typed marker metadata missing angle/shape resource";
        return false;
    }

    out->kind = desc_kind;
    out->point_like_kind = point_like_kind;
    out->vbuf_ids[out->vbuf_count++] = color_id;
    out->vbuf_ids[out->vbuf_count++] = size_id;
    if (require_marker_resources)
    {
        out->vbuf_ids[out->vbuf_count++] = angle_id;
        out->vbuf_ids[out->vbuf_count++] = shape_id;
    }
    if (selection_id != 0)
    {
        if (item_state_style_id == 0)
        {
            if (error != NULL)
                *error = "typed point-like metadata missing item-state style resource";
            return false;
        }
        out->vbuf_ids[out->vbuf_count++] = selection_id;
        out->has_item_state = true;
        out->item_state_style_buffer_id = item_state_style_id;
    }
    out->topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
    out->material_buffer_id = _scene_visual_desc_resource(emitter, meta->material_id);
    return true;
}



/**
 * Resolve point descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_point_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    return _scene_point_like_visual_desc_from_metadata(
        emitter, meta, DVZ_SCENE_VISUAL_DESC_POINT, DVZ_SCENE_POINT_LIKE_POINT, false, out, error);
}
