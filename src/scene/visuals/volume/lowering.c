/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Volume visual lowering                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "volume/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "sample_profile.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve volume texture descriptor kind from the retained field profile.
 *
 * @param visual the retained volume visual
 * @param out the output descriptor kind
 * @return whether the descriptor kind was resolved
 */
static bool _volume_desc_kind(const DvzVisual* visual, DvzSceneVisualDescKind* out)
{
    ANN(visual);
    ANN(out);
    *out = DVZ_SCENE_VISUAL_DESC_VOLUME;
    if (visual->field == NULL)
        return true;

    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, DVZ_FIELD_DIM_3D,
            &profile))
    {
        return true;
    }
    if (_scene_sample_profile_is_signed_label(&profile))
        *out = DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT;
    else if (_scene_sample_profile_is_unsigned_label(&profile))
        *out = DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT;
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve volume visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_volume_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_VOLUME_PROXY;
    return _volume_desc_kind(visual, &out->desc_kind);
}
