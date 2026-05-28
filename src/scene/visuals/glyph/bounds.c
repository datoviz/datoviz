/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Glyph visual bounds                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "bounds_internal.h"
#include "glyph/internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Compute bounds for glyph visuals from anchor positions and local pixel bounds.
 *
 * @param visual the glyph visual
 * @param out output bounds
 * @return whether bounds were produced
 */
bool _glyph_bounds_from_rect(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    const DvzVisualAttr* bounds = _bounds_attr(visual, "bounds", 4 * sizeof(float));
    if (position == NULL || bounds == NULL || position->item_count != bounds->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* rect = (const float*)bounds->data;
    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
        _bounds_include_point(out, x + (double)rect[4 * i + 0], y + (double)rect[4 * i + 1], z);
        _bounds_include_point(out, x + (double)rect[4 * i + 2], y + (double)rect[4 * i + 3], z);
    }
    return out->valid;
}



/**
 * Resolve bounds for a glyph visual through the visual-family registry.
 *
 * @param visual the glyph visual
 * @param out output bounds
 * @param out_force_3d output flag indicating whether flat bounds should still be treated as 3D
 * @return whether bounds were produced
 */
bool _scene_glyph_visual_bounds(const DvzVisual* visual, DvzBounds* out, bool* out_force_3d)
{
    ANN(visual);
    ANN(out);
    ANN(out_force_3d);
    *out_force_3d = false;
    return _glyph_bounds_from_rect(visual, out);
}
