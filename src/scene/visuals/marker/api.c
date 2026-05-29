/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Marker visual API                                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return default marker styling.
 *
 * @return default marker style descriptor
 */
DvzMarkerStyle dvz_marker_style(void)
{
    DvzMarkerStyle style = {
        .edge_color = {0, 0, 0, 255},
        .stroke_width = 0.0f,
        .aspect = DVZ_SHAPE_ASPECT_FILLED,
    };
    return style;
}



/**
 * Convert a marker style to the shared point-like material payload.
 *
 * @param style the marker style
 * @return equivalent point style descriptor
 */
DvzPointStyleDesc _marker_style_to_point_style(const DvzMarkerStyle* style)
{
    ANN(style);
    DvzPointStyleDesc out = {
        .edge_color = style->edge_color,
        .stroke_width = style->stroke_width,
        .aspect = style->aspect,
    };
    return out;
}



/**
 * Create a marker visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_marker(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_MARKER, flags);
    if (visual == NULL)
        return NULL;
    _visual_family_state(visual)->topology = DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST;
    _visual_family_state(visual)->material_params_dirty = true;
    return visual;
}



/**
 * Configure marker fill/stroke styling.
 *
 * @param visual the marker visual
 * @param style the marker style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
int dvz_marker_set_style(DvzVisual* visual, const DvzMarkerStyle* style)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MARKER)
    {
        log_error("dvz_marker_set_style requires a marker visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update marker style"))
        return -1;

    DvzMarkerStyle marker_style = style != NULL ? *style : dvz_marker_style();
    if (!isfinite(marker_style.stroke_width) || marker_style.stroke_width < 0.0f)
    {
        log_error("marker stroke_width must be finite and nonnegative");
        return -1;
    }
    if (marker_style.aspect < DVZ_SHAPE_ASPECT_FILLED ||
        marker_style.aspect > DVZ_SHAPE_ASPECT_OUTLINE)
    {
        log_error("marker aspect must be filled, stroke, or outline");
        return -1;
    }

    DvzPointStyleDesc point_style = _marker_style_to_point_style(&marker_style);
    visual->material.point_style = point_style;
    visual->material.point_style_enabled = _point_style_enabled(&point_style);
    _visual_material_mark_dirty(visual);
    return 0;
}
