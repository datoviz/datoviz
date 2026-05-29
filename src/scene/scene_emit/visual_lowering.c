/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual lowering                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "scene_emit/visual_lowering.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve retained visual family state into reusable renderable lowering facts.
 *
 * @param visual the retained visual
 * @param out the output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_visual_lowering_resolve(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual->type);
    if (ops == NULL || ops->resolve_lowering == NULL)
        return false;
    return ops->resolve_lowering(visual, out);
}



/**
 * Return the reusable renderable primitive kind emitted by one retained visual.
 *
 * @param visual the retained visual
 * @return renderable primitive kind
 */
DvzRenderableKind _scene_visual_lowering_renderable_kind(const DvzVisual* visual)
{
    ANN(visual);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return DVZ_RENDERABLE_NONE;
    return lowering.renderable_kind;
}



/**
 * Return the descriptor kind emitted by one retained visual.
 *
 * @param visual the retained visual
 * @return descriptor kind
 */
DvzSceneVisualDescKind _scene_visual_lowering_desc_kind(const DvzVisual* visual)
{
    ANN(visual);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return DVZ_SCENE_VISUAL_DESC_NONE;
    return lowering.desc_kind;
}



/**
 * Fill family-owned FramePlan metadata fields from one retained visual.
 *
 * @param visual the retained visual
 * @param metadata the metadata being built
 * @return whether the family-owned metadata was valid
 */
bool _scene_visual_lowering_fill_metadata(
    const DvzVisual* visual, DvzFramePlanVisualMeta* metadata)
{
    ANN(visual);
    ANN(metadata);
    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return false;
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual->type);
    if (ops == NULL)
        return false;
    if (ops->fill_metadata == NULL)
        return true;
    return ops->fill_metadata(visual, &lowering, metadata);
}



/**
 * Return whether one visual samples the panel volume-occlusion target.
 *
 * @param visual the retained visual
 * @return whether volume occlusion is enabled for the visual
 */
bool _scene_visual_lowering_volume_occluded(const DvzVisual* visual)
{
    ANN(visual);
    return _visual_family_state(visual)->volume_occluded;
}
