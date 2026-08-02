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
#include "render_contract/render_contract.h"
#include "registry/registry.h"
#include "scene_emit/visual_lowering.h"
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
void _scene_visual_pass_caps_resolve(
    DvzSceneVisualDescKind kind, DvzAlphaMode alpha_mode, DvzControllerMode controller_mode,
    bool has_normals, bool has_material_resource, bool depth_cue_enabled, bool depth_test_enabled,
    DvzSceneVisualLayer baseline_layer, uint32_t baseline_product_caps,
    uint32_t baseline_phase_participation, DvzSceneVisualPassCaps* out)
{
    ANN(out);
    dvz_memset(out, sizeof(DvzSceneVisualPassCaps), 0, sizeof(DvzSceneVisualPassCaps));

    bool primitive = _scene_visual_desc_is_primitive(kind);
    bool textured_mesh = kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH;
    bool raster_mesh = primitive || textured_mesh;
    bool stroke = _scene_visual_desc_is_stroke(kind);
    bool sphere = _scene_visual_desc_is_sphere(kind);
    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool splat = kind == DVZ_SCENE_VISUAL_DESC_SPLAT;
    bool image = _scene_visual_desc_is_image(kind);
    bool volume = _scene_visual_desc_is_volume(kind);
    bool fixed = controller_mode == DVZ_CONTROLLER_FIXED;
    bool wboit = _scene_alpha_mode_is_wboit(alpha_mode);
    bool depth_peel = _scene_alpha_mode_is_depth_peel(alpha_mode);
    bool transparent_blend = _scene_alpha_mode_is_blended(alpha_mode);

    out->kind = kind;
    out->alpha_mode = alpha_mode;
    out->controller_mode = controller_mode;
    out->layer = baseline_layer;
    out->product_caps = baseline_product_caps;
    out->phase_participation = baseline_phase_participation;
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
                        (raster_mesh || stroke || point_like || splat || sphere) && !fixed &&
                        depth_test_enabled;
    out->can_write_depth =
        (raster_mesh || stroke || point_like || splat || sphere) && !fixed && depth_test_enabled;
    out->can_depth_test =
        (raster_mesh || stroke || point_like || splat || sphere) && !fixed && depth_test_enabled;
    out->samples_depth = volume && !fixed;
    out->needs_depth_attachment = out->can_depth_test || out->samples_depth;
    out->eligible_for_depth_postprocess = out->draws_in_opaque_pass && out->writes_depth;
    out->eligible_for_gbuffer =
        out->draws_in_opaque_pass && ((primitive && has_normals) || sphere) && out->writes_depth;
    out->uses_common_set = kind != DVZ_SCENE_VISUAL_DESC_NONE;
    out->needs_material_layout =
        (primitive && has_normals) || stroke || sphere || (point_like && has_material_resource);
    out->uses_material_set = out->needs_material_layout && has_material_resource;
    out->uses_image_set = image || textured_mesh;
    out->uses_labels_set =
        kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
        kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT;
    out->uses_glyph_set = kind == DVZ_SCENE_VISUAL_DESC_GLYPH;
    out->uses_volume_set = volume;
    out->supports_depth_cue = (primitive && has_normals) || point_like || sphere;
    out->depth_cue_enabled = out->supports_depth_cue && depth_cue_enabled;

    const bool surface_layer = baseline_layer == DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE ||
                               baseline_layer == DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED;
    if (volume)
        out->layer = DVZ_SCENE_VISUAL_LAYER_VOLUME;
    else if (surface_layer && alpha_mode == DVZ_ALPHA_MASK)
        out->layer = DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED;
    else if (surface_layer && (wboit || depth_peel || transparent_blend))
        out->layer = DVZ_SCENE_VISUAL_LAYER_TRANSPARENT;

    if (!out->writes_color)
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SCENE_COLOR;
    if (!out->can_write_depth)
    {
        out->phase_participation &= ~(uint32_t)DVZ_SCENE_VISUAL_PHASE_SURFACE_CAPTURE;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_DEPTH;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_NORMAL;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_COVERAGE;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_CONSUME_AMBIENT_VISIBILITY;
    }
    if (!out->eligible_for_gbuffer)
    {
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_NORMAL;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_CONSUME_AMBIENT_VISIBILITY;
    }

    switch (out->layer)
    {
    case DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE:
    case DVZ_SCENE_VISUAL_LAYER_SURFACE_MASKED:
        break;
    case DVZ_SCENE_VISUAL_LAYER_TRANSPARENT:
        out->phase_participation = DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_DEPTH;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_NORMAL;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_COVERAGE;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_CONSUME_AMBIENT_VISIBILITY;
        if (wboit)
        {
            out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SCENE_COLOR;
            out->product_caps |=
                DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_TRANSPARENT_ACCUMULATION |
                DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_TRANSPARENT_TRANSMITTANCE;
        }
        else if (depth_peel)
        {
            out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SCENE_COLOR;
            out->product_caps |=
                DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_TRANSPARENT_ACCUMULATION |
                DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_TRANSPARENT_PEEL_DEPTH;
        }
        break;
    case DVZ_SCENE_VISUAL_LAYER_VOLUME:
        out->phase_participation = DVZ_SCENE_VISUAL_PHASE_VOLUME_SHADING;
        if (transparent_blend)
            out->phase_participation |= DVZ_SCENE_VISUAL_PHASE_TRANSPARENT_SHADING;
        break;
    case DVZ_SCENE_VISUAL_LAYER_OVERLAY:
        out->phase_participation = DVZ_SCENE_VISUAL_PHASE_OVERLAY;
        break;
    case DVZ_SCENE_VISUAL_LAYER_QUERY:
        out->phase_participation = DVZ_SCENE_VISUAL_PHASE_QUERY;
        break;
    default:
        out->phase_participation = DVZ_SCENE_VISUAL_PHASE_NONE;
        break;
    }
}



