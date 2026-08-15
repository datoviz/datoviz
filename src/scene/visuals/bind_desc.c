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
#include "registry/registry.h"


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

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops((DvzVisualType)visual->visual_type);
    if (ops == NULL || ops->resolve_bind_desc == NULL)
        return false;
    if (!ops->resolve_bind_desc(visual, controller_mode, out))
        return false;
    if (
        out->uses_material_set1 || out->uses_item_state_style_set1 ||
        out->uses_textured_mesh_set1)
    {
        out->panel_light_buffer_id = visual->panel_light_buffer_id;
    }
    return true;
}


/**
 * Return whether scene occlusion sampling must occupy bind set 2 for one visual.
 *
 * @param visual the visual descriptor
 * @param provider typed work provider being prepared
 * @return whether scene occlusion should use set 2 instead of set 1
 */
bool _scene_visual_bind_desc_uses_scene_occlusion_set2(
    const DvzSceneVisualDesc* visual, DvzSceneWorkProviderKey provider)
{
    ANN(visual);
    bool surface_capture = provider == DVZ_SCENE_WORK_PROVIDER_SURFACE_CAPTURE;
    return visual->image_texture_id != 0 || visual->volume_texture_id != 0 ||
           (visual->material_buffer_id != 0 && !surface_capture);
}


/**
 * Apply render-pass binding policy after a visual bind descriptor has been resolved.
 *
 * @param bind bind descriptor to update
 * @param provider typed work provider being prepared
 * @param sampled_depth_id sampled depth texture id, or zero
 * @param sampled_depth_is_volume_occlusion whether the sampled depth comes from volume occlusion
 * @param scene_occlusion_depth_id scene occlusion depth texture id, or zero
 */
void _scene_visual_bind_desc_apply_pass_policy(
    DvzSceneVisualBindDesc* bind, DvzSceneWorkProviderKey provider, uint64_t sampled_depth_id,
    bool sampled_depth_is_volume_occlusion, uint64_t scene_occlusion_depth_id)
{
    ANN(bind);

    bool scene_occlusion_pass = provider == DVZ_SCENE_WORK_PROVIDER_SCENE_OCCLUSION;
    if (scene_occlusion_pass)
    {
        bind->uses_image_set1 = false;
        bind->uses_textured_mesh_set1 = false;
        bind->image_texture_id = 0;
        bind->uses_glyph_set1 = false;
        bind->glyph_texture_id = 0;
        bind->uses_material_set1 = false;
        bind->material_buffer_id = 0;
        bind->panel_light_buffer_id = 0;
        bind->uses_item_state_style_set1 = false;
        bind->item_state_style_buffer_id = 0;
        bind->uses_scene_occlusion_set2 = false;
        bind->scene_occlusion_depth_texture_id = 0;
    }

    bool volume_depth_producer_pass =
        provider == DVZ_SCENE_WORK_PROVIDER_VOLUME_OCCLUSION || scene_occlusion_pass;
    if (bind->uses_volume_set1 && !volume_depth_producer_pass && !bind->volume_occluded)
        bind->volume_occlusion.enabled = false;
    if (bind->uses_volume_set1 && volume_depth_producer_pass)
        bind->volume_occlusion.enabled = true;
    if (bind->uses_volume_set1)
    {
        if (volume_depth_producer_pass)
            bind->volume_bind_variant = 2;
        else if (sampled_depth_is_volume_occlusion && bind->volume_occluded)
            bind->volume_bind_variant = 1;
        else
            bind->volume_bind_variant = 0;
    }
    if (
        bind->uses_volume_set1 && sampled_depth_id != 0 &&
        (!sampled_depth_is_volume_occlusion || bind->volume_occluded))
    {
        bind->volume_depth_texture_id = sampled_depth_id;
    }
    if (bind->uses_scene_occlusion_set2)
        bind->scene_occlusion_depth_texture_id = scene_occlusion_depth_id;
}
