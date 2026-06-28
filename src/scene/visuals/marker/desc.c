/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual descriptor lowering                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"

#include "point/internal.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_marker_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    if (!_scene_point_like_visual_desc_from_metadata(
            emitter, meta, DVZ_SCENE_VISUAL_DESC_MARKER, DVZ_SCENE_POINT_LIKE_MARKER, true, out,
            error))
        return false;

    uint64_t tex_rect_id = _scene_visual_desc_resource(emitter, meta->tex_rect_id);
    uint64_t tex_id = _scene_visual_desc_resource(emitter, meta->texture_id);
    if (tex_rect_id != 0)
    {
        if (out->has_item_state)
        {
            if (error != NULL)
                *error = "bitmap marker symbols do not yet support item_state styling";
            return false;
        }
        if (tex_id == 0)
        {
            if (error != NULL)
                *error = "typed marker metadata missing symbol atlas texture resource";
            return false;
        }
        out->vbuf_ids[out->vbuf_count++] = tex_rect_id;
        out->image_texture_id = tex_id;
        out->glyph_atlas_encoding = meta->glyph_atlas_encoding;
        out->glyph_distance_range_px =
            meta->glyph_distance_range_px > 0.0f ? meta->glyph_distance_range_px : 4.0f;
        out->image_nearest_sampler = false;
    }
    return true;
}
