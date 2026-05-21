/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual pass capabilities                                                               */
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
 * Resolve common pass capabilities from normalized visual facts.
 *
 * @param kind the visual descriptor kind
 * @param alpha_mode the visual alpha mode
 * @param controller_mode the panel controller attachment mode
 * @param has_normals whether the visual has normals
 * @param has_material_resource whether a material uniform resource exists
 * @param depth_cue_enabled whether retained material state enables depth cueing
 * @param out the output pass capabilities
 */
static void _scene_visual_pass_caps_resolve(
    DvzSceneVisualDescKind kind, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    bool has_normals, bool has_material_resource, bool depth_cue_enabled, bool depth_test_enabled,
    DvzSceneVisualPassCaps* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPassCaps), 0, sizeof(DvzSceneVisualPassCaps));

    bool primitive = _scene_visual_desc_is_primitive(kind);
    bool stroke = _scene_visual_desc_is_stroke(kind);
    bool sphere = _scene_visual_desc_is_sphere(kind);
    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool image = _scene_visual_desc_is_image(kind);
    bool volume = _scene_visual_desc_is_volume(kind);
    bool fixed = controller_mode == DVZ_CONTROLLER_FIXED;
    bool wboit = _scene_alpha_mode_is_wboit(alpha_mode);
    bool depth_peel = _scene_alpha_mode_is_depth_peel(alpha_mode);
    bool transparent_blend = _scene_alpha_mode_is_blended(alpha_mode);

    out->kind = kind;
    out->alpha_mode = alpha_mode;
    out->controller_mode = controller_mode;
    out->fixed_controller = fixed;
    out->has_normals = has_normals;
    out->depth_test_enabled = depth_test_enabled;
    out->draws_in_wboit_pass = wboit;
    out->draws_in_depth_peel_pass = depth_peel;
    out->draws_in_transparent_blend_pass = transparent_blend;
    out->draws_in_opaque_pass = !wboit && !depth_peel && !transparent_blend;
    out->uses_source_over_blend = _scene_alpha_mode_is_blended(alpha_mode);
    out->writes_color = kind != DVZ_SCENE_VISUAL_DESC_NONE;
    out->writes_depth = out->draws_in_opaque_pass &&
                        (primitive || stroke || point_like || sphere) && !fixed &&
                        depth_test_enabled;
    out->can_write_depth =
        (primitive || stroke || point_like || sphere) && !fixed && depth_test_enabled;
    out->can_depth_test =
        (primitive || stroke || point_like || sphere) && !fixed && depth_test_enabled;
    out->samples_depth = volume && !fixed;
    out->needs_depth_attachment = out->can_depth_test || out->samples_depth;
    out->eligible_for_depth_postprocess = out->draws_in_opaque_pass && out->writes_depth;
    out->eligible_for_gbuffer =
        out->draws_in_opaque_pass && ((primitive && has_normals) || sphere) && out->writes_depth;
    out->uses_common_set = kind != DVZ_SCENE_VISUAL_DESC_NONE;
    out->needs_material_layout =
        (primitive && has_normals) || stroke || sphere || (point_like && has_material_resource);
    out->uses_material_set = out->needs_material_layout && has_material_resource;
    out->uses_image_set = image;
    out->uses_volume_set = volume;
    out->supports_depth_cue = (primitive && has_normals) || point_like || sphere;
    out->depth_cue_enabled = out->supports_depth_cue && depth_cue_enabled;
}



/**
 * Resolve pass capabilities from one retained visual attachment.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param out the output pass capabilities
 * @return whether capabilities were resolved
 */
bool _scene_visual_pass_caps_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(attach);
    ANN(out);

    DvzSceneVisualDescKind kind = DVZ_SCENE_VISUAL_DESC_NONE;
    switch (visual->type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
        kind = visual->type == DVZ_VISUAL_TYPE_PIXEL    ? DVZ_SCENE_VISUAL_DESC_PIXEL
               : visual->type == DVZ_VISUAL_TYPE_MARKER ? DVZ_SCENE_VISUAL_DESC_MARKER
                                                        : DVZ_SCENE_VISUAL_DESC_POINT;
        break;
    case DVZ_VISUAL_TYPE_SPHERE:
        kind = DVZ_SCENE_VISUAL_DESC_SPHERE;
        break;
    case DVZ_VISUAL_TYPE_SEGMENT:
        kind = DVZ_SCENE_VISUAL_DESC_SEGMENT;
        break;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
        kind = DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        break;
    case DVZ_VISUAL_TYPE_PATH:
        kind = _scene_visual_has_dense_attr(visual, "line_width")
                   ? DVZ_SCENE_VISUAL_DESC_PATH
                   : DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        break;
    case DVZ_VISUAL_TYPE_IMAGE:
        kind = DVZ_SCENE_VISUAL_DESC_IMAGE;
        break;
    case DVZ_VISUAL_TYPE_GLYPH:
        kind = DVZ_SCENE_VISUAL_DESC_GLYPH;
        break;
    case DVZ_VISUAL_TYPE_VOLUME:
        kind = DVZ_SCENE_VISUAL_DESC_VOLUME;
        break;
    case DVZ_VISUAL_TYPE_NONE:
    default:
        return false;
    }

    bool has_normals = false;
    if (_scene_visual_desc_is_primitive(kind))
    {
        int normal_idx = _attr_index(visual, "normal");
        has_normals = normal_idx >= 0 && visual->attrs[normal_idx].data != NULL &&
                      visual->attrs[normal_idx].item_count > 0;
    }

    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool stroked_path =
        visual->type == DVZ_VISUAL_TYPE_PATH && _scene_visual_has_dense_attr(visual, "line_width");
    bool has_material_resource =
        has_normals || visual->type == DVZ_VISUAL_TYPE_SEGMENT || stroked_path ||
        visual->type == DVZ_VISUAL_TYPE_SPHERE ||
        (point_like && visual->material.depth_cue_enabled) ||
        (visual->type == DVZ_VISUAL_TYPE_POINT && visual->material.point_style_enabled) ||
        visual->type == DVZ_VISUAL_TYPE_MARKER;
    _scene_visual_pass_caps_resolve(
        kind, visual->alpha_mode, attach->controller_mode, has_normals, has_material_resource,
        visual->material.depth_cue_enabled, visual->depth_test_enabled, out);
    return true;
}