/**
 * Resolve generic pass capabilities from retained visual and family lowering facts.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param lowering resolved visual lowering facts
 * @param out output pass capabilities
 * @return whether capabilities were resolved
 */
bool _scene_visual_default_pass_caps(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(attach);
    ANN(lowering);
    ANN(out);

    DvzSceneVisualDescKind kind = lowering->desc_kind;
    bool has_normals =
        (_scene_visual_desc_is_primitive(kind) || kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH) &&
        _scene_visual_has_dense_attr(visual, "normal");

    bool point_like = kind == DVZ_SCENE_VISUAL_DESC_POINT || kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                      kind == DVZ_SCENE_VISUAL_DESC_MARKER;
    bool stroke = lowering->renderable_kind == DVZ_RENDERABLE_STROKE_QUAD ||
                  lowering->renderable_kind == DVZ_RENDERABLE_PATH_STROKE;
    bool has_material_resource =
        has_normals || stroke || _scene_visual_desc_is_sphere(kind) ||
        (point_like && lowering->needs_material_params);
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual->type);
    if (ops == NULL)
        return false;
    _scene_visual_pass_caps_resolve(
        kind, visual->alpha_mode, attach->controller_mode, has_normals, has_material_resource,
        visual->material.depth_cue_enabled, visual->depth_test_enabled, ops->baseline_layer,
        ops->baseline_product_caps, ops->baseline_phase_participation, out);
    return true;
}



/**
 * Resolve pass capabilities from one retained visual attachment.
 *
 * @param visual the retained visual
 * @param attach the panel attachment
 * @param out the output pass capabilities
 * @return whether capabilities were resolved
 */
void _scene_visual_pass_caps_apply_generated_role(
    DvzGeneratedVisualRole role, DvzSceneVisualPassCaps* caps)
{
    ANN(caps);
    bool generated_overlay = false;
    switch (role)
    {
    case DVZ_GENERATED_VISUAL_PANEL_BORDER:
    case DVZ_GENERATED_VISUAL_AXIS_MARKS:
    case DVZ_GENERATED_VISUAL_AXIS_TEXT:
    case DVZ_GENERATED_VISUAL_COLORBAR_RAMP:
    case DVZ_GENERATED_VISUAL_COLORBAR_MARKS:
    case DVZ_GENERATED_VISUAL_COLORBAR_TEXT:
    case DVZ_GENERATED_VISUAL_LEGEND_MARKS:
    case DVZ_GENERATED_VISUAL_LEGEND_TEXT:
    case DVZ_GENERATED_VISUAL_SCALEBAR_LINE:
    case DVZ_GENERATED_VISUAL_SCALEBAR_TEXT:
    case DVZ_GENERATED_VISUAL_OVERLAY_CARD:
    case DVZ_GENERATED_VISUAL_OVERLAY_TEXT:
    case DVZ_GENERATED_VISUAL_BOUNDS_OVERLAY:
        generated_overlay = true;
        break;
    default:
        break;
    }
    if (!generated_overlay)
        return;

    caps->layer = DVZ_SCENE_VISUAL_LAYER_OVERLAY;
    caps->phase_participation = DVZ_SCENE_VISUAL_PHASE_OVERLAY;
    caps->draws_in_opaque_pass = false;
    caps->draws_in_wboit_pass = false;
    caps->draws_in_depth_peel_pass = false;
    caps->draws_in_transparent_blend_pass = true;
    caps->writes_depth = false;
    caps->eligible_for_depth_postprocess = false;
    caps->eligible_for_gbuffer = false;
    caps->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_DEPTH;
    caps->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_NORMAL;
    caps->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_COVERAGE;
    caps->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_CONSUME_AMBIENT_VISIBILITY;
}



