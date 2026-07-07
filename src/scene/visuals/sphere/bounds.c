/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sphere visual bounds                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "bounds_internal.h"
#include "sphere/internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Apply one item-state style scale to an item radius.
 *
 * @param radius current radius
 * @param style item-state style vec4
 * @return scaled radius
 */
static double _sphere_bounds_apply_style_scale(double radius, const float style[4])
{
    ANN(style);
    const uint32_t flags = (uint32_t)(style[0] + 0.5f);
    const double scale = (double)style[3];
    if ((flags & DVZ_ITEM_STATE_VISUAL_SCALE) != 0 && isfinite(scale) && scale >= 0.0)
        radius *= scale;
    return radius;
}



/**
 * Return the effective radius scale for one item-state value.
 *
 * This mirrors the shader-side applyItemStateScale/apply_item_state_scale precedence.
 *
 * @param visual the sphere visual
 * @param item_state item-state bit mask
 * @return multiplicative radius scale
 */
static double _sphere_bounds_item_state_radius_scale(
    const DvzVisual* visual, uint32_t item_state)
{
    ANN(visual);
    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state == NULL)
        return 1.0;

    double radius = 1.0;
    if ((item_state & DVZ_ITEM_STATE_SELECTED) == 0)
        radius = _sphere_bounds_apply_style_scale(radius, state->item_state_style_params.unselected);
    if ((item_state & DVZ_ITEM_STATE_SELECTED) != 0)
        radius = _sphere_bounds_apply_style_scale(radius, state->item_state_style_params.selected);
    if ((item_state & DVZ_ITEM_STATE_HOVERED) != 0)
        radius = _sphere_bounds_apply_style_scale(radius, state->item_state_style_params.hovered);
    return radius;
}


/**
 * Return the shader-side radius scale from a visual-local transform.
 *
 * Sphere shaders keep impostors isotropic after model transforms by multiplying the input radius
 * by the maximum model-axis length.
 *
 * @param visual the sphere visual
 * @return model-space radius scale
 */
static double _sphere_bounds_local_radius_scale(const DvzVisual* visual)
{
    ANN(visual);
    if (!visual->has_local_transform)
        return 1.0;

    double max_scale = 0.0;
    for (uint32_t col = 0; col < 3; col++)
    {
        const double x = (double)visual->local_transform[col][0];
        const double y = (double)visual->local_transform[col][1];
        const double z = (double)visual->local_transform[col][2];
        const double scale = sqrt(x * x + y * y + z * z);
        if (isfinite(scale) && scale > max_scale)
            max_scale = scale;
    }
    return max_scale > 0.0 ? max_scale : 1.0;
}



/**
 * Transform one sphere center through the visual-local transform.
 *
 * @param visual the sphere visual
 * @param x source/output X coordinate
 * @param y source/output Y coordinate
 * @param z source/output Z coordinate
 */
