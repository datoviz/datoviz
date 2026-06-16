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
#include "_visual_pipeline_internal.h"
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
    if (_visual_family_state(visual)->field == NULL)
        return true;

    DvzSceneSampleProfile profile = {0};
    if (!_scene_sample_profile_resolve(
            _visual_family_state(visual)->field->desc.format, _visual_family_state(visual)->field->desc.semantic, DVZ_FIELD_DIM_3D,
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



/**
 * Fill volume visual FramePlan metadata.
 *
 * @param visual the retained visual
 * @param lowering resolved lowering facts
 * @param metadata the metadata being built
 * @return whether metadata was filled
 */
bool _scene_volume_visual_fill_metadata(
    const DvzVisual* visual, const DvzVisualLowering* lowering,
    DvzFramePlanVisualMeta* metadata)
{
    ANN(visual);
    ANN(lowering);
    ANN(metadata);

    if (!_scene_visual_desc_is_volume(lowering->desc_kind))
        return true;
    metadata->has_volume = true;
    metadata->volume_state = _visual_family_state(visual)->volume;
    metadata->volume_occluded = _visual_family_state(visual)->volume_occluded;

    DvzSceneSampleProfile profile = {0};
    metadata->volume_transfer_rgba =
        _visual_family_state(visual)->field != NULL &&
        _scene_sample_profile_resolve(
            _visual_family_state(visual)->field->desc.format, _visual_family_state(visual)->field->desc.semantic, _visual_family_state(visual)->field->desc.dim,
            &profile) &&
        _scene_sample_profile_is_direct_rgba(&profile);
    if (_visual_family_state(visual)->field != NULL)
        metadata->volume_color_role = _visual_family_state(visual)->field->desc.color_role;
    return true;
}



/**
 * Resolve volume visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_volume_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    ANN(visual);
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualBindDesc), 0, sizeof(DvzSceneVisualBindDesc));
    out->uses_scene_occlusion_set2 = visual->scene_occluded;
    out->scene_occlusion = visual->scene_occlusion;
    out->controller_mode = controller_mode;

    DvzSceneVisualPassCaps caps = {0};
    if (!_scene_visual_pass_caps_from_desc(visual, DVZ_ALPHA_OPAQUE, controller_mode, &caps))
        return false;
    out->uses_common_set0 = caps.uses_common_set;
    out->uses_fixed_common = caps.fixed_controller;
    out->uses_volume_set1 = caps.uses_volume_set;
    out->volume_texture_id = visual->volume_texture_id;
    out->volume_transfer_texture_id = visual->volume_transfer_texture_id;
    out->volume_label_lookup_buffer_id = visual->volume_label_lookup_buffer_id;
    out->volume_label_lookup_buffer_size = visual->volume_label_lookup_buffer_size;
    out->volume_visual_index = visual->volume_visual_index;
    out->volume_transfer_rgba = visual->volume_transfer_rgba;
    out->volume_color_role = visual->volume_color_role;
    out->volume_occluded = visual->volume_occluded;
    out->volume_occlusion = visual->volume_occlusion;
    out->volume_state = visual->volume_state;
    if (
        visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
        visual->kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT)
        out->volume_state.sampling = DVZ_VOLUME_SAMPLING_NEAREST;
    return true;
}