bool _scene_visual_pass_caps_from_visual(
    const DvzVisual* visual, const DvzPanelAttach* attach, DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(attach);
    ANN(out);

    DvzVisualLowering lowering = {0};
    if (!_scene_visual_lowering_resolve(visual, &lowering))
        return false;
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual->type);
    if (ops == NULL || ops->resolve_pass_caps == NULL)
        return false;
    if (!ops->resolve_pass_caps(visual, attach, &lowering, out))
        return false;

    if (attach->has_generated_role)
        _scene_visual_pass_caps_apply_generated_role(attach->generated_role, out);

    if (out->layer == DVZ_SCENE_VISUAL_LAYER_OVERLAY)
    {
        out->layer = DVZ_SCENE_VISUAL_LAYER_OVERLAY;
        out->phase_participation = DVZ_SCENE_VISUAL_PHASE_OVERLAY;
        out->draws_in_opaque_pass = false;
        out->draws_in_wboit_pass = false;
        out->draws_in_depth_peel_pass = false;
        out->draws_in_transparent_blend_pass = true;
        out->writes_depth = false;
        out->eligible_for_depth_postprocess = false;
        out->eligible_for_gbuffer = false;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_DEPTH;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_NORMAL;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SURFACE_COVERAGE;
        out->product_caps &= ~(uint32_t)DVZ_SCENE_VISUAL_PRODUCT_CAP_CONSUME_AMBIENT_VISIBILITY;
    }
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

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(
        _scene_visual_family_desc_default_type(visual->kind));
    DvzSceneVisualLayer baseline_layer = _scene_visual_desc_is_volume(visual->kind)
                                             ? DVZ_SCENE_VISUAL_LAYER_VOLUME
                                             : DVZ_SCENE_VISUAL_LAYER_SURFACE_OPAQUE;
    uint32_t baseline_product_caps = DVZ_SCENE_VISUAL_PRODUCT_CAP_PRODUCE_SCENE_COLOR;
    uint32_t baseline_phase_participation = DVZ_SCENE_VISUAL_PHASE_OPAQUE_SHADING;
    if (ops != NULL)
    {
        baseline_layer = ops->baseline_layer;
        baseline_product_caps = ops->baseline_product_caps;
        baseline_phase_participation = ops->baseline_phase_participation;
    }
    _scene_visual_pass_caps_resolve(
        visual->kind, alpha_mode, controller_mode, visual->has_normal,
        visual->material_buffer_id != 0, visual->depth_cue_enabled, visual->depth_test_enabled,
        baseline_layer, baseline_product_caps, baseline_phase_participation, out);
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
        if (meta->has_draw_contract)
        {
            if ((meta->draw_depth_policy &
                 (DVZ_SCENE_DEPTH_POLICY_TEST | DVZ_SCENE_DEPTH_POLICY_WRITE)) != 0)
                return true;
            continue;
        }
        if (meta->has_metadata)
        {
            bool stroked_path = _scene_visual_meta_is_stroked_path(&emitter->resources, meta);
            DvzRenderableKind renderable_kind =
                _scene_visual_meta_renderable_kind(&emitter->resources, meta);
            bool segment_like = renderable_kind == DVZ_RENDERABLE_STROKE_QUAD || stroked_path;
            uint64_t pos_buf = _scene_visual_resource_lookup_label(
                &emitter->resources, segment_like ? meta->position_start_id : meta->position_id);
            if (pos_buf == 0)
                continue;
            bool has_color =
                _scene_visual_resource_lookup_label(&emitter->resources, meta->color_id) != 0;
            DvzSceneVisualDescKind desc_kind =
                _scene_visual_meta_desc_kind(&emitter->resources, meta);
            bool point_like = desc_kind == DVZ_SCENE_VISUAL_DESC_POINT ||
                              desc_kind == DVZ_SCENE_VISUAL_DESC_PIXEL ||
                              desc_kind == DVZ_SCENE_VISUAL_DESC_MARKER;
            if (point_like)
            {
                if (has_color)
                    return true;
                continue;
            }
            if (desc_kind == DVZ_SCENE_VISUAL_DESC_SPLAT)
            {
                bool has_sigma =
                    _scene_visual_resource_lookup_label(&emitter->resources, meta->sigma_id) != 0;
                bool has_angle =
                    _scene_visual_resource_lookup_label(&emitter->resources, meta->angle_id) != 0;
                if (has_color && has_sigma && has_angle)
                    return true;
                continue;
            }
            if (desc_kind == DVZ_SCENE_VISUAL_DESC_SPHERE)
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
            if (desc_kind != DVZ_SCENE_VISUAL_DESC_PRIMITIVE &&
                desc_kind != DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH)
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
