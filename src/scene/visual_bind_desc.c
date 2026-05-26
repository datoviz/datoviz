/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual bind descriptors                                                                */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene_resource_key.h"
#include "_shader_registry.h"
#include "_technique.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve bind-group role metadata for one visual descriptor.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_visual_bind_desc(
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

    switch (visual->kind)
    {
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
    case DVZ_SCENE_VISUAL_DESC_POINT:
    case DVZ_SCENE_VISUAL_DESC_MARKER:
    case DVZ_SCENE_VISUAL_DESC_SPHERE:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_material_set1 = caps.uses_material_set;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_material_set1 = caps.uses_material_set;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
    case DVZ_SCENE_VISUAL_DESC_PATH:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_material_set1 = caps.uses_material_set;
        out->material_buffer_id = visual->material_buffer_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_image_set1 = caps.uses_image_set;
        out->image_texture_id = visual->image_texture_id;
        return true;

    case DVZ_SCENE_VISUAL_DESC_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_LABELS_UINT:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_labels_set1 = caps.uses_image_set;
        out->labels_texture_id = visual->image_texture_id;
        out->labels_visual_index = visual->labels_visual_index;
        out->labels_state = visual->labels_state;
        return true;

    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_glyph_set1 = caps.uses_image_set;
        out->glyph_texture_id = visual->image_texture_id;
        out->glyph_atlas_encoding = visual->glyph_atlas_encoding;
        out->glyph_distance_range_px =
            visual->glyph_distance_range_px > 0.0f ? visual->glyph_distance_range_px : 4.0f;
        return true;

    case DVZ_SCENE_VISUAL_DESC_VOLUME:
        out->uses_common_set0 = caps.uses_common_set;
        out->uses_fixed_common = caps.fixed_controller;
        out->uses_volume_set1 = caps.uses_volume_set;
        out->volume_texture_id = visual->volume_texture_id;
        out->volume_transfer_texture_id = visual->volume_transfer_texture_id;
        out->volume_visual_index = visual->volume_visual_index;
        out->volume_transfer_rgba = visual->volume_transfer_rgba;
        out->volume_occluded = visual->volume_occluded;
        out->volume_occlusion = visual->volume_occlusion;
        out->volume_state = visual->volume_state;
        return true;

    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return false;
    }
}
