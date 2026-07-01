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
    if (position == NULL || radius_attr == NULL ||
        position->item_count != radius_attr->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* radius = (const float*)radius_attr->data;
    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double r = (double)radius[i];
        if (!isfinite(r) || r < 0.0)
            continue;
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
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
