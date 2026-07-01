/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Path visual descriptor lowering                                                              */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "path/internal.h"

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_visual_pipeline_internal.h"
#include "primitive/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve stroked path descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_path_stroke_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    ANN(out);
    if (!_scene_visual_desc_set_primary_position(
            emitter, meta, meta->position_start_id,
            "typed stroke metadata missing position_start resource", out, error))
        return false;

    uint64_t curr_id = _scene_visual_desc_resource(emitter, meta->position_id);
    uint64_t next_id = _scene_visual_desc_resource(emitter, meta->position_end_id);
    uint64_t post_id = _scene_visual_desc_resource(emitter, meta->position_next_id);
    uint64_t color_id = _scene_visual_desc_resource(emitter, meta->color_id);
    uint64_t line_width_id = _scene_visual_desc_resource(emitter, meta->line_width_id);
    uint64_t flags_id = _scene_visual_desc_resource(emitter, meta->path_flags_id);
    uint64_t distance_id = _scene_visual_desc_resource(emitter, meta->path_distance_id);
    uint64_t index_id = _scene_visual_desc_resource(emitter, meta->index_id);
    uint64_t material_id = _scene_visual_desc_resource(emitter, meta->material_id);
    if (curr_id == 0 || next_id == 0 || post_id == 0 || color_id == 0 || line_width_id == 0 ||
        flags_id == 0 || distance_id == 0 || index_id == 0 || material_id == 0)
    {
        if (error != NULL)
            *error = "typed stroked path metadata missing adjacency/color/width/flags/"
                     "distance/index resource";
        return false;
    }

    out->kind = DVZ_SCENE_VISUAL_DESC_PATH;
    out->vbuf_ids[out->vbuf_count++] = curr_id;
    out->vbuf_ids[out->vbuf_count++] = next_id;
    out->vbuf_ids[out->vbuf_count++] = post_id;
    out->vbuf_ids[out->vbuf_count++] = color_id;
    out->vbuf_ids[out->vbuf_count++] = line_width_id;
    out->vbuf_ids[out->vbuf_count++] = flags_id;
    out->vbuf_ids[out->vbuf_count++] = distance_id;
    out->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    out->index_buffer_id = index_id;
    out->material_buffer_id = material_id;
    return _scene_visual_desc_finish_index(
        emitter, meta, out, "typed path index count exceeds uint32", error);
}



/**
 * Resolve path descriptor metadata.
 *
 * @param emitter the persistent emitter
 * @param meta the typed visual metadata
 * @param out the output visual descriptor
 * @param error optional diagnostic output
 * @return whether descriptor metadata was resolved
 */
bool _scene_path_visual_desc_from_metadata(
    DvzFramePlanEmitter* emitter, const DvzFramePlanVisualMeta* meta, DvzSceneVisualDesc* out,
    const char** error)
{
    if (_scene_visual_meta_is_stroked_path(&emitter->resources, meta))
        return _scene_path_stroke_visual_desc_from_metadata(emitter, meta, out, error);
    return _scene_primitive_visual_desc_from_metadata(emitter, meta, out, error);
}
