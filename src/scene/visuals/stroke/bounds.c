/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stroke visual bounds                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_visual_internal.h"
#include "bounds_internal.h"
#include "segment/internal.h"
#include "stroke/internal.h"
#include "vector/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Compute bounds for segment endpoint attributes.
 *
 * @param visual the segment visual
 * @param out output bounds
 * @return whether bounds were produced
 */
bool _stroke_bounds_from_segment(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* start = _bounds_attr(visual, "position_start", 3 * sizeof(float));
    const DvzVisualAttr* end = _bounds_attr(visual, "position_end", 3 * sizeof(float));
    if (start == NULL || end == NULL || start->item_count != end->item_count)
        return false;
    _bounds_include_vec3f(out, (const float*)start->data, start->item_count);
    _bounds_include_vec3f(out, (const float*)end->data, end->item_count);
    return out->valid;
}



/**
 * Resolve bounds for a segment visual through the visual-family registry.
 *
 * @param visual the segment visual
 * @param out output bounds
 * @param out_force_3d output flag indicating whether flat bounds should still be treated as 3D
 * @return whether bounds were produced
 */
bool _scene_segment_visual_bounds(
    const DvzVisual* visual, DvzBounds* out, bool* out_force_3d)
{
    ANN(visual);
    ANN(out);
    ANN(out_force_3d);
    *out_force_3d = false;
    return _stroke_bounds_from_segment(visual, out);
}



/**
 * Compute bounds for straight vector endpoint attributes.
 *
 * @param visual the vector visual
 * @param out output bounds
 * @return whether bounds were produced
 */
bool _stroke_bounds_from_vector(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    const DvzVisualAttr* vector = _bounds_attr(visual, "vector", 3 * sizeof(float));
    if (position == NULL || vector == NULL || position->item_count != vector->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* vec = (const float*)vector->data;
    float scale = visual->vector.scale;
    float tail_factor = 0.0f;
    float head_factor = 1.0f;
    if (visual->vector.anchor == DVZ_VECTOR_ANCHOR_CENTER)
    {
        tail_factor = -0.5f;
        head_factor = 0.5f;
    }
    else if (visual->vector.anchor == DVZ_VECTOR_ANCHOR_HEAD)
    {
        tail_factor = -1.0f;
        head_factor = 0.0f;
    }

    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double start[3] = {0};
        double end[3] = {0};
        for (uint32_t k = 0; k < 3; k++)
        {
            double delta = (double)vec[3 * i + k] * (double)scale;
            start[k] = (double)pos[3 * i + k] + (double)tail_factor * delta;
            end[k] = (double)pos[3 * i + k] + (double)head_factor * delta;
        }
        _bounds_include_point(out, start[0], start[1], start[2]);
        _bounds_include_point(out, end[0], end[1], end[2]);
    }
    return out->valid;
}



/**
 * Resolve bounds for a vector visual through the visual-family registry.
 *
 * @param visual the vector visual
 * @param out output bounds
 * @param out_force_3d output flag indicating whether flat bounds should still be treated as 3D
 * @return whether bounds were produced
 */
bool _scene_vector_visual_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d)
{
    ANN(visual);
    ANN(out);
    ANN(out_force_3d);
    *out_force_3d = false;
    if (_stroke_bounds_from_vector(visual, out))
        return true;
    return _scene_visual_default_bounds(visual, out, out_force_3d);
}
