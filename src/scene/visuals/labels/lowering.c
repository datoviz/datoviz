/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Labels visual lowering                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "labels/internal.h"

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve label texture descriptor kind from the retained field profile.
 *
 * @param visual the retained labels visual
 * @param out the output descriptor kind
 * @return whether the field profile is supported
 */
static bool _labels_desc_kind(const DvzVisual* visual, DvzSceneVisualDescKind* out)
{
    ANN(visual);
    ANN(out);
    if (visual->field == NULL)
        return false;
    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile))
    {
        return false;
    }
    if (_scene_sample_profile_is_signed_label(&profile))
    {
        *out = DVZ_SCENE_VISUAL_DESC_LABELS_SINT;
        return true;
    }
    if (_scene_sample_profile_is_unsigned_label(&profile))
    {
        *out = DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
        return true;
    }
    return false;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve labels visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_labels_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_TEXTURED_QUAD;
    return _labels_desc_kind(visual, &out->desc_kind);
}



/**
 * Fill labels visual FramePlan metadata.
 *
 * @param visual the retained visual
 * @param lowering resolved lowering facts
 * @param metadata the metadata being built
 * @return whether metadata was filled
 */
bool _scene_labels_visual_fill_metadata(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata)
{
    ANN(visual);
    ANN(lowering);
    ANN(metadata);

    if (
        lowering->desc_kind != DVZ_SCENE_VISUAL_DESC_LABELS_SINT &&
        lowering->desc_kind != DVZ_SCENE_VISUAL_DESC_LABELS_UINT)
    {
        return true;
    }

    metadata->has_labels = true;
    metadata->labels_state = visual->labels;
    bool generated_quads = _scene_visual_has_dense_attr(visual, "extent") ||
                           _scene_visual_has_dense_attr(visual, "extent_px");
    if (!generated_quads)
        return true;
    if (visual->image_gpu.vertex_count > UINT32_MAX)
        return false;
    if (visual->image_gpu.vertex_count > 0)
        metadata->vertex_count = (uint32_t)visual->image_gpu.vertex_count;
    metadata->image_pixel_space = visual->image_gpu.pixel_space;
    return true;
}
