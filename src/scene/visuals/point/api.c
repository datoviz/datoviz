/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Point visual API                                                                             */
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

#define DVZ_POINT_STYLE_DESC_KNOWN_FLAGS 0u

/**
 * Return default point styling.
 *
 * @return default point style descriptor
 */
DvzPointStyleDesc dvz_point_style_desc(void)
{
    DvzPointStyleDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzPointStyleDesc),
        .edge_color = {0, 0, 0, 255},
        .stroke_width_px = 0.0f,
        .aspect = DVZ_SHAPE_ASPECT_FILLED,
    };
    return desc;
}



/**
 * Return whether one point style needs the style shader path.
 *
 * @param style the point style descriptor
 * @return whether style parameters differ from the shader defaults
 */
bool _point_style_enabled(const DvzPointStyleDesc* style)
{
    ANN(style);
    return style->aspect != DVZ_SHAPE_ASPECT_FILLED;
}



/**
 * Store point style data into the shared material payload used by point shaders.
 *
 * @param params the material parameter payload
 * @param style the point style descriptor
 */
void _point_style_sync_params(DvzSceneMaterialParams* params, const DvzPointStyleDesc* style)
{
    ANN(params);
    ANN(style);
    const bool stroke_enabled =
        style->aspect == DVZ_SHAPE_ASPECT_STROKE || style->aspect == DVZ_SHAPE_ASPECT_OUTLINE;
    params->params[0] = stroke_enabled && style->stroke_width_px > 0.0f ? style->stroke_width_px : 0.0f;
    params->params[1] = (float)style->aspect;
    params->params[2] = 0.0f;
    params->params[3] = 0.0f;
    params->base_color_factor[0] = (float)style->edge_color.r / 255.0f;
    params->base_color_factor[1] = (float)style->edge_color.g / 255.0f;
    params->base_color_factor[2] = (float)style->edge_color.b / 255.0f;
    params->base_color_factor[3] = (float)style->edge_color.a / 255.0f;
}



/**
 * Create a point visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_point(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_POINT, flags);
    if (visual == NULL)
        return NULL;
    return visual;
}



/**
 * Configure circular point fill/stroke styling.
 *
 * @param visual the point visual
 * @param desc the point style descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
int dvz_point_set_style(DvzVisual* visual, const DvzPointStyleDesc* desc)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_POINT)
    {
        log_error("dvz_point_set_style requires a point visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update point style"))
        return -1;

    if (desc != NULL && !DVZ_STRUCT_VALID(desc, DvzPointStyleDesc, DVZ_POINT_STYLE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzPointStyleDesc ABI prologue");
        return -1;
    }
    DvzPointStyleDesc style = desc != NULL ? *desc : dvz_point_style_desc();
    if (!isfinite(style.stroke_width_px) || style.stroke_width_px < 0.0f)
    {
        log_error("point stroke_width_px must be finite and nonnegative");
        return -1;
    }
    if (style.aspect < DVZ_SHAPE_ASPECT_FILLED || style.aspect > DVZ_SHAPE_ASPECT_OUTLINE)
    {
        log_error("point aspect must be filled, stroke, or outline");
        return -1;
    }

    visual->material.point_style = style;
    visual->material.point_style_enabled = _point_style_enabled(&style);
    _visual_material_mark_dirty(visual);
    return 0;
}
