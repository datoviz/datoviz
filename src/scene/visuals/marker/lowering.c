/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual lowering                                                                       */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "marker/internal.h"
#include "point/internal.h"

#include "_alloc.h"
#include "_assertions.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve marker visual lowering facts.
 *
 * @param visual the retained visual
 * @param out output lowering facts
 * @return whether lowering facts were resolved
 */
bool _scene_marker_visual_lowering(const DvzVisual* visual, DvzVisualLowering* out)
{
    ANN(visual);
    ANN(out);

    dvz_memset(out, sizeof(DvzVisualLowering), 0, sizeof(DvzVisualLowering));
    out->draw_position_attr = "position";
    out->renderable_kind = DVZ_RENDERABLE_POINT_LIKE;
    out->desc_kind = DVZ_SCENE_VISUAL_DESC_MARKER;
    out->point_like_kind = DVZ_SCENE_POINT_LIKE_MARKER;
    out->has_point_like_kind = true;
    out->needs_material_params =
        _visual_family_state(visual)->symbol_source_kind == DVZ_SYMBOL_SOURCE_NONE ||
        _visual_family_state(visual)->symbol_source_kind == DVZ_SYMBOL_SOURCE_BUILTIN;
    out->material_params_screen_scaled = out->needs_material_params;
    return true;
}



/**
 * Resolve marker pass capabilities.
 *
 * Built-in marker symbols use the point-like material set for SDF style parameters. Texture-backed
 * marker symbols use set 1 for either bitmap image sampling or distance-field glyph sampling.
 *
 * @param visual the retained visual
 * @param attach panel attachment
 * @param lowering resolved visual lowering facts
 * @param out output pass capabilities
 * @return whether capabilities were resolved
 */
bool _scene_marker_visual_pass_caps(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out)
{
    ANN(visual);
    ANN(attach);
    ANN(lowering);
    ANN(out);

    if (!_scene_visual_default_pass_caps(visual, attach, lowering, out))
        return false;

    const DvzSymbolSourceKind kind = _visual_family_state(visual)->symbol_source_kind;
    if (kind == DVZ_SYMBOL_SOURCE_BITMAP)
    {
        out->needs_material_layout = false;
        out->uses_material_set = false;
        out->uses_image_set = true;
        out->uses_glyph_set = false;
    }
    else if (kind == DVZ_SYMBOL_SOURCE_SDF || kind == DVZ_SYMBOL_SOURCE_MSDF)
    {
        out->needs_material_layout = false;
        out->uses_material_set = false;
        out->uses_image_set = true;
        out->uses_glyph_set = true;
    }
    return true;
}



/**
 * Resolve marker visual bind-group role metadata.
 *
 * @param visual the visual descriptor
 * @param controller_mode the visual's panel controller attachment mode
 * @param out the output bind descriptor
 * @return whether a bind descriptor was resolved
 */
bool _scene_marker_visual_bind_desc(
    const DvzSceneVisualDesc* visual, DvzControllerMode controller_mode,
    DvzSceneVisualBindDesc* out)
{
    if (!_scene_point_like_visual_bind_desc(visual, controller_mode, out))
        return false;
    if (visual->image_texture_id != 0)
    {
        out->uses_material_set1 = false;
        out->material_buffer_id = 0;
        if (visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA ||
            visual->glyph_atlas_encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB)
        {
            out->uses_glyph_set1 = true;
            out->glyph_texture_id = visual->image_texture_id;
            out->glyph_atlas_encoding = visual->glyph_atlas_encoding;
            out->glyph_distance_range_px =
                visual->glyph_distance_range_px > 0.0f ? visual->glyph_distance_range_px : 4.0f;
        }
        else
        {
            out->uses_image_set1 = true;
            out->image_texture_id = visual->image_texture_id;
            out->image_nearest_sampler = visual->image_nearest_sampler;
        }
    }
    return true;
}
