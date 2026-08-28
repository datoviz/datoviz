/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual descriptor kinds                                                                */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "_visual_pipeline.h"
#include "_visual_pipeline_internal.h"
#include "datoviz/drp2/enums.h"
#include "registry/registry.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve a resource id from a non-empty FramePlan resource key.
 *
 * @param state the resource state
 * @param key the resource key
 * @return the resource id, or zero when absent
 */
uint64_t _scene_visual_resource_lookup_label(const ConverterState* state, const char* key)
{
    ANN(state);
    if (key == NULL || key[0] == '\0')
        return 0;
    return _resource_lookup_id(state, key);
}


/**
 * Return the descriptor kind represented by typed visual metadata.
 *
 * @param state resource id state
 * @param meta typed visual metadata
 * @return visual descriptor kind
 */
DvzSceneVisualDescKind _scene_visual_meta_desc_kind(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta)
{
    ANN(state);
    ANN(meta);
    if (meta->desc_kind != DVZ_SCENE_VISUAL_DESC_NONE)
        return (DvzSceneVisualDescKind)meta->desc_kind;

    if (meta->renderable_kind != DVZ_RENDERABLE_NONE)
    {
        DvzRenderableKind renderable_kind = (DvzRenderableKind)meta->renderable_kind;
        if (renderable_kind == DVZ_RENDERABLE_STROKE_QUAD)
            return DVZ_SCENE_VISUAL_DESC_SEGMENT;
        if (renderable_kind == DVZ_RENDERABLE_PATH_STROKE)
            return DVZ_SCENE_VISUAL_DESC_PATH;
        if (renderable_kind == DVZ_RENDERABLE_INDEXED_MESH)
            return DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
        if (renderable_kind == DVZ_RENDERABLE_TEXTURED_QUAD)
            return DVZ_SCENE_VISUAL_DESC_IMAGE;
        if (renderable_kind == DVZ_RENDERABLE_VOLUME_PROXY)
            return DVZ_SCENE_VISUAL_DESC_VOLUME;
    }
    return _scene_visual_family_desc_kind((DvzVisualType)meta->visual_type);
}


/**
 * Return the renderable primitive kind represented by typed visual metadata.
 *
 * @param state resource id state
 * @param meta typed visual metadata
 * @return renderable primitive kind
 */
DvzRenderableKind _scene_visual_meta_renderable_kind(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta)
{
    ANN(state);
    ANN(meta);
    if (meta->renderable_kind != DVZ_RENDERABLE_NONE)
        return (DvzRenderableKind)meta->renderable_kind;

    return _scene_visual_family_renderable_kind((DvzVisualType)meta->visual_type);
}


/**
 * Return whether typed path metadata has derived stroke resources.
 *
 * @param state the resource state
 * @param meta the typed visual metadata
 * @return whether the path should use the segment stroke pipeline
 */
bool _scene_visual_meta_is_stroked_path(
    const ConverterState* state, const DvzFramePlanVisualMeta* meta)
{
    ANN(state);
    ANN(meta);
    if (_scene_visual_meta_renderable_kind(state, meta) != DVZ_RENDERABLE_PATH_STROKE)
        return false;
    return _scene_visual_resource_lookup_label(state, meta->position_start_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->position_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->position_end_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->position_next_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->line_width_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->path_flags_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->path_distance_id) != 0 &&
           _scene_visual_resource_lookup_label(state, meta->index_id) != 0;
}


/**
 * Return a compact visual descriptor kind name for diagnostics.
 *
 * @param kind visual descriptor kind
 * @return the visual descriptor kind name
 */
const char* _scene_visual_desc_kind_name(DvzSceneVisualDescKind kind)
{
    switch (kind)
    {
    case DVZ_SCENE_VISUAL_DESC_POINT:
        return "point";
    case DVZ_SCENE_VISUAL_DESC_PIXEL:
        return "pixel";
    case DVZ_SCENE_VISUAL_DESC_SPLAT:
        return "splat";
    case DVZ_SCENE_VISUAL_DESC_MARKER:
        return "marker";
    case DVZ_SCENE_VISUAL_DESC_SPHERE:
        return "sphere";
    case DVZ_SCENE_VISUAL_DESC_SEGMENT:
        return "segment";
    case DVZ_SCENE_VISUAL_DESC_PATH:
        return "path";
    case DVZ_SCENE_VISUAL_DESC_PRIMITIVE:
        return "primitive";
    case DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH:
        return "textured_mesh";
    case DVZ_SCENE_VISUAL_DESC_IMAGE:
        return "image";
    case DVZ_SCENE_VISUAL_DESC_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_LABELS_UINT:
        return "labels";
    case DVZ_SCENE_VISUAL_DESC_GLYPH:
        return "glyph";
    case DVZ_SCENE_VISUAL_DESC_VOLUME:
    case DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT:
    case DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT:
        return "volume";
    case DVZ_SCENE_VISUAL_DESC_NONE:
    default:
        return "unknown";
    }
}