/**
 * Resolve pass capabilities from one FramePlan visual descriptor.
 *
 * @param visual the visual descriptor
 * @param alpha_mode the visual alpha mode
 * @param controller_mode the panel controller attachment mode
 * @param out the output pass capabilities
 * @return whether capabilities were resolved
 */
bool _scene_visual_pass_caps_from_desc(
    const DvzSceneVisualDesc* visual, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(out);
    if (visual->kind == DVZ_SCENE_VISUAL_DESC_NONE)
        return false;

    _scene_visual_pass_caps_resolve(
        visual->kind, alpha_mode, controller_mode, visual->has_normal,
        visual->material_buffer_id != 0, visual->depth_cue_enabled, visual->depth_test_enabled,
        out);
    return true;
}



/**
 * Return whether a scene render node needs a depth attachment for fixed-function depth testing.
 *
 * @param emitter the persistent emitter
 * @param render the render node
 * @return whether the render node contains depth-tested geometry
 */
bool _scene_render_needs_depth(DvzFramePlanEmitter* emitter, const DvzFramePlanNode* render)
{
    ANN(emitter);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER || render->u.render.visual_count == 0)
        return false;

    for (uint32_t i = 0; i < render->u.render.visual_count; i++)
    {
        if (render->u.render.controller_modes[i] == DVZ_CONTROLLER_FIXED)
            continue;

        const DvzFramePlanVisualMeta* meta = &render->u.render.visual_metadata[i];
        if (meta->has_metadata)
        {
            bool segment_like = meta->visual_type == DVZ_VISUAL_TYPE_SEGMENT ||
                                _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
            uint64_t pos_buf = _scene_visual_resource_lookup_label(
                &emitter->resources, segment_like ? meta->position_start_id : meta->position_id);
            if (pos_buf == 0)
                continue;
            bool has_color =
                _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id) != 0;
            DvzScenePointLikeKind point_like_kind = DVZ_SCENE_POINT_LIKE_POINT;
            if (_scene_visual_meta_point_like_kind(meta->visual_type, &point_like_kind))
            {
                if (has_color)
                    return true;
                continue;
            }
            if (meta->visual_type == DVZ_VISUAL_TYPE_SPHERE)
            {
                bool has_size =
                    _scene_visual_resource_lookup_label(&emitter->resources, meta->size_id) != 0;
                if (has_color && has_size)
                    return true;
                continue;
            }
            if (segment_like)
            {
                bool has_end = _scene_visual_resource_lookup_label(
                                   &emitter->resources, meta->position_end_id) != 0;
                bool has_line_width = _scene_visual_resource_lookup_label(
                                          &emitter->resources, meta->line_width_id) != 0;
                bool has_index =
                    _scene_visual_resource_lookup_label(&emitter->resources, meta->index_id) != 0;
                if (has_end && has_color && has_line_width && has_index)
                    return true;
                continue;
            }
            if (!_scene_visual_meta_is_primitive(meta->visual_type))
                continue;
            bool has_topology = _resource_topology(&emitter->resources, pos_buf) != UINT32_MAX ||
                                meta->topology != UINT32_MAX;
            if (has_color && has_topology)
                return true;
            continue;
        }

        uint64_t pos_buf = _scene_render_visual_resource_id(
            emitter, render->u.render.visuals[i], DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION);
        if (pos_buf == 0)
            continue;

        bool has_topology = _resource_topology(&emitter->resources, pos_buf) != UINT32_MAX;
        bool has_color =
            _scene_render_visual_resource_id(
                emitter, render->u.render.visuals[i], DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR) != 0;
        if (has_color && has_topology)
            return true;
    }

    return false;
}
