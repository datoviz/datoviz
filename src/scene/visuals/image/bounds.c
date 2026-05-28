/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Image visual bounds                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "bounds_internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Compute bounds for image visuals.
 *
 * @param visual the image visual
 * @param out output bounds
 * @return whether bounds were produced
 */
bool _image_bounds_from_extent(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    if (position == NULL)
        return false;

    const DvzVisualAttr* extent = _bounds_attr(visual, "extent", 2 * sizeof(float));
    if (extent == NULL)
    {
        _bounds_include_vec3f(out, (const float*)position->data, position->item_count);
        return out->valid;
    }
    if (position->item_count != extent->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* ext = (const float*)extent->data;
    const DvzVisualAttr* anchor = _bounds_attr(visual, "anchor", 2 * sizeof(float));
    const float* anc =
        anchor != NULL && anchor->item_count == position->item_count ? (const float*)anchor->data
                                                                     : NULL;

    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
        double w = (double)ext[2 * i + 0];
        double h = (double)ext[2 * i + 1];
        double ax = anc != NULL ? (double)anc[2 * i + 0] : 0.0;
        double ay = anc != NULL ? (double)anc[2 * i + 1] : 0.0;
        double x0 = x - 0.5 * (ax + 1.0) * w;
        double y0 = y - 0.5 * (ay + 1.0) * h;
        _bounds_include_point(out, x0, y0, z);
        _bounds_include_point(out, x0 + w, y0 + h, z);
    }
    return out->valid;
}
