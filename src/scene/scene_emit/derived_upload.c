/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual derived upload emission                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "_scene.h"
#include "scene_emit/internal.h"
#include "scene_emit/visual_lowering.h"
#include "image/internal.h"
#include "stroke/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit family-owned derived geometry uploads before the generic dense-attribute path.
 *
 * @param figure the figure
 * @param plan the destination frame plan
 * @param visual the visual
 * @param visual_index the scene visual index
 * @param out_skip_dense_attrs whether generic dense attr uploads should be skipped
 * @param out_finished_visual whether no later generic upload path should run for this visual
 * @return whether emission can continue for this visual
 */
bool _scene_emit_visual_family_derived_uploads(
    const DvzFigure* figure, DvzFramePlan* plan, DvzVisual* visual, uint32_t visual_index,
    bool* out_skip_dense_attrs, bool* out_finished_visual)
{
    ANN(figure);
    ANN(plan);
    ANN(visual);
    ANN(out_skip_dense_attrs);
    ANN(out_finished_visual);
    *out_skip_dense_attrs = false;
    *out_finished_visual = false;

    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return true;

    if (lowering.renderable_kind == DVZ_RENDERABLE_STROKE_QUAD &&
        lowering.desc_kind == DVZ_SCENE_VISUAL_DESC_SEGMENT)
    {
        DvzVisualUploadPayload payloads[DVZ_VISUAL_UPLOAD_PAYLOAD_MAX] = {0};
        uint32_t payload_count = 0;
        bool dirty = _scene_visual_attrs_dirty(visual);
        if (lowering.needs_vector_params_sync)
        {
            _vector_sync_params(visual);
            dirty = dirty || visual->vector.stroke_gpu.dirty;
            if (dirty && _stroke_quad_vector_cache_rebuild(visual) &&
                _stroke_quad_vector_upload_payloads(visual, payloads, &payload_count))
            {
                _scene_emit_visual_buffer_payloads(
                    figure, plan, visual, visual_index, payloads, payload_count, 0);
            }
        }
        else
        {
            dirty = dirty || visual->segment.gpu.dirty;
            if (dirty && _stroke_quad_segment_cache_rebuild(visual) &&
                _stroke_quad_segment_upload_payloads(visual, payloads, &payload_count))
            {
                _scene_emit_visual_buffer_payloads(
                    figure, plan, visual, visual_index, payloads, payload_count, 0);
            }
        }
        *out_finished_visual = true;
        return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
    }

    if (lowering.renderable_kind == DVZ_RENDERABLE_PATH_STROKE)
    {
        if (lowering.needs_vector_params_sync)
            _vector_sync_params(visual);
        DvzVisualUploadPayload payloads[DVZ_VISUAL_UPLOAD_PAYLOAD_MAX] = {0};
        uint32_t payload_count = 0;
        DvzPathGpuCache* cache =
            visual->type == DVZ_VISUAL_TYPE_VECTOR ? &visual->vector.path_gpu : &visual->path.gpu;
        bool dirty = cache->dirty || _scene_visual_attrs_dirty(visual);
        if (dirty && _path_stroke_cache_rebuild(visual) &&
            _path_stroke_upload_payloads(visual, payloads, &payload_count))
        {
            _scene_emit_visual_buffer_payloads(
                figure, plan, visual, visual_index, payloads, payload_count, 0);
        }
        *out_finished_visual = true;
        return _scene_emit_visual_material_upload(figure, plan, visual, visual_index);
    }

    bool handled_image_quads = false;
    DvzVisualUploadPayload image_payloads[DVZ_VISUAL_UPLOAD_PAYLOAD_MAX] = {0};
    uint32_t image_payload_count = 0;
    if (!_image_generated_quad_derived_upload_payloads(
            figure, visual, _scene_visual_attrs_dirty(visual), image_payloads,
            &image_payload_count, &handled_image_quads))
    {
        return false;
    }
    if (handled_image_quads)
    {
        if (image_payload_count > 0)
            _scene_emit_visual_buffer_payloads(
                figure, plan, visual, visual_index, image_payloads, image_payload_count,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        *out_skip_dense_attrs = true;
    }
    return true;
}
