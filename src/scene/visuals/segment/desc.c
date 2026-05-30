/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Segment visual descriptor lowering                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "segment/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve stroke-quad descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_stroke_quad_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_start_id,
            "typed stroke metadata missing position_start resource", out, error))
        return false;

    uint64_t end_id = _scene_visual_desc_resource(emitter, meta->position_end_id);
    uint64_t color_id = _scene_visual_desc_resource(emitter, meta->color_id);
    uint64_t line_width_id = _scene_visual_desc_resource(emitter, meta->line_width_id);
    uint64_t index_id = _scene_visual_desc_resource(emitter, meta->index_id);
    uint64_t material_id = _scene_visual_desc_resource(emitter, meta->material_id);
    if (end_id == 0 || color_id == 0 || line_width_id == 0 || index_id == 0 ||
        material_id == 0)
    {
        if (error != NULL)
            *error = "typed segment metadata missing endpoint/color/width/index/cap resource";
        return false;
    }

    out->kind = DVZ_SCENE_VISUAL_DESC_SEGMENT;
    out->vbuf_ids[out->vbuf_count++] = end_id;
    out->vbuf_ids[out->vbuf_count++] = color_id;
    out->vbuf_ids[out->vbuf_count++] = line_width_id;
    out->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    out->index_buffer_id = index_id;
    out->material_buffer_id = material_id;
    return _scene_visual_desc_finish_index(
        emitter, meta, out, "typed segment index count exceeds uint32", error);
}



/**
 * Resolve segment descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_segment_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    return _scene_stroke_quad_visual_desc_from_metadata(emitter, meta, out, error);
}