/**
 * Return the default visual family for one descriptor kind.
 *
 * @param kind visual descriptor kind
 * @return the visual type, or NONE when the kind has no default family
 */
DvzVisualType _scene_visual_desc_default_type(DvzSceneVisualDescKind kind)
{
    return _scene_visual_family_desc_default_type(kind);
}


/**
 * Return whether one retained visual has dense data for an attribute.
 *
 * @param visual the retained visual
 * @param name the attribute name
 * @return whether dense data exists
 */
bool _scene_visual_has_dense_attr(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    int attr_idx = _attr_index(visual, name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}


/**
 * Return whether a visual descriptor uses the primitive pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is primitive-like
 */
bool _scene_visual_desc_is_primitive(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_PRIMITIVE;
}


/**
 * Return whether a visual descriptor uses the textured mesh pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is textured-mesh-like
 */
bool _scene_visual_desc_is_textured_mesh(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH;
}



/**
 * Return whether a visual descriptor uses a sampled-image bind group.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is image-like
 */
bool _scene_visual_desc_is_image(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_IMAGE ||
           kind == DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH ||
           kind == DVZ_SCENE_VISUAL_DESC_LABELS_SINT ||
           kind == DVZ_SCENE_VISUAL_DESC_LABELS_UINT || kind == DVZ_SCENE_VISUAL_DESC_GLYPH;
}


/**
 * Return whether a visual descriptor uses a sampled-volume bind group.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is volume-like
 */
bool _scene_visual_desc_is_volume(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_VOLUME ||
           kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT ||
           kind == DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT;
}


/**
 * Return whether a visual descriptor uses the sphere impostor pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is sphere-like
 */
bool _scene_visual_desc_is_sphere(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_SPHERE;
}


/**
 * Return whether a visual descriptor uses the glyph atlas pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is glyph-like
 */
bool _scene_visual_desc_is_glyph(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_GLYPH;
}


/**
 * Return whether a visual descriptor uses the segment analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is segment-like
 */
bool _scene_visual_desc_is_segment(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_SEGMENT;
}


/**
 * Return whether a visual descriptor uses the path analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is path-like
 */
bool _scene_visual_desc_is_path(DvzSceneVisualDescKind kind)
{
    return kind == DVZ_SCENE_VISUAL_DESC_PATH;
}


/**
 * Return whether a visual descriptor uses an analytic stroke pipeline family.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor is stroke-like
 */
bool _scene_visual_desc_is_stroke(DvzSceneVisualDescKind kind)
{
    return _scene_visual_desc_is_segment(kind) || _scene_visual_desc_is_path(kind);
}



/**
 * Return whether a visual descriptor uses analytic coverage blending in normal opaque passes.
 *
 * @param kind the visual descriptor kind
 * @return whether the descriptor uses coverage blending
 */
bool _scene_visual_desc_uses_coverage_blend(DvzSceneVisualDescKind kind)
{
    return _scene_visual_desc_is_stroke(kind) || kind == DVZ_SCENE_VISUAL_DESC_POINT ||
           kind == DVZ_SCENE_VISUAL_DESC_MARKER;
}



/**
 * Resolve the backend-specific point-like visual lowering policy.
 *
 * @param kind the point-like visual family
 * @param shader_format the target shader format
 * @param item_count number of logical items in the visual
 * @param out the output lowering descriptor
 * @return whether the lowering descriptor was resolved
 */
bool _scene_point_like_lowering_desc(
    DvzScenePointLikeKind kind, DvzSceneShaderFormat shader_format, uint32_t item_count,
    DvzScenePointLikeLoweringDesc* out)
{
    ANN(out);
    dvz_memset(
        out, sizeof(DvzScenePointLikeLoweringDesc), 0, sizeof(DvzScenePointLikeLoweringDesc));
    out->kind = kind;

    // Spheres are lowered to quads on every backend, not just WGSL. A native point sprite is
    // sized by gl_PointSize, which the device clamps to VkPhysicalDeviceLimits::pointSizeRange,
    // and a point primitive is clipped by its centre vertex. A sphere that is large on screen,
    // or whose centre leaves the frustum, therefore loses part or all of its silhouette. Quads
    // have neither limit, so the impostor stays correct at any camera distance or viewport size.
    if (shader_format == DVZ_SCENE_SHADER_FORMAT_WGSL || kind == DVZ_SCENE_POINT_LIKE_SPHERE)
    {
        out->lowering = DVZ_SCENE_POINT_LIKE_LOWERING_INSTANCED_QUADS;
        out->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out->vertex_step_mode = DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE;
        out->draw_vertex_count = 6;
        out->draw_instance_count = item_count;
        return true;
    }

    out->lowering = DVZ_SCENE_POINT_LIKE_LOWERING_NATIVE_POINTS;
    out->topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
    out->vertex_step_mode = DVZ_DRP2_VERTEX_STEP_MODE_VERTEX;
    out->draw_vertex_count = item_count;
    out->draw_instance_count = 1;
    return true;
}
