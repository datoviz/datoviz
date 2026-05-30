/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Mesh visual lowering                                                                         */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "mesh/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "_visual_pipeline_internal.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve mesh visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_mesh_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_INDEXED_MESH;
    out->desc_kind = _visual_family_state(visual)->field != NULL
                         ? _scene_visual_family_desc_kind(DVZ_VISUAL_TYPE_MESH)
                         : _scene_visual_family_desc_kind(DVZ_VISUAL_TYPE_PRIMITIVE);
    out->needs_material_params = _scene_visual_has_dense_attr(visual, "normal");
    return true;
}



/**
 * Resolve mesh visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_mesh_visual_bind_desc(
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
    out->uses_material_set1 = caps.uses_material_set;
    out->material_buffer_id = visual->material_buffer_id;

    if (_scene_visual_desc_is_textured_mesh(visual->kind))
    {
        out->uses_image_set1 = caps.uses_image_set;
        out->uses_textured_mesh_set1 = caps.uses_image_set;
        out->image_texture_id = visual->image_texture_id;
        out->image_nearest_sampler = visual->image_nearest_sampler;
    }
    return true;
}
