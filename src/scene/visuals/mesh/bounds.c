/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mesh visual bounds                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "bounds_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Include one transformed AABB corner in bounds.
 *
 * @param out output bounds
 * @param transform column-major mat4 transform
 * @param x input x coordinate
 * @param y input y coordinate
 * @param z input z coordinate
 */
static void
_mesh_bounds_include_transformed_point(
    DvzBounds* out, const float* transform, double x, double y, double z)
{
    ANN(out);
    ANN(transform);
    double tx = (double)transform[0] * x + (double)transform[4] * y +
                (double)transform[8] * z + (double)transform[12];
    double ty = (double)transform[1] * x + (double)transform[5] * y +
                (double)transform[9] * z + (double)transform[13];
    double tz = (double)transform[2] * x + (double)transform[6] * y +
                (double)transform[10] * z + (double)transform[14];
    double tw = (double)transform[3] * x + (double)transform[7] * y +
                (double)transform[11] * z + (double)transform[15];
    if (tw != 0.0)
    {
        tx /= tw;
        ty /= tw;
        tz /= tw;
    }
    _bounds_include_point(out, tx, ty, tz);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Expand mesh bounds by per-instance transforms when available.
 *
 * @param visual the mesh visual
 * @param base base position bounds
 * @param out output bounds
 * @return whether bounds were produced
 */
bool _mesh_bounds_from_instances(
    const DvzVisual* visual, const DvzBounds* base, DvzBounds* out)
{
    ANN(visual);
    ANN(base);
    ANN(out);
    const DvzVisualAttr* transforms =
        _bounds_attr(visual, "instance_transform", 16 * sizeof(float));
    if (transforms == NULL)
    {
        _bounds_include_point(out, base->min[0], base->min[1], base->min[2]);
        _bounds_include_point(out, base->max[0], base->max[1], base->max[2]);
        return out->valid;
    }

    const float* transform = (const float*)transforms->data;
    for (uint64_t i = 0; i < transforms->item_count; i++)
    {
        const float* mat = &transform[16 * i];
        for (uint32_t x = 0; x < 2; x++)
        {
            for (uint32_t y = 0; y < 2; y++)
            {
                for (uint32_t z = 0; z < 2; z++)
                {
                    double px = x == 0 ? base->min[0] : base->max[0];
                    double py = y == 0 ? base->min[1] : base->max[1];
                    double pz = z == 0 ? base->min[2] : base->max[2];
                    _mesh_bounds_include_transformed_point(out, mat, px, py, pz);
                }
            }
        }
    }
    return out->valid;
}