static void _sphere_bounds_apply_local_center(
    const DvzVisual* visual, double* x, double* y, double* z)
{
    ANN(visual);
    ANN(x);
    ANN(y);
    ANN(z);
    if (!visual->has_local_transform)
        return;

    const double sx = *x;
    const double sy = *y;
    const double sz = *z;
    const double tx = (double)visual->local_transform[0][0] * sx +
                      (double)visual->local_transform[1][0] * sy +
                      (double)visual->local_transform[2][0] * sz +
                      (double)visual->local_transform[3][0];
    const double ty = (double)visual->local_transform[0][1] * sx +
                      (double)visual->local_transform[1][1] * sy +
                      (double)visual->local_transform[2][1] * sz +
                      (double)visual->local_transform[3][1];
    const double tz = (double)visual->local_transform[0][2] * sx +
                      (double)visual->local_transform[1][2] * sy +
                      (double)visual->local_transform[2][2] * sz +
                      (double)visual->local_transform[3][2];
    const double tw = (double)visual->local_transform[0][3] * sx +
                      (double)visual->local_transform[1][3] * sy +
                      (double)visual->local_transform[2][3] * sz +
                      (double)visual->local_transform[3][3];
    if (tw != 0.0)
    {
        *x = tx / tw;
        *y = ty / tw;
        *z = tz / tw;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Compute bounds for sphere centers expanded by radii.
 *
 * @param visual the sphere visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _sphere_bounds_from_radius(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    const DvzVisualAttr* radius_attr = _bounds_attr(visual, "radius", sizeof(float));
    const DvzVisualAttr* item_state_attr = _bounds_attr(visual, "item_state", sizeof(uint32_t));
    if (position == NULL || radius_attr == NULL ||
        position->item_count != radius_attr->item_count)
        return false;
    if (item_state_attr != NULL && item_state_attr->item_count != radius_attr->item_count)
        item_state_attr = NULL;

    const float* pos = (const float*)position->data;
    const float* radius = (const float*)radius_attr->data;
    const uint32_t* item_state =
        item_state_attr != NULL ? (const uint32_t*)item_state_attr->data : NULL;
    const double local_radius_scale = _sphere_bounds_local_radius_scale(visual);
    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double r = (double)radius[i];
        if (!isfinite(r) || r < 0.0)
            continue;
        if (item_state != NULL)
            r *= _sphere_bounds_item_state_radius_scale(visual, item_state[i]);
        r *= local_radius_scale;
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
        _sphere_bounds_apply_local_center(visual, &x, &y, &z);
        _bounds_include_point(out, x - r, y - r, z - r);
        _bounds_include_point(out, x + r, y + r, z + r);
    }
    return out->valid;
}



/**
 * Resolve bounds for a sphere visual through the visual-family registry.
 *
 * @param visual the sphere visual
 * @param out output bounds
 * @param out_force_3d output flag indicating whether flat bounds should still be treated as 3D
 * @return whether bounds were produced
 */
bool _scene_sphere_visual_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d)
{
    ANN(visual);
    ANN(out);
    ANN(out_force_3d);
    *out_force_3d = true;
    return _sphere_bounds_from_radius(visual, out);
}



/**
 * Expand exact sphere bounds for the generated 3D wire overlay.
 *
 * Sphere visual bounds remain the semantic data-space AABB of the rendered spheres. The generated
 * wire overlay uses a conservative presentation box so projected wire edges do not visually cut
 * through billboard-impostor spheres at oblique camera angles.
 *
 * @param visual the sphere visual
 * @param bounds exact bounds to expand in place
 */
void _scene_sphere_visual_expand_overlay_bounds(const DvzVisual* visual, DvzBounds* bounds)
{
    ANN(visual);
    ANN(bounds);
    if (visual->type != DVZ_VISUAL_TYPE_SPHERE || !bounds->valid)
        return;

    const DvzVisualAttr* radius_attr = _bounds_attr(visual, "radius", sizeof(float));
    const DvzVisualAttr* item_state_attr = _bounds_attr(visual, "item_state", sizeof(uint32_t));
    if (radius_attr == NULL)
        return;
    if (item_state_attr != NULL && item_state_attr->item_count != radius_attr->item_count)
        item_state_attr = NULL;

    const float* radius = (const float*)radius_attr->data;
    const uint32_t* item_state =
        item_state_attr != NULL ? (const uint32_t*)item_state_attr->data : NULL;
    const double local_radius_scale = _sphere_bounds_local_radius_scale(visual);
    double max_radius = 0.0;
    for (uint64_t i = 0; i < radius_attr->item_count; i++)
    {
        double r = (double)radius[i];
        if (!isfinite(r) || r < 0.0)
            continue;
        if (item_state != NULL)
            r *= _sphere_bounds_item_state_radius_scale(visual, item_state[i]);
        r *= local_radius_scale;
        if (r > max_radius)
            max_radius = r;
    }
    if (!(max_radius > 0.0))
        return;

    const double pad = (sqrt(3.0) - 1.0) * max_radius;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        bounds->min[dim] -= pad;
        bounds->max[dim] += pad;
    }
}
